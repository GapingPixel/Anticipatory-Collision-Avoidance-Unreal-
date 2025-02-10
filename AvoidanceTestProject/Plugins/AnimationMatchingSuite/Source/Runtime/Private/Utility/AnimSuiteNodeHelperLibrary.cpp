// Copyright MuuKnighted Games 2024. All rights reserved.

#include "Utility/AnimSuiteNodeHelperLibrary.h"

#include "AnimGraph/AnimNode_PoseRecorder.h"
#include "Utility/AnimSuiteMathLibrary.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_Inertialization.h"
#include "Animation/AnimNode_SequencePlayer.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimSuiteNodeHelperLibrary, Verbose, All);


//-------------------------------------
// Blend Space Matcher
//-------------------------------------

float UAnimSuiteNodeHelperLibrary::GetMatchedScaledAccumulatedTime(const FBlendSpaceMatcherReference& BlendSpaceMatchPlayer)
{
	float OutScaledAccumulatedTime = 0.0f;
	BlendSpaceMatchPlayer.CallAnimNodeFunction<FAnimNode_BlendSpacePlayerMatcher>(
		TEXT("GetMatchedScaledAccumulatedTime"),
		[&OutScaledAccumulatedTime](const FAnimNode_BlendSpacePlayerMatcher& InBlendSpacePlayer)
		{
			const UBlendSpace* CurrentBlendSpace = InBlendSpacePlayer.GetBlendSpace();
			if (CurrentBlendSpace)
			{
				const FVector BlendInput = InBlendSpacePlayer.GetPosition(); // gets the coordinates currently being sampled by the blendspace
				const FVector ClampedBlendInput = CurrentBlendSpace->GetClampedAndWrappedBlendInput(BlendInput);
				TArray<FBlendSampleData> BlendSamples;
				int32 TriangulationIndex = 0;
				CurrentBlendSpace->GetSamplesFromBlendInput(ClampedBlendInput, BlendSamples, TriangulationIndex, true);
				const float AnimLength = CurrentBlendSpace->GetAnimationLengthFromSampleData(BlendSamples);
				OutScaledAccumulatedTime = InBlendSpacePlayer.GetAccumulatedTime() * AnimLength;
			}
			else
			{
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"GetMatchedScaledAccumulatedTime\": The Blend Space is null."));	
			}
		});

	return OutScaledAccumulatedTime;
}

FBlendSpaceMatcherReference UAnimSuiteNodeHelperLibrary::ConvertToMatchedBlendSpacePlayer(
	const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result)
{
	return FAnimNodeReference::ConvertToType<FBlendSpaceMatcherReference>(Node, Result);
}

FBlendSpaceMatcherReference UAnimSuiteNodeHelperLibrary::SetMatchedBlendSpace(const FBlendSpaceMatcherReference& BlendSpaceMatchedPlayer, UBlendSpace* BlendSpace)
{
	BlendSpaceMatchedPlayer.CallAnimNodeFunction<FAnimNode_BlendSpacePlayerMatcher>(
		TEXT("SetMatchedBlendSpace"),
		[BlendSpace](FAnimNode_BlendSpacePlayerMatcher& InBlendSpaceMatcher)
		{
			if (!InBlendSpaceMatcher.SetBlendSpace(BlendSpace))
			{
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetMatchedBlendSpace\": The Blend Space could not be set on the Blend Space Player (Matching) node. The value is not dynamic. Set it as Always Dynamic."));
			}
		});
	return BlendSpaceMatchedPlayer;
}

FBlendSpaceMatcherReference UAnimSuiteNodeHelperLibrary::SetMatchedBlendSpaceWithInertialBlending(const FAnimUpdateContext& UpdateContext,
								const FBlendSpaceMatcherReference& BlendSpaceMatchedPlayer, UBlendSpace* BlendSpace, float BlendTime)
{
	BlendSpaceMatchedPlayer.CallAnimNodeFunction<FAnimNode_BlendSpacePlayerMatcher>(
		TEXT("SetBlendSpace"),
		[BlendSpace, &UpdateContext, BlendTime](FAnimNode_BlendSpacePlayerMatcher& InBlendSpaceMatcher)
		{
			const UBlendSpace* CurrentBlendSpace = InBlendSpaceMatcher.GetBlendSpace();
			const bool bBlendSpaceChanged = (CurrentBlendSpace != BlendSpace);
			if (!InBlendSpaceMatcher.SetBlendSpace(BlendSpace))
			{
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetMatchedBlendSpaceWithInertialBlending\": The Blend Space could not be set on the Blend Space Player (Matching) node. The value is not dynamic. Set it as Always Dynamic."));
			}

			if (bBlendSpaceChanged && BlendTime > 0.0f)
			{
				if (const FAnimationUpdateContext* AnimationUpdateContext = UpdateContext.GetContext())
				{
					if (UE::Anim::IInertializationRequester* InertializationRequester = AnimationUpdateContext->GetMessage<UE::Anim::IInertializationRequester>())
					{
						InertializationRequester->RequestInertialization(BlendTime);
					}
				}
				else
				{
					UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetMatchedBlendSpaceWithInertialBlending\": Called with invalid context."));
				}
			}
		});

	return BlendSpaceMatchedPlayer;
}


//-------------------------------------
// Sequence Matcher
//-------------------------------------

FSequenceMatcherReference UAnimSuiteNodeHelperLibrary::ConvertToMatchedSequencePlayer(const FAnimNodeReference& Node,
	EAnimNodeReferenceConversionResult& Result)
{
	return FAnimNodeReference::ConvertToType<FSequenceMatcherReference>(Node, Result);
}

FSequenceMatcherReference UAnimSuiteNodeHelperLibrary::SetMatchedAccumulatedTime(const FSequenceMatcherReference& SequencePlayer, float Time)
{
	SequencePlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("SetAccumulatedTime"),
		[Time](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			InSequencePlayer.SetAccumulatedTime(Time);
		});

	return SequencePlayer;
}

FSequenceMatcherReference UAnimSuiteNodeHelperLibrary::SetMatchedStartPosition(const FSequenceMatcherReference& SequencePlayer, float StartPosition)
{
	SequencePlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("SetStartPosition"),
		[StartPosition](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			if(!InSequencePlayer.SetStartPosition(StartPosition))
			{
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetStartPosition\": Could not set the Start Position on the Sequence Player (Matching) node. The value is not dynamic. Set it to Always Dynamic."));
			}
		});

	return SequencePlayer;
}

FSequenceMatcherReference UAnimSuiteNodeHelperLibrary::SetMatchedPlayRate(const FSequenceMatcherReference& SequencePlayer, float PlayRate)
{
	SequencePlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("SetPlayRate"),
		[PlayRate](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			if(!InSequencePlayer.SetPlayRate(PlayRate))
			{
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetPlayRate\": Could not set the Play Rate on the Sequence Player (Matching) node. The value is not dynamic. Set it to Always Dynamic."));
			}
		});

	return SequencePlayer;
}

FSequenceMatcherReference UAnimSuiteNodeHelperLibrary::SetMatchedSequence(const FSequenceMatcherReference& SequencePlayer, UAnimSequenceBase* Sequence)
{
	SequencePlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("SetSequence"),
		[Sequence](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			if(!InSequencePlayer.SetSequence(Sequence))
			{
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetPlayRate\": Could not set the Sequence on the Sequence Player (Matching) node. The value is not dynamic. Set it to Always Dynamic."));
			}
		});

	return SequencePlayer;
}

UAnimSequenceBase* UAnimSuiteNodeHelperLibrary::GetMatchedSequencePure(const FSequenceMatcherReference& SequencePlayer)
{
	UAnimSequenceBase* SequenceBase = nullptr;
	
	SequencePlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("GetSequence"),
		[&SequenceBase](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			SequenceBase = InSequencePlayer.GetSequence();
		});

	return SequenceBase;
}

float UAnimSuiteNodeHelperLibrary::GetMatchedAccumulatedTime(
	const FSequenceMatcherReference& SequenceMatchedPlayer)
{
	float AccumulatedTime = 0.0f;
	SequenceMatchedPlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("GetMatchedAccumulatedTime"),
		[&AccumulatedTime](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
	{
		AccumulatedTime = InSequencePlayer.GetAccumulatedTime();	
	});

	return AccumulatedTime;
}

float UAnimSuiteNodeHelperLibrary::GetMatchingStartPosition(const FSequenceMatcherReference& SequenceMatchedPlayer)
{
	float StartPosition = 0.0f;
	SequenceMatchedPlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("GetMatchingStartPosition"),
		[&StartPosition](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			StartPosition = InSequencePlayer.GetStartPosition();
		});

	return StartPosition;
}

float UAnimSuiteNodeHelperLibrary::GetMatchingPlayRate(const FSequenceMatcherReference& SequenceMatchedPlayer)
{
	float PlayRate = 1.f;
	SequenceMatchedPlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("GetMatchingPlayRate"),
		[&PlayRate](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			PlayRate = InSequencePlayer.GetPlayRate();
		});

	return PlayRate;
}

bool UAnimSuiteNodeHelperLibrary::GetMatchingLoopAnimation(const FSequenceMatcherReference& SequenceMatchedPlayer)
{
	bool bLoopAnimation = false;
	SequenceMatchedPlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
		TEXT("GetMatchingLoopAnimation"),
		[&bLoopAnimation](FAnimNode_SequencePlayerMatcher& InSequencePlayer)
		{
			bLoopAnimation = InSequencePlayer.IsLooping();
		});

	return bLoopAnimation;
}

float UAnimSuiteNodeHelperLibrary::ComputeMatchedPlayRateFromDuration( const FSequenceMatcherReference& SequenceMatchedPlayer, float Duration)
{
	float PlayRate = 1.0f;
	if (Duration > 0.f)
	{
		SequenceMatchedPlayer.CallAnimNodeFunction<FAnimNode_SequencePlayerMatcher>(
			TEXT("ComputeMatchedPlayRateFromDuration"),
			[&PlayRate, Duration](const FAnimNode_SequencePlayerMatcher& InSequencePlayer)
			{
				if (const UAnimSequenceBase* Sequence = InSequencePlayer.GetSequence())
				{
					PlayRate = Sequence->GetPlayLength() / Duration;
				}
			});
	}
	return PlayRate;
}


//-------------------------------------
// Blend Space (standard/native)
//-------------------------------------

FBlendSpacePlayerReturnValues UAnimSuiteNodeHelperLibrary::SetNormalizedTimeByDistanceMatching(const FAnimUpdateContext& UpdateContext,
                                                                                                    const FBlendSpacePlayerReference& BlendSpacePlayer, const float MatchingDistance, const float PrevNormalizedTime, const FVector2D PlayRateClamp,
                                                                                                    const FName DistanceCurveName, const bool bAdvanceTimeNaturally, const bool bUseOnlyHighestWeightedSample)
{
	FBlendSpacePlayerReturnValues ReturnValues; // a custom struct that includes a float and a FBlendSpacePlayerReference
	
	BlendSpacePlayer.CallAnimNodeFunction<FAnimNode_BlendSpacePlayer>(
		TEXT("SetNormalizedTimeByDistanceMatching"),
		[&UpdateContext, &ReturnValues, &PlayRateClamp, &MatchingDistance, &PrevNormalizedTime, &DistanceCurveName, &bAdvanceTimeNaturally, &bUseOnlyHighestWeightedSample](FAnimNode_BlendSpacePlayer& InBlendSpacePlayer)
		{
			if (InBlendSpacePlayer.GetAccumulatedTime() >= 1.0f)
			{
				InBlendSpacePlayer.SetAccumulatedTime(1.0f);
			}
			else
			{
				const UBlendSpace* CurrentBlendSpace = InBlendSpacePlayer.GetBlendSpace();
				const FVector BlendInput = InBlendSpacePlayer.GetPosition(); // gets the coordinates currently being sampled by the blendspace
				const FVector ClampedBlendInput = CurrentBlendSpace->GetClampedAndWrappedBlendInput(BlendInput);
				TArray<FBlendSampleData> BlendSampleData;
				int32 TriangulationIndex = 0; //@TODO: how can I access the CachedTriangulationIndex?
				CurrentBlendSpace->GetSamplesFromBlendInput(ClampedBlendInput, BlendSampleData, TriangulationIndex, true);
				
				/** Calculate the unclamped normalized matching time. */
				const float DeltaTime = UpdateContext.GetContext()->GetDeltaTime();
				const float NormalizedMatchingTime = UAnimSuiteMathLibrary::CalculateNormalizedTime(CurrentBlendSpace,
					MatchingDistance, PrevNormalizedTime, BlendSampleData, DeltaTime, DistanceCurveName, PlayRateClamp,
					bAdvanceTimeNaturally, bUseOnlyHighestWeightedSample);
				
				/** Set the accumulated time in the asset player.*/
				InBlendSpacePlayer.SetAccumulatedTime(NormalizedMatchingTime);
				ReturnValues.NormalizedTime = NormalizedMatchingTime;
			}
		});
	
	ReturnValues.BlendSpacePlayer = BlendSpacePlayer;
	return ReturnValues;
}

FBlendSpacePlayerReturnValues UAnimSuiteNodeHelperLibrary::SetNormalizedTimeByPoseMatching(
	const FAnimUpdateContext& UpdateContext, const FBlendSpacePlayerReference& BlendSpacePlayer, const float SampleRate,
	const bool bShouldMatchVelocity, const bool bUseOnlyHighestWeightedSample, const bool bShouldInertiallyBlend, const float InertialBlendTime)
{
	FBlendSpacePlayerReturnValues ReturnValues; // a custom struct that includes a float and a FBlendSpacePlayerReference
	
	BlendSpacePlayer.CallAnimNodeFunction<FAnimNode_BlendSpacePlayer>(
		TEXT("SetNormalizedTimeByPoseMatching"),
		[&ReturnValues, &UpdateContext, bShouldMatchVelocity, SampleRate, bUseOnlyHighestWeightedSample, bShouldInertiallyBlend, InertialBlendTime](FAnimNode_BlendSpacePlayer& InBlendSpacePlayer)
		{
			if (const FAnimationUpdateContext* AnimationUpdateContext = UpdateContext.GetContext())
			{
				/** Request inertial blending. */
				if (bShouldInertiallyBlend == true && InertialBlendTime > 0.0f)
				{
					if (UE::Anim::IInertializationRequester* InertializationRequester = AnimationUpdateContext->GetMessage<UE::Anim::IInertializationRequester>())
					{
						InertializationRequester->RequestInertialization(InertialBlendTime);
					}
					else
					{
						UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetNormalizedTimeByPoseMatching\": Inertialization Requester is null."));
					}
				}

				/** Get relevant Blend Space data. */
				UBlendSpace* CurrentBlendSpace = InBlendSpacePlayer.GetBlendSpace();
				const FVector BlendInput = InBlendSpacePlayer.GetPosition(); // gets the coordinates currently being sampled by the blendspace
				const FVector ClampedBlendInput = CurrentBlendSpace->GetClampedAndWrappedBlendInput(BlendInput);
				TArray<FBlendSampleData> BlendSampleData;
				int32 TriangulationIndex = 0; //@TODO: how can I access the CachedTriangulationIndex?
				CurrentBlendSpace->GetSamplesFromBlendInput(ClampedBlendInput, BlendSampleData, TriangulationIndex, true);
				TArray<FPoseBoneData> CurrentSnapshotPose;

				/** Get the pose-matched time. */
				FAMSDebugData DebugData; //@TODO: remove this temp fix and replace with real fix. (No info is sent by this function.)
				float PoseMatchedNormalizedTime = UAnimSuiteMathLibrary::DetermineInitialTime(*AnimationUpdateContext, CurrentSnapshotPose, CurrentBlendSpace,
														BlendSampleData, SampleRate, DebugData, InBlendSpacePlayer.IsLooping(), bShouldMatchVelocity, bUseOnlyHighestWeightedSample);
				PoseMatchedNormalizedTime = FMath::Clamp(PoseMatchedNormalizedTime, 0.f, 1.0f);

				/** Set the accumulated time in the asset player.*/
				InBlendSpacePlayer.SetAccumulatedTime(PoseMatchedNormalizedTime);
				ReturnValues.NormalizedTime = PoseMatchedNormalizedTime;
			}
			else
			{
				/** Set the accumulated time in the asset player. */
				InBlendSpacePlayer.SetAccumulatedTime(0.0f);
				ReturnValues.NormalizedTime = 0.0f;
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetNormalizedTimeByPoseMatching\": Called with invalid context. Normalized Time set to 0.0f."));
			}
		});

	ReturnValues.BlendSpacePlayer = BlendSpacePlayer;
	return ReturnValues;
}

FBlendSpacePlayerReference UAnimSuiteNodeHelperLibrary::SetNormalizedTime(const FBlendSpacePlayerReference& BlendSpacePlayer,
                                                                          float NormalizedTime)
{
	BlendSpacePlayer.CallAnimNodeFunction<FAnimNode_BlendSpacePlayer>(
		TEXT("SetNormalizedTime"),
		[NormalizedTime](FAnimNode_BlendSpacePlayer& InBlendSpacePlayer)
		{
			const float ClampedNormalizedTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
			InBlendSpacePlayer.SetAccumulatedTime(ClampedNormalizedTime);
		});
	
	return BlendSpacePlayer;
}

float UAnimSuiteNodeHelperLibrary::GetScaledAccumulatedTime(const FBlendSpacePlayerReference& BlendSpacePlayer)
{
	float OutScaledAccumulatedTime = 0.0f;
	BlendSpacePlayer.CallAnimNodeFunction<FAnimNode_BlendSpacePlayer>(
		TEXT("GetScaledAccumulatedTime"),
		[&OutScaledAccumulatedTime](FAnimNode_BlendSpacePlayer& InBlendSpacePlayer)
		{
			const UBlendSpace* CurrentBlendSpace = InBlendSpacePlayer.GetBlendSpace();
			const FVector BlendInput = InBlendSpacePlayer.GetPosition(); // gets the coordinates currently being sampled by the blendspace
			const FVector ClampedBlendInput = CurrentBlendSpace->GetClampedAndWrappedBlendInput(BlendInput);
			TArray<FBlendSampleData> BlendSamples;
			int32 TriangulationIndex = 0;
			CurrentBlendSpace->GetSamplesFromBlendInput(ClampedBlendInput, BlendSamples, TriangulationIndex, true);
			const float AnimLength = CurrentBlendSpace->GetAnimationLengthFromSampleData(BlendSamples);
			OutScaledAccumulatedTime = InBlendSpacePlayer.GetAccumulatedTime() * AnimLength;
		});

	return OutScaledAccumulatedTime;
}


//-------------------------------------
// Sequence (standard/native)
//-------------------------------------

FSequencePlayerReturnValues UAnimSuiteNodeHelperLibrary::SetExplicitTimeByPoseMatching(
	const FAnimUpdateContext& UpdateContext, const FSequencePlayerReference& SequencePlayer, const float SampleRate,
	const bool bShouldMatchVelocity, const bool bShouldInertiallyBlend, const float InertialBlendTime)
{
	FSequencePlayerReturnValues ReturnValues; // a custom struct that includes a float and a FBlendSpacePlayerReference
	
	SequencePlayer.CallAnimNodeFunction<FAnimNode_SequencePlayer>(
		TEXT("SetExplicitTimeByPoseMatching"),
		[&ReturnValues, &UpdateContext, bShouldMatchVelocity, SampleRate, bShouldInertiallyBlend, InertialBlendTime](FAnimNode_SequencePlayer& InSequencePlayer)
		{
			if (const FAnimationUpdateContext* AnimationUpdateContext = UpdateContext.GetContext())
			{
				/** Request inertialization. */
				if (bShouldInertiallyBlend && InertialBlendTime > 0.0f)
				{
					if (UE::Anim::IInertializationRequester* InertializationRequester = AnimationUpdateContext->GetMessage<UE::Anim::IInertializationRequester>())
					{
						InertializationRequester->RequestInertialization(InertialBlendTime);
					}
					else
					{
						UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetExplicitTimeByPoseMatching\": Inertialization Requester is null."));
					}
				}
				
				UAnimSequence* Sequence = Cast<UAnimSequence>(InSequencePlayer.GetSequence());
				TArray<FPoseBoneData> CurrentSnapshotPose;

				/** Get the pose-matched time. */
				FAMSDebugData DebugData; // @TODO: find a way to send debug info. Maybe an actor component? (No info is currently sent by this function.)
				float PoseMatchedExplicitTime = UAnimSuiteMathLibrary::DetermineInitialTime(*AnimationUpdateContext, CurrentSnapshotPose,
									Sequence, SampleRate, DebugData, InSequencePlayer.IsLooping(), bShouldMatchVelocity);
				
				PoseMatchedExplicitTime = FMath::Clamp(PoseMatchedExplicitTime, 0.0f, Sequence->GetPlayLength());

				/** Set the accumulated time in the asset player. */
				InSequencePlayer.SetAccumulatedTime(PoseMatchedExplicitTime);
				ReturnValues.ExplicitTime = PoseMatchedExplicitTime;
			}
			else
			{
				/** Set the accumulated time in the asset player. */
				InSequencePlayer.SetAccumulatedTime(0.0f);
				ReturnValues.ExplicitTime = 0.0f;
				UE_LOG(LogAnimSuiteNodeHelperLibrary, Warning, TEXT("\"SetExplicitTimeByPoseMatching\": Called with invalid context. Explicit Time set to 0.0f."));
			}
		});

	ReturnValues.SequencePlayer = SequencePlayer;
	return ReturnValues;
}


