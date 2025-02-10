// Copyright MuuKnighted Games 2024. All rights reserved.

#include "AnimGraph/AnimNode_PoseRecorder.h"

#include "Utility/AnimSuiteMathLibrary.h"
#include "Animation/AnimInstanceProxy.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "AnimationMatchingSuiteNodes"

FCachedPose::FCachedPose()
	: PoseDeltaTime(UE_KINDA_SMALL_NUMBER)
{
	CachedBoneData.Empty(6);
}

void FCachedPose::CachePoseBoneData(FCSPose<FCompactPose>& Pose)
{
	for (TTuple<FName, FCachedPoseBone>& Entry : CachedBoneData)
	{
		Entry.Value.PrevTransform = Entry.Value.Transform;
		Entry.Value.Transform = Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(Entry.Value.BoneID)); // FCompactBonePoseIndex is used for the subset of bones that are part of the mesh for the current LOD
		Entry.Value.Velocity = (Entry.Value.Transform.GetTranslation() - Entry.Value.PrevTransform.GetTranslation()) /
									FMath::Max(UE_SMALL_NUMBER * 100, PoseDeltaTime);
	}
}

void FCachedPose::CalculateVelocity()
{
	for (TTuple<FName, FCachedPoseBone>& Entry : CachedBoneData)
	{
		Entry.Value.Velocity = (Entry.Value.Transform.GetTranslation() - Entry.Value.PrevTransform.GetTranslation()) /
									FMath::Max(UE_SMALL_NUMBER * 100, PoseDeltaTime);
	}
}
  
void FCachedPose::ZeroVelocity()
{
	for (TTuple<FName, FCachedPoseBone>& Entry : CachedBoneData)
	{
		Entry.Value.PrevTransform = Entry.Value.Transform;
		Entry.Value.Velocity = FVector::ZeroVector;
	}
}

IMPLEMENT_ANIMGRAPH_MESSAGE(IPoseMatchRequester);
const FName IPoseMatchRequester::Attribute("PoseGrabber");

FPoseMatchRequester::FPoseMatchRequester(const FAnimationBaseContext& InContext, FAnimNode_PoseRecorder* InNode)
	: Node(*InNode)
	, NodeID(InContext.GetCurrentNodeId())
	, Proxy(*InContext.AnimInstanceProxy)
{
}

FAnimNode_PoseRecorder& FPoseMatchRequester::GetNode()
{
	return Node;
}

void FPoseMatchRequester::AddDebugRecord(const FAnimInstanceProxy& InSourceProxy, int32 InSourceNodeID)
{
#if WITH_EDITORONLY_DATA
	Proxy.RecordNodeAttribute(InSourceProxy, NodeID, InSourceNodeID, IPoseMatchRequester::Attribute);
#endif
	TRACE_ANIM_NODE_ATTRIBUTE(Proxy, InSourceProxy, NodeID, InSourceNodeID, IPoseMatchRequester::Attribute);
}

FAnimNode_PoseRecorder::FAnimNode_PoseRecorder()
	: bWereBonesCachedThisFrame(false)
	, AnimInstanceProxy(nullptr)
{
}

void FAnimNode_PoseRecorder::CachePoseBones()
{
	if (AnimInstanceProxy == nullptr)
	{
		return;
	}
	
	/** Empty the CachedBoneData map and create room for BonesToCache.Num() + 1 elements. */
	CachedPose.CachedBoneData.Empty(BonesToCache.Num() + 1);
	
	/** Get the temporary array of bone indices required this frame, which should be a subset of the Skeleton and Mesh's
		RequiredBones array. */
	const FBoneContainer& RequiredBoneContainer = AnimInstanceProxy->GetRequiredBones(); // Temporary array of bone indices required this frame and should be a subset of the Skeleton and Mesh's RequiredBones.
	
	/** */
	for (FBoneReference& BoneRef : BonesToCache)
	{
		/** Check whether the BoneRef exists in the container of required bones. */
		BoneRef.Initialize(RequiredBoneContainer);

		/** If the BoneRef is valid to evaluate, find its transform. */
		if (BoneRef.IsValidToEvaluate())
		{
			/** Add the BoneName as a key to the map, and add the BoneID, as an index of the FCompactPoseIndex, to the
				struct corresponding to the key. */
			CachedPose.CachedBoneData.FindOrAdd(BoneRef.BoneName);
			CachedPose.CachedBoneData[BoneRef.BoneName].BoneID = BoneRef.GetCompactPoseIndex(RequiredBoneContainer).GetInt();
		}
	}

	bWereBonesCachedThisFrame = true;
}

FCachedPose& FAnimNode_PoseRecorder::GetCachedPose()
{
	return CachedPose;
}

void FAnimNode_PoseRecorder::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread); 

	FAnimNode_Base::Initialize_AnyThread(Context);

	/** Push the PoseMatchRequester message onto the shared context stack so more anim nodes can use the Pose Grabber node. @TODO: necessary here? */  
	UE::Anim::TScopedGraphMessage<FPoseMatchRequester> PoseMatchRequester(Context, Context, this);
	
	Source.Initialize(Context);	
	AnimInstanceProxy = Context.AnimInstanceProxy;
}

void FAnimNode_PoseRecorder::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread);

	FAnimNode_Base::CacheBones_AnyThread(Context);
	Source.CacheBones(Context);
	
	CachePoseBones();
}

void FAnimNode_PoseRecorder::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread);
	
	/** Push the PoseMatchRequester message onto the shared context stack so more anim nodes can use the Pose Grabber node. */  
	UE::Anim::TScopedGraphMessage<FPoseMatchRequester> PoseMatchRequester(Context, Context, this);

	Source.Update(Context);

	CachedPose.PoseDeltaTime = Context.GetDeltaTime();
}

void FAnimNode_PoseRecorder::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread);

	/** Modifies the Output to have the correct AnimInstanceProxy, node ID, etc. */
	Source.Evaluate(Output);
	
	/** Convert the pose transform to Component Space for caching purposes. */
	FComponentSpacePoseContext CS_Output(Output.AnimInstanceProxy);
	CS_Output.Pose.InitPose(Output.Pose);
	
	/** Cache the pose transform in Component Space. */
	CachedPose.CachePoseBoneData(CS_Output.Pose);
	
	/** If the bones were cached this frame (due to an LOD change or a node pushing a bone-cache request), zero out the
	    velocity. */
	if (bWereBonesCachedThisFrame)
	{
		CachedPose.ZeroVelocity();
		bWereBonesCachedThisFrame = false;
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG

	if (Output.AnimInstanceProxy != nullptr && GetShowDebugShapes())
	{
		FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

		switch (GetDebugLevel())
		{
			case EDebugPoseMatchLevel::ShowSelectedPosePosition:
			{
				for (TTuple<FName, FCachedPoseBone>& Entry: CachedPose.CachedBoneData)
				{
					FVector BonePoint = ComponentTransform.TransformPosition(Entry.Value.Transform.GetTranslation());
					Output.AnimInstanceProxy->AnimDrawDebugSphere(BonePoint, GetPositionDrawScale(), 15, FColor::Green, false, -1.0f, 0.5f);
				}
			} break;
			case EDebugPoseMatchLevel::ShowSelectedPosePositionAndVelocity:
			{
				for (TTuple<FName, FCachedPoseBone>& Entry: CachedPose.CachedBoneData)
				{
					FVector BonePoint = ComponentTransform.TransformPosition(Entry.Value.Transform.GetTranslation());
					Output.AnimInstanceProxy->AnimDrawDebugSphere(BonePoint, GetPositionDrawScale(), 15, FColor::Green, false, -1.0f, 0.5f);

					FVector BoneVelocity = ComponentTransform.TransformVector(Entry.Value.Velocity).GetSafeNormal();
					if (!BoneVelocity.IsZero())
					{
						Output.AnimInstanceProxy->AnimDrawDebugDirectionalArrow(BonePoint, BonePoint + BoneVelocity * GetVelocityDrawScale(), 10.0f, FColor::Green, false, -1.0f, 1.0f);
					}
				}
			} break;
		}
	}
#endif
}

void FAnimNode_PoseRecorder::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData);

	const FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);
	Source.GatherDebugData(DebugData);
}

bool FAnimNode_PoseRecorder::GetShowDebugShapes() const
{
	return GET_ANIM_NODE_DATA(bool, bShowDebugShapes);
}

bool FAnimNode_PoseRecorder::SetShowDebugShapes(bool bInShowDebugShapes)
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

EDebugPoseMatchLevel FAnimNode_PoseRecorder::GetDebugLevel() const
{
	return GET_ANIM_NODE_DATA(EDebugPoseMatchLevel, DebugLevel);
}

bool FAnimNode_PoseRecorder::SetDebugLevel(EDebugPoseMatchLevel InDebugLevel)
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

float FAnimNode_PoseRecorder::GetPositionDrawScale() const
{
	return GET_ANIM_NODE_DATA(float, PositionDrawScale);
}

bool FAnimNode_PoseRecorder::SetPositionDrawScale(float InPositionDrawScale)
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

float FAnimNode_PoseRecorder::GetVelocityDrawScale() const
{
	return GET_ANIM_NODE_DATA(float, VelocityDrawScale);
}

bool FAnimNode_PoseRecorder::SetVelocityDrawScale(float InVelocityDrawScale)
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








