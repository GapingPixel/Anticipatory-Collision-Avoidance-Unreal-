// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimSuiteTypes.h"
#include "AnimSuiteMathLibrary.generated.h"

UCLASS(BlueprintType)
class ANIMATIONMATCHINGSUITE_API UAnimSuiteMathLibrary : public UBlueprintFunctionLibrary 
{
	GENERATED_BODY()

public:

//-------------------------------------
// Pose Matching 
//-------------------------------------
	
	/**
	 * Determines the initial time at which to begin playing a Sequence.
	 * @param Context:				The update context passed around during animation tree update
	 * @param CurrentSnapshotPose:	An array of positions and velocities for cached bones.
	 * @param AnimSequence:			The Blend Space for which to determine the initial play time.
	 * @param SampleRate:			The rate, in samples per second, to extract poses from the Blend Space.
	 * @param DebugData:			The struct containing the transforms and velocities of the minimum cost pose. These are used to draw debug shapes. @TODO: change the debug approach.
	 * @param bIsLooping:			Indicates whether the Sequence is allowed to loop
	 * @param bMatchVelocity:		Indicates whether to match the velocity when determining the output time.
	 * @param PositionWeight:		The coefficient by which the pose position differences are multiplied when finding the lowest cost. If this value is greater than the VelocityWeight, the pose-search algorithm will favor position over velocity (and vice versa).
	 * @param VelocityWeight:		The coefficient by which the pose velocity differences are multiplied when finding the lowest cost. If this value is greater than the PositionWeight, the pose-search algorithm will favor velocity over position (and vice versa).
	 * @param bSaveDebugData:		Indicates whether to save debug data. This costs a lot of performance. Turn it to true ONLY for testing.
	 * @param MatchingRange:		The setting governing the segment of the animation upon which to perform the pose search.
	 * @param InitialTime:			The frame at which to begin the pose search if MatchingRange = EMatchingRange::CustomRange.
	 * @param FinalTime:			The frame at which to end the pose search if MatchingRange = EMatchingRange::CustomRange.
	 * @return Returns the time at which to begin playing the Sequence.
	 */
	static float DetermineInitialTime(const FAnimationUpdateContext& Context, TArray<FPoseBoneData>& CurrentSnapshotPose,
		UAnimSequence* AnimSequence, float SampleRate, FAMSDebugData& DebugData, bool bIsLooping = false, bool bMatchVelocity = true,
		float PositionWeight = 1.0f, float VelocityWeight = 1.0f, bool bSaveDebugData = false, EMatchingRange MatchingRange = EMatchingRange::FullRange,
		float InitialTime = 0.0f, float FinalTime = 0.2f);

	/**
	 * Determines the initial time at which to begin playing a Blend Space.
	 * @param Context:							The update context passed around during animation tree update
	 * @param CurrentSnapshotPose:				An array of positions and velocities for cached bones.
	 * @param BlendSpace:						The Blend Space for which to determine the initial play time.
	 * @param BlendSampleData:					An array of Blend Space sample data (animations, play rate, time, weight, etc.).
	 * @param SampleRate:						The rate, in samples per second, to extract poses from the Blend Space.
	 * @param DebugData:						The struct containing the transforms and velocities of the minimum cost pose. These are used to draw debug shapes. @TODO: change the debug approach.
	 * @param bIsLooping:						Indicates whether the Blend Space is allowed to loop.
	 * @param bMatchVelocity:					Indicates whether velocities should be calculated to contribute in the pose match.
	 * @param bUseOnlyHighestWeightedSample:	Indicates whether to ONLY use the highest weighted Blend Sample in determining the time.
	 * @param PositionWeight:					The coefficient by which the pose position differences are multiplied when finding the lowest cost. If this value is greater than the VelocityWeight, the pose-search algorithm will favor position over velocity (and vice versa).
	 * @param VelocityWeight:					The coefficient by which the pose velocity differences are multiplied when finding the lowest cost. If this value is greater than the PositionWeight, the pose-search algorithm will favor velocity over position (and vice versa).
	 * @param bSaveDebugData:					Indicates whether to save debug data. This costs a lot of performance. Turn it to true ONLY for testing.
	 * @return Returns the time at which to begin playing the Blend Space.
	 */
	static float DetermineInitialTime(const FAnimationUpdateContext& Context, TArray<FPoseBoneData>& CurrentSnapshotPose,
		UBlendSpace* BlendSpace, TArray<FBlendSampleData>& BlendSampleData, float SampleRate, FAMSDebugData& DebugData, bool bIsLooping = false, bool bMatchVelocity = true,
		bool bUseOnlyHighestWeightedSample = false, float PositionWeight = 1.0f, float VelocityWeight = 1.0f, const bool bSaveDebugData = false);
	
	/**
	 * Extracts all the match-bone transforms in Component Space.
	 * @param OutBoneTransform_CS:	The resulting transforms to be outputted by this function.
	 * @param Sequence:				The animation Sequence analyzed.
	 * @param MatchBoneNames:		The array of bone names cached by the Pose Grabber.
	 * @param AnimInstanceProxy:	The proxy object passed around during animation tree update in lieu of a UAnimInstance.
	 * @param Time:					The time at which to analyze the input Sequence.
	 * @param TimeInterval:		The time between samples.
	 * @param bIsLooping:		Indicates whether the current animation loops. (This can help reduce calculation cost.)
	 */
	static void ExtractBoneTransforms_CS(TArray<FTransform>& OutBoneTransform_CS, UAnimSequence* Sequence, TArray<FName>& MatchBoneNames,
									FAnimInstanceProxy& AnimInstanceProxy, float Time, const float TimeInterval, bool bIsLooping = true);

	/**
	 * Extracts all the match-bone transforms in Component Space.
	 * @param OutBoneTransform_CS:			The resulting transforms to be outputted by this function.
	 * @param BlendSampleData:				The data corresponding to the relevant Blend Samples (i.e. the relevant Sequences).
	 * @param MatchBoneNames:				The array of bone names cached by the Pose Grabber.
	 * @param AnimInstanceProxy:			The proxy object passed around during animation tree update in lieu of a UAnimInstance.
	 * @param NormalizedTime:				The normalized time at which to analyze the input Blend Space.
	 * @param NormalizedTimeInterval:		The normalized interval of time between samples.
	 * @param bIsLooping:					Indicates whether the current animation loops. (This can help reduce calculation cost.)
	 * @param bUseHighestWeightedSample:	Indicates whether to only use the highest weight Blend Sample during calculations.
	 * @param HighestWeightedSampleIdx:		The index of the highest weight Blend Sample.
	 */
	static void ExtractBoneTransforms_CS(TArray<FTransform>& OutBoneTransform_CS, TArray<FBlendSampleData>& BlendSampleData,
				const TArray<FName>& MatchBoneNames, FAnimInstanceProxy& AnimInstanceProxy, float NormalizedTime, const float NormalizedTimeInterval,
				const bool bIsLooping, const bool bUseHighestWeightedSample, const int32 HighestWeightedSampleIdx);

	/**
	 * Get the highest weight Blend Sample (i.e. Sequence) and the corresponding index from the supplied Blend Sample Data.
	 * @param BlendSampleData:		The struct containing data (weight, Sequence, etc.) pertaining to the relevant Blend Samples of a Blend Space asset.
	 * @param HighestWeightIndex:	The index of the highest weight Blend Sample.
	 * @return Returns the Sequence of the highest weight Blend Sample.
	 */
	static UAnimSequence* GetHighestWeightSample(const TArray<FBlendSampleData> BlendSampleData, int32& HighestWeightIndex);

	/**
	 * Gets the animation track index from a given bone name.
	 * @param Sequence:	The animation Sequence on which to act.
	 * @param BoneName: The bone name for which to retrieve the corresponding index.
	 * @return Returns the animation track index.
	 */
	static int32 GetAnimTrackIndexFromBoneName(UAnimSequence* Sequence, const FName& BoneName);
	
	/**
	 * Extrapolates a bone-related transform beyond the limits of an animation asset. As with all extrapolation, this is
	 * useful only for small ventures beyond the start and end of the asset's data.
	 * @param SampleToExtrapolate:		The transform to extrapolate, such as a root motion delta transform. (Any transform is valid, however.)
	 * @param BeginTime:				The beginning of the time range of known data used to perform extrapolation.
	 * @param EndTime:					The end of the time range of known data used to perform extrapolation.
	 * @param ExtrapolationTime:		The time value to which to extrapolate the known data.
	 * @return Returns the extrapolated transform of the given data.
	 */
	static FTransform ExtrapolateTransform(FTransform SampleToExtrapolate, float BeginTime, float EndTime,
									float ExtrapolationTime);
	
	/**
	 * Extrapolates a bone-related transform beyond the limits of an animation asset. As with all extrapolation, this is
	 * useful only for small ventures beyond the start and end of the asset's data.
	 * @param SampleToExtrapolate:		The transform to extrapolate, such as a root motion delta transform. (Any transform is valid, however.)
	 * @param BeginTime:				The beginning of the time range of known data used to perform extrapolation.
	 * @param EndTime:					The end of the time range of known data used to perform extrapolation.
	 * @param ExtrapolationTime:		The time value to which to extrapolate the known data.
	 * @param ExtrapolationParameters:	The parameters for how to extrapolate.
	 * @return Returns the extrapolated transform of the given data.
	 */
	static FTransform ExtrapolateTransform(FTransform SampleToExtrapolate, float BeginTime, float EndTime,
									float ExtrapolationTime, const FPoseExtrapolationParameters& ExtrapolationParameters);

	/**
	 * Calculates the of pose costs based on difference in transforms (positions).
	 * @param SourceBoneTransforms:	The bone transforms of the source (i.e. the cached pose of the previous frame).
	 * @param CandidateBoneTransforms: The bone transforms of the target (i.e. the target Sequence or Blend Space to be searched).
	 * @return Returns the sum of the unnormalized position costs.
	 */
	static float CalculatePoseCost(const TArray<FTransform>& SourceBoneTransforms, const TArray<FTransform>& CandidateBoneTransforms);

	/**
	 * Calculates the of pose costs based on difference in velocities.
	 * @param SourceBoneVelocities:	The bone velocities of the source (i.e. the cached pose of the previous frame).
	 * @param CandidateBoneVelocities: The bone velocities of the target (i.e. the target Sequence or Blend Space to be searched).
	 * @return Returns the sum of the unnormalized velocity costs.
	 */
	static float CalculatePoseCost(const TArray<FVector>& SourceBoneVelocities,
	                               const TArray<FVector>& CandidateBoneVelocities);

	/**
	 * Calculates velocities by on the given transforms and time interval. A simple backward finite difference is used.
	 * @param OutVelocities:		The velocities computed.
	 * @param CurrentTransforms:	The transforms corresponding to "this frame," whatever that may mean in the context.
	 * @param PrevTransforms:		The transforms corresponding to the "previous frame," whatever that may mean in the context.
	 * @param TimeInterval:			The time difference to used in the backward finite difference calculation.
	 */
	static void CalculatePoseVelocities(TArray<FVector>& OutVelocities, const TArray<FTransform>& CurrentTransforms,
								const TArray<FTransform>& PrevTransforms, const float TimeInterval);

	/**
	 * Find the index of minimum cost between two array after normalizing the arrays. (Each value in both arrays is normalized
	 * to range [0,1].
	 * @TODO: bring the min/max calculation into the function rather than requiring they be inputs.
	 * @param ArrayX:	The first array of unnormalized costs.	
	 * @param MinX:		The minimum value of ArrayX.
	 * @param MaxX:		The maximum value of ArrayX.
	 * @param ArrayY:	The second array of unnormalized costs.
	 * @param MinY:		The minimum value of ArrayY.
	 * @param MaxY:		The maximum value of ArrayY.
	 * @return Returns the index corresponding to the lowest cost element.
	 */
	static int32 FindNormalizedMinCostIndex(TArray<float>& ArrayX, const float MinX, const float MaxX, TArray<float>& ArrayY,
										const float MinY, const float MaxY);
	
	/**
	 * Sets the Bone Container using all the bones from the input Skeleton.
	 * @param Skeleton:	The current skeleton of the AnimInstanceProxy.
	 * @return Returns the native transient structure containing bone information.
	 */
	static FBoneContainer SetBoneContainer(USkeleton* Skeleton);

	/**
	 * Get the bone transform (pose) for the provided bone name.
	 * @param Pose:		The animation pose from which to extract the bone pose.
	 * @param BoneName:	The name of the bone from which to get the transform.
	 * @param Space:	The space in which the transform should be retrieved.
	 */
	static const FTransform& GetBonePose(const FAMSPose& Pose, FName BoneName, EAMSPoseSpaces Space = EAMSPoseSpaces::Component);
	
	/**
	 * Evaluates an animation Sequence (base) to generate a valid animation pose
	 * @param AnimationSequenceBase:	The animation Sequence (base) to evaluate the pose from.
	 * @param Time:						The time at which the pose should be evaluated.
	 * @param EvaluationOptions:		The options determining the way the pose should be evaluated.
	 * @param Pose:						The animation pose storing the evaluated data.
	 */
	static void GetAnimPoseAtTime(const UAnimSequenceBase* AnimationSequenceBase, double Time, FAMSPoseEvaluationOptions EvaluationOptions, FAMSPose& Pose);

	/**
	 * Evaluates an animation Sequence Base to generate a valid Anim Pose instance
	 * @param AnimationSequenceBase:	The animation Sequence (base) to evaluate the pose from.
	 * @param Times:					The array of times at which the pose should be evaluated.
	 * @param EvaluationOptions:		The options determining the way the pose should be evaluated.
	 * @param InOutPoses:				The animation poses storing the evaluated data.
	 */
	static void GetAnimPoseAtTimeIntervals(const UAnimSequenceBase* AnimationSequenceBase, TArray<double> Times, FAMSPoseEvaluationOptions EvaluationOptions, TArray<FAMSPose>& InOutPoses);

	
//-------------------------------------
// Distance Matching 
//-------------------------------------

/**
	 * Calculates the playrate-clamped normalized time value.
	 * @param BlendSpace:					The BlendSpace to operate on.
	 * @param MatchingDistance:				The distance to match the animation pose to.
	 * @param PrevNormalizedTime:			The previous update's normalized time value.
	 * @param BlendSampleData:				The data relating to animation Sequences within the BlendSpace.
	 * @param DeltaTime:					The time between the previous and current time of the Engine.
	 * @param DistanceCurveName:			The name of the curve to match the pose to.
	 * @param PlayRateClamp:				The [min,max] playrate clamp.
	 * @param bAdvanceTimeNaturally:		Indicates whether to advance time naturally (i.e. without distance matching).
	 * @param bUseHighestWeightedSample:	Indicates whether to use ONLY the highest weighted sample when calculating the time.
	 * @param bIsLooping:					Indicates whether the animation is allowed to loop.
	 * @return Returns the playrate-clamped normalized time.
	 */
	static float CalculateNormalizedTime(const UBlendSpace* BlendSpace, const float MatchingDistance, const float PrevNormalizedTime,
									TArray<FBlendSampleData>& BlendSampleData, const float DeltaTime, const FName DistanceCurveName,
									const FVector2D PlayRateClamp, const bool bAdvanceTimeNaturally = false, const bool bUseHighestWeightedSample = false,
									const bool bIsLooping = false);
	
	/**
	 * Calculates the playrate-clamped explicit time value.
	 * @param Sequence:					The Sequence to operate on.
	 * @param MatchingDistance:			The distance to match the animation pose to.
	 * @param PrevExplicitTime:			The previous update's normalized time value.
	 * @param DeltaTime:				The time between the previous and current time of the Engine.
	 * @param DistanceCurveName:		The name of the curve to match the pose to.
	 * @param PlayRateClamp:			The [min,max] playrate clamp.
	 * @param bAdvanceTimeNaturally:	Indicates whether to advance time naturally (i.e. without distance matching).
	 * @param bIsLooping:				Indicates whether the animation is allowed to loop.
	 */
	static float CalculateExplicitTime(const UAnimSequenceBase* Sequence, const float MatchingDistance, const float PrevExplicitTime,
					const float DeltaTime, const FName DistanceCurveName, const FVector2D PlayRateClamp, const bool bAdvanceTimeNaturally = false,
					const bool bIsLooping = false);
	
	/**
	 * Gets the curve UID of the given curve name on the input animation sequence.
	 * @param InAnimSequence:	The sequence to operate on.
	 * @param CurveName:		The name of the curve to the UID for.
	 * @return Returns the curve UID.
	 */
	static USkeleton::AnimCurveUID GetCurveUID(const UAnimSequenceBase* InAnimSequence, FName CurveName);

	/**
	 * Gets the time (x-axis) value of an animation corresponding to the distance (y-axis) value of a given curve within the animation.
	 * @param InAnimSequence:	The animation sequence to operate on.
	 * @param InValue:			The value for which we want the corresponding time. (This "value" is often a distance or rotation value.)
	 * @param CurveName:		The name of the curve from which our InValue is of like kind (e.g. DistanceCurve, RotationCurve, etc.).
	 * @return Returns the time (x-axis) value of an animation corresponding to the distance (y-axis) value of a given curve
	 */
	static float GetAnimTimeFromCurveValue(const UAnimSequenceBase* InAnimSequence, const float InValue, const FName CurveName);

	/**
	 * Gets the curve value (on the y axis) of an animation corresponding to a given time (x-axis) value.
	 * @param InAnimSequence:	The animation sequence to operate on.
	 * @param InTime:			The time for which we want the corresponding y-axis value.
	 * @param CurveName:		The name of the curve from which data are extracted.
	 * @return Returns the y-axis value of an animation curve corresponding the given time (x-axis) value.
	 */
	static float GetCurveValueFromAnimTime(const UAnimSequenceBase* InAnimSequence, const float& InTime, const FName CurveName);
	
	/** 
	 * Gets the distance traveled by taking the difference between the last key's value and the first key's value.
	 * @param InAnimSequence:	The animation sequence to operate on.
	 * @param CurveName:		The name of the curve to the UID for.
	 * @return Returns the difference in value between the first and last key of the animation.
	 */
	static float GetDistanceRange(const UAnimSequenceBase* InAnimSequence, const FName CurveName);
	
	/**
	* Gets the new sequence time given a the DistanceTraveled since the previous update.
	* @param AnimSequence:			The animation sequence to operate on.
	* @param CurrentTime:			The current time of the animation sequence.
	* @param DistanceTraveled:		The distance traveled by the authored root motion.
	* @param DistanceCurveName:		The name of the distance curve on the animation sequence.
	* @param StuckLoopThreshold:	Due to a monotonic curve assumption, this value is the number of iterations after which the algorithm is abandoned to avoid an infinite loop.
	* @param bAllowLooping:			Indicates whether looping should be considered in the calculation.
	* @return Returns the new time resulting from the distance traveled by the authored root motion.
	*/
	static float GetTimeAfterDistanceTraveled(const UAnimSequenceBase* AnimSequence, const float CurrentTime, const float DistanceTraveled,
									const FName DistanceCurveName, const int32 StuckLoopThreshold = 5, const bool bAllowLooping = false);

	
};





