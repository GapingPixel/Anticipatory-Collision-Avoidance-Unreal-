// Copyright MuuKnighted Games 2024. All rights reserved.

#include "AnimGraph/AnimNode_SequencePlayerMatcher.h"

#include "Animation/AnimMontage.h"
#include "Utility/AnimSuiteMathLibrary.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimNode_Inertialization.h"
#include "Animation/ExposedValueHandler.h"
#include "Logging/TokenizedMessage.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_SequencePlayerMatcher)

#if WITH_EDITORONLY_DATA
#include "Animation/AnimBlueprintGeneratedClass.h"
#endif

#define LOCTEXT_NAMESPACE "AnimationMatchingSuiteNodes"

FAnimNode_SequencePlayerMatcher::FAnimNode_SequencePlayerMatcher()
	: PreviousSequence(nullptr)
	, BlendProfile(nullptr)
	, Sequence(nullptr)
	, bSequenceChanged(false)
	, bIsBeingReinitialized(true)
	, bWasReinitialized(false)
	, ExplicitTime(0.0f)
	, PrevExplicitTime(0.0f)
	, PreviousMatchingDistance(0.0f)
	, bTriggerSmoothTimeFollowingPoseMatch(false)
	, bTeleportToExplicitTime(false)
	, bIsDistanceMatching(false)
	, PoseDebugData(FAMSDebugData())
	, bDrawDebugThisFrame(false)
	, DeltaTime(UE_SMALL_NUMBER)
{
}

void FAnimNode_SequencePlayerMatcher::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread);

	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
	
	bIsBeingReinitialized = true;

}

void FAnimNode_SequencePlayerMatcher::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	DeltaTime = Context.GetDeltaTime(); 
	
	GetEvaluateGraphExposedInputs().Execute(Context);

	UpdateInternal(Context);
}

void FAnimNode_SequencePlayerMatcher::Evaluate_AnyThread(FPoseContext& Output)
{
	FAnimNode_SequencePlayerBase::Evaluate_AnyThread(Output);
}

void FAnimNode_SequencePlayerMatcher::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateInternal)

	PrevExplicitTime = ExplicitTime;
	
	TObjectPtr<UAnimSequenceBase> CurrentSequenceBase = GetSequence();
	if (CurrentSequenceBase && !ensureMsgf(!CurrentSequenceBase->IsA<UAnimMontage>(), TEXT("Sequence players do not support anim montages.")))
	{
		CurrentSequenceBase = nullptr;
	}

	if (CurrentSequenceBase != nullptr && CurrentSequenceBase->GetSkeleton() != nullptr)
	{
		/** If the Sequence changed and inertial blending should be requested, request inertial blending. */
		bSequenceChanged = CurrentSequenceBase != PreviousSequence;
		if (bIsBeingReinitialized == false && bSequenceChanged && GetShouldInertiallyBlendUponSequenceChange() && GetInertialBlendTime() > 0.0f)
		{
			UE::Anim::IInertializationRequester* InertializationRequester = Context.GetMessage<UE::Anim::IInertializationRequester>();
			if (InertializationRequester)
			{
				InertializationRequester->RequestInertialization(GetInertialBlendTime() , GetBlendProfile());
				InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
			}
		}
	 
		const bool bHadJustBeenInitialized = bIsBeingReinitialized;

		/** If the Sequence has changed or this node has been initialized, perform reinitialization. */
		if (bSequenceChanged || bIsBeingReinitialized)
		{
			Reinitialize(Context);
			InternalTimeAccumulator = ExplicitTime;
		}
		
		/** If reinitialization is not necessary, but distance matching is required, perform distance matching. */
		else if ((GetMatchingType() == EMatchingType::DistanceMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch) && !GetShouldAdvanceTimeNaturally())
		{
			/** Negate the matching distance value, if applicable. */
			float InMatchingDistance = GetMatchingDistance();
			InMatchingDistance = (GetNegateDistanceValue() && InMatchingDistance > 0.0f) ? -InMatchingDistance : InMatchingDistance;
		
			/** Set the internal time to be the clamped explicit time. Apply smoothing, if applicable.
				(Calling the GetShouldSmoothTimeFollowingPoseMatch function again here as a condition is done to account 
				for the case of the user disabling smoothing at runtime in the middle of distance matching.) */
			if (bTriggerSmoothTimeFollowingPoseMatch && GetShouldSmoothTimeFollowingPoseMatch() && !GetShouldAdvanceTimeNaturally())
			{
				const float DesiredExplicitTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequenceBase, InMatchingDistance,
								ExplicitTime, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(), GetShouldAdvanceTimeNaturally(),
								IsLooping());
		
				/** Linearly interpolate. */
				ExplicitTime = ExplicitTime + (1 - GetSmoothingAlpha()) * (DesiredExplicitTime - ExplicitTime);
				InternalTimeAccumulator = ExplicitTime;
			}
			else
			{
				// ExplicitTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequenceBase, InMatchingDistance,
				// 				ExplicitTime, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(), GetShouldAdvanceTimeNaturally());
				const float DesiredExplicitTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequenceBase, InMatchingDistance,
								ExplicitTime, Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(), GetShouldAdvanceTimeNaturally(),
								IsLooping());
				
				/** Linearly interpolate. */
				ExplicitTime = ExplicitTime + (1 - GetSmoothingAlpha()) * (DesiredExplicitTime - ExplicitTime);
				InternalTimeAccumulator = ExplicitTime;
			}
			
			bIsDistanceMatching = true;
		}
		else if ((GetMatchingType() == EMatchingType::DistanceMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch) && GetShouldAdvanceTimeNaturally())
		{
			bIsDistanceMatching = false;
		}
		
		/** Override key values of the Tick Record depending on the matching type and whether reinitialization occured. */
		bWasReinitialized = (bHadJustBeenInitialized != bIsBeingReinitialized) || bSequenceChanged;
		bTeleportToExplicitTime = (bWasReinitialized && GetMatchingType() != EMatchingType::None) || GetMatchingType() == EMatchingType::DistanceMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch;

		// UE_LOG(LogTemp, Warning, TEXT("bTeleportToExplicitTime = %hhd"),bTeleportToExplicitTime);
		// UE_LOG(LogTemp, Warning, TEXT("Time Remaining: %f"), GetCurrentAssetLength() - GetCurrentAssetTimePlayRateAdjusted());
		
		const float AdjustedPlayRate = bIsDistanceMatching
										? 0.0f
										: PlayRateScaleBiasClampState.ApplyTo(GetPlayRateScaleBiasClampConstants(), FMath::IsNearlyZero(GetPlayRateBasis()) ? 0.f : (GetPlayRate() / GetPlayRateBasis()), Context.GetDeltaTime());
		CreateTickRecordForNode(Context, CurrentSequenceBase, IsLooping(), AdjustedPlayRate, IsEvaluator());
		
	}

#if WITH_EDITORONLY_DATA
	if (FAnimBlueprintDebugData* DebugData = Context.AnimInstanceProxy->GetAnimBlueprintDebugData())
	{
		DebugData->RecordSequencePlayer(Context.GetCurrentNodeId(), GetAccumulatedTime(), CurrentSequenceBase != nullptr
									? CurrentSequenceBase->GetPlayLength()
									: 0.0f, CurrentSequenceBase != nullptr ? CurrentSequenceBase->GetNumberOfSampledKeys() : 0);
	}
#endif

	/** Cache and reset essential info.  */
	PreviousMatchingDistance = GetMatchingDistance();
	bSequenceChanged = false;
	PreviousSequence = CurrentSequenceBase;

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
	
	TRACE_ANIM_SEQUENCE_PLAYER(Context, *this);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Name"), CurrentSequenceBase != nullptr ? CurrentSequenceBase->GetFName() : NAME_None);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Sequence"), CurrentSequenceBase);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Playback Time"), InternalTimeAccumulator);
}

void FAnimNode_SequencePlayerMatcher::Reinitialize(const FAnimationUpdateContext& Context)
{
	const TObjectPtr<UAnimSequenceBase> CurrentSequence = GetSequence();

	switch (GetMatchingType())
	{
		/** If no matching is being done. */
		case EMatchingType::None:
		{
			if (CurrentSequence == nullptr)
			{
				UE_LOG(LogAnimation, Warning, TEXT("\"Reinitialize (Sequence Player: Matching)\": The Sequence asset is invalid."));
				bIsDistanceMatching = false;
				return;
			}
				
			if (GetResetPlayTimeWhenSequenceChanges())
			{
				const float CurrentStartPosition = GetStartPosition();
				const float CurrentPlayRate = GetPlayRate();
				const float CurrentPlayRateBasis = GetPlayRateBasis();
				const float AdjustedPlayRate = PlayRateScaleBiasClampState.ApplyTo(GetPlayRateScaleBiasClampConstants(), FMath::IsNearlyZero(CurrentPlayRateBasis) ? 0.f : (CurrentPlayRate / CurrentPlayRateBasis), 0.f);
				const float EffectivePlayrate = CurrentSequence->RateScale * AdjustedPlayRate;
				ExplicitTime = FMath::Clamp(CurrentStartPosition, 0.0f, CurrentSequence->GetPlayLength());
				if (CurrentStartPosition == 0.0f && EffectivePlayrate < 0.0f)
				{
					/** Jump to the end of the Sequence. */
					ExplicitTime = CurrentSequence->GetPlayLength();
				}
			}
			bDrawDebugThisFrame = false;
			bIsDistanceMatching = false;
		} break;

		/** If only pose matching is being done. */
		case EMatchingType::PoseMatch:
		{
			if (CurrentSequence == nullptr)
			{
				UE_LOG(LogAnimation, Warning, TEXT("\"Reinitialize (Sequence Player: Matching)\": The Sequence asset is invalid."));
				bIsDistanceMatching = false;
				return;
			}

			/** Get the pose-matched time. */
			const TObjectPtr<UAnimSequence> AnimSequence = Cast<UAnimSequence>(CurrentSequence);
			const float PosWeight = GetShouldMatchVelocity() ? GetPositionWeight() : 1.0f;
			const float VelWeight = GetShouldMatchVelocity() ? GetVelocityWeight() : 1.0f;
			const float PoseMatchedExplicitTime = UAnimSuiteMathLibrary::DetermineInitialTime(Context, CurrentSnapshotPose, AnimSequence,
											GetSampleRate(),PoseDebugData, IsLooping(), GetShouldMatchVelocity(), PosWeight,
											VelWeight, GetShowDebugShapes(), GetMatchingRange(), GetInitialTime(),
											GetFinalTime());

			ExplicitTime = FMath::Clamp(PoseMatchedExplicitTime, 0.0f, CurrentSequence->GetPlayLength());

			bDrawDebugThisFrame = true;
			bIsDistanceMatching = false;
		} break;

		/** If only distance matching is being done. */
		case EMatchingType::DistanceMatch:
		{
			if (CurrentSequence == nullptr)
			{
				UE_LOG(LogAnimation, Warning, TEXT("\"Reinitialize (Sequence Player: Matching)\": The Sequence asset is invalid."));
				bIsDistanceMatching = false;
				return;
			}

			/** If the Sequence asset changed, find the time in the switched-to Sequence as if that asset had been
				playing in the previous frame. This time will serve as the effective previous play time. This approach
				allows the proper current time to be found using the input PlayRateClamp. */
			if (bSequenceChanged && !bIsBeingReinitialized)
			{
				/** Get the explicit time for the previous update, as if the newly switched-to Sequence were playing in the
					previous update. This is necessary for playrate clamping to work without hindering the play time of the
					new asset. */
				float InPrevMatchingDistance = PreviousMatchingDistance;
				InPrevMatchingDistance = (GetNegateDistanceValue() && InPrevMatchingDistance > 0.0f) ? -InPrevMatchingDistance : InPrevMatchingDistance;
				const float DistanceMatchedPrevTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InPrevMatchingDistance, 0.0f,
											Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector, GetShouldAdvanceTimeNaturally(),
											IsLooping());

				/** Get the explicit time for the current update. */
				float InCurrentMatchingDistance = GetMatchingDistance();
				InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? -InCurrentMatchingDistance : InCurrentMatchingDistance;
				ExplicitTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InCurrentMatchingDistance, DistanceMatchedPrevTime, Context.GetDeltaTime(),
														GetDistanceCurveName(), GetPlayRateClamp(), GetShouldAdvanceTimeNaturally(),
														IsLooping());
			}
			else // bIsBeingReinitialized == true
			{
				/** If the Start Distance is nonzero (and the PlayRateClamp is positive), find the normalized time corresponding
					to that distance and treat it as the "previous time" from which to determine the current NormalizedTime value
					(since the allowed NormalizedDeltaTime is determine by the previous time and the playrate clamp). */
				if (GetStartDistance() != 0.0f && GetPlayRateClamp().X > 0.0f && GetPlayRateClamp().Y > 0.0f)
				{
					/** Get the explicit time used as the "previous" explicit time. */
					const float InitialMatchingDistance = (GetNegateDistanceValue() && GetStartDistance() > 0.0f) ? -GetStartDistance() : GetStartDistance();
					const float DistanceMatchedPrevTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InitialMatchingDistance, 0.0f,
											Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector, GetShouldAdvanceTimeNaturally(),
											IsLooping());

					/** Get the explicit time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? -InCurrentMatchingDistance : InCurrentMatchingDistance;
					ExplicitTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InCurrentMatchingDistance, DistanceMatchedPrevTime,
													Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),GetShouldAdvanceTimeNaturally(),
													IsLooping());
				}
				else
				{
					//@TODO: This is a test. And for some reason it works... Weird. Why does the previous time need to be calculated here? Not doing so results in bad starting times.
					/** Get the explicit time as the "previous" explicit time. */
					float InPrevMatchingDistance = GetMatchingDistance();
					InPrevMatchingDistance = (GetNegateDistanceValue() && InPrevMatchingDistance > 0.0f) ? -InPrevMatchingDistance : InPrevMatchingDistance;
					const float DistanceMatchedPrevTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InPrevMatchingDistance, 0.0f,
													Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector, GetShouldAdvanceTimeNaturally(),
													IsLooping());

					/** Get the explicit time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? -InCurrentMatchingDistance : InCurrentMatchingDistance;
					ExplicitTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InCurrentMatchingDistance, DistanceMatchedPrevTime,
														Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(), GetShouldAdvanceTimeNaturally(),
														IsLooping());
					
				}
			}
			bDrawDebugThisFrame = false;
			bIsDistanceMatching = true;
				
		} break;

		/** If pose matching AND distance matching are occuring. */
		case EMatchingType::PoseAndDistanceMatch:
		{
			if (CurrentSequence == nullptr)
			{
				UE_LOG(LogAnimation, Warning, TEXT("\"Reinitialize (Sequence Player: Matching)\": The Sequence asset is invalid."));
				return;
			}

			/** Get the pose-matched normalized time. */
			const TObjectPtr<UAnimSequence> AnimSequence = Cast<UAnimSequence>(CurrentSequence);
			const float PosWeight = GetShouldMatchVelocity() ? GetPositionWeight() : 1.0f;
			const float VelWeight = GetShouldMatchVelocity() ? GetVelocityWeight() : 1.0f;
			const float PoseMatchedTime = UAnimSuiteMathLibrary::DetermineInitialTime(Context, CurrentSnapshotPose, AnimSequence,
																			GetSampleRate(), PoseDebugData, IsLooping(), GetShouldMatchVelocity(),
																			PosWeight, VelWeight, GetShowDebugShapes(),
																			GetMatchingRange(), GetInitialTime(), GetFinalTime());

				
			/** If the Sequence asset changed, find the time in the switched-to Sequence as if that asset had been
				playing in the previous frame. This time will serve as the effective previous play time. This approach
				allows the proper current time to be found using the input PlayRateClamp. */
			float DistanceMatchedTime;
			if (bSequenceChanged && !bIsBeingReinitialized)
			{
				/** Get the explicit time for the previous update, as if the newly switched-to Sequence were playing in the
					previous update. This is necessary for playrate clamping to work without hindering the play time of the
					new asset. */
				float InPrevMatchingDistance = PreviousMatchingDistance;
				InPrevMatchingDistance = (GetNegateDistanceValue() && InPrevMatchingDistance > 0.0f) ? -InPrevMatchingDistance : InPrevMatchingDistance;
				const float DistanceMatchedPrevTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InPrevMatchingDistance, 0.0f,
											Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector, GetShouldAdvanceTimeNaturally(),
											IsLooping());

				/** Get the explicit time for the current update. */
				float InCurrentMatchingDistance = GetMatchingDistance();
				InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? -InCurrentMatchingDistance : InCurrentMatchingDistance;
				DistanceMatchedTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InCurrentMatchingDistance, DistanceMatchedPrevTime, Context.GetDeltaTime(),
														GetDistanceCurveName(), GetPlayRateClamp(), GetShouldAdvanceTimeNaturally(),
														IsLooping());
			}
			else // bIsBeingReinitialized == true
			{
				/** If the Start Distance is nonzero (and the PlayRateClamp is positive), find the normalized time corresponding
					to that distance and treat it as the "previous time" from which to determine the current NormalizedTime value
					(since the allowed NormalizedDeltaTime is determine by the previous time and the playrate clamp). */
				if (GetStartDistance() != 0.0f && GetPlayRateClamp().X > 0.0f && GetPlayRateClamp().Y > 0.0f)
				{
					/** Get the explicit time used as the "previous" explicit time. */
					const float InitialMatchingDistance = (GetNegateDistanceValue() && GetStartDistance() > 0.0f) ? -GetStartDistance() : GetStartDistance();
					const float DistanceMatchedPrevTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InitialMatchingDistance, 0.0f,
											Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector, GetShouldAdvanceTimeNaturally(),
											IsLooping());

					/** Get the explicit time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? -InCurrentMatchingDistance : InCurrentMatchingDistance;
					DistanceMatchedTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InCurrentMatchingDistance, DistanceMatchedPrevTime,
													Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(),GetShouldAdvanceTimeNaturally(),
													IsLooping());
				}
				else
				{
					//@TODO: This is a test. And for some reason it works... Weird. Why does the previous time need to be calculated here? Not doing so results in bad starting times.
					/** Get the explicit time as the "previous" explicit time. */
					float InPrevMatchingDistance = GetMatchingDistance();
					InPrevMatchingDistance = (GetNegateDistanceValue() && InPrevMatchingDistance > 0.0f) ? -InPrevMatchingDistance : InPrevMatchingDistance;
					const float DistanceMatchedPrevTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InPrevMatchingDistance, 0.0f,
													Context.GetDeltaTime(), GetDistanceCurveName(), FVector2D::ZeroVector, GetShouldAdvanceTimeNaturally(),
													IsLooping());

					/** Get the explicit time for the current update. */
					float InCurrentMatchingDistance = GetMatchingDistance();
					InCurrentMatchingDistance = (GetNegateDistanceValue() && InCurrentMatchingDistance > 0.0f) ? -InCurrentMatchingDistance : InCurrentMatchingDistance;
					DistanceMatchedTime = UAnimSuiteMathLibrary::CalculateExplicitTime(CurrentSequence, InCurrentMatchingDistance, DistanceMatchedPrevTime,
														Context.GetDeltaTime(), GetDistanceCurveName(), GetPlayRateClamp(), GetShouldAdvanceTimeNaturally(),
														IsLooping());
					
				}

				/** If the time acquired by pose matching is less than the time acquired by distance matching, use the pose-matched time. (Otherwise, the
				    animation will be played in reverse on the next frame.) If not, use distance matching for this frame and the following frames. */
				bIsDistanceMatching = DistanceMatchedTime < PoseMatchedTime;
				ExplicitTime = bIsDistanceMatching ? DistanceMatchedTime : PoseMatchedTime;

				/** Trigger smoothing, but only if the pose-matched time needs to "catch up to" the distance-matched time. We don't want the catch-up to be too fast,
				since that will cause snapping, so smoothing is implemented (if allowed by input settings). */
				bTriggerSmoothTimeFollowingPoseMatch = !bIsDistanceMatching && GetShouldSmoothTimeFollowingPoseMatch();

				/** Only draw debug shapes if the pose-matched time is used. */
				bDrawDebugThisFrame = !bIsDistanceMatching;
				
				// UE_LOG(LogAnimation, Warning, TEXT("DistanceMatchedExplicitTime = %f"), DistanceMatchedTime);
				// UE_LOG(LogAnimation, Warning, TEXT("PoseMatchedExplicitTime = %f"), PoseMatchedTime);
			} break;
		}
	}

	/** Reset the initialization "flag" to prevent this scope from being entered at the incorrect time. */
	bIsBeingReinitialized = false;
}

bool FAnimNode_SequencePlayerMatcher::GetResetPlayTimeWhenSequenceChanges() const
{
	return GET_ANIM_NODE_DATA(bool, bResetPlayTimeWhenSequenceChanges);
}

bool FAnimNode_SequencePlayerMatcher::SetResetPlayTimeWhenSequenceChanges(bool bInReset)
{
#if WITH_EDITORONLY_DATA
	bResetPlayTimeWhenSequenceChanges = bInReset;
#endif

	if (bool* bResetPlayTimeWhenSequenceChangesPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bResetPlayTimeWhenSequenceChanges))
	{
		*bResetPlayTimeWhenSequenceChangesPtr = bInReset;
		return true;
	}

	return false;
}

bool FAnimNode_SequencePlayerMatcher::SetSequence(UAnimSequenceBase* InSequence)
{
	// @TODO: If I make an array of Sequences as a possible input, change the logic here.

	Sequence = InSequence;
	return true;
}

bool FAnimNode_SequencePlayerMatcher::SetLoopAnimation(bool bInLoopAnimation)
{
#if WITH_EDITORONLY_DATA
	bLoopAnimation = bInLoopAnimation;
#endif
	
	if(bool* bLoopAnimationPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bLoopAnimation))
	{
		*bLoopAnimationPtr = bInLoopAnimation;
		return true;
	}

	return false;
}

UAnimSequenceBase* FAnimNode_SequencePlayerMatcher::GetSequence() const
{
	return Sequence;
}

float FAnimNode_SequencePlayerMatcher::GetSampleRate() const
{
	return GET_ANIM_NODE_DATA(float, SampleRate);
}

float FAnimNode_SequencePlayerMatcher::GetPlayRateBasis() const
{
	return GET_ANIM_NODE_DATA(float, PlayRateBasis);
}

float FAnimNode_SequencePlayerMatcher::GetPlayRate() const
{
	return bIsDistanceMatching ? 0.0f : GET_ANIM_NODE_DATA(float, PlayRate);
}

const FInputScaleBiasClampConstants& FAnimNode_SequencePlayerMatcher::GetPlayRateScaleBiasClampConstants() const
{
	return GET_ANIM_NODE_DATA(FInputScaleBiasClampConstants, PlayRateScaleBiasClampConstants);
}

float FAnimNode_SequencePlayerMatcher::GetStartPosition() const
{
	return GET_ANIM_NODE_DATA(float, StartPosition);
}

bool FAnimNode_SequencePlayerMatcher::SetStartPosition(float InStartPosition)
{
#if WITH_EDITORONLY_DATA
	StartPosition = InStartPosition;
	GET_MUTABLE_ANIM_NODE_DATA(float, StartPosition) = InStartPosition;
#endif
	
	if(float* StartPositionPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, StartPosition))
	{
		*StartPositionPtr = InStartPosition;
		return true;
	}

	return false;
}

bool FAnimNode_SequencePlayerMatcher::IsLooping() const
{
	//return (GetMatchingType() == EMatchingType::DistanceMatch || GetMatchingType() == EMatchingType::PoseAndDistanceMatch) ? false : GET_ANIM_NODE_DATA(bool, bLoopAnimation);
	return GET_ANIM_NODE_DATA(bool, bLoopAnimation);
}

bool FAnimNode_SequencePlayerMatcher::SetSampleRate(float InSampleRate)
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


bool FAnimNode_SequencePlayerMatcher::SetPlayRate(float InPlayRate)
{
#if WITH_EDITORONLY_DATA
	PlayRate = InPlayRate;
	GET_MUTABLE_ANIM_NODE_DATA(float, PlayRate) = InPlayRate;
#endif
	
	if(float* PlayRatePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, PlayRate))
	{
		*PlayRatePtr = InPlayRate;
		return true;
	}

	return false;
}

float FAnimNode_SequencePlayerMatcher::GetCurrentAssetTime() const
{
	return ExplicitTime;
}

float FAnimNode_SequencePlayerMatcher::GetCurrentAssetTimePlayRateAdjusted() const
{
	
	UAnimSequenceBase* CurrentSequence = GetSequence();
	float EffectivePlayrate;
	
	if (IsEvaluator())
	{
		EffectivePlayrate = CurrentSequence ? (ExplicitTime - PrevExplicitTime)/DeltaTime : 1.0f;
	}
	else
	{
		const float SequencePlayRate = (CurrentSequence ? CurrentSequence->RateScale : 1.0f);
		const float CurrentPlayRate = GetPlayRate();
		const float CurrentPlayRateBasis = GetPlayRateBasis();
		const float AdjustedPlayRate = PlayRateScaleBiasClampState.ApplyTo(GetPlayRateScaleBiasClampConstants(), FMath::IsNearlyZero(CurrentPlayRateBasis) ? 0.f : (CurrentPlayRate / CurrentPlayRateBasis));
		EffectivePlayrate = SequencePlayRate * AdjustedPlayRate;
	}
	
	return (EffectivePlayrate < 0.0f) ? GetCurrentAssetLength() - InternalTimeAccumulator : InternalTimeAccumulator;
}

float FAnimNode_SequencePlayerMatcher::GetCurrentAssetLength() const
{
	UAnimSequenceBase* CurrentSequence = GetSequence();
	return CurrentSequence ? CurrentSequence->GetPlayLength() : 0.0f;
}

FName FAnimNode_SequencePlayerMatcher::GetGroupName() const
{
	return (bWasReinitialized && GetMatchingType() != EMatchingType::None) ? NAME_None : GET_ANIM_NODE_DATA(FName, GroupName);
}

bool FAnimNode_SequencePlayerMatcher::SetGroupName(FName InGroupName)
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

EAnimGroupRole::Type FAnimNode_SequencePlayerMatcher::GetGroupRole() const
{
	return GET_ANIM_NODE_DATA(TEnumAsByte<EAnimGroupRole::Type>, GroupRole);
}

bool FAnimNode_SequencePlayerMatcher::SetGroupRole(EAnimGroupRole::Type InRole)
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

EAnimSyncMethod FAnimNode_SequencePlayerMatcher::GetGroupMethod() const
{
	return (bWasReinitialized && GetMatchingType() != EMatchingType::None) ? EAnimSyncMethod::DoNotSync : GET_ANIM_NODE_DATA(EAnimSyncMethod, Method);
}

bool FAnimNode_SequencePlayerMatcher::SetGroupMethod(EAnimSyncMethod InMethod)
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

bool FAnimNode_SequencePlayerMatcher::GetIgnoreForRelevancyTest() const
{
	return GET_ANIM_NODE_DATA(bool, bIgnoreForRelevancyTest);
}

bool FAnimNode_SequencePlayerMatcher::SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest)
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

EMatchingType FAnimNode_SequencePlayerMatcher::GetMatchingType() const
{
	return GET_ANIM_NODE_DATA(EMatchingType, MatchingType);
}

bool FAnimNode_SequencePlayerMatcher::SetMatchingType(EMatchingType InMatchingType)
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

EMatchingRange FAnimNode_SequencePlayerMatcher::GetMatchingRange() const
{
	return GET_ANIM_NODE_DATA(EMatchingRange, MatchingRange);
}

bool FAnimNode_SequencePlayerMatcher::SetMatchingRange(EMatchingRange InMatchingRange)
{
#if WITH_EDITORONLY_DATA
	MatchingRange = InMatchingRange;
	GET_MUTABLE_ANIM_NODE_DATA(EMatchingRange, MatchingRange) = InMatchingRange;
#endif

	if (EMatchingRange* MatchingRangePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(EMatchingRange, MatchingRange))
	{
		*MatchingRangePtr = InMatchingRange;
		return true;
	}

	return false;
}

float FAnimNode_SequencePlayerMatcher::GetInitialTime() const
{
	return GET_ANIM_NODE_DATA(float, InitialTime);
}

bool FAnimNode_SequencePlayerMatcher::SetInitialTime(float InInitialTime)
{
#if WITH_EDITORONLY_DATA
	InitialTime = InInitialTime;
	GET_MUTABLE_ANIM_NODE_DATA(float, InitialTime) = InInitialTime;
#endif
	
	if(float* InitialTimePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, InitialTime))
	{
		*InitialTimePtr = InInitialTime;
		return true;
	}

	return false;
}

float FAnimNode_SequencePlayerMatcher::GetFinalTime() const
{
	return GET_ANIM_NODE_DATA(float, FinalTime);
}

bool FAnimNode_SequencePlayerMatcher::SetFinalTime(float InFinalTime)
{
#if WITH_EDITORONLY_DATA
	FinalTime = InFinalTime;
	GET_MUTABLE_ANIM_NODE_DATA(float, FinalTime) = InFinalTime;
#endif
	
	if(float* FinalTimePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, FinalTime))
	{
		*FinalTimePtr = InFinalTime;
		return true;
	}

	return false;
}

bool FAnimNode_SequencePlayerMatcher::GetShouldMatchVelocity() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldMatchVelocity);
}

bool FAnimNode_SequencePlayerMatcher::SetShouldMatchVelocity(bool bInShouldMatchVelocity)
{
#if WITH_EDITORONLY_DATA
	bShouldMatchVelocity = bInShouldMatchVelocity;
#endif

	if(bool* bShouldMatchVelocityPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bShouldMatchVelocity))
	{
		*bShouldMatchVelocityPtr = bInShouldMatchVelocity;
		return true;
	}

	return false;
}

float FAnimNode_SequencePlayerMatcher::GetPositionWeight() const
{
	return GET_ANIM_NODE_DATA(float, PositionWeight);
}

bool FAnimNode_SequencePlayerMatcher::SetPositionWeight(float InPositionWeight)
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

float FAnimNode_SequencePlayerMatcher::GetVelocityWeight() const
{
	return GET_ANIM_NODE_DATA(float, VelocityWeight);
}

bool FAnimNode_SequencePlayerMatcher::SetVelocityWeight(float InVelocityWeight)
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

float FAnimNode_SequencePlayerMatcher::GetMatchingDistance() const
{
	return GET_ANIM_NODE_DATA(float, MatchingDistance);
}

bool FAnimNode_SequencePlayerMatcher::SetMatchingDistance(float InMatchingDistance)
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

FName FAnimNode_SequencePlayerMatcher::GetDistanceCurveName() const
{
	return FName(GET_ANIM_NODE_DATA(FName, DistanceCurveName));
}

bool FAnimNode_SequencePlayerMatcher::SetDistanceCurveName(FName InDistanceCurveName)
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

FVector2D FAnimNode_SequencePlayerMatcher::GetPlayRateClamp() const
{
	return FVector2D(GET_ANIM_NODE_DATA(FVector2D, PlayRateClamp));
}

bool FAnimNode_SequencePlayerMatcher::SetPlayRateClamp(FVector2D InPlayRateClamp)
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

bool FAnimNode_SequencePlayerMatcher::GetShouldAdvanceTimeNaturally() const
{
	return GET_ANIM_NODE_DATA(bool, bAdvanceTimeNaturally);
}

bool FAnimNode_SequencePlayerMatcher::SetShouldAdvanceTimeNaturally(bool bInAdvanceTimeNaturally)
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

bool FAnimNode_SequencePlayerMatcher::GetNegateDistanceValue() const
{
	return GET_ANIM_NODE_DATA(bool, bNegateDistanceValue);
}

bool FAnimNode_SequencePlayerMatcher::SetNegateDistanceValue(bool bInNegateDistanceValue)
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

bool FAnimNode_SequencePlayerMatcher::GetShouldSmoothTimeFollowingPoseMatch() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldSmoothTimeFollowingPoseMatch);
}

bool FAnimNode_SequencePlayerMatcher::SetShouldSmoothTimeFollowingPoseMatch(
	bool bInShouldSmoothTimeFollowingPoseMatch)
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

float FAnimNode_SequencePlayerMatcher::GetSmoothingAlpha() const
{
	return GET_ANIM_NODE_DATA(float, SmoothingAlpha);
}

bool FAnimNode_SequencePlayerMatcher::SetSmoothingAlpha(float InSmoothingAlpha)
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

float FAnimNode_SequencePlayerMatcher::GetStartDistance() const
{
	return GET_ANIM_NODE_DATA(float, StartDistance);
}

bool FAnimNode_SequencePlayerMatcher::SetStartDistance(float InStartDistance)
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

bool FAnimNode_SequencePlayerMatcher::GetShouldInertiallyBlendUponSequenceChange() const
{
	return GET_ANIM_NODE_DATA(bool, bShouldInertiallyBlendUponSequenceChange);
}

bool FAnimNode_SequencePlayerMatcher::SetShouldInertiallyBlendUponSequenceChange(
	bool bInShouldInertiallyBlendUponSequenceChange)
{
#if WITH_EDITORONLY_DATA
	bShouldInertiallyBlendUponSequenceChange = bInShouldInertiallyBlendUponSequenceChange;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bInShouldInertiallyBlendUponSequenceChange) = bShouldInertiallyBlendUponSequenceChange;
#endif

	if (bool* bShouldInertiallyBlendUponSequenceChangePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bInShouldInertiallyBlendUponSequenceChange))
	{
		*bShouldInertiallyBlendUponSequenceChangePtr = bInShouldInertiallyBlendUponSequenceChange;
		return true;
	}

	return false;
}

UBlendProfile* FAnimNode_SequencePlayerMatcher::GetBlendProfile() const
{
	return BlendProfile;
}

float FAnimNode_SequencePlayerMatcher::GetInertialBlendTime() const
{
	return GET_ANIM_NODE_DATA(float, InertialBlendTime);
}

bool FAnimNode_SequencePlayerMatcher::SetInertialBlendTime(float InInertialBlendTime)
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

bool FAnimNode_SequencePlayerMatcher::GetShowDebugShapes() const
{
	return GET_ANIM_NODE_DATA(bool, bShowDebugShapes);
}

bool FAnimNode_SequencePlayerMatcher::SetShowDebugShapes(bool bInShowDebugShapes)
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

EDebugPoseMatchLevel FAnimNode_SequencePlayerMatcher::GetDebugLevel() const
{
	return GET_ANIM_NODE_DATA(EDebugPoseMatchLevel, DebugLevel);
}

bool FAnimNode_SequencePlayerMatcher::SetDebugLevel(EDebugPoseMatchLevel InDebugLevel)
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

float FAnimNode_SequencePlayerMatcher::GetPositionDrawScale() const
{
	return GET_ANIM_NODE_DATA(float, PositionDrawScale);
}

bool FAnimNode_SequencePlayerMatcher::SetPositionDrawScale(float InPositionDrawScale)
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

float FAnimNode_SequencePlayerMatcher::GetVelocityDrawScale() const
{
	return GET_ANIM_NODE_DATA(float, VelocityDrawScale);
}

bool FAnimNode_SequencePlayerMatcher::SetVelocityDrawScale(float InVelocityDrawScale)
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

#undef LOCTEXT_NAMESPACE
























