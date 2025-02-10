// Copyright MuuKnighted Games 2024. All rights reserved.

#include "AnimGraph/AnimNode_BlendSpacePlayerMatcher.h"

#include "Utility/AnimSuiteMathLibrary.h"
#include "Utility/AnimSuiteTrace.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_Inertialization.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSync.h"
#include "Animation/AnimSyncScope.h"
#include "Animation/BlendSpace.h"
#include "DrawDebugHelpers.h"
#include "Animation/BlendSpace1D.h"
#include "Trace/Trace.inl"

#define LOCTEXT_NAMESPACE "AnimationMatchingSuiteNodes"

FAnimNode_BlendSpacePlayerMatcher::FAnimNode_BlendSpacePlayerMatcher()
	: BlendProfile(nullptr)
	, BlendSpace(nullptr)
	, bBlendSpaceChanged(false)
	, bIsBeingReinitialized(true)
	, bWasReinitialized(false)
	, NormalizedTime(0.0f)
	, PreviousMatchingDistance(0.0f)
	, bTriggerSmoothTimeFollowingPoseMatch(false)
	, bTeleportToNormalizedTime(false)
	, bIsDistanceMatching(false)
	, PrevBlendInputAxisValues(FVector2D::ZeroVector)
	, XMin(0.0f)
	, XMax(0.0f)
	, YMin(0.0f)
	, YMax(0.0f)
	, PoseDebugData(FAMSDebugData())
	, bDrawDebugThisFrame(false)
{
}

void FAnimNode_BlendSpacePlayerMatcher::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
		
	GetEvaluateGraphExposedInputs().Execute(Context);

	/** Set initial values. */
	NormalizedTime = 0.0f;
	bIsBeingReinitialized = true;
	PreviousBlendSpace = GetBlendSpace();
	PrevBlendInputAxisValues = FVector2D(GetPosition().X, GetPosition().Y); // set here so as to not trigger inertialization twice (not that it would really matter)

	/** Get the axes limits of the current BlendSpace. */
	if (GetBlendSpace() != nullptr)
	{
		XMin = GetBlendSpace()->GetBlendParameter(0).Min;
		XMax = GetBlendSpace()->GetBlendParameter(0).Max;
		YMin = GetBlendSpace()->GetBlendParameter(1).Min;
		YMax = GetBlendSpace()->GetBlendParameter(1).Max;
	}
}

void FAnimNode_BlendSpacePlayerMatcher::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
}

void FAnimNode_BlendSpacePlayerMatcher::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);

	UpdateInternal(Context);
		
}

void FAnimNode_BlendSpacePlayerMatcher::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateInternal)

	TObjectPtr<UBlendSpace> CurrentBlendSpace = GetBlendSpace();
	if (CurrentBlendSpace != nullptr && CurrentBlendSpace->GetSkeleton() != nullptr)
	{
		/** If the BlendSpace changed, get the axis limits of the new Blend Space asset. And if inertial blending should be requested, request inertial blending. */
		bBlendSpaceChanged = CurrentBlendSpace != PreviousBlendSpace;
		if (!bIsBeingReinitialized && bBlendSpaceChanged)
		{
			/** Get the axes limits of the new BlendSpace. */
			XMin = GetBlendSpace()->GetBlendParameter(0).Min;
			XMax = GetBlendSpace()->GetBlendParameter(0).Max;
			YMin = GetBlendSpace()->GetBlendParameter(1).Min;
			YMax = GetBlendSpace()->GetBlendParameter(1).Max;

			if (GetShouldInertiallyBlendUponBlendSpaceChange() && GetInertialBlendTime() > 0.0f)
			{
				UE::Anim::IInertializationRequester* InertializationRequester = Context.GetMessage<UE::Anim::IInertializationRequester>();
				if (InertializationRequester)
				{
					InertializationRequester->RequestInertialization(GetInertialBlendTime(), GetBlendProfile());
					InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
				}
			}
		}
		
		const FVector BlendInputPosition = GetPosition();
		
		/** If blending is desired upon rapid axis value change, check whether axis values have deltas larger than the trigger thresholds.
		    But don't perform this check if the node is reinitializing or if the Blend Space changed. */
		if (!bBlendSpaceChanged && !bIsBeingReinitialized && GetShouldInertiallyBlendUponRapidAxisValueChange())
		{
			bool bShouldTriggerInertialization = false;
			float NormalizedXAxisInput = (BlendInputPosition.X - XMin) / (XMax - XMin);
			float PrevNormalizedXAxisInput = (PrevBlendInputAxisValues.X - XMin) / (XMax - XMin);
			bShouldTriggerInertialization = FMath::Abs(NormalizedXAxisInput - PrevNormalizedXAxisInput) >= GetBlendTriggerThresholdDeltas().X;

			/** Only calculate the Y dimension if the Blend Space asset is NOT a 1D Blend Space.*/
			if (!GetBlendSpace()->IsA<UBlendSpace1D>() && !bShouldTriggerInertialization)
			{
				float NormalizedYAxisInput = (BlendInputPosition.Y - YMin) / (YMax - YMin);
				float PrevNormalizedYAxisInput = (PrevBlendInputAxisValues.Y - YMin) / (YMax - YMin);
				bShouldTriggerInertialization = FMath::Abs(NormalizedYAxisInput - PrevNormalizedYAxisInput) >= GetBlendTriggerThresholdDeltas().Y;
			}

			if (bShouldTriggerInertialization)
			{
				UE::Anim::IInertializationRequester* InertializationRequester = Context.GetMessage<UE::Anim::IInertializationRequester>();
				if (InertializationRequester)
				{
					InertializationRequester->RequestInertialization(GetInertialBlendTime(), GetBlendProfile());
					InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
					//UE_LOG(LogTemp, Warning, TEXT("Inertialization was trigger by Blend Space rapid axis value change. "));
				}
			}
		}

		const bool bHadJustBeenInitialized = bIsBeingReinitialized;
		
		/** If the Blend Space has changed or this node has been initialized, perform reinitialization. */
		if (bBlendSpaceChanged || bIsBeingReinitialized)
		{
			Reinitialize(Context);
			InternalTimeAccumulator = NormalizedTime;
		}
		/** If reinitialization is not necessary, but distance matching is required, perform distance matching. */
		else if (GetMatchingType() == EMatchingType::DistanceMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch)
		{
			/** Negate the matching distance value, if applicable. */
			float InMatchingDistance = GetMatchingDistance();
			InMatchingDistance = (GetNegateDistanceValue() && InMatchingDistance > 0.0f) ? -InMatchingDistance : InMatchingDistance;

			/** Get the relevant blend samples. */
			const FVector ClampedBlendInput = CurrentBlendSpace->GetClampedAndWrappedBlendInput(BlendInputPosition);
			TArray<FBlendSampleData> BlendSampleData;
			CurrentBlendSpace->GetSamplesFromBlendInput(ClampedBlendInput, BlendSampleData, CachedTriangulationIndex, true);
			
			/** Set the internal time to be the clamped normalized time. Apply smoothing, if applicable.
			    (Calling the GetShouldSmoothTimeFollowingPoseMatch function again here as a condition is done to account 
			    for the case of the user disabling smoothing at runtime in the middle of distance matching.) */
			//if (bTriggerSmoothTimeFollowingPoseMatch && GetShouldSmoothTimeFollowingPoseMatch() && !GetShouldAdvanceTimeNaturally()) // @TODO: should I keep this removed? 
			if(GetSmoothingAlpha() > 0.0f && !GetShouldAdvanceTimeNaturally())
			{
				float DesiredNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(CurrentBlendSpace, InMatchingDistance, NormalizedTime,
																BlendSampleData, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
																GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());

				/** Linearly interpolate. */
				NormalizedTime = NormalizedTime + (1 - GetSmoothingAlpha()) * (DesiredNormalizedTime - NormalizedTime);
				InternalTimeAccumulator = NormalizedTime;
			} 
			else
			{
				NormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(CurrentBlendSpace, InMatchingDistance, NormalizedTime,
																BlendSampleData, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
																GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
				InternalTimeAccumulator = NormalizedTime;
			}

			// @TODO: Frame-to-frame blending does not work as I expected. Remove for now. I need to figure out what I'm missing. It may requires the poses are not already being interpolated by the animation asset (like in motion matching).
			/** If frame-to-frame inertial blending is desired, request inertial blending. */ 
			if (GetShouldInertiallyBlendFrameToFrame() && (GetMatchingType() == EMatchingType::DistanceMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch) && !GetShouldAdvanceTimeNaturally())
			{
				UE::Anim::IInertializationRequester* InertializationRequester = Context.GetMessage<UE::Anim::IInertializationRequester>();
				if (InertializationRequester)
				{
					InertializationRequester->RequestInertialization(GetInertialBlendTime());
					InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
					//UE_LOG(LogTemp, Warning, TEXT("Inertialization requested with blend time of %f when switching from %s to %s"), GetInertialBlendTime(), *PreviousBlendSpace->GetName(), *CurrentBlendSpace->GetName());
				}
			}
			
			bIsDistanceMatching = true;
		}

		
		
		/** Override key values of the Tick Record depending on the matching type and whether reinitialization occured. */
		bWasReinitialized = (bHadJustBeenInitialized != bIsBeingReinitialized) || bBlendSpaceChanged;
		bTeleportToNormalizedTime = (bWasReinitialized && GetMatchingType() != EMatchingType::None) || GetMatchingType() == EMatchingType::DistanceMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch;
		
		/** Create a tick record and push it into the closest scope. */
		UE::Anim::FAnimSyncGroupScope& SyncScope = Context.GetMessageChecked<UE::Anim::FAnimSyncGroupScope>();
		FAnimTickRecord TickRecord(
			CurrentBlendSpace, BlendInputPosition, BlendSampleDataCache, BlendFilter, GetLoop(), GetPlayRate(), bTeleportToNormalizedTime, 
			IsEvaluator(), Context.GetFinalBlendWeight(), /*inout*/ InternalTimeAccumulator, MarkerTickRecord);
		TickRecord.RootMotionWeightModifier = Context.GetRootMotionWeightModifier();
		TickRecord.DeltaTimeRecord = &DeltaTimeRecord;

		UE::Anim::FAnimSyncParams SyncParams(GetGroupName(), GetGroupRole(), GetGroupMethod());
		TickRecord.GatherContextData(Context);

		SyncScope.AddTickRecord(TickRecord, SyncParams, UE::Anim::FAnimSyncDebugInfo(Context));

		TRACE_ANIM_TICK_RECORD(Context, TickRecord);

#if WITH_EDITORONLY_DATA
		if (FAnimBlueprintDebugData* DebugData = Context.AnimInstanceProxy->GetAnimBlueprintDebugData())
		{
			DebugData->RecordBlendSpacePlayer(Context.GetCurrentNodeId(), CurrentBlendSpace, BlendInputPosition, BlendFilter.GetFilterLastOutput());
		}
#endif
		
		/** Cache and reset essential info.  */
		PreviousMatchingDistance = GetMatchingDistance();
		bBlendSpaceChanged = false;
		PreviousBlendSpace = CurrentBlendSpace;
		PrevBlendInputAxisValues = FVector2D(BlendInputPosition.X, BlendInputPosition.Y);
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	
	if (Context.AnimInstanceProxy != nullptr && GetShowDebugShapes() && bDrawDebugThisFrame)
	{		
		const FTransform ComponentTransform = Context.AnimInstanceProxy->GetComponentTransform();

		switch (GetDebugLevel())
		{
			case EDebugPoseMatchLevel::ShowSelectedPosePosition:
			{
				if (!PoseDebugData.AreTransformsEmpty())
				{
					for (FTransform& BoneTransform : PoseDebugData.DebugBoneTransforms)
					{
						FVector BonePoint = ComponentTransform.TransformPosition(BoneTransform.GetTranslation());
						Context.AnimInstanceProxy->AnimDrawDebugSphere(BonePoint, GetPositionDrawScale(), 12.0f, FColor::Magenta, false, 0.5f, 0.5f);
					}
				}
			} break;
			case EDebugPoseMatchLevel::ShowSelectedPosePositionAndVelocity:
			{
				if (!PoseDebugData.AreTransformsEmpty())
				{
					for (int32 i = 0; i < PoseDebugData.DebugBoneTransforms.Num(); ++i)
					{
						FVector BonePoint = ComponentTransform.TransformPosition(PoseDebugData.DebugBoneTransforms[i].GetTranslation());
						Context.AnimInstanceProxy->AnimDrawDebugSphere(BonePoint, GetPositionDrawScale(), 12.0f, FColor::Magenta, false, 0.5f, 0.5f);

						if (!PoseDebugData.AreVelocitiesEmpty())
						{
							FVector BoneVelocity = ComponentTransform.TransformVector(PoseDebugData.DebugBoneVelocities[i]).GetSafeNormal();
							if (!BoneVelocity.IsZero())
							{
								Context.AnimInstanceProxy->AnimDrawDebugDirectionalArrow(BonePoint, BonePoint + BoneVelocity * GetVelocityDrawScale(), 10.0f, FColor::Magenta, false, 0.5f, 1.0f);
							}
						}
					}
				}
				
			} break;
		}
		bDrawDebugThisFrame = false;
	}

#endif
	
	TRACE_POSEMATCHBLENDSPACE_PLAYER(Context, *this);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Name"), CurrentBlendSpace ? *CurrentBlendSpace->GetName() : TEXT("None"));
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Blend Space"), CurrentBlendSpace);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Playback Time"), InternalTimeAccumulator);
}

void FAnimNode_BlendSpacePlayerMatcher::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread) 

	UBlendSpace* CurrentBlendSpace = GetBlendSpace();
	if (CurrentBlendSpace != nullptr && CurrentBlendSpace->GetSkeleton() != nullptr)
	{
		FAnimationPoseData AnimationPoseData(Output);
		CurrentBlendSpace->GetAnimationPose(BlendSampleDataCache, FAnimExtractContext(static_cast<double>(InternalTimeAccumulator),
			Output.AnimInstanceProxy->ShouldExtractRootMotion(), DeltaTimeRecord, GetLoop()), AnimationPoseData);
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FAnimNode_BlendSpacePlayerMatcher::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	UBlendSpace* CurrentBlendSpace = GetBlendSpace();
	if (CurrentBlendSpace != nullptr)
	{
		DebugLine += FString::Printf(TEXT("('%s' Play Time: %.3f)"), *CurrentBlendSpace->GetName(), InternalTimeAccumulator);

		DebugData.AddDebugItem(DebugLine, true);
	}
}

float FAnimNode_BlendSpacePlayerMatcher::GetCurrentAssetTime() const
{
	if(const FBlendSampleData* HighestWeightedSample = GetHighestWeightedSample())
	{
		return HighestWeightedSample->Time;
	}

	/** No sample */
	return 0.0f;
}

float FAnimNode_BlendSpacePlayerMatcher::GetCurrentAssetTimePlayRateAdjusted() const
{
	float Length = GetCurrentAssetLength();
	return GetPlayRate() < 0.0f ? Length - InternalTimeAccumulator * Length : Length * InternalTimeAccumulator;
}

float FAnimNode_BlendSpacePlayerMatcher::GetCurrentAssetLength() const
{
	if(const FBlendSampleData* HighestWeightedSample = GetHighestWeightedSample())
	{
		UBlendSpace* CurrentBlendSpace = GetBlendSpace();
		if (CurrentBlendSpace != nullptr)
		{
			const FBlendSample& Sample = CurrentBlendSpace->GetBlendSample(HighestWeightedSample->SampleDataIndex);
			return Sample.Animation->GetPlayLength();
		}
	}

	/** No sample */
	return 0.0f;
}

void FAnimNode_BlendSpacePlayerMatcher::Reinitialize(const FAnimationUpdateContext& Context)
{
	// /** If PoseMatch or PoseAndDistanceMatch are chosen but pose-matching functionality is toggled off, set the MatchingType appropriately.*/ 
	// if (!GetPoseMatchingIsActive() && (GetMatchingType() == EMatchingType::PoseMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch))
	// {
	// 	if (GetMatchingType() == EMatchingType::PoseMatch)
	// 	{
	// 		SetMatchingType(EMatchingType::None);
	// 	}
	// 	else // if GetMatchingType() == EMatchingType::PoseAndDistanceMatch
	// 	{
	// 		SetMatchingType(EMatchingType::DistanceMatch);
	// 	}
	// }

	const TObjectPtr<UBlendSpace> CurrentBlendSpace = GetBlendSpace();
	switch (GetMatchingType())
	{
		/** If no matching is being done. */
		case EMatchingType::None:
		{
			BlendSampleDataCache.Empty();
				
			if (GetResetPlayTimeWhenBlendSpaceChanges())
			{
				float CurrentStartPosition = GetStartPosition();

				NormalizedTime = FMath::Clamp(CurrentStartPosition, 0.0f, 1.0f);
				if (CurrentStartPosition == 0.0f && GetPlayRate() < 0.0f)
				{
					/** Jump to the end of the Blend Space. */
					NormalizedTime = 1.0f;
				}
			}

			bIsDistanceMatching = false;
		} break;

		/** If only pose matching is being done. */
		case EMatchingType::PoseMatch:
		{
			if (CurrentBlendSpace == nullptr)
			{
				UE_LOG(LogAnimation, Warning, TEXT("\"Reinitialize (Blend Space Player: Matching)\": The Blend Space asset is invalid."));
				bIsDistanceMatching = false;
				return;
			}
				
			/** Get the Blend Sample Data Cache based on input. */
			const FVector ClampedBlendInput = GetBlendSpace()->GetClampedAndWrappedBlendInput(GetPosition());
			GetBlendSpace()->GetSamplesFromBlendInput(ClampedBlendInput, BlendSampleDataCache, CachedTriangulationIndex, true);

			const float PosWeight = GetShouldMatchVelocity() ? GetPositionWeight() : 1.0f;
			const float VelWeight = GetShouldMatchVelocity() ? GetVelocityWeight() : 1.0f;
			const float PoseMatchedNormalizedTime = UAnimSuiteMathLibrary::DetermineInitialTime(Context, CurrentSnapshotPose, GetBlendSpace(),
													BlendSampleDataCache, GetSampleRate(), PoseDebugData, GetLoop(), GetShouldMatchVelocity(),
													GetUseOnlyHighestWeightedSampleForPoseMatching(), PosWeight, VelWeight, GetShowDebugShapes());	//@TODO: should DeltaTime be subtracted from this to allow the TickRecord to update it next?
			NormalizedTime = FMath::Clamp(PoseMatchedNormalizedTime, 0.0f, 1.0f);

			bDrawDebugThisFrame = true;
			bIsDistanceMatching = false;
		} break;

		/** If only distance matching is being done. */
		case EMatchingType::DistanceMatch:
		{
			if (CurrentBlendSpace == nullptr)
			{
				UE_LOG(LogAnimation, Warning, TEXT("\"Reinitialize (Blend Space Player: Matching)\": The Blend Space asset is invalid."));
				bIsDistanceMatching = false;
				return;
			}

			/** Get the Blend Sample Data Cache based on input. */
			const FVector ClampedBlendInput = GetBlendSpace()->GetClampedAndWrappedBlendInput(GetPosition()); 
			GetBlendSpace()->GetSamplesFromBlendInput(ClampedBlendInput, BlendSampleDataCache, CachedTriangulationIndex, true);
				
			/** If the Blend Space asset changed, find the time in the switched-to Blend Space as if that asset had been
			    playing in the previous frame. This time will serve as the effective previous play time. This approach
			    allows the proper current time to be found using the input PlayRateClamp. */
			if (bBlendSpaceChanged && !bIsBeingReinitialized)
			{
				/** Get the normalized time for the previous update, as if the newly switched-to Blend Space were playing in the
				    previous update. This is necessary for playrate clamping to work without hindering the play time of the
				    new asset. */
				float InPrevMatchingDistance = PreviousMatchingDistance;
				InPrevMatchingDistance = (GetNegateDistanceValue() && InPrevMatchingDistance > 0.0f) ? -InPrevMatchingDistance : InPrevMatchingDistance;
				const float DistanceMatchedPrevNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InPrevMatchingDistance, 0.0f,
															BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector,
															GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());

				/** Get the normalized time for the current update. */
				float InCurrentMatchingDistance = GetMatchingDistance();
				InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? - InCurrentMatchingDistance : InCurrentMatchingDistance;
				NormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InCurrentMatchingDistance, DistanceMatchedPrevNormalizedTime,
															BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
															GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
			}
			else // bIsBeingReinitialized == true
			{
				/** If the Start Distance is nonzero (and the PlayRateClamp is positive), find the normalized time corresponding
				    to that distance and treat it as the "previous time" from which to determine the current NormalizedTime value
				    (since the allowed NormalizedDeltaTime is determine by the previous time and the playrate clamp). */
				if (GetStartDistance() != 0.0f && GetPlayRateClamp().X > 0.0f && GetPlayRateClamp().Y > 0.0f)
				{
					/** Get the normalized time used as the "previous" normalized time. */
					const float InitialMatchingDistance = (GetNegateDistanceValue() && GetStartDistance() > 0.0f) ? -GetStartDistance() : GetStartDistance();
					const float DistanceMatchedPrevNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InitialMatchingDistance, 0.0f,
															BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector,
															GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());

					/** Get the normalized time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? - InCurrentMatchingDistance : InCurrentMatchingDistance;
					NormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InCurrentMatchingDistance, DistanceMatchedPrevNormalizedTime,
																BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
																GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
				}
				else
				{
					//@TODO: This is a test. And for some reason it works... Weird. Why does the previous time need to be calculated here? Not doing so results in bad starting times.
					/** Get the normalized time used as the "previous" normalized time. */
					float InPrevMatchingDistance = GetMatchingDistance();
					InPrevMatchingDistance = (GetNegateDistanceValue() && InPrevMatchingDistance > 0.0f) ? -InPrevMatchingDistance : InPrevMatchingDistance;
					const float DistanceMatchedPrevNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InPrevMatchingDistance, 0.0f,
															BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector,
															GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
		
					/** Get the normalized time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? - InCurrentMatchingDistance : InCurrentMatchingDistance;
					NormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InCurrentMatchingDistance, DistanceMatchedPrevNormalizedTime,
																BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
																GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
					
				}
			}
				
			bIsDistanceMatching = true;
		} break;

		/** If pose matching AND distance matching are occuring. */
		case EMatchingType::PoseAndDistanceMatch:
		{
			if (CurrentBlendSpace == nullptr)
			{
				UE_LOG(LogAnimation, Warning, TEXT("\"Reinitialize (Blend Space Player: Matching)\": The Blend Space asset is invalid."));
				bIsDistanceMatching = false;
				return;
			}
				
			/** Get the Blend Sample Data Cache based on input. */
			const FVector ClampedBlendInput = GetBlendSpace()->GetClampedAndWrappedBlendInput(GetPosition());
			GetBlendSpace()->GetSamplesFromBlendInput(ClampedBlendInput, BlendSampleDataCache, CachedTriangulationIndex, true);

			/** Get the pose-matched normalized time. */
			const float PosWeight = GetShouldMatchVelocity() ? GetPositionWeight() : 1.0f;
			const float VelWeight = GetShouldMatchVelocity() ? GetVelocityWeight() : 1.0f;
			const float PoseMatchedNormalizedTime = UAnimSuiteMathLibrary::DetermineInitialTime(Context, CurrentSnapshotPose, GetBlendSpace(),
													BlendSampleDataCache, GetSampleRate(), PoseDebugData, GetLoop(), GetShouldMatchVelocity(),
													GetUseOnlyHighestWeightedSampleForPoseMatching(), PosWeight, VelWeight, GetShowDebugShapes());

			/** If the Blend Space asset changed, find the time in the switched-to Blend Space as if that asset had been
				playing in the previous frame. This time will serve as the effective previous play time. This approach
				allows the proper current time to be found using the input PlayRateClamp. */
			float DistanceMatchedNormalizedTime;
			if (bBlendSpaceChanged && !bIsBeingReinitialized)
			{
				/** Get the normalized time for the previous update, as if the newly switched-to Blend Space were playing in the
					previous update. This is necessary for playrate clamping to work without hindering the play time of the
					new asset. */
				float InPrevMatchingDistance = PreviousMatchingDistance;
				InPrevMatchingDistance = (GetNegateDistanceValue() && InPrevMatchingDistance > 0.0f) ? -InPrevMatchingDistance : InPrevMatchingDistance;
				const float DistanceMatchedPrevNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InPrevMatchingDistance, 0.0f,
															BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector,
															GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());

				/** Get the normalized time for the current update. */
				float InCurrentMatchingDistance = GetMatchingDistance();
				InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? - InCurrentMatchingDistance : InCurrentMatchingDistance;
				DistanceMatchedNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InCurrentMatchingDistance, DistanceMatchedPrevNormalizedTime,
															BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
															GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
			}
			else // bIsBeingReinitialized == true
			{
				/** If the Start Distance is nonzero (and the PlayRateClamp is positive), find the normalized time corresponding
				    to that distance and treat it as the "previous time" from which to determine the current NormalizedTime value
				    (since the allowed NormalizedDeltaTime is determine by the previous time and the playrate clamp). */
				if (GetStartDistance() != 0.0f && GetPlayRateClamp().X > 0.0f && GetPlayRateClamp().Y > 0.0f)
				{
					/** Get the normalized time used as the "previous" normalized time. */
					const float InitialMatchingDistance = (GetNegateDistanceValue() && GetStartDistance() > 0.0f) ? -GetStartDistance() : GetStartDistance();
					const float DistanceMatchedPrevNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InitialMatchingDistance, 0.0f,
															BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector,
															GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());

					/** Get the normalized time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? - InCurrentMatchingDistance : InCurrentMatchingDistance;
					DistanceMatchedNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InCurrentMatchingDistance, DistanceMatchedPrevNormalizedTime,
																BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
																GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
				}
				else
				{
					/** Get the normalized time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? - InCurrentMatchingDistance : InCurrentMatchingDistance;
					DistanceMatchedNormalizedTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(GetBlendSpace(), InCurrentMatchingDistance, NormalizedTime,
																BlendSampleDataCache, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),
																GetShouldAdvanceTimeNaturally(), GetUseOnlyHighestWeightedSampleForDistanceMatching(), GetLoop());
				}
			}
				
			/** If the time acquired by pose matching is less than the time acquired by distance matching, use the pose-matched time. (Otherwise, the
			    animation will be played in reverse on the next frame.) */
			bIsDistanceMatching = PoseMatchedNormalizedTime > DistanceMatchedNormalizedTime;
			NormalizedTime = bIsDistanceMatching ? DistanceMatchedNormalizedTime : PoseMatchedNormalizedTime;
				
			/** Trigger smoothing, but only if the pose-matched time needs to "catch up to" the distance-matched time. We don't want the catch-up to be too fast,
			    since that will cause snapping, so smoothing is implemented (if allowed by input settings). */
			bTriggerSmoothTimeFollowingPoseMatch = !bIsDistanceMatching && GetShouldSmoothTimeFollowingPoseMatch();

			/** Only draw debug shapes if the pose-matched time is used. */
			bDrawDebugThisFrame = !bIsDistanceMatching;
			
		} break;
	}	
	
	if (CurrentBlendSpace != nullptr)
	{
		CurrentBlendSpace->InitializeFilter(&BlendFilter);
	}
			
	/** Reset the initialization "flag" to prevent this scope from being entered at the incorrect time. */
	bIsBeingReinitialized = false;
}

UAnimationAsset* FAnimNode_BlendSpacePlayerMatcher::GetAnimAsset() const
{
	return GetBlendSpace();
}

FName FAnimNode_BlendSpacePlayerMatcher::GetGroupName() const
{
	return (bWasReinitialized && GetMatchingType() != EMatchingType::None) ? NAME_None : GET_ANIM_NODE_DATA(FName, GroupName);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetGroupName(FName InGroupName)
{
#if WITH_EDITORONLY_DATA
	GroupName = InGroupName;
#endif

	if(FName* GroupNamePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(FName, GroupName))
	{
		*GroupNamePtr = InGroupName;
		return true;
	}

	return false;
}

EAnimGroupRole::Type FAnimNode_BlendSpacePlayerMatcher::GetGroupRole() const
{
	return GET_ANIM_NODE_DATA(TEnumAsByte<EAnimGroupRole::Type>, GroupRole);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetGroupRole(EAnimGroupRole::Type InRole)
{
#if WITH_EDITORONLY_DATA
	GroupRole = InRole;
#endif
	
	if(TEnumAsByte<EAnimGroupRole::Type>* GroupRolePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(TEnumAsByte<EAnimGroupRole::Type>, GroupRole))
	{
		*GroupRolePtr = InRole;
		return true;
	}

	return false;
}

EAnimSyncMethod FAnimNode_BlendSpacePlayerMatcher::GetGroupMethod() const
{
	return (bWasReinitialized && GetMatchingType() != EMatchingType::None) ? EAnimSyncMethod::DoNotSync : GET_ANIM_NODE_DATA(EAnimSyncMethod, Method);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetGroupMethod(EAnimSyncMethod InMethod)
{
#if WITH_EDITORONLY_DATA
	Method = InMethod;
#endif

	if(EAnimSyncMethod* MethodPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(EAnimSyncMethod, Method))
	{
		*MethodPtr = InMethod;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetIgnoreForRelevancyTest() const
{
	return GET_ANIM_NODE_DATA(bool, bIgnoreForRelevancyTest);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest)
{
#if WITH_EDITORONLY_DATA
	bIgnoreForRelevancyTest = bInIgnoreForRelevancyTest;
#endif

	if(bool* bIgnoreForRelevancyTestPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bIgnoreForRelevancyTest))
	{
		*bIgnoreForRelevancyTestPtr = bInIgnoreForRelevancyTest;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetTimeFromEnd(float CurrentTime) const
{
	/** Blend Spaces use a normalized time value. */
	const float PlayLength = 1.0f;
	return GetBlendSpace() != nullptr ? PlayLength - CurrentTime : 0.0f;
}

void FAnimNode_BlendSpacePlayerMatcher::SnapToPosition(const FVector& NewPosition)
{
	const int32 NumAxis = FMath::Min(BlendFilter.FilterPerAxis.Num(), 3);
	for (int32 Idx = 0; Idx < NumAxis; Idx++)
	{
		BlendFilter.FilterPerAxis[Idx].SetToValue(static_cast<float>(NewPosition[Idx]));
	}
}

UBlendSpace* FAnimNode_BlendSpacePlayerMatcher::GetBlendSpace() const
{
	return BlendSpace;
}

FVector FAnimNode_BlendSpacePlayerMatcher::GetPosition() const
{
	return FVector(GET_ANIM_NODE_DATA(float, X), GET_ANIM_NODE_DATA(float, Y), 0.0f);
}

float FAnimNode_BlendSpacePlayerMatcher::GetStartPosition() const
{
	return GET_ANIM_NODE_DATA(float, StartPosition);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetStartPosition(float InStartPosition)
{
#if WITH_EDITORONLY_DATA
	StartPosition = InStartPosition;
	GET_MUTABLE_ANIM_NODE_DATA(float, StartPosition) = InStartPosition;
#endif

	if (float* StartPositionPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, StartPosition))
	{
		*StartPositionPtr = InStartPosition;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetPlayRate() const
{
	/** Modifying the PlayRate for pose matching causes playback issues, but NOT modifying the PlayRate during distance
	    matching causes playback issues. */
	return bIsDistanceMatching ? 0.0f : GET_ANIM_NODE_DATA(float, PlayRate);
}

bool FAnimNode_BlendSpacePlayerMatcher::GetLoop() const
{
	return GET_ANIM_NODE_DATA(bool, bLoop);
}

bool FAnimNode_BlendSpacePlayerMatcher::GetResetPlayTimeWhenBlendSpaceChanges() const
{
	return GET_ANIM_NODE_DATA(bool, bResetPlayTimeWhenBlendSpaceChanges);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetResetPlayTimeWhenBlendSpaceChanges(bool bInReset)
{
#if WITH_EDITORONLY_DATA
	bResetPlayTimeWhenBlendSpaceChanges = bInReset;
#endif

	if (bool* bResetPlayTimeWhenBlendSpaceChangesPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bResetPlayTimeWhenBlendSpaceChanges))
	{
		*bResetPlayTimeWhenBlendSpaceChangesPtr = bInReset;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::SetBlendSpace(UBlendSpace* InBlendSpace)
{
	BlendSpace = InBlendSpace;
	return true;
}

bool FAnimNode_BlendSpacePlayerMatcher::SetPosition(FVector InPosition)
{
#if WITH_EDITORONLY_DATA
	X = static_cast<float>(InPosition[0]);
	Y = static_cast<float>(InPosition[1]);
	GET_MUTABLE_ANIM_NODE_DATA(float, X) = static_cast<float>(InPosition[0]);
	GET_MUTABLE_ANIM_NODE_DATA(float, Y) = static_cast<float>(InPosition[1]);
#endif

	float* XPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, X);
	float* YPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, Y);

	if (XPtr && YPtr)
	{
		*XPtr = static_cast<float>(InPosition[0]);
		*YPtr = static_cast<float>(InPosition[1]);
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::SetPlayRate(float InPlayRate)
{
#if WITH_EDITORONLY_DATA
	PlayRate = InPlayRate;
	GET_MUTABLE_ANIM_NODE_DATA(float, PlayRate) = InPlayRate;
#endif

	if (float* PlayRatePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, PlayRate))
	{
		*PlayRatePtr = InPlayRate;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::SetLoop(bool bInLoop)
{
#if WITH_EDITORONLY_DATA
	bLoop = bInLoop;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bLoop) = bInLoop;
#endif

	if (bool* bLoopPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bLoop))
	{
		*bLoopPtr = bInLoop;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::IsEvaluator() const
{
	return bTeleportToNormalizedTime;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetShouldInertiallyBlendUponBlendSpaceChange() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldInertiallyBlendUponBlendSpaceChange);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetShouldInertiallyBlendUponBlendSpaceChange(bool bInShouldInertiallyBlendUponBlendSpaceChange)
{
#if WITH_EDITORONLY_DATA
	bShouldInertiallyBlendUponBlendSpaceChange = bInShouldInertiallyBlendUponBlendSpaceChange;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bShouldInertiallyBlendUponBlendSpaceChange) = bInShouldInertiallyBlendUponBlendSpaceChange;
#endif

	if (bool* bShouldInertiallyBlendUponBlendSpaceChangePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bShouldInertiallyBlendUponBlendSpaceChange))
	{
		*bShouldInertiallyBlendUponBlendSpaceChangePtr = bInShouldInertiallyBlendUponBlendSpaceChange;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetShouldInertiallyBlendUponRapidAxisValueChange() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldInertiallyBlendUponRapidAxisValueChange);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetShouldInertiallyBlendUponRapidAxisValueChange(
	bool bInShouldInertiallyBlendUponRapidAxisValueChange)
{
#if WITH_EDITORONLY_DATA
	bShouldInertiallyBlendUponRapidAxisValueChange = bInShouldInertiallyBlendUponRapidAxisValueChange;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bShouldInertiallyBlendUponRapidAxisValueChange) = bInShouldInertiallyBlendUponRapidAxisValueChange;
#endif

	if (bool* bShouldInertiallyBlendUponRapidAxisValueChangePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bShouldInertiallyBlendUponRapidAxisValueChange))
	{
		*bShouldInertiallyBlendUponRapidAxisValueChangePtr = bInShouldInertiallyBlendUponRapidAxisValueChange;
		return true;
	}

	return false;
}

FVector2D FAnimNode_BlendSpacePlayerMatcher::GetBlendTriggerThresholdDeltas() const
{
	return GET_ANIM_NODE_DATA(FVector2D, BlendTriggerThresholdDeltas);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetBlendTriggerThresholdDeltas(FVector2D InBlendTriggerThresholdDeltas)
{
#if WITH_EDITORONLY_DATA
	BlendTriggerThresholdDeltas = InBlendTriggerThresholdDeltas;
	GET_MUTABLE_ANIM_NODE_DATA(FVector2D, BlendTriggerThresholdDeltas) = InBlendTriggerThresholdDeltas;
#endif

	if (FVector2D* BlendTriggerThresholdDeltasPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(FVector2D, BlendTriggerThresholdDeltas))
	{
		*BlendTriggerThresholdDeltasPtr = InBlendTriggerThresholdDeltas;
		return true;
	}

	return false;
}


float FAnimNode_BlendSpacePlayerMatcher::GetInertialBlendTime() const
{
	return GET_ANIM_NODE_DATA(float, InertialBlendTime);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetInertialBlendTime(float InInertialBlendTime)
{
#if WITH_EDITORONLY_DATA
	InertialBlendTime = InInertialBlendTime;
	GET_MUTABLE_ANIM_NODE_DATA(float, InertialBlendTime) = InInertialBlendTime;
#endif

	if (float* InertialBlendTimePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, InertialBlendTime))
	{
		*InertialBlendTimePtr = InInertialBlendTime;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetSampleRate() const
{
	return GET_ANIM_NODE_DATA(float, SampleRate);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetSampleRate(float InSampleRate)
{
#if WITH_EDITORONLY_DATA
	SampleRate = InSampleRate;
	GET_MUTABLE_ANIM_NODE_DATA(float, SampleRate) = InSampleRate;
#endif
	
	if(float* SampleRatePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, SampleRate))
	{
		*SampleRatePtr = InSampleRate;
		return true;
	}

	return false;
}

const FBlendSampleData* FAnimNode_BlendSpacePlayerMatcher::GetHighestWeightedSample() const
{
	if(BlendSampleDataCache.Num() == 0)
	{
		return nullptr;
	}

	const FBlendSampleData* HighestSample = &BlendSampleDataCache[0];

	for(int32 Idx = 1; Idx < BlendSampleDataCache.Num(); ++Idx)
	{
		if(BlendSampleDataCache[Idx].TotalWeight > HighestSample->TotalWeight)
		{
			HighestSample = &BlendSampleDataCache[Idx];
		}
	}

	return HighestSample;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetShouldMatchVelocity() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldMatchVelocity);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetShouldMatchVelocity(bool bInShouldMatchVelocity)
{
#if WITH_EDITORONLY_DATA
	bShouldMatchVelocity = bInShouldMatchVelocity;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bShouldMatchVelocity) = bInShouldMatchVelocity;
#endif

	if(bool* bShouldMatchVelocityPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bShouldMatchVelocity))
	{
		*bShouldMatchVelocityPtr = bInShouldMatchVelocity;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetPositionWeight() const
{
	return GET_ANIM_NODE_DATA(float, PositionWeight);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetPositionWeight(float InPositionWeight)
{
#if WITH_EDITORONLY_DATA
	PositionWeight = InPositionWeight;
	GET_MUTABLE_ANIM_NODE_DATA(float, PositionWeight) = InPositionWeight;
#endif
	
	if(float* PositionWeightPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, PositionWeight))
	{
		*PositionWeightPtr = InPositionWeight;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetVelocityWeight() const
{
	return GET_ANIM_NODE_DATA(float, VelocityWeight);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetVelocityWeight(float InVelocityWeight)
{
#if WITH_EDITORONLY_DATA
	VelocityWeight = InVelocityWeight;
	GET_MUTABLE_ANIM_NODE_DATA(float, VelocityWeight) = InVelocityWeight;
#endif
	
	if(float* VelocityWeightPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, VelocityWeight))
	{
		*VelocityWeightPtr = InVelocityWeight;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetShowDebugShapes() const
{
	return GET_ANIM_NODE_DATA(bool, bShowDebugShapes);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetShowDebugShapes(bool bInShowDebugShapes)
{
#if WITH_EDITORONLY_DATA
	bShowDebugShapes = bInShowDebugShapes;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bShowDebugShapes) = bInShowDebugShapes;
#endif

	if(bool* bShowDebugShapesPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bShowDebugShapes))
	{
		*bShowDebugShapesPtr = bInShowDebugShapes;
		return true;
	}

	return false;
}

EDebugPoseMatchLevel FAnimNode_BlendSpacePlayerMatcher::GetDebugLevel() const
{
	return GET_ANIM_NODE_DATA(EDebugPoseMatchLevel, DebugLevel);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetDebugLevel(EDebugPoseMatchLevel InDebugLevel)
{
#if WITH_EDITORONLY_DATA
	DebugLevel = InDebugLevel;
	GET_MUTABLE_ANIM_NODE_DATA(EDebugPoseMatchLevel, DebugLevel) = InDebugLevel;
#endif

	if(EDebugPoseMatchLevel* DebugLevelPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(EDebugPoseMatchLevel, DebugLevel))
	{
		*DebugLevelPtr = InDebugLevel;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetPositionDrawScale() const
{
	return GET_ANIM_NODE_DATA(float, PositionDrawScale);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetPositionDrawScale(float InPositionDrawScale)
{
#if WITH_EDITORONLY_DATA
	PositionDrawScale = InPositionDrawScale;
	GET_MUTABLE_ANIM_NODE_DATA(float, PositionDrawScale) = InPositionDrawScale;
#endif
	
	if(float* PositionDrawScalePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, PositionDrawScale))
	{
		*PositionDrawScalePtr = InPositionDrawScale;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetVelocityDrawScale() const
{
	return GET_ANIM_NODE_DATA(float, VelocityDrawScale);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetVelocityDrawScale(float InVelocityDrawScale)
{
#if WITH_EDITORONLY_DATA
	VelocityDrawScale = InVelocityDrawScale;
	GET_MUTABLE_ANIM_NODE_DATA(float, VelocityDrawScale) = InVelocityDrawScale;
#endif
	
	if(float* VelocityDrawScalePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, VelocityDrawScale))
	{
		*VelocityDrawScalePtr = InVelocityDrawScale;
		return true;
	}

	return false;
}

EMatchingType FAnimNode_BlendSpacePlayerMatcher::GetMatchingType() const
{
	return GET_ANIM_NODE_DATA(EMatchingType, MatchingType);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetMatchingType(EMatchingType InMatchingType)
{
#if WITH_EDITORONLY_DATA
	MatchingType = InMatchingType;
	GET_MUTABLE_ANIM_NODE_DATA(EMatchingType, MatchingType) = InMatchingType;
#endif

	if (EMatchingType* MatchingTypePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(EMatchingType, MatchingType))
	{
		*MatchingTypePtr = InMatchingType;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetMatchingDistance() const
{
	return GET_ANIM_NODE_DATA(float, MatchingDistance);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetMatchingDistance(float InMatchingDistance)
{
#if WITH_EDITORONLY_DATA
	MatchingDistance = InMatchingDistance;
	GET_MUTABLE_ANIM_NODE_DATA(float, MatchingDistance) = InMatchingDistance;
#endif

	if (float* MatchingDistancePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, MatchingDistance))
	{
		*MatchingDistancePtr = InMatchingDistance;
		return true;
	}

	return false;
}

FName FAnimNode_BlendSpacePlayerMatcher::GetDistanceCurveName() const
{
	return FName(GET_ANIM_NODE_DATA(FName, DistanceCurveName));
}

bool FAnimNode_BlendSpacePlayerMatcher::SetDistanceCurveName(FName InDistanceCurveName)
{
#if WITH_EDITORONLY_DATA
	DistanceCurveName = InDistanceCurveName;
	GET_MUTABLE_ANIM_NODE_DATA(FName, DistanceCurveName) = InDistanceCurveName;
#endif

	if (FName* bDistanceCurveNamePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(FName, DistanceCurveName))
	{
		*bDistanceCurveNamePtr = InDistanceCurveName;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetShouldAdvanceTimeNaturally() const
{
	return GET_ANIM_NODE_DATA(bool, bAdvanceTimeNaturally);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetShouldAdvanceTimeNaturally(bool bInAdvanceTimeNaturally)
{
#if WITH_EDITORONLY_DATA
	bAdvanceTimeNaturally = bInAdvanceTimeNaturally;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bAdvanceTimeNaturally) = bInAdvanceTimeNaturally;
#endif

	if (bool* bAdvanceTimeNaturallyPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bAdvanceTimeNaturally))
	{
		*bAdvanceTimeNaturallyPtr = bInAdvanceTimeNaturally;
		return true;
	}

	return false;
}

FVector2D FAnimNode_BlendSpacePlayerMatcher::GetPlayRateClamp() const
{
	return FVector2D(GET_ANIM_NODE_DATA(FVector2D, PlayRateClamp));
}

bool FAnimNode_BlendSpacePlayerMatcher::SetPlayRateClamp(FVector2D InPlayRateClamp)
{
#if WITH_EDITORONLY_DATA
	PlayRateClamp = InPlayRateClamp;
	GET_MUTABLE_ANIM_NODE_DATA(FVector2D, PlayRateClamp) = InPlayRateClamp;
#endif

	if (FVector2D* PlayRateClampPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(FVector2D, PlayRateClamp))
	{
		*PlayRateClampPtr = InPlayRateClamp;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetNegateDistanceValue() const
{
	return GET_ANIM_NODE_DATA(bool, bNegateDistanceValue);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetNegateDistanceValue(bool bInNegateDistanceValue)
{
#if WITH_EDITORONLY_DATA
	bNegateDistanceValue = bInNegateDistanceValue;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bNegateDistanceValue) = bInNegateDistanceValue;
#endif

	if (bool* bInNegateDistanceValuePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bNegateDistanceValue))
	{
		*bInNegateDistanceValuePtr = bInNegateDistanceValue;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetStartDistance() const
{
	return GET_ANIM_NODE_DATA(float, StartDistance);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetStartDistance(float InStartDistance)
{
#if WITH_EDITORONLY_DATA
	StartDistance = InStartDistance;
	GET_MUTABLE_ANIM_NODE_DATA(float, StartDistance) = InStartDistance;
#endif

	if (float* StartDistancePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, StartDistance))
	{
		*StartDistancePtr = InStartDistance;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetShouldSmoothTimeFollowingPoseMatch() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldSmoothTimeFollowingPoseMatch);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetShouldSmoothTimeFollowingPoseMatch(bool bInShouldSmoothTimeFollowingPoseMatch)
{
#if WITH_EDITORONLY_DATA
	bShouldSmoothTimeFollowingPoseMatch = bInShouldSmoothTimeFollowingPoseMatch;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bShouldSmoothTimeFollowingPoseMatch) = bInShouldSmoothTimeFollowingPoseMatch;
#endif

	if (bool* bInShouldSmoothTimeFollowingPoseMatchPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bShouldSmoothTimeFollowingPoseMatch))
	{
		*bInShouldSmoothTimeFollowingPoseMatchPtr = bInShouldSmoothTimeFollowingPoseMatch;
		return true;
	}

	return false;
}

float FAnimNode_BlendSpacePlayerMatcher::GetSmoothingAlpha() const
{
	return GET_ANIM_NODE_DATA(float, SmoothingAlpha);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetSmoothingAlpha(float InSmoothingAlpha)
{
#if WITH_EDITORONLY_DATA
	SmoothingAlpha = InSmoothingAlpha;
	GET_MUTABLE_ANIM_NODE_DATA(float, SmoothingAlpha) = InSmoothingAlpha;
#endif

	if (float* SmoothingAlphaPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, SmoothingAlpha))
	{
		*SmoothingAlphaPtr = InSmoothingAlpha;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetShouldInertiallyBlendFrameToFrame() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldInertiallyBlendFrameToFrame);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetShouldInertiallyBlendFrameToFrame(bool bInShouldInertiallyBlendFrameToFrame)
{
#if WITH_EDITORONLY_DATA
	bShouldInertiallyBlendFrameToFrame = bInShouldInertiallyBlendFrameToFrame;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bShouldInertiallyBlendFrameToFrame) = bInShouldInertiallyBlendFrameToFrame;
#endif

	if (bool* bInbShouldInertiallyBlendFrameToFramePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bShouldInertiallyBlendFrameToFrame))
	{
		*bInbShouldInertiallyBlendFrameToFramePtr = bInShouldInertiallyBlendFrameToFrame;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetUseOnlyHighestWeightedSampleForDistanceMatching() const
{
	return GET_ANIM_NODE_DATA(bool, bUseOnlyHighestWeightedSampleForDistanceMatching);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetUseOnlyHighestWeightedSampleForDistanceMatching(bool bInUseOnlyHighestWeightedSampleForDistanceMatching)
{
#if WITH_EDITORONLY_DATA
	bUseOnlyHighestWeightedSampleForDistanceMatching = bInUseOnlyHighestWeightedSampleForDistanceMatching;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bUseOnlyHighestWeightedSampleForDistanceMatching) = bInUseOnlyHighestWeightedSampleForDistanceMatching;
#endif

	if (bool* bInUseOnlyHighestWeightedSampleForDistanceMatchingPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bUseOnlyHighestWeightedSampleForDistanceMatching))
	{
		*bInUseOnlyHighestWeightedSampleForDistanceMatchingPtr = bInUseOnlyHighestWeightedSampleForDistanceMatching;
		return true;
	}

	return false;
}

bool FAnimNode_BlendSpacePlayerMatcher::GetUseOnlyHighestWeightedSampleForPoseMatching() const
{
	return GET_ANIM_NODE_DATA(bool, bUseOnlyHighestWeightedSampleForPoseMatching);
}

bool FAnimNode_BlendSpacePlayerMatcher::SetUseOnlyHighestWeightedSampleForPoseMatching(
	bool bInUseOnlyHighestWeightedSampleForPoseMatching)
{
#if WITH_EDITORONLY_DATA
	bUseOnlyHighestWeightedSampleForPoseMatching = bInUseOnlyHighestWeightedSampleForPoseMatching;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bUseOnlyHighestWeightedSampleForPoseMatching) = bInUseOnlyHighestWeightedSampleForPoseMatching;
#endif

	if (bool* bInUseOnlyHighestWeightedSampleForPoseMatchingPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bUseOnlyHighestWeightedSampleForPoseMatching))
	{
		*bInUseOnlyHighestWeightedSampleForPoseMatchingPtr = bInUseOnlyHighestWeightedSampleForPoseMatching;
		return true;
	}

	return false;
}

UBlendProfile* FAnimNode_BlendSpacePlayerMatcher::GetBlendProfile() const
{
	return BlendProfile;
}


#undef LOCTEXT_NAMESPACE















