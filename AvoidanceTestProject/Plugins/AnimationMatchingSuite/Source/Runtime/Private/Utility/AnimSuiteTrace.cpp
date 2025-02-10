// Copyright MuuKnighted Games 2024. All rights reserved.

#include "Utility/AnimSuiteTrace.h"

#include "Animation/AnimInstanceProxy.h"
#include "AnimGraph/AnimNode_BlendSpacePlayerMatcher.h"
#include "Trace/Trace.inl"

#if ANIM_TRACE_ENABLED

UE_TRACE_EVENT_BEGIN(Animation, PoseMatchBlendSpacePlayer)
	UE_TRACE_EVENT_FIELD(uint64, Cycle)
	UE_TRACE_EVENT_FIELD(uint64, AnimInstanceId)
	UE_TRACE_EVENT_FIELD(uint64, BlendSpaceId)
	UE_TRACE_EVENT_FIELD(int32, NodeId)
	UE_TRACE_EVENT_FIELD(float, PositionX)
	UE_TRACE_EVENT_FIELD(float, PositionY)
	UE_TRACE_EVENT_FIELD(float, PositionZ)
	UE_TRACE_EVENT_FIELD(float, FilteredPositionX)
	UE_TRACE_EVENT_FIELD(float, FilteredPositionY)
	UE_TRACE_EVENT_FIELD(float, FilteredPositionZ)
	UE_TRACE_EVENT_END()

void FAnimSuiteGraphRuntimeTrace::OutputPoseMatchBlendSpacePlayer(const FAnimationBaseContext& InContext,
	const FAnimNode_BlendSpacePlayerMatcher& InNode)
{
	bool bEventEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AnimationChannel);
	if (!bEventEnabled)
	{
		return;
	}

	check(InContext.AnimInstanceProxy);

	TRACE_OBJECT(InContext.AnimInstanceProxy->GetAnimInstanceObject());
	TRACE_OBJECT(InNode.GetBlendSpace());

	FVector SampleCoordinates = InNode.GetPosition();
	FVector FilteredPosition = InNode.GetFilteredPosition();

	UE_TRACE_LOG(Animation, PoseMatchBlendSpacePlayer, AnimationChannel)
		<< PoseMatchBlendSpacePlayer.Cycle(FPlatformTime::Cycles64())
		<< PoseMatchBlendSpacePlayer.AnimInstanceId(FObjectTrace::GetObjectId(InContext.AnimInstanceProxy->GetAnimInstanceObject()))
		<< PoseMatchBlendSpacePlayer.BlendSpaceId(FObjectTrace::GetObjectId(InNode.GetBlendSpace()))
		<< PoseMatchBlendSpacePlayer.NodeId(InContext.GetCurrentNodeId())
		<< PoseMatchBlendSpacePlayer.PositionX(static_cast<float>(SampleCoordinates.X))
		<< PoseMatchBlendSpacePlayer.PositionY(static_cast<float>(SampleCoordinates.Y))
		<< PoseMatchBlendSpacePlayer.PositionZ(static_cast<float>(SampleCoordinates.Z))
		<< PoseMatchBlendSpacePlayer.FilteredPositionX(static_cast<float>(FilteredPosition.X))
		<< PoseMatchBlendSpacePlayer.FilteredPositionY(static_cast<float>(FilteredPosition.Y))
		<< PoseMatchBlendSpacePlayer.FilteredPositionZ(static_cast<float>(FilteredPosition.Z));
}

#endif