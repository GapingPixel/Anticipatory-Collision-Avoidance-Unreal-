// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNode_PoseRecorder.h"
#include "Utility/AnimSuiteTypes.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"


#include "AnimNode_SequencePlayerMatcher.generated.h"

/**
 * This node uses pose matching and distance matching to how the Sequence plays. 
 */
USTRUCT(BlueprintInternalUseOnly)
struct ANIMATIONMATCHINGSUITE_API FAnimNode_SequencePlayerMatcher : public FAnimNode_SequencePlayerBase
{
	GENERATED_BODY()

	friend class UAnimGraphNode_SequencePlayerMatcher;

public:

	FAnimNode_SequencePlayerMatcher(); 
	
	// FAnimNode_SequencePlayerBase interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual UAnimSequenceBase* GetSequence() const override;
	virtual bool SetSequence(UAnimSequenceBase* InSequence) override;
	virtual float GetPlayRateBasis() const override;
	virtual const FInputScaleBiasClampConstants& GetPlayRateScaleBiasClampConstants() const override;
	virtual float GetStartPosition() const override;
	virtual bool SetStartPosition(float InStartPosition) override;
	virtual bool IsLooping() const override;
	virtual bool SetLoopAnimation(bool bInLoopAnimation) override;
	virtual float GetPlayRate() const override;
	virtual bool SetPlayRate(float InPlayRate) override;
	virtual float GetCurrentAssetTime() const override;
	virtual float GetCurrentAssetTimePlayRateAdjusted() const override;
	virtual float GetCurrentAssetLength() const override;

	// FAnimNode_AssetPlayerBase interface
	virtual FName GetGroupName() const override;
	virtual bool SetGroupName(FName InGroupName) override;
	virtual EAnimGroupRole::Type GetGroupRole() const override;
	virtual bool SetGroupRole(EAnimGroupRole::Type InRole) override;
	virtual EAnimSyncMethod GetGroupMethod() const override;
	virtual bool SetGroupMethod(EAnimSyncMethod InMethod) override;
	virtual bool GetIgnoreForRelevancyTest() const override;
	virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest) override;

	// Custom functions
	virtual bool GetResetPlayTimeWhenSequenceChanges() const;
	virtual bool SetResetPlayTimeWhenSequenceChanges(bool bInReset);
	virtual EMatchingType GetMatchingType() const;
	virtual bool SetMatchingType(EMatchingType InMatchingType);
	virtual EMatchingRange GetMatchingRange() const;
	virtual bool SetMatchingRange(EMatchingRange InMatchingRange);
	virtual float GetInitialTime() const;
	virtual bool SetInitialTime(float InInitialFrame);
	virtual float GetFinalTime() const;
	virtual bool SetFinalTime(float InFinalFrame);
	virtual float GetSampleRate() const;
	virtual bool SetSampleRate(float InSampleRate);
	virtual bool GetShouldMatchVelocity() const;
	virtual bool SetShouldMatchVelocity(bool bInMatchVelocity);
	virtual float GetPositionWeight() const;
	virtual bool SetPositionWeight(float InPositionWeight);
	virtual float GetVelocityWeight() const;
	virtual bool SetVelocityWeight(float InVelocityWeight);
	virtual float GetMatchingDistance() const;
	virtual bool SetMatchingDistance(float InMatchingDistance);
	virtual FName GetDistanceCurveName() const;
	virtual bool SetDistanceCurveName(FName InDistanceCurveName);
	virtual FVector2D GetPlayRateClamp() const;
	virtual bool SetPlayRateClamp(FVector2D InPlayRateClamp);
	virtual bool GetShouldAdvanceTimeNaturally() const;
	virtual bool SetShouldAdvanceTimeNaturally(bool bInAdvanceTimeNaturally);
	virtual bool GetNegateDistanceValue() const;
	virtual bool SetNegateDistanceValue(bool bInNegateDistanceValue);
	virtual bool GetShouldSmoothTimeFollowingPoseMatch() const;
	virtual bool SetShouldSmoothTimeFollowingPoseMatch(bool bInShouldSmoothTimeFollowingPoseMatch);
	virtual float GetSmoothingAlpha() const;
	virtual bool SetSmoothingAlpha(float InSmoothingAlpha);
	virtual float GetStartDistance() const;
	virtual bool SetStartDistance(float InStartDistance);
	virtual bool GetShouldInertiallyBlendUponSequenceChange() const;
	virtual bool SetShouldInertiallyBlendUponSequenceChange(bool bInShouldInertiallyBlendUponSequenceChange);
	virtual UBlendProfile* GetBlendProfile() const;
	virtual float GetInertialBlendTime() const;
	virtual bool SetInertialBlendTime(float InInertialBlendTime);
	virtual bool GetShowDebugShapes() const;
	virtual bool SetShowDebugShapes(bool bInShowDebugShapes);
	virtual EDebugPoseMatchLevel GetDebugLevel() const;
	virtual bool SetDebugLevel (EDebugPoseMatchLevel InDebugLevel);
	virtual float GetPositionDrawScale() const;
	virtual bool SetPositionDrawScale(float InPositionDrawScale);
	virtual float GetVelocityDrawScale() const;
	virtual bool SetVelocityDrawScale(float InVelocityDrawScale);
	//virtual bool GetCanLoop() const;

	virtual bool ShouldTeleportToTime() const { return bTeleportToExplicitTime; }
	virtual bool IsEvaluator() const { return bTeleportToExplicitTime; }

	
	// More custom functions
protected:
	void UpdateInternal(const FAnimationUpdateContext& Context);

private:
	void Reinitialize(const FAnimationUpdateContext& Context);

protected:
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> PreviousSequence;

private:
#if WITH_EDITORONLY_DATA
	
	/** The group name. (GroupName = NAME_None if the node is not part of any group.) Pose matching disables syncing during node initialization and Sequence asset switching.*/
	UPROPERTY(EditAnywhere, Category = "Sync", meta=(FoldProperty))
	FName GroupName = NAME_None;
	
	/** The role this player can assume within the group (ignored if GroupIndex is INDEX_NONE). Pose matching disables syncing during node initialization and Sequence asset switching. */
	UPROPERTY(EditAnywhere, Category = "Sync", meta=(FoldProperty))
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;
	
	/** The method by which synchronization is determined. Pose matching disables syncing during node initialization and Sequence asset switching. */
	UPROPERTY(EditAnywhere, Category = "Sync", meta=(FoldProperty))
	EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;

	/** If true, "relevant anim" nodes that look for the highest weighted animation in a state will ignore this node. */
	UPROPERTY(EditAnywhere, Category = "Relevancy", meta=(FoldProperty, PinHiddenByDefault))
	bool bIgnoreForRelevancyTest = false;

	/** The play rate multiplier. Can be negative, which will cause the animation to play in reverse. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None || MatchingType == EMatchingType::PoseMatch", EditConditionHides))
	float PlayRate = 1.0f;

	/** The Basis in which the PlayRate is expressed. This is used to rescale PlayRate inputs.
		For example, a Basis of 100 means the PlayRate input will be divided by 100. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None || MatchingType == EMatchingType::PoseMatch", EditConditionHides))
	float PlayRateBasis = 1.0f;
	
	/** Additional scaling, offsetting, and clamping of PlayRate input. Performed after PlayRateBasis. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (DisplayName = "PlayRateScaleBiasClamp", FoldProperty, EditCondition = "MatchingType == EMatchingType::None || MatchingType == EMatchingType::PoseMatch", EditConditionHides))
	FInputScaleBiasClampConstants PlayRateScaleBiasClampConstants;

	/** Indicates whether the animation should loop back to the start when it reaches the end. */
	//UPROPERTY(EditAnywhere, Category = "Settings", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None || MatchingType == EMatchingType::PoseMatch", EditConditionHides))
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (PinHiddenByDefault, FoldProperty))
	bool bLoopAnimation = true;

	/** The start position between 0 and the length of the Sequence to use when initializing. When looping, play time will still jump back to the beginning (i.e. 0) when reaching the Sequence end is reached.
	    This function is internally overridden if pose matching or distance matching are enabled. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None", EditConditionHides))
	float StartPosition = 0.0f;

	/** Indicates whether the play time should be reset when the Sequence changes. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (DefaultValue = "false", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None", EditConditionHides))
	bool bResetPlayTimeWhenSequenceChanges = false;
	
	/** The matching type used to determine how the asset player is driven. */
	UPROPERTY(EditAnywhere, Category = "Matching", meta = (PinHiddenByDefault, FoldProperty))
	EMatchingType MatchingType = EMatchingType::PoseMatch;

	/** The setting governing the segment of the animation upon which to perform the pose search. */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	EMatchingRange MatchingRange = EMatchingRange::FullRange;

	/** The time at which to begin the pose search. */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (DisplayName = "Search Time Range Start", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingRange == EMatchingRange::CustomRange", EditConditionHides))
	float InitialTime = 0.0f;

	/** The time at which to end the pose search. This value must be greater than the InitialFrame value. */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (DisplayName = "Search Time Range End", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingRange == EMatchingRange::CustomRange", EditConditionHides))
	float FinalTime = 0.2f;
	
	/** The rate (i.e. fps) at which to sample poses from the Sequence. The default rate of 30.0f should be sufficient. */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (PinHiddenByDefault, FoldProperty, ClampMin = 10.0f, ClampMax = 180.0f, EditCondition = "MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	float SampleRate = 30.0f;

	/** Indicates whether velocity should be matched. This costs more performance but produces better results. */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bShouldMatchVelocity = true;

	/** The coefficient by which the pose position differences are multiplied when finding the lowest cost. If this value is greater than the VelocityWeight, the pose-search algorithm will favor position over velocity (and vice versa).*/
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (ClampMin = 0.0f, PinHiddenByDefault, FoldProperty, EditCondition = "(MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch) && bShouldMatchVelocity", EditConditionHides))
	float PositionWeight = 1.0f;

	/** The coefficient by which the pose velocity differences are multiplied when finding the lowest cost. If this value is greater than the PositionWeight, the pose-search algorithm will favor velocity over position (and vice versa).*/
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (ClampMin = 0.0f, PinHiddenByDefault, FoldProperty, EditCondition = "(MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch) && bShouldMatchVelocity", EditConditionHides))
	float VelocityWeight = 1.0f;
	
	/** The distance to match the animation pose to. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	float MatchingDistance = 0.0f;

	/** The name the distance curve to extract data from. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	FName DistanceCurveName = FName(TEXT("Distance"));

	/** The vector clamp of the allowed playrate, such that (X = min, Y = max). This has no effect if both values are set to zero. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	FVector2D PlayRateClamp = FVector2D(0.6f, 5.0f);

	/** Indicates whether to advance the time of the Sequence asset naturally (i.e. without distance matching.) This will override any smoothing applied. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bAdvanceTimeNaturally = false;

	/** Indicates whether the distance value should be negated. If true, the input Matching Distance, if greater than 
		zero, will be made negative for curve extraction purposes. NOTE: Negative inputs will not be modified. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bNegateDistanceValue = false;

	/** Indicates whether to smooth the playrate after pose matching has occurred. This smoothing will be on top of whatever PlayRateClamp values are set. Suggestion: use only
	    in the Stop and PrePivot states when the PlayRateClamp is ideally zeroed (removing the clamp's effect altogether, as done by Epic in the Lyra Starter Game). */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (DefaultValue = "false", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bShouldSmoothTimeFollowingPoseMatch = false;

	/** The smoothing alpha applied to the normalized time in the following way: CurrentTime + (1 - SmoothingAlpha) * (TargetTime - CurrentTime). This smoothing is applied ON TOP OF the PlayRateClamp,
	    meaning this smoothing alpha is best used in the Stop or PrePivot states, for example, when the PlayRateClamp is zeroed (removing the clamp altogether).
	    For best results, feed in a value that begins at 0.5 and drops to 0 as the character approaches the distance-match marker. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (ClampMin = 0.0f, ClampMax = 0.99f, PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	float SmoothingAlpha = 0.0f;
	
	/** The initial matching distance to use when initializing. Usually, the Start Distance should be set to the previous update's
	 *  Matching Distance to help prevent accelerated playrate when blending into this asset player. This should ONLY be set to a
	 *  nonzero value if this asset player will blended with another asset player. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (DefaultValue = "0.0f", ClampMin = 0.0f, PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	float StartDistance = 0.0f;
	
	/** Indicates whether inertial blending should be requested when the Sequence changes. If this behavior is desired,
	    bResetPlayTimeWhenSequenceChanges must also be set to FALSE. (This is required due to the non-overridable (non-virtual)
	    UpdateInternal function in the BlendSpace class.) */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, FoldProperty))
	bool bShouldInertiallyBlendUponSequenceChange = true;
	
	/** The blend duration used when inertial blending is requested and successfully activated. */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "bShouldInertiallyBlendUponSequenceChange", EditConditionHides))
	float InertialBlendTime = 0.2f;

	/** Indicates whether debug shapes should be drawn. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (DisplayName = "Show Debug Shapes (Pose Matching)", DefaultValue = "false", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bShowDebugShapes = false;

	/** The level of debug shapes to draw on screen. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "bShowDebugShapes && (MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch)", EditConditionHides))
	EDebugPoseMatchLevel DebugLevel = EDebugPoseMatchLevel::ShowSelectedPosePosition;

	/** The value by which to scale the radius of the debug sphere centered on the bone position. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (FoldProperty, PinHiddenByDefault, EditCondition = "bShowDebugShapes && (MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch)", EditConditionHides))
	float PositionDrawScale = 5.0f;
	
	/** The value by which to scale the debug arrow corresponding to the normalized velocity of the bone. */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (FoldProperty, PinHiddenByDefault, EditCondition = "bShowDebugShapes && ((MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch) && DebugLevel == EDebugPoseMatchLevel::ShowSelectedPosePositionAndVelocity)", EditConditionHides))
	float VelocityDrawScale = 20.0f;
	

#endif

	/** The Blend Profile to use when inertialization is requested. If none are selected, the default will be chosen by the Inertialization node. */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, EditCondition = "bShouldInertiallyBlendUponSequenceChange", EditConditionHides))
	TObjectPtr<UBlendProfile> BlendProfile;

	/** The animation sequence asset to play. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (PinHiddenByDefault, DisallowedClasses="/Script/Engine.AnimMontage"))
	TObjectPtr<UAnimSequenceBase> Sequence;

private:
	/** Indicates whether the Sequence asset was changed this frame. */
	bool bSequenceChanged;
	
	/** The array of cached pose bone data. */
	TArray<FPoseBoneData> CurrentSnapshotPose;

	/** Indicates whether the asset player just become relevant. This begins as true and gets set to false during the first update, that way pose matching only occurs once: upon becoming relevant. */
	bool bIsBeingReinitialized;

	/** Indicates whether the asset player was initialized this frame (and the functions called for initialization have already been called). */
	bool bWasReinitialized;

	/** The time driving the asset player. */
	float ExplicitTime;

	float PrevExplicitTime;
	
	/** The matching distance of the previous update. */
	float PreviousMatchingDistance;

	/** Indicates whether smoothing should be used after */
	bool bTriggerSmoothTimeFollowingPoseMatch;

	/** Indicates whether the asset player should teleport to the explicit time. If true, time is not advanced, meaning
		notifies aren't triggered, root motion is not extracted, etc. If false, time is advanced, meaning notifies will
		be triggered, root motion will be extracted (if applicable), etc. */
	bool bTeleportToExplicitTime;
	
	/** Indicates whether distance matching is occuring this frame. Even though the MatchType may be set to DistanceMatch,
		this bool will internally track whether distance matching is actually allowed on the current frame, that way the
		PlayRate can be set to 0.0f. If the PlayRate is nonzero, playback issues occur. */
	bool bIsDistanceMatching;

	/** The transforms for drawing debug spheres. */
	FAMSDebugData PoseDebugData;

	/** Indicates whether debug shapes should be drawn. Currently, this is defaulted as false and reset when pose matching is called. */
	bool bDrawDebugThisFrame;

	float DeltaTime;
};







