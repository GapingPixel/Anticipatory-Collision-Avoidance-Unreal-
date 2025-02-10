// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "Utility/AnimSuiteTypes.h"
#include "AnimNode_BlendSpacePlayerMatcher.generated.h"


USTRUCT(BlueprintInternalUseOnly)
struct ANIMATIONMATCHINGSUITE_API FAnimNode_BlendSpacePlayerMatcher : public FAnimNode_AssetPlayerBase 
{
	GENERATED_BODY()

	friend class UAnimGraphNode_BlendSpacePlayerMatcher;

public:

	FAnimNode_BlendSpacePlayerMatcher();

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_AssetPlayerBase interface
	virtual float GetCurrentAssetTime() const override;
	virtual float GetCurrentAssetTimePlayRateAdjusted() const override;
	virtual float GetCurrentAssetLength() const override;
	virtual UAnimationAsset* GetAnimAsset() const override;
	virtual FName GetGroupName() const override;
	virtual bool SetGroupName(FName InGroupName) override;
	virtual EAnimGroupRole::Type GetGroupRole() const override;
	virtual bool SetGroupRole(EAnimGroupRole::Type InRole) override;
	virtual EAnimSyncMethod GetGroupMethod() const override;
	virtual bool SetGroupMethod(EAnimSyncMethod InMethod) override;
	virtual bool GetIgnoreForRelevancyTest() const override;
	virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest) override;
	float GetTimeFromEnd(float CurrentTime) const;
	FVector GetFilteredPosition() const { return BlendFilter.GetFilterLastOutput(); }
	void SnapToPosition(const FVector& NewPosition);
	// End of FAnimNode_AssetPlayerBase interface

	// FAnimNode_BlendSpacePlayerBase interface
	virtual UBlendSpace* GetBlendSpace() const;
	virtual FVector GetPosition() const;
	virtual float GetStartPosition() const;
	virtual bool SetStartPosition(float InStartPosition);
	virtual float GetPlayRate() const;
	virtual bool GetLoop() const;
	virtual bool GetResetPlayTimeWhenBlendSpaceChanges() const;
	virtual bool SetResetPlayTimeWhenBlendSpaceChanges(bool bInReset);
	virtual bool SetBlendSpace(UBlendSpace* InBlendSpace);
	virtual bool SetPosition(FVector InPosition);
	virtual bool SetPlayRate(float InPlayRate);
	virtual bool SetLoop(bool bInLoop);
	// End of FAnimNode_BlendSpacePlayerBase interface

	// FAnimNode_BlendSpacePlayer interface
	virtual bool ShouldTeleportToTime() const { return bTeleportToNormalizedTime; }
	virtual bool IsEvaluator() const;
	// End of FAnimNode_BlendSpacePlayer interface
	

	// Custom functions
	virtual bool GetShouldInertiallyBlendUponBlendSpaceChange() const;
	virtual bool SetShouldInertiallyBlendUponBlendSpaceChange(bool bInShouldInertiallyBlendUponBlendSpaceChange);
	virtual bool GetShouldInertiallyBlendUponRapidAxisValueChange() const;
	virtual bool SetShouldInertiallyBlendUponRapidAxisValueChange(bool bInShouldInertiallyBlendUponRapidAxisValueChange);
	virtual FVector2D GetBlendTriggerThresholdDeltas() const;
	virtual bool SetBlendTriggerThresholdDeltas(FVector2D InBlendTriggerThresholdDeltas);
	virtual float GetInertialBlendTime() const;
	virtual bool SetInertialBlendTime(float InInertialBlendTime);
	virtual float GetSampleRate() const;
	virtual bool SetSampleRate(float InSampleRate);
	virtual bool GetShouldMatchVelocity() const;
	virtual bool SetShouldMatchVelocity(bool bInShouldMatchVelocity);
	virtual float GetPositionWeight() const;
	virtual bool SetPositionWeight(float InPositionWeight);
	virtual float GetVelocityWeight() const;
	virtual bool SetVelocityWeight(float InVelocityWeight);
	virtual bool GetShowDebugShapes() const;
	virtual bool SetShowDebugShapes(bool bInShowDebugShapes);
	virtual EDebugPoseMatchLevel GetDebugLevel() const;
	virtual bool SetDebugLevel (EDebugPoseMatchLevel InDebugLevel);
	virtual float GetPositionDrawScale() const;
	virtual bool SetPositionDrawScale(float InPositionDrawScale);
	virtual float GetVelocityDrawScale() const;
	virtual bool SetVelocityDrawScale(float InVelocityDrawScale);
	virtual EMatchingType GetMatchingType() const;
	virtual bool SetMatchingType(EMatchingType InMatchingType);
	virtual float GetMatchingDistance() const;
	virtual bool SetMatchingDistance(float InMatchingDistance);
	virtual FName GetDistanceCurveName() const;
	virtual bool SetDistanceCurveName(FName InDistanceCurveName);
	virtual bool GetShouldAdvanceTimeNaturally() const;
	virtual bool SetShouldAdvanceTimeNaturally(bool bInAdvanceTimeNaturally);
	virtual FVector2D GetPlayRateClamp() const;
	virtual bool SetPlayRateClamp(FVector2D InPlayRateClamp);
	virtual bool GetNegateDistanceValue() const;
	virtual bool SetNegateDistanceValue(bool bInNegateDistanceValue);
	virtual float GetStartDistance() const;
	virtual bool SetStartDistance(float InStartDistance);
	virtual bool GetShouldSmoothTimeFollowingPoseMatch() const;
	virtual bool SetShouldSmoothTimeFollowingPoseMatch(bool bInShouldSmoothTimeFollowingPoseMatch);
	virtual float GetSmoothingAlpha() const;
	virtual bool SetSmoothingAlpha(float InSmoothingAlpha);
	virtual bool GetShouldInertiallyBlendFrameToFrame() const;
	virtual bool SetShouldInertiallyBlendFrameToFrame(bool InShouldInertiallyBlendFrameToFrame);
	virtual bool GetUseOnlyHighestWeightedSampleForDistanceMatching() const;
	virtual bool SetUseOnlyHighestWeightedSampleForDistanceMatching(bool bInUseOnlyHighestWeightedSampleForDistanceMatching);
	virtual bool GetUseOnlyHighestWeightedSampleForPoseMatching() const;
	virtual bool SetUseOnlyHighestWeightedSampleForPoseMatching(bool bInUseOnlyHighestWeightedSampleForPoseMatching);
	virtual UBlendProfile* GetBlendProfile() const;
	
protected:

	void UpdateInternal(const FAnimationUpdateContext& Context);
	
private:

	void Reinitialize(const FAnimationUpdateContext& Context);
	
	const FBlendSampleData* GetHighestWeightedSample() const;
	
protected:
	
	/** The filter used to dampen coordinate changes. */
	FBlendFilter BlendFilter;

	/** The cache of samples used to determine blend weights. */
	TArray<FBlendSampleData> BlendSampleDataCache;

	/** The previous position in the triangulation/segmentation */
	int32 CachedTriangulationIndex = -1;

	/** The Blend Space of the previous frame.*/
	UPROPERTY(Transient)
	TObjectPtr<UBlendSpace> PreviousBlendSpace = nullptr;
	
private:

#if WITH_EDITORONLY_DATA
	
	/** The group name. (GroupName = NAME_None if it is not part of any group). */
	UPROPERTY(EditAnywhere, Category = "Sync", meta = (FoldProperty))
	FName GroupName = NAME_None;
	
	/** The role this asset player can assume within the group (ignored if GroupIndex is INDEX_NONE). */
	UPROPERTY(EditAnywhere, Category = "Sync", meta = (FoldProperty))
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;
	
	/** The method by which synchronization is determined. */
	UPROPERTY(EditAnywhere, Category = "Sync", meta = (FoldProperty))
	EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;
	
	/** Indicates whether "Relevant anim" nodes that look for the highest weighted animation in a state will ignore this node. */
	UPROPERTY(EditAnywhere, Category = "Relevancy", meta = (FoldProperty, PinHiddenByDefault))
	bool bIgnoreForRelevancyTest = false;

	/** The X coordinate to sample in the Blend Space. */
	UPROPERTY(EditAnywhere, Category = "Coordinates", meta = (PinShownByDefault, FoldProperty))
	float X = 0.0f;

	/** The Y coordinate to sample in the Blend Space. */
	UPROPERTY(EditAnywhere, Category = "Coordinates", meta = (PinShownByDefault, FoldProperty))
	float Y = 0.0f;
	
	/** The play rate multiplier. Can be negative, which will cause the animation to play in reverse. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (DefaultValue = "1.0", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None || MatchingType == EMatchingType::PoseMatch", EditConditionHides))
	float PlayRate = 1.0f;

	/** The matching type used to determine how the asset player is driven. */
	UPROPERTY(EditAnywhere, Category = "Matching", meta = (PinHiddenByDefault, FoldProperty))
	EMatchingType MatchingType = EMatchingType::PoseMatch;
	
	/** The rate (i.e. fps) at which to sample poses from the Sequence. The default rate of 30.0f should be sufficient. */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (FoldProperty, ClampMin = 10.0f, ClampMax = 180.0f, EditCondition = "MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	float SampleRate = 30.0f;

	/** Indicates whether velocity should be matched. This costs more performance but may produce better results. */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bShouldMatchVelocity = false;

	/** The coefficient by which the pose position differences are multiplied when finding the lowest cost. If this value is greater than the VelocityWeight, the pose-search algorithm will favor position over velocity (and vice versa).*/
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (ClampMin = 0.0f, PinHiddenByDefault, FoldProperty, EditCondition = "(MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch) && bShouldMatchVelocity", EditConditionHides))
	float PositionWeight = 1.0f;

	/** The coefficient by which the pose velocity differences are multiplied when finding the lowest cost. If this value is greater than the PositionWeight, the pose-search algorithm will favor velocity over position (and vice versa).*/
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (ClampMin = 0.0f, PinHiddenByDefault, FoldProperty, EditCondition = "(MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch) && bShouldMatchVelocity", EditConditionHides))
	float VelocityWeight = 1.0f;
	
	/** Indicates whether only the highest weighted sample (Sequence) should be used when determining the pose to match.
		This will save some performance but will produce slightly different results than using the blended pose produced by all relevant samples.  */
	UPROPERTY(EditAnywhere, Category = "Pose Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::PoseMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bUseOnlyHighestWeightedSampleForPoseMatching = true;
	
	/** The distance to match the animation pose to. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	float MatchingDistance = 0.0f;

	/** The name the distance curve to extract data from. IMPORTANT: All sequence in the BlendSpace must have a distance curve in order to achieve good results. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	FName DistanceCurveName = FName(TEXT("Distance"));
	
	/** The vector clamp of the allowed playrate, such that (X = min, Y = max). This has no effect if both values are set to zero. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	FVector2D PlayRateClamp = FVector2D(0.6f, 5.0f);

	/** Indicates whether to advance the time of the Blend Space asset naturally (i.e. without distance matching.) This will override any smoothing applied. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bAdvanceTimeNaturally = false;

	/** Indicates whether the distance value should be negated. If true, the input Matching Distance, if greater than 
	    zero, will be made negative for curve extraction purposes. NOTE: Negative inputs will not be modified. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bNegateDistanceValue = false;

	/** Indicates whether only the highest weighted sample (Sequence) should be used when calculating the normalized time used to drive the Blend Space player.
	    This will save some performance but will produce slightly different results than using all relevant samples to drive playback. */
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bUseOnlyHighestWeightedSampleForDistanceMatching = true;
	
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
	UPROPERTY(EditAnywhere, Category = "Distance Matching", meta = (DefaultValue = "0.0f", ClampMin = 0.0f,  PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	float StartDistance = 0.0f;
	
	/** Indicates whether inertial blending should be requested when the Blend Space changes. If this behavior is desired,
	    bResetPlayTimeWhenBlendSpaceChanges must also be set to FALSE. (This is required due to the non-overridable (non-virtual)
	    UpdateInternal function in the BlendSpace class.) */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, FoldProperty))
	bool bShouldInertiallyBlendUponBlendSpaceChange = true;

	//@TODO: This does not seem to behave as expected. Remove for now. I must be missing something. This may only work if interpolation is not already being performed within the anim asset being played. Can interpolation be turned off at runtime? 
	/** Indicates whether inertial blending should be requested each frame during distance matching AFTER a pose match has occurred.
	    This, in addition to the SmoothAlpha, can help smooth animation as it jumps quickly through frames.
	    NOTE: Frame-to-frame blending will only activate during distance matching following a pose match. */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::DistanceMatch || MatchingType == EMatchingType::PoseAndDistanceMatch", EditConditionHides))
	bool bShouldInertiallyBlendFrameToFrame = false;

	/** Indicates whether inertial blending should be requested when axis values change rapidly, where the rapidity is defined in the BlendTriggerThresholdDeltas */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, FoldProperty))
	bool bShouldInertiallyBlendUponRapidAxisValueChange = false;

	/** The normalized X/Y Blend Input deltas that trigger an inertial blend request. The deltas are normalized based on the axis values of the Blend Space asset. */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, FoldProperty, ClampMin = 0.0f, ClampMax = 1.0f, EditCondition = "bShouldInertiallyBlendUponRapidAxisValueChange", EditConditionHides))
	FVector2D BlendTriggerThresholdDeltas = FVector2D(0.3f, 0.3f);
	
	/** The blend duration used when inertial blending is requested and successfully activated. */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, FoldProperty, EditCondition = "bShouldInertiallyBlendUponBlendSpaceChange || bShouldInertiallyBlendUponRapidAxisValueChange", EditConditionHides))
	float InertialBlendTime = 0.2f;
	
	/** Should the animation loop back to the start when it reaches the end? */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (DefaultValue = "true", PinHiddenByDefault, FoldProperty))
	bool bLoop = true;

	/** Indicates whether the play time should be reset when the Blend Space changes. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (DefaultValue = "false", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None", EditConditionHides))
	bool bResetPlayTimeWhenBlendSpaceChanges = false;

	/** The position at which to begin playing the asset when the Blend Space changes and/or this node is initialized. However, this value will internally disabled if matching is active. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (DefaultValue = "false", PinHiddenByDefault, FoldProperty, EditCondition = "MatchingType == EMatchingType::None", EditConditionHides))
	float StartPosition = 0.0f;
	
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

	/** The set of per-bone scales that tweak the weights of specific bones when inertialization is requested. If no profile is selected, the default will be chosen by the Inertialization node. */
	UPROPERTY(EditAnywhere, Category = "Blending", meta = (PinHiddenByDefault, EditCondition = "bShouldInertiallyBlendUponBlendSpaceChange", EditConditionHides))
	TObjectPtr<UBlendProfile> BlendProfile;
	
	/** The array of cached pose bone data. These data will be gathered from the PoseRecorder node (i.e. the custom Pose Grabber node.) */
	TArray<FPoseBoneData> CurrentSnapshotPose;
	
	/** The blendspace asset to play. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (PinHiddenByDefault))
	TObjectPtr<UBlendSpace> BlendSpace;

	/** Indicates whether the Blend Space asset was changed this frame. */
	bool bBlendSpaceChanged;
	
	/** Indicates whether the evaluator just become relevant. This begins as true and gets set to false during the first update. */
	bool bIsBeingReinitialized;

	/** Indicates whether the asset player was initialized this frame (and the functions called for initialization have already been called). */
	bool bWasReinitialized;
	
	/** The normalized time in the range [0,1].*/
	float NormalizedTime;
	
	/** The matching distance of the previous update. */
	float PreviousMatchingDistance;

	/** The module containing functions used to extract data from animation curves. */
	//FDistanceMatchingModule DistanceMatchingModule;

	/** Indicates whether smoothing should be used after */
	bool bTriggerSmoothTimeFollowingPoseMatch;

	/** Indicates whether the asset player should teleport to the normalized time. If true, time is not advanced, meaning
	    notifies aren't triggered, root motion is not extracted, etc. If false, time is advanced, meaning notifies will
	    be triggered, root motion will be extracted (if applicable), etc. */
	bool bTeleportToNormalizedTime;

	/** Indicates whether distance matching is occuring this frame. Even though the MatchType may be set to DistanceMatch,
	    this bool will internally track whether distance matching is actually allowed on the current frame, that way the
	    PlayRate can be set to 0.0f. If the PlayRate is nonzero, playback issues occur. */
	bool bIsDistanceMatching;

	/** The previous update's Blend Input axis values. */
	FVector2D PrevBlendInputAxisValues;

	/** The min/max axis values of the two axes of the Blend Space asset. */
	float XMin;
	float XMax;
	float YMin;
	float YMax;

	/** The transforms for drawing debug spheres. */
	FAMSDebugData PoseDebugData;

	/** Indicates whether debug shapes should be drawn. Currently, this is defaulted as false and reset when pose matching is called. */
	bool bDrawDebugThisFrame;
};








