// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Utility/AnimSuiteTypes.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_PoseRecorder.generated.h"

USTRUCT()
struct FCachedPoseBone
{
	GENERATED_BODY()

public:

	/** The compact pose index. */
	int32 BoneID;

	/** The transform of the previous frame. */
	FTransform PrevTransform;

	/** The transform of the current frame. */
	FTransform Transform;

	/** The velocity calculated using the transform difference between the current and previous frames. */
	FVector Velocity;
	
};


USTRUCT()
struct FCachedPose
{
	GENERATED_BODY()

public:

	/** The DeltaTime value of the FAnimationUpdateContext struct. */
	float PoseDeltaTime;

	/** The map of the bone name to the corresponding cached bone data. */
	TMap<FName, FCachedPoseBone> CachedBoneData;

	FCachedPose();

	/** Caches the transforms and velocities of the matching bones. */
	void CachePoseBoneData(FCSPose<FCompactPose>& Pose);

	/** Calculates the velocity based on the location different between the current and previous pose update. */
	void CalculateVelocity();

	/** Zeros the velocity. */
	void ZeroVelocity(); 
};

/**
 * The event that can be subscribed to in order to request pose matching.
 */
class ANIMATIONMATCHINGSUITE_API IPoseMatchRequester : public UE::Anim::IGraphMessage
{
	DECLARE_ANIMGRAPH_MESSAGE(IPoseMatchRequester);
	
public:

	static const FName Attribute;

	virtual struct FAnimNode_PoseRecorder& GetNode() = 0;
	
	virtual void AddDebugRecord(const FAnimInstanceProxy& InSourceProxy, int32 InSourceNodeID) = 0;
};

/**
 * 
 */
class FPoseMatchRequester : public IPoseMatchRequester
{

public:

	FPoseMatchRequester(const FAnimationBaseContext& InContext, FAnimNode_PoseRecorder* InNode);

private:
	
	virtual struct FAnimNode_PoseRecorder& GetNode() override;

	virtual void AddDebugRecord(const FAnimInstanceProxy& InSourceProxy, int32 InSourceNodeID) override;
	
private:

	/** The Node to target. */
	FAnimNode_PoseRecorder& Node;

	/** The Node index. */
	int32 NodeID;

	/** The Proxy currently executing. */
	FAnimInstanceProxy& Proxy;
	
};


USTRUCT(BlueprintInternalUseOnly)
struct ANIMATIONMATCHINGSUITE_API FAnimNode_PoseRecorder : public FAnimNode_Base
{
	GENERATED_BODY()

public:

	/** Constructor */
	FAnimNode_PoseRecorder();

	/**  */
	void CachePoseBones();

	/**
	 * Gets the cached pose.
	 * @return Returns the cached pose.
	 */
	FCachedPose& GetCachedPose();
	
	// ===== FAnimNode_Base =====

	/**
	 * Called when the node first runs. If the node is inside a state machine or cached pose branch then this can be called
	 * multiple times. This can be called on any thread.
	 * @param Context:	The initialization context structure passed around during animation tree initialization, providing access to relevant data.
	 */
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

	/**
	 * Called to cache any bones that this node needs to track (e.g. in a FBoneReference). This is usually called at startup
	 * when LOD switches occur. This can be called on any thread.
	 * @param Context:	The context structure passed around when the RequiredBones array changes and cached bones indices have to be refreshed (e.g. when the LOD switches).
	 */
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;

	/**
	 * Called to update the state of the graph relative to this node. Generally, this should configure any weights (etc.)
	 * that could affect the poses that will need to be evaluated. This function is what usually executes EvaluateGraphExposedInputs.
	 * This can be called on any thread.
	 * @param Context:	The update context passed around during animation tree update.
	 */
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;

	/**
	 * Called to evaluate Local Space bones transforms according to the weights set up in Update(). The user should implement
	 * either Evaluate or EvaluateComponentSpace, but not both of these. This function can be called on any thread.
	 * @param Output:	The evaluation context passed around during animation tree evaluation.
	 */
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	/**
	 * Called to gather on-screen debug data. This is called on the game thread.
	 * @param DebugData:	The structure containing relevant debug data.
	 */
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	// ===== End of FAnimNode_Base

	// ==== Custom functions
	virtual bool GetShowDebugShapes() const;
	virtual bool SetShowDebugShapes(bool bInShowDebugShapes);
	virtual EDebugPoseMatchLevel GetDebugLevel() const;
	virtual bool SetDebugLevel (EDebugPoseMatchLevel InDebugLevel);
	virtual float GetPositionDrawScale() const;
	virtual bool SetPositionDrawScale(float InPositionDrawScale);
	virtual float GetVelocityDrawScale() const;
	virtual bool SetVelocityDrawScale(float InVelocityDrawScale);
	

public:

	/** The input source pose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Links")
	FPoseLink Source;

	/** The bone references to cache for pose-matching purposes. (This cannot be made BlueprintReadWrite.) */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (/*FoldProperty,*/ DisplayName = "Bones to Cache"))
	TArray<FBoneReference> BonesToCache;
	
#if WITH_EDITORONLY_DATA
	
	/** Indicates whether to draw debug shapes. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (FoldProperty, PinHiddenByDefault))
	bool bShowDebugShapes = false;

	/** The level of debug shapes to draw on screen. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (FoldProperty, PinHiddenByDefault, EditCondition = "bShowDebugShapes"))
	EDebugPoseMatchLevel DebugLevel = EDebugPoseMatchLevel::ShowSelectedPosePosition;

	/** The value by which to scale the radius of the debug sphere centered on the bone position. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (FoldProperty, PinHiddenByDefault, EditCondition = "bShowDebugShapes"))
	float PositionDrawScale = 5.0f;
	
	/** The value by which to scale the debug arrow corresponding to the normalized velocity of the bone. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (FoldProperty, PinHiddenByDefault, EditCondition = "bShowDebugShapes"))
	float VelocityDrawScale = 20.0f;

#endif
	
	
private:
	
	bool bWereBonesCachedThisFrame;
	
	/** The pose structure cached during the previous animation tree update. */
	FCachedPose CachedPose;

	/** The proxy object passed around during the animation tree in lieu of UAnimInstance. */
	FAnimInstanceProxy* AnimInstanceProxy;
	
};
