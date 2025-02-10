// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "AnimGraph/AnimNode_BlendSpacePlayerMatcher.h"
#include "Animation/AnimNodeReference.h"
#include "BlendSpacePlayerLibrary.h"
#include "SequencePlayerLibrary.h"
#include "AnimGraph/AnimNode_SequencePlayerMatcher.h"
#include "AnimNodes/AnimNode_BlendSpaceEvaluator.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimSuiteNodeHelperLibrary.generated.h"

USTRUCT(BlueprintType)
struct FBlendSpacePlayerReturnValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Matching Suite|BlendSpace Player|Return Values")
	FBlendSpacePlayerReference BlendSpacePlayer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Matching Suite|BlendSpace Player|Return Values")
	float NormalizedTime = 0.0f;
	
};

USTRUCT(BlueprintType)
struct FSequencePlayerReturnValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Matching Suite|Sequence Player|Return Values")
	FSequencePlayerReference SequencePlayer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Matching Suite|Sequence Player|Return Values")
	float ExplicitTime  = 0.0f;
	
};

USTRUCT()
struct FBlendSpaceMatcherReference: public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_BlendSpacePlayerMatcher FInternalNodeType;
	
};

USTRUCT()
struct FSequenceMatcherReference: public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_SequencePlayerMatcher FInternalNodeType;
};

/**
 * The function library that exposes functions to be applied to blendspace evaluator nodes.
 */
UCLASS()
class ANIMATIONMATCHINGSUITE_API UAnimSuiteNodeHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

//-------------------------------------
// Blend Space Matcher
//-------------------------------------

	/**
	 * Gets the scaled accumulated time (in seconds) of the BlendSpace (Match) Player. This can be used to ease in the Stride
	 * Warping Alpha and smoothly increase the minimum playrate. (See the Lyra sample project for an example of how
	 * this is done for Sequence assets.)
	 * @param BlendSpaceMatchedPlayer:	The BlendSpace (Match) Player being acted on.
	 * @return Returns the scaled accumulated time.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Get Scaled Accumulated Time (Matching)"))
	static float GetMatchedScaledAccumulatedTime(const FBlendSpaceMatcherReference& BlendSpaceMatchedPlayer);

	/**
	 * Gets a blend space player context from an anim node context.
	 * @param Node:		The reference to the anim node to which a function is bound, within which this conversion function is nested and called. This reference does not persist and is only valid for the call in which it was retrieved.
	 * @param Result:	The result of the anim node reference conversion. (Succeeded = 1. Failed = 0.)
	 * @return Returns a BlendSpace Player (Matching) Reference.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result", DisplayName = "Convert to Blend Space Player (Matching)"))
	static FBlendSpaceMatcherReference ConvertToMatchedBlendSpacePlayer(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);

	/**
	 * Gets a blend space player context from an anim node context (pure).
	 * @param Node:						The reference to the anim node to which a function is bound, within which this conversion function is nested and called. This reference does not persist and is only valid for the call in which it was retrieved.
	 * @param BlendSpaceMatchedPlayer:	The BlendSpace Player (Matching) to act on.
	 * @param Result:					The boolean result of the anim node reference conversion. 
	 * @return Returns a BlendSpace Player (Matching) Reference.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Convert to Blend Space Player (Matching) (Pure)"))
	static void ConvertToMatchedBlendSpacePlayerPure(const FAnimNodeReference& Node, FBlendSpaceMatcherReference& BlendSpaceMatchedPlayer, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		BlendSpaceMatchedPlayer = ConvertToMatchedBlendSpacePlayer(Node, ConversionResult);
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}

	/**
	 * Sets the current BlendSpace of the BlendSpace Player (Matching).
	 * @param BlendSpaceMatchedPlayer:	The BlendSpace Player (Matching) to act on.
	 * @param BlendSpace:				The BlendSpace asset to set the asset player to.
	 * @return Returns a reference to the BlendSpace Player (Matching).
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Set Blend Space (Matching)"))
	static FBlendSpaceMatcherReference SetMatchedBlendSpace(const FBlendSpaceMatcherReference& BlendSpaceMatchedPlayer,
																UBlendSpace* BlendSpace);

	/**
	 * Sets the current BlendSpace of the BlendSpace Player (Matching) with an inertial blend time.
	 * @param UpdateContext:			The update context provided in the anim node function.
	 * @param BlendSpaceMatchedPlayer:	The BlendSpace Player (Matching) to act on.
	 * @param BlendSpace:				The BlendSpace asset to set the asset player to.
	 * @param BlendTime:				The time to apply the inertial blend.
	 * @return Returns a reference to the BlendSpace Player (Matching).
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Set Blend Space with Inertial Blending (Matching)"))
	static FBlendSpaceMatcherReference SetMatchedBlendSpaceWithInertialBlending(const FAnimUpdateContext& UpdateContext,
		const FBlendSpaceMatcherReference& BlendSpaceMatchedPlayer, UBlendSpace* BlendSpace, float BlendTime = 0.2f);
	

//-------------------------------------
// Sequence Matcher
//-------------------------------------

	/**
	 * Get a Sequence Player (Matching) context from an anim node context.
	 * @param Node:		The reference to the anim node to which a function is bound, within which this conversion function is nested and called.
	 * @param Result:	The success or failure of the conversion.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result", DisplayName = "Convert to Sequence Player (Matching)"))
	static FSequenceMatcherReference ConvertToMatchedSequencePlayer(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);

	/**
	 * Get a Sequence Player context from an anim node context (pure).
	 * @param Node:				The reference to the anim node to which a function is bound, within which this conversion function is nested and called.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @param bResult:			The success (true) or failure (false) result of the conversion.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Convert to Sequence Player (Matching) (Pure)"))
	static void ConvertToMatchedSequencePlayerPure(const FAnimNodeReference& Node, FSequenceMatcherReference& SequencePlayer, bool& bResult)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		SequencePlayer = ConvertToMatchedSequencePlayer(Node, ConversionResult);
		bResult = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}

	/**
	 * Set the current explicit time of the Sequence Player.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @param Time:				The time to set the Sequence Player (Matching) node to.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Set Accumulated Time (Matching)"))
	static FSequenceMatcherReference SetMatchedAccumulatedTime(const FSequenceMatcherReference& SequencePlayer, float Time);

	/** 
	 * Set the Start Position of the Sequence Player. 
	 * If this is called from On Become Relevant or On Initial Update then it should be accompanied by a call to
	 * SetMatchedAccumulatedTime to achieve the desired effect of resetting the play time of a Sequence Player.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @param StartPosition:	The Start Position (time) to set the Sequence Player (Matching) node to.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Set Start Position (Matching)"))
	static FSequenceMatcherReference SetMatchedStartPosition(const FSequenceMatcherReference& SequencePlayer, float StartPosition);

	/**
	 * Set the Play Rate of the Sequence Player.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @param PlayRate:			The play rate to set the Sequence Player (Matching) node to.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Set Play Rate (Matching)"))
	static FSequenceMatcherReference SetMatchedPlayRate(const FSequenceMatcherReference& SequencePlayer, float PlayRate);

	/**
	 * Set the current Sequence of the Sequence Player.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @param Sequence:			The Sequence to set the node to.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Set Sequence (Matching)"))
	static FSequenceMatcherReference SetMatchedSequence(const FSequenceMatcherReference& SequencePlayer, UAnimSequenceBase* Sequence);
	
	/**
	 * Get the current Sequence of the Sequence Player (Matching) node.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @return Returns the current Sequence.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Get Sequence (Matching)"))
	static UAnimSequenceBase* GetMatchedSequencePure(const FSequenceMatcherReference& SequencePlayer);
	
	/**
	 * Gets the current explicit time of the Sequence Player (Matching) node.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @return Returns the current explicit time.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Get Accumulated Time (Matching)"))
	static float GetMatchedAccumulatedTime(const FSequenceMatcherReference& SequencePlayer);

	/**
	 * Get the start position of the Sequence Player node
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @return Returns the start position of the Sequence Player node.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Get Start Position (Matching)"))
	static float GetMatchingStartPosition(const FSequenceMatcherReference& SequencePlayer);

	/**
	 * Get the play rate of the Sequence Player.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @return Returns the play rate of the Sequence Player node.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Get Play Rate (Matching)"))
	static float GetMatchingPlayRate(const FSequenceMatcherReference& SequencePlayer);

	/**
	 * Get the looping state of the Sequence Player.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @return Returns the bool indicating whether the Sequence Player is set to loop.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "Get Is Looping (Matching)"))
	static bool GetMatchingLoopAnimation(const FSequenceMatcherReference& SequencePlayer);
	
	/**
	 * Computes the playrate to provide when playing this animation Sequence if a specific animation duration is desired.
	 * @param SequencePlayer:	The Sequence Player (Matching) node to act on.
	 * @param Duration:			The desired duration.
	 * @return Returns the adjusted playrate.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe, DisplayName = "ComputePlayRateFromDuration (Matching)"))
	static float ComputeMatchedPlayRateFromDuration(const FSequenceMatcherReference& SequencePlayer, float Duration = 1.0f);

	
//-------------------------------------
// Blend Space (standard/native)
//-------------------------------------

	/**
	 * Sets the normalized time on the BlendSpace Player by using distance matching.
	 * @param UpdateContext:					The update context provided in the anim node function.
	 * @param BlendSpacePlayer:					The BlendSpace Player being acted on.
	 * @param PlayRateClamp:					The range [min,max] of allowed playrate.
	 * @param MatchingDistance:					The translational distance to match the BlendSpace Player to.
	 * @param PrevNormalizedTime:				The normalized time of the previous tick.
	 * @param DistanceCurveName:				The name of the animation curve from which to extract information.
	 * @param bAdvanceTimeNaturally:			Indicates whether to advance time without distance matching.
	 * @param bUseOnlyHighestWeightedSample:	Indicates whether only the highest weighted sample (Sequence) should be used when determining the pose to match.
												This will save some performance but will produce slightly different results than using the blended pose produced by all relevant samples.
	 * @return Returns the reference to the BlendSpace Player and the current normalized time.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta=(BlueprintThreadSafe))
	static FBlendSpacePlayerReturnValues SetNormalizedTimeByDistanceMatching(const FAnimUpdateContext& UpdateContext,
			const FBlendSpacePlayerReference& BlendSpacePlayer, const float MatchingDistance, const float PrevNormalizedTime,
			const FVector2D PlayRateClamp = FVector2D(0.6f, 5.0f), const FName DistanceCurveName = FName(TEXT("Distance")),
			const bool bAdvanceTimeNaturally = false, const bool bUseOnlyHighestWeightedSample = true);

	/**
	 * Sets the normalized time on the BlendSpace Player by using pose matching.
	 * @param UpdateContext:					The update context provided in the anim node function.
	 * @param BlendSpacePlayer:					The BlendSpace Player being acted on.
	 * @param SampleRate:						The rate (i.e. fps) at which to sample poses from the Sequence. The default rate of 30.0f should be sufficient.
	 * @param bShouldMatchVelocity:				Indicates whether velocity should be matched. This costs more performance but may produce better results.
	 * @param bUseOnlyHighestWeightedSample:	Indicates whether only the highest weighted sample (Sequence) should be used when determining the pose to match.
												This will save some performance but will produce slightly different results than using the blended pose produced by all relevant samples.
	 * @param bShouldInertiallyBlend:			Indicates whether this node should request inertial blending. (An Inertialization node is needed in the Anim Graph for this to work.)
	 * @param InertialBlendTime:				The requested blend duration send to the Inertialization node.
	 * @return Returns the reference to the BlendSpace Player and the current normalized time.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta=(BlueprintThreadSafe))
	static FBlendSpacePlayerReturnValues SetNormalizedTimeByPoseMatching(const FAnimUpdateContext& UpdateContext,
			const FBlendSpacePlayerReference& BlendSpacePlayer, const float SampleRate = 30.0f, const bool bShouldMatchVelocity = false,
			const bool bUseOnlyHighestWeightedSample = true, const bool bShouldInertiallyBlend = true, const float InertialBlendTime = 0.2f);
	
	/**
	 * Directly sets the normalized time on the BlendSpace Player. Note: normalized time is a unitless time in the range [0,1].
	 * @param BlendSpacePlayer:	The BlendSpace Player being acted on.
	 * @param NormalizedTime:	The unitless time in a range of [0,1] representing the full length of the BlendSpace time.
	 * @return Returns the reference to the BlendSpace Player.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta=(BlueprintThreadSafe))
	static FBlendSpacePlayerReference SetNormalizedTime(const FBlendSpacePlayerReference& BlendSpacePlayer, float NormalizedTime = 0.0f);

	/**
	 * Gets the scaled accumulated time (in seconds) of the BlendSpace Player. This can be used to ease in the Stride
	 * Warping Alpha and smoothly increase the minimum playrate. (See the Lyra sample project for an example of how
	 * this is done for sequences.)
	 * @param BlendSpacePlayer:	The BlendSpace Player being acted on.
	 * @return Returns the scaled accumulated time.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Matching Suite|Node Helpers", meta = (BlueprintThreadSafe))
	static float GetScaledAccumulatedTime(const FBlendSpacePlayerReference& BlendSpacePlayer);

	
//-------------------------------------
// Sequence (standard/native)
//-------------------------------------

	/**
	 * Sets the explicit time on the Sequence Player by using pose matching.
	 * @param UpdateContext:					The update context provided in the anim node function.
	 * @param SequencePlayer:					The BlendSpace Player being acted on.
	 * @param SampleRate:						The rate (i.e. fps) at which to sample poses from the Sequence. The default rate of 30.0f should be sufficient.
	 * @param bShouldMatchVelocity:				Indicates whether velocity should be matched. This costs more performance but may produce better results.
	 * @param bShouldInertiallyBlend:			Indicates whether this node should request inertial blending. (An Inertialization node is needed in the Anim Graph for this to work.)
	 * @param InertialBlendTime:				The requested blend duration send to the Inertialization node.
	 * @return Returns the reference to the Sequence Player and the current explicit time.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Matching Suite|Node Helpers", meta=(BlueprintThreadSafe))
	static FSequencePlayerReturnValues SetExplicitTimeByPoseMatching(const FAnimUpdateContext& UpdateContext,
			const FSequencePlayerReference& SequencePlayer, const float SampleRate = 30.0f, const bool bShouldMatchVelocity = false,
			const bool bShouldInertiallyBlend = true, const float InertialBlendTime = 0.2f);

	
};

