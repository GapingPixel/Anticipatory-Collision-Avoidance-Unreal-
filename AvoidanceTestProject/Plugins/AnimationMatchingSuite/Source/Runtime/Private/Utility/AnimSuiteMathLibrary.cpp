// Copyright MuuKnighted Games 2024. All rights reserved.

#include "Utility/AnimSuiteMathLibrary.h"

#include "Utility/AnimSuiteTypes.h"
#include "AnimGraph/AnimNode_PoseRecorder.h"
#include "Animation/AnimCurveCompressionCodec_UniformIndexable.h"
#include "Animation/AnimInstanceProxy.h"


//-------------------------------------
// Pose Matching 
//-------------------------------------

float UAnimSuiteMathLibrary::DetermineInitialTime(const FAnimationUpdateContext& Context, TArray<FPoseBoneData>& CurrentSnapshotPose,
		UAnimSequence* AnimSequence, float SampleRate, FAMSDebugData& DebugData, bool bIsLoopingAnim, bool bMatchVelocity, float PositionWeight,
		float VelocityWeight, bool bSaveDebugData, EMatchingRange MatchingRange, float InitialTime, float FinalTime)
{
	if (MatchingRange == EMatchingRange::CustomRange && InitialTime >= FinalTime)
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"Determine Initial Time (Sequence)\": The InitialTime = %f and the FinalTime = %f. "
											"This is not allowed. The IntialTime must be greater than the FinalTime. Check your settings. Returning "
											"0.0f for the pose-matched initial time."), InitialTime, FinalTime);
		return 0.0f;
	}
	
	if (AnimSequence == nullptr) 
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"Determine Initial Time (Sequence)\": An invalid Animation Sequence was supplied during pose matching. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	const float PlayLength = AnimSequence->GetPlayLength();
	if (MatchingRange == EMatchingRange::CustomRange && FinalTime > PlayLength)
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"Determine Initial Time (Sequence)\": The FinalTime = %f and the PlayLength = %f. This is not allowed."
											" The FinalTime cannot be greater than the Sequence's PlayLength."), FinalTime, PlayLength);
		return 0.0f;
	}
	
	USkeleton* Skeleton = Context.AnimInstanceProxy->GetSkeleton();
	if (Skeleton == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Sequence)\": The animation Proxy object has an invalid Skeleton. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	IPoseMatchRequester* PoseMatchRequester = Context.GetMessage<IPoseMatchRequester>();
	if (PoseMatchRequester == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Sequence)\": The PoseMatchRequester is null. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	FAnimNode_PoseRecorder* PoseRecorderNode = &PoseMatchRequester->GetNode();
	if (PoseRecorderNode == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Sequence)\": The PoseRecorderNode (i.e. the Pose Grabber node) is null. "
								   "Check the Anim Graph. Make sure the Pose Grabber node is in the graph and executing. "
									"Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	const int32 NumOfCachedBones = PoseRecorderNode->BonesToCache.Num();
	if (NumOfCachedBones == 0)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Sequence)\": There are no cached bones in the PoseRecorderNode (i.e. the Pose Grabber node). "
								   "Add some bones to cache poses for. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	/** Retrieve the cached bone data from the Pose Recorder (Snapshot) node. */
	FCachedPose CachedPose = PoseRecorderNode->GetCachedPose();
	TArray<FName> BoneNames;
	TArray<int32> BoneIDs;
	TArray<FTransform> SourceBoneTransforms;
	TArray<FVector> SourceBoneVelocities;
	CurrentSnapshotPose.Empty(NumOfCachedBones); // empty the array so as to not continue adding elements every time this function is called.
	for (int32 i = 0; i < NumOfCachedBones; ++i)
	{
		/** Add the location and velocity of the pose cached in the Pose Grabber node to the local array. @TODO: Is this necessary, or only convenient?*/
		BoneNames.Add(PoseRecorderNode->BonesToCache[i].BoneName);
		CurrentSnapshotPose.Add(FPoseBoneData(CachedPose.CachedBoneData[BoneNames[i]].Transform,
													CachedPose.CachedBoneData[BoneNames[i]].Velocity,
													CachedPose.CachedBoneData[BoneNames[i]].BoneID));
		SourceBoneTransforms.Add(CurrentSnapshotPose[i].Transform);
		SourceBoneVelocities.Add(CurrentSnapshotPose[i].Velocity);
		BoneIDs.Add(CurrentSnapshotPose[i].BoneID);
	}

	/** Set the range to search: [BeginTime, EndTime].*/
	float BeginTime = 0.0f;
	float EndTime = PlayLength;
	if (MatchingRange == EMatchingRange::CustomRange)
	{
		BeginTime = InitialTime;
		EndTime = FinalTime;
	}

	/** Declare structs to hold time and cost data. */
	TArray<float> SequenceTimeValues;
	TArray<float> PositionCosts;
	TArray<float> VelocityCosts;

	/** Debug data. */
	int32 NumOfTimeSamples = (EndTime - BeginTime) * SampleRate;
	TArray<TArray<FTransform>> StoredTransformDataForDebug;
	TArray<TArray<FVector>> StoredVelocityDataForDebug;
	if (bSaveDebugData)
	{
		StoredTransformDataForDebug.Reserve(NumOfTimeSamples);
		StoredVelocityDataForDebug.Reserve(NumOfTimeSamples);
	}
	
	/** Scrub through the anim Sequence, getting the transform of every bone of interest at every sample time. */
	const float SampleInterval = 1 / SampleRate;
	float CurrentTime = BeginTime;
	while (CurrentTime <= EndTime)
	{
		float PositionCost = 0.0f;
		float VelocityCost = 0.0f;

		/** Calculate the cost of the pose difference. */
		TArray<FTransform> CandidateBoneTransforms;
		ExtractBoneTransforms_CS(CandidateBoneTransforms, AnimSequence, BoneNames, *Context.AnimInstanceProxy, CurrentTime, SampleInterval, bIsLoopingAnim);
		TArray<FVector> CandidateBoneVelocities;
		if (bMatchVelocity)
		{
			TArray<FTransform> CandidatePrevBoneTransforms;
			ExtractBoneTransforms_CS(CandidatePrevBoneTransforms, AnimSequence, BoneNames, *Context.AnimInstanceProxy, CurrentTime - SampleInterval, SampleInterval, bIsLoopingAnim);
			CalculatePoseVelocities(CandidateBoneVelocities, CandidateBoneTransforms, CandidatePrevBoneTransforms, SampleInterval);
			VelocityCost = CalculatePoseCost(SourceBoneVelocities, CandidateBoneVelocities);
			VelocityCost *= VelocityWeight; // multiply by weight parameter
		}
		PositionCost = CalculatePoseCost(SourceBoneTransforms, CandidateBoneTransforms);
		PositionCost *= PositionWeight; // multiply by weight parameter

		if (bSaveDebugData)
		{ 
			StoredTransformDataForDebug.Add(CandidateBoneTransforms);
			if (bMatchVelocity)
			{
				StoredVelocityDataForDebug.Add(CandidateBoneVelocities);
			}
		}
		
		/** Store the current time and the location delta sum in their separate arrays. */
		SequenceTimeValues.Add(CurrentTime);
		PositionCosts.Add(PositionCost);
		if (bMatchVelocity)
		{
			VelocityCosts.Add(VelocityCost);
		}

		CurrentTime += SampleInterval;
	}

	/** Get the array index corresponding to the minimum cost. */
	/** If velocity is matched. */
	if (bMatchVelocity)
	{
		int32 PositionMinCostIndex;
		float PositionMinCost = FMath::Min(PositionCosts, &PositionMinCostIndex);
		int32 PositionMaxCostIndex;
		float PositionMaxCost = FMath::Max(PositionCosts, &PositionMaxCostIndex);
		int32 VelocityMinCostIndex;
		int32 VelocityMaxCostIndex;
		float VelocityMinCost = FMath::Min(VelocityCosts, &VelocityMinCostIndex);
		float VelocityMaxCost = FMath::Max(VelocityCosts, &VelocityMaxCostIndex);

		int32 IndexOfMinValue = FindNormalizedMinCostIndex(PositionCosts, PositionMinCost, PositionMaxCost,
												VelocityCosts, VelocityMinCost, VelocityMaxCost);

		if (bSaveDebugData)
		{
			DebugData = FAMSDebugData(StoredTransformDataForDebug[IndexOfMinValue], StoredVelocityDataForDebug[IndexOfMinValue]);
		}
		
		return SequenceTimeValues[IndexOfMinValue];
	}

	/** If velocity isn't matched. */
	int32 IndexOfMinValue;
	FMath::Min(PositionCosts, &IndexOfMinValue);

	if (bSaveDebugData)
	{
		DebugData = FAMSDebugData(StoredTransformDataForDebug[IndexOfMinValue]);
	}
	
	return SequenceTimeValues[IndexOfMinValue]; 
}

float UAnimSuiteMathLibrary::DetermineInitialTime(const FAnimationUpdateContext& Context, TArray<FPoseBoneData>& CurrentSnapshotPose,
	UBlendSpace* BlendSpace, TArray<FBlendSampleData>& BlendSampleData, float SampleRate, FAMSDebugData& DebugData, bool bIsLooping, bool bMatchVelocity, bool bUseOnlyHighestWeightedSample,
	float PositionWeight, float VelocityWeight, bool bSaveDebugData)
{
	if (BlendSpace == nullptr) 
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"Determine Initial Time (Blend Space)\": An invalid Blend Space was supplied during pose matching. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	USkeleton* Skeleton = Context.AnimInstanceProxy->GetSkeleton();
	if (Skeleton == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Blend Space)\": The animation Proxy object has an invalid Skeleton. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	IPoseMatchRequester* PoseMatchRequester = Context.GetMessage<IPoseMatchRequester>();
	if (PoseMatchRequester == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Blend Space)\": The PoseMatchRequester is null. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	FAnimNode_PoseRecorder* PoseRecorderNode = &PoseMatchRequester->GetNode();
	if (PoseRecorderNode == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Blend Space)\": The PoseRecorderNode (i.e. the Pose Grabber node) is null. "
								   "Check the Anim Graph. Make sure the Pose Grabber node is in the graph and executing. "
									"Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	const int32 NumOfCachedBones = PoseRecorderNode->BonesToCache.Num();
	if (NumOfCachedBones == 0)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"Determine Initial Time (Blend Space)\": There are no cached bones in the PoseRecorderNode (i.e. the Pose Grabber node). "
								   "Add some bones to cache poses for. Returning 0.0f for the pose-matched initial time."));
		return 0.0f;
	}

	/** Retrieve the cached bone data from the Pose Recorder (Snapshot) node. */
	FCachedPose CachedPose = PoseRecorderNode->GetCachedPose();
	TArray<FName> BoneNames;
	TArray<int32> BoneIDs;
	TArray<FTransform> SourceBoneTransforms;
	TArray<FVector> SourceBoneVelocities;
	CurrentSnapshotPose.Empty(NumOfCachedBones); // empty the array so as to not continue adding elements every time this function is called.
	for (int32 i = 0; i < NumOfCachedBones; ++i)
	{
		/** Add the location and velocity of the pose cached in the Pose Grabber node to the local array. @TODO: Is this necessary, or only convenient?*/
		BoneNames.Add(PoseRecorderNode->BonesToCache[i].BoneName);
		CurrentSnapshotPose.Add(FPoseBoneData(CachedPose.CachedBoneData[BoneNames[i]].Transform,
													CachedPose.CachedBoneData[BoneNames[i]].Velocity,
													CachedPose.CachedBoneData[BoneNames[i]].BoneID));
		SourceBoneTransforms.Add(CurrentSnapshotPose[i].Transform);
		SourceBoneVelocities.Add(CurrentSnapshotPose[i].Velocity);
		BoneIDs.Add(CurrentSnapshotPose[i].BoneID);
	}
	
	/** Initialize the index used later. If the highest-weighted Blend Sample is used, the starting index will correspond
	    to that sample; otherwise, the starting index will be 0, that way all relevant Blend Samples can be used in the
	    pose cost algorithm. */
	int32 StartingSampleIdx = 0;
	
	/** Get the normalized sample interval based on the weighted blend samples. This prevents the Blend Space
		from being sampled at a frequency less than intended by the user who adjusted the Sample Rate.
		If only the highest weighted sample is to be used, base the normalized time solely on that. */
	float NumOfWeightedTimeSamples = 0.0f;
	if (bUseOnlyHighestWeightedSample)
	{
		UAnimSequence* HighestWeightedAnimSequence = GetHighestWeightSample(BlendSampleData, StartingSampleIdx);
		NumOfWeightedTimeSamples = HighestWeightedAnimSequence->GetPlayLength() * SampleRate;
	}
	else
	{
		const float ScaledPlayLength = BlendSpace->GetAnimationLengthFromSampleData(BlendSampleData);
		NumOfWeightedTimeSamples = ScaledPlayLength * SampleRate;
	}
	const float NormalizedTimeInterval = 1.0f / NumOfWeightedTimeSamples;
	
	/** Set values for the following loop. */
	const float TimeInterval = 1 / SampleRate;
	float CurrentNormalizedTime = 0.0f;

	/** Declare structs to hold time and cost data. */
	TArray<float> BlendSpaceTimeValues;
	TArray<float> PositionCosts;
	TArray<float> VelocityCosts;

	/** Debug data. */
	TArray<TArray<FTransform>> StoredTransformDataForDebug;
	TArray<TArray<FVector>> StoredVelocityDataForDebug;
	if (bSaveDebugData)
	{
		StoredTransformDataForDebug.Reserve(NumOfWeightedTimeSamples);
		StoredVelocityDataForDebug.Reserve(NumOfWeightedTimeSamples);
	}
	
	/** Scrub through the anim Blend Space, getting the transform of every bone of interest at every sample time. */
	while (CurrentNormalizedTime <= 1.0f)
	{
		float PositionCost = 0.0f;
		float VelocityCost = 0.0f;

		/** Calculate the cost of the pose difference. */
		TArray<FTransform> CandidateBoneTransforms;
		TArray<FVector> CandidateBoneVelocities;
		ExtractBoneTransforms_CS(CandidateBoneTransforms, BlendSampleData, BoneNames, *Context.AnimInstanceProxy, CurrentNormalizedTime, NormalizedTimeInterval, bIsLooping, bUseOnlyHighestWeightedSample, StartingSampleIdx);
		if (bMatchVelocity) 
		{
			TArray<FTransform> CandidatePrevBoneTransforms;
			ExtractBoneTransforms_CS(CandidatePrevBoneTransforms, BlendSampleData, BoneNames, *Context.AnimInstanceProxy, CurrentNormalizedTime - NormalizedTimeInterval,
										NormalizedTimeInterval, bIsLooping, bUseOnlyHighestWeightedSample, StartingSampleIdx);
			CalculatePoseVelocities(CandidateBoneVelocities, CandidateBoneTransforms, CandidatePrevBoneTransforms, TimeInterval);
			VelocityCost = CalculatePoseCost(SourceBoneVelocities, CandidateBoneVelocities);
			VelocityCost *= VelocityWeight; // multiply by weight parameter
		}
		PositionCost = CalculatePoseCost(SourceBoneTransforms, CandidateBoneTransforms);
		PositionCost *= PositionWeight; // multiply by weight parameter

		if (bSaveDebugData)
		{
			StoredTransformDataForDebug.Add(CandidateBoneTransforms);
			if (bMatchVelocity)
			{
				StoredVelocityDataForDebug.Add(CandidateBoneVelocities);
			}
		}

		/** Store the current time and the location delta sum in their separate arrays. */
		BlendSpaceTimeValues.Add(CurrentNormalizedTime);
		PositionCosts.Add(PositionCost);
		if (bMatchVelocity)
		{
			VelocityCosts.Add(VelocityCost);
		}
				
		CurrentNormalizedTime += NormalizedTimeInterval;
	}

	/** Get the array index corresponding to the minimum cost. */
	/** If velocity is matched. */
	if (bMatchVelocity)
	{
		int32 PositionMinCostIndex;
		float PositionMinCost = FMath::Min(PositionCosts, &PositionMinCostIndex);
		int32 PositionMaxCostIndex;
		float PositionMaxCost = FMath::Max(PositionCosts, &PositionMaxCostIndex);
		int32 VelocityMinCostIndex;
		int32 VelocityMaxCostIndex;
		float VelocityMinCost = FMath::Min(VelocityCosts, &VelocityMinCostIndex);
		float VelocityMaxCost = FMath::Max(VelocityCosts, &VelocityMaxCostIndex);

		int32 IndexOfMinValue = FindNormalizedMinCostIndex(PositionCosts, PositionMinCost, PositionMaxCost,
												VelocityCosts, VelocityMinCost, VelocityMaxCost);
		
		if (bSaveDebugData)
		{
			DebugData = FAMSDebugData(StoredTransformDataForDebug[IndexOfMinValue], StoredVelocityDataForDebug[IndexOfMinValue]);
		}
		
		return BlendSpaceTimeValues[IndexOfMinValue];
	}
	/** If velocity isn't matched. */
	int32 IndexOfMinValue;
	FMath::Min(PositionCosts, &IndexOfMinValue);
	//UE_LOG(LogAnimation, Log, TEXT("Pose-Matched Time (Blend Space): %f"), BlendSpaceTimeValues[IndexOfMinValue]);

	if (bSaveDebugData)
	{
		DebugData = FAMSDebugData(StoredTransformDataForDebug[IndexOfMinValue]);
	}
	
	return BlendSpaceTimeValues[IndexOfMinValue];
}

void UAnimSuiteMathLibrary::ExtractBoneTransforms_CS(TArray<FTransform>& OutBoneTransform_CS, UAnimSequence* Sequence, TArray<FName>& MatchBoneNames,
														FAnimInstanceProxy& AnimInstanceProxy, float Time, const float TimeInterval, bool bIsLooping)
{
	OutBoneTransform_CS.Empty();
	
	FAMSPoseEvaluationOptions EvaluationOptions;
	EvaluationOptions.OptionalSkeletalMesh = AnimInstanceProxy.GetSkelMeshComponent()->GetSkeletalMeshAsset(); 
	//EvaluationOptions.bShouldRetarget = false;
	EvaluationOptions.EvaluationType = EAMSAnimDataEvalType::Compressed;
	
	if (Time < 0.0f && bIsLooping)
	{
		/** Time is negative here, so we're subtracting Time from the full playlength. */
		Time = Sequence->GetPlayLength() + Time;
		
		FAMSPose AnimPose;
		GetAnimPoseAtTime(Sequence, Time, EvaluationOptions, AnimPose);
		for (const FName& BoneName : MatchBoneNames)
		{
			OutBoneTransform_CS.Add(GetBonePose(AnimPose, BoneName, EAMSPoseSpaces::Component));
		}
	}
	else if (Time > Sequence->GetPlayLength() && bIsLooping)
	{
		/** Time is positive here, so we're subtracting the full playlength from the Time. */
		Time = Time - Sequence->GetPlayLength();

		FAMSPose AnimPose;
		GetAnimPoseAtTime(Sequence, Time, EvaluationOptions, AnimPose);
		for (const FName& BoneName : MatchBoneNames)
		{
			OutBoneTransform_CS.Add(GetBonePose(AnimPose, BoneName, EAMSPoseSpaces::Component));
		}
	}
	else if (Time < 0.0f)
	{
		/** Define interval edges over which to compute the finite difference. */
		const float InitialTime = Time - (TimeInterval / 2.0f);
		const float FinalTime = Time + (TimeInterval / 2.0f);

		/** Set the time to define the end of the range of in-Sequence values that will be used for extrpolation. */
		const float ExtrapolationSampleTime = 5 * TimeInterval; // @TODO: Get rid of the magic number. Set elsewhere. (Accessible to user?) The total value should remain larger than the TimeInterval.
		
		FAMSPose AnimPose_ExtrapolationStart;
		GetAnimPoseAtTime(Sequence, 0.0f, EvaluationOptions, AnimPose_ExtrapolationStart);
		FAMSPose AnimPose_ExtrapolationEnd;
		GetAnimPoseAtTime(Sequence, ExtrapolationSampleTime, EvaluationOptions, AnimPose_ExtrapolationEnd);
	
		for (const FName& BoneName : MatchBoneNames)
		{
			/** Get the delta between the start/end transforms of the extrapolation interval, and then find the bone transform
            	delta at the time point beyond the animation track (i.e. at NEGATIVE time, in this case). */
			const FTransform BoneTransform_Start = GetBonePose(AnimPose_ExtrapolationStart, BoneName, EAMSPoseSpaces::Component);
			const FTransform BoneTransform_End = GetBonePose(AnimPose_ExtrapolationEnd, BoneName, EAMSPoseSpaces::Component);
			const FTransform SampleDataToExtrapolate = BoneTransform_End.GetRelativeTransform(BoneTransform_Start);
			const FTransform ExtrapolatedBoneTransform = ExtrapolateTransform(SampleDataToExtrapolate, 0.0f, ExtrapolationSampleTime, InitialTime);

			/** Add the extrapolated result to the transform array to be output by this function. */
			OutBoneTransform_CS.Add(ExtrapolatedBoneTransform);
		}
	}
	else if (Time > Sequence->GetPlayLength())
	{
		/** Define interval edges over which to compute the finite difference. */
		const float InitialTime = Time - (TimeInterval / 2.0f);
		const float FinalTime = Time + (TimeInterval / 2.0f);

		const float AnimPlayLength = Sequence->GetPlayLength();
		
		/** Set the time to define the end of the range of in-Sequence values that will be used for extrpolation. */
		const float ExtrapolationSampleTime = 5 * TimeInterval; // @TODO: Get rid of the magic number. Set elsewhere. (Accessible to user?) The total value should remain larger than the TimeInterval.
		
		FAMSPose AnimPose_ExtrapolationStart;
		GetAnimPoseAtTime(Sequence, AnimPlayLength - ExtrapolationSampleTime, EvaluationOptions, AnimPose_ExtrapolationStart);
		FAMSPose AnimPose_ExtrapolationEnd;
		GetAnimPoseAtTime(Sequence, AnimPlayLength, EvaluationOptions, AnimPose_ExtrapolationEnd);
	
		for (const FName& BoneName : MatchBoneNames)
		{
			/** Get the delta between the start/end transforms of the extrapolation interval, and then find the joint transform
				delta at the time point beyond the animation track (i.e. at POSITIVE time, in this case). */
			const FTransform BoneTransform_Start = GetBonePose(AnimPose_ExtrapolationStart, BoneName, EAMSPoseSpaces::Component);
			const FTransform BoneTransform_End = GetBonePose(AnimPose_ExtrapolationEnd, BoneName, EAMSPoseSpaces::Component);
			const FTransform SampleDataToExtrapolate = BoneTransform_End.GetRelativeTransform(BoneTransform_Start);
			const FTransform ExtrapolatedBoneTransform = ExtrapolateTransform(SampleDataToExtrapolate, AnimPlayLength - ExtrapolationSampleTime, AnimPlayLength, FinalTime);

			/** Add the extrapolated result to the transform array to be output by this function. */
			OutBoneTransform_CS.Add(ExtrapolatedBoneTransform);
		}
	}
	else
	{
		FAMSPose AnimPose;
		GetAnimPoseAtTime(Sequence, Time, EvaluationOptions, AnimPose);
		for (const FName& BoneName : MatchBoneNames)
		{
			OutBoneTransform_CS.Add(GetBonePose(AnimPose, BoneName, EAMSPoseSpaces::Component));
		}
	}
}

void UAnimSuiteMathLibrary::ExtractBoneTransforms_CS(TArray<FTransform>& OutBoneTransform_CS, TArray<FBlendSampleData>& BlendSampleData,
	const TArray<FName>& BoneNames, FAnimInstanceProxy& AnimInstanceProxy, float NormalizedTime, const float NormalizedSampleInterval,
	const bool bIsLooping, const bool bUseHighestWeightedSample, const int32 HighestWeightedSampleIdx)
{
	if (BlendSampleData.Num() == 0 )
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"ExtractSingleBoneTransform_CS (Blend Space)\": The number of Blend Samples is zero."));
		return;
	}

	OutBoneTransform_CS.Empty();
	
	FAMSPoseEvaluationOptions EvaluationOptions;
	EvaluationOptions.OptionalSkeletalMesh = AnimInstanceProxy.GetSkelMeshComponent()->GetSkeletalMeshAsset(); 
	//EvaluationOptions.bShouldRetarget = false;
	EvaluationOptions.EvaluationType = EAMSAnimDataEvalType::Compressed;

	bool bIsFirstIteration = true;
	const int32 StartingIndex = bUseHighestWeightedSample ? HighestWeightedSampleIdx : 0;
	const int32 NumOfBlendSamples = bUseHighestWeightedSample ? StartingIndex + 1 : BlendSampleData.Num();
	for (int32 i = StartingIndex; i < NumOfBlendSamples; ++i)
	{
		UAnimSequence* Sequence = BlendSampleData[i].Animation;
		if (Sequence == nullptr)
		{
			UE_LOG(LogAnimation, Warning, TEXT("\"ExtractSingleBoneTransform_CS (Blend Space)\": The Sequence (UAnimSequence) corresponding "
												"to a Blend Sample is invalid. Blend Sample skipped."));
			continue;
		}
		
		const ScalarRegister SampleWeight(BlendSampleData[i].GetClampedWeight()); // see ScalarRegister.h
		float Time = Sequence->GetPlayLength() * NormalizedTime;
		float TimeInterval = Sequence->GetPlayLength() * NormalizedSampleInterval;
		
		if (Time < 0.0f && bIsLooping)
		{
			/** Time is negative here, so we're subtracting Time from the full playlength. */
			Time = Sequence->GetPlayLength() + Time;
			
			FAMSPose AnimPose;
			GetAnimPoseAtTime(Sequence, Time, EvaluationOptions, AnimPose);
			
			for (int32 j = 0; j < BoneNames.Num(); ++j)
			{
				if (bIsFirstIteration)
				{
					OutBoneTransform_CS.Add(FTransform::Identity);
					FTransform BoneTransform = GetBonePose(AnimPose, BoneNames[j], EAMSPoseSpaces::Component);
					OutBoneTransform_CS[j].Accumulate(BoneTransform, SampleWeight);
				}
				else
				{
					FTransform BoneTransform = GetBonePose(AnimPose, BoneNames[j], EAMSPoseSpaces::Component);
					OutBoneTransform_CS[j].Accumulate(BoneTransform, SampleWeight);
				}
			}
			
			bIsFirstIteration = false;
		}
		else if (Time > Sequence->GetPlayLength() && bIsLooping)
		{
			/** Time is positive here, so we're subtracting the full playlength from the Time. */
			Time = Time - Sequence->GetPlayLength();
			
			FAMSPose AnimPose;
			GetAnimPoseAtTime(Sequence, Time, EvaluationOptions, AnimPose);
			
			for (int32 j = 0; j < BoneNames.Num(); ++j)
			{
				if (bIsFirstIteration)
				{
					OutBoneTransform_CS.Add(FTransform::Identity);
					FTransform BoneTransform = GetBonePose(AnimPose, BoneNames[j], EAMSPoseSpaces::Component);
					OutBoneTransform_CS[j].Accumulate(BoneTransform, SampleWeight);
				}
				else
				{
					FTransform BoneTransform = GetBonePose(AnimPose, BoneNames[j], EAMSPoseSpaces::Component);
					OutBoneTransform_CS[j].Accumulate(BoneTransform, SampleWeight);
				}
			}
			
			bIsFirstIteration = false;
		}
		else if (Time < 0.0f)
		{
			/** Define interval edges over which to compute the finite difference. */
			const float InitialTime = Time - (TimeInterval / 2.0f);
			const float FinalTime = Time + (TimeInterval / 2.0f);

			/** Set the time to define the end of the range of in-Sequence values that will be used for extrpolation. */
			const float ExtrapolationSampleTime = 5 * TimeInterval; // @TODO: Get rid of the magic number. Set elsewhere. (Accessible to user?) The total value should remain larger than the TimeInterval.

			FAMSPose AnimPose_ExtrapolationStart;
			GetAnimPoseAtTime(Sequence, 0.0f, EvaluationOptions, AnimPose_ExtrapolationStart);
			FAMSPose AnimPose_ExtrapolationEnd;
			GetAnimPoseAtTime(Sequence, ExtrapolationSampleTime, EvaluationOptions, AnimPose_ExtrapolationEnd);

			for (int32 j = 0; j < BoneNames.Num(); ++j)
			{
				if (bIsFirstIteration)
				{
					OutBoneTransform_CS.Add(FTransform::Identity);
					
					/** Get the delta between the start/end transforms of the extrapolation interval, and then find the bone transform
						delta at the time point beyond the animation track (i.e. at NEGATIVE time, in this case). */
					const FTransform BoneTransform_Start = GetBonePose(AnimPose_ExtrapolationStart, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform BoneTransform_End = GetBonePose(AnimPose_ExtrapolationEnd, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform SampleDataToExtrapolate = BoneTransform_End.GetRelativeTransform(BoneTransform_Start);
					const FTransform ExtrapolatedBoneTransform = ExtrapolateTransform
																(SampleDataToExtrapolate,
																0.0f,
																ExtrapolationSampleTime,
																InitialTime);

					OutBoneTransform_CS[j].Accumulate(ExtrapolatedBoneTransform, SampleWeight);
				}
				else
				{
					/** Get the delta between the start/end transforms of the extrapolation interval, and then find the bone transform
						delta at the time point beyond the animation track (i.e. at NEGATIVE time, in this case). */
					const FTransform BoneTransform_Start = GetBonePose(AnimPose_ExtrapolationStart, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform BoneTransform_End = GetBonePose(AnimPose_ExtrapolationEnd, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform SampleDataToExtrapolate = BoneTransform_End.GetRelativeTransform(BoneTransform_Start);
					const FTransform ExtrapolatedBoneTransform = ExtrapolateTransform
																(SampleDataToExtrapolate,
																0.0f,
																ExtrapolationSampleTime,
																InitialTime);

					OutBoneTransform_CS[j].Accumulate(ExtrapolatedBoneTransform, SampleWeight);
				}
			}
			
			bIsFirstIteration = false;
		}
		else if (Time > Sequence->GetPlayLength())
		{
			/** Define interval edges over which to compute the finite difference. */
			const float InitialTime = Time - (TimeInterval / 2.0f);
			const float FinalTime = Time + (TimeInterval / 2.0f);

			/** Set the time to define the end of the range of in-Sequence values that will be used for extrpolation. */
			const float ExtrapolationSampleTime = 5 * TimeInterval; // @TODO: Get rid of the magic number. Set elsewhere. (Accessible to user?) The total value should remain larger than the TimeInterval.

			FAMSPose AnimPose_ExtrapolationStart;
			GetAnimPoseAtTime(Sequence, Sequence->GetPlayLength() - ExtrapolationSampleTime, EvaluationOptions, AnimPose_ExtrapolationStart);
			FAMSPose AnimPose_ExtrapolationEnd;
			GetAnimPoseAtTime(Sequence, Sequence->GetPlayLength(), EvaluationOptions, AnimPose_ExtrapolationEnd);

			for (int32 j = 0; j < BoneNames.Num(); ++j)
			{
				if (bIsFirstIteration)
				{
					OutBoneTransform_CS.Add(FTransform::Identity);
					
					/** Get the delta between the start/end transforms of the extrapolation interval, and then find the bone transform
						delta at the time point beyond the animation track (i.e. at POSITIVE time, in this case). */
					const FTransform BoneTransform_Start = GetBonePose(AnimPose_ExtrapolationStart, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform BoneTransform_End = GetBonePose(AnimPose_ExtrapolationEnd, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform SampleDataToExtrapolate = BoneTransform_End.GetRelativeTransform(BoneTransform_Start);
					const FTransform ExtrapolatedBoneTransform = ExtrapolateTransform
																(SampleDataToExtrapolate,
																Sequence->GetPlayLength() - ExtrapolationSampleTime,
																Sequence->GetPlayLength(),
																FinalTime);

					OutBoneTransform_CS[j].Accumulate(ExtrapolatedBoneTransform, SampleWeight);
				}
				else
				{
					/** Get the delta between the start/end transforms of the extrapolation interval, and then find the bone transform
						delta at the time point beyond the animation track (i.e. at POSITIVE time, in this case). */
					const FTransform BoneTransform_Start = GetBonePose(AnimPose_ExtrapolationStart, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform BoneTransform_End = GetBonePose(AnimPose_ExtrapolationEnd, BoneNames[j], EAMSPoseSpaces::Component);
					const FTransform SampleDataToExtrapolate = BoneTransform_End.GetRelativeTransform(BoneTransform_Start);
					const FTransform ExtrapolatedBoneTransform = ExtrapolateTransform
																(SampleDataToExtrapolate,
																Sequence->GetPlayLength() - ExtrapolationSampleTime,
																Sequence->GetPlayLength(),
																FinalTime);

					OutBoneTransform_CS[j].Accumulate(ExtrapolatedBoneTransform, SampleWeight);
				}
			}
			
			bIsFirstIteration = false;
		}
		else
		{
			FAMSPose AnimPose;
			GetAnimPoseAtTime(Sequence, Time, EvaluationOptions, AnimPose);
			
			for (int32 j = 0; j < BoneNames.Num(); ++j)
			{
				if (bIsFirstIteration)
				{
					OutBoneTransform_CS.Add(FTransform::Identity);
					//FTransform BoneTransform = UAnimPoseExtensions::GetBonePose(AnimPose, BoneNames[j], EAnimPoseSpaces::World);
					FTransform BoneTransform = GetBonePose(AnimPose, BoneNames[j], EAMSPoseSpaces::Component);
					OutBoneTransform_CS[j].Accumulate(BoneTransform, SampleWeight);
				}
				else
				{
					//FTransform BoneTransform = UAnimPoseExtensions::GetBonePose(AnimPose, BoneNames[j], EAnimPoseSpaces::World);
					FTransform BoneTransform = GetBonePose(AnimPose, BoneNames[j], EAMSPoseSpaces::Component);
					OutBoneTransform_CS[j].Accumulate(BoneTransform, SampleWeight);
				}
			}
		
			bIsFirstIteration = false;
		}
	}
}

UAnimSequence* UAnimSuiteMathLibrary::GetHighestWeightSample(const TArray<FBlendSampleData> BlendSampleData, int32& HighestWeightIndex)
{
	UAnimSequence* HighestWeightSequence = BlendSampleData[0].Animation;
	float HighestWeight = BlendSampleData[HighestWeightIndex].GetClampedWeight();
	for (int32 I = 1; I < BlendSampleData.Num(); I++)
	{
		if (BlendSampleData[I].GetClampedWeight() > HighestWeight)
		{
			HighestWeightIndex = I;
			HighestWeightSequence = BlendSampleData[I].Animation;
			HighestWeight = BlendSampleData[I].GetClampedWeight();
		}
	}
	return HighestWeightSequence;
}

int32 UAnimSuiteMathLibrary::GetAnimTrackIndexFromBoneName(UAnimSequence* Sequence, const FName& BoneName)
{
	/** Get the reference skeleton bone index from the supplied bone name. */ 
	const int32 RefSkelBoneID = Sequence->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(BoneName);
	if (RefSkelBoneID == INDEX_NONE)
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"GetAnimTrackIndexFromBoneName\": An invalid Reference Skeleton Bone Index was found along the bone-to-root path."));
		return 0;
	}

	/** Convert the reference skeleton bone index to its corresponding animation track index. */
	const int32 AnimTrackBoneID = Sequence->CompressedData.GetTrackIndexFromSkeletonIndex(RefSkelBoneID);

	return AnimTrackBoneID;
}

FTransform UAnimSuiteMathLibrary::ExtrapolateTransform(FTransform SampleToExtrapolate, float BeginTime, float EndTime,
                                                       float ExtrapolationTime)
{
	const float TimeDelta = EndTime - BeginTime;
	check(!FMath::IsNearlyZero(TimeDelta));

	/** Convert ExtrapolationTime to a positive number to avoid dealing with negative extrapolation and subsequent
		requisite transform inversions. */
	const float AbsExtrapolationTime = FMath::Abs(ExtrapolationTime);
	const float AbsTimeDelta = FMath::Abs(TimeDelta);
	const FTransform AbsTimeSampleToExtrapolate = (ExtrapolationTime >= 0.0f) ? SampleToExtrapolate : SampleToExtrapolate.Inverse();
	
	/** Get the number of time samples we need (for integration) based on the division of extrapolation time by the delta time. */
	const float SampleMultiplier = AbsExtrapolationTime / AbsTimeDelta;
	float IntegralNumSamples;
	const float RemainingSampleFraction = FMath::Modf(SampleMultiplier, &IntegralNumSamples);
	const int32 NumSamples = static_cast<int32>(IntegralNumSamples);

	/** Add full samples to the extrapolated motion. */
	FTransform ExtrapolateTransform = FTransform::Identity;
	for (int i = 0; i < NumSamples; ++i)
	{
		ExtrapolateTransform = AbsTimeSampleToExtrapolate * ExtrapolateTransform;
	}

	/** Blend the remaining partial sample. */
	FTransform RemainingExtrapolatedTransform;
	RemainingExtrapolatedTransform.Blend(FTransform::Identity, AbsTimeSampleToExtrapolate, RemainingSampleFraction);

	/** Return the total. */
	ExtrapolateTransform = RemainingExtrapolatedTransform * ExtrapolateTransform;
	return ExtrapolateTransform;
}

FTransform UAnimSuiteMathLibrary::ExtrapolateTransform(FTransform SampleToExtrapolate, float BeginTime, float EndTime,
	float ExtrapolationTime, const FPoseExtrapolationParameters& ExtrapolationParameters)
{
	const float TimeDelta = EndTime - BeginTime;
	check(!FMath::IsNearlyZero(TimeDelta));

	/** Linear motion checks. */
	const FVector LinearVelocityToExtrapolate = SampleToExtrapolate.GetTranslation() / TimeDelta;
	const float LinearSpeedToExtrapolate = LinearVelocityToExtrapolate.Size();
	const bool bCanExtrapolateTranslation = LinearSpeedToExtrapolate >= ExtrapolationParameters.LinearSpeedThreshold;

	/** Rotational motion checks. */
	const float RotationalSpeedExtrapolate = SampleToExtrapolate.GetRotation().GetAngle() / TimeDelta;
	const bool bCanExtrapolateRotation = FMath::RadiansToDegrees(RotationalSpeedExtrapolate) >= ExtrapolationParameters.RotationalSpeedThreshold;

	if (!bCanExtrapolateTranslation && !bCanExtrapolateRotation)
	{
		return FTransform::Identity;
	}

	if (!bCanExtrapolateTranslation)
	{
		SampleToExtrapolate.SetTranslation(FVector::ZeroVector);
	}

	if (!bCanExtrapolateRotation)
	{
		SampleToExtrapolate.SetRotation(FQuat::Identity);
	}

	/** Convert ExtrapolationTime to a positive number to avoid dealing with negative extrapolation and subsequent
	    requisite transform inversions. */
	const float AbsExtrapolationTime = FMath::Abs(ExtrapolationTime);
	const float AbsTimeDelta = FMath::Abs(TimeDelta);
	const FTransform AbsTimeSampleToExtrapolate = (ExtrapolationTime >= 0.0f) ? SampleToExtrapolate : SampleToExtrapolate.Inverse();
	
	/** Get the number of time samples we need (for integration) based on the division of extrapolation time by the delta time. */
	const float SampleMultiplier = AbsExtrapolationTime / AbsTimeDelta;
	float IntegralNumSamples;
	const float RemainingSampleFraction = FMath::Modf(SampleMultiplier, &IntegralNumSamples);
	const int32 NumSamples = static_cast<int32>(IntegralNumSamples);

	/** Add full samples to the extrapolated motion. */
	FTransform ExtrapolateTransform = FTransform::Identity;
	for (int i = 0; i < NumSamples; ++i)
	{
		ExtrapolateTransform = AbsTimeSampleToExtrapolate * ExtrapolateTransform;
	}

	/** Blend the remaining partial sample. */
	FTransform RemainingExtrapolatedTransform;
	RemainingExtrapolatedTransform.Blend(FTransform::Identity, AbsTimeSampleToExtrapolate, RemainingSampleFraction);

	/** Return the total. */
	ExtrapolateTransform = RemainingExtrapolatedTransform * ExtrapolateTransform;
	return ExtrapolateTransform;
}

float UAnimSuiteMathLibrary::CalculatePoseCost(const TArray<FTransform>& SourceBoneTransforms, const TArray<FTransform>& CandidateBoneTransforms)
{
	if (SourceBoneTransforms.Num() != CandidateBoneTransforms.Num())
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"CalculatePoseCost\": There are %d source bones and %d target bones being sampled. "
									 "The number of bones between source and target must be the same."), SourceBoneTransforms.Num(), CandidateBoneTransforms.Num());
		return 0.0f;
	}

	float Cost = 0.0f;
	for (int32 i = 0; i < SourceBoneTransforms.Num(); ++i)
	{
		Cost += FVector::DistSquared(SourceBoneTransforms[i].GetTranslation(), CandidateBoneTransforms[i].GetTranslation());
	}
	return Cost;
}

float UAnimSuiteMathLibrary::CalculatePoseCost(const TArray<FVector>& SourceBoneVectors, const TArray<FVector>& TargetBoneVectors)
{
	if (SourceBoneVectors.Num() != TargetBoneVectors.Num())
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"CalculatePoseCost\": There are %d source bones and %d target bones being sampled. "
									 "The number of bone velocities between source and target must be the same."), SourceBoneVectors.Num(), TargetBoneVectors.Num());
		return 0.0f;
	}

	float Cost = 0.0f;
	for (int32 i = 0; i < SourceBoneVectors.Num(); ++i)
	{
		Cost = Cost + FVector::DistSquared(SourceBoneVectors[i], TargetBoneVectors[i]);
	}
	
	return Cost;
}

void UAnimSuiteMathLibrary::CalculatePoseVelocities(TArray<FVector>& Velocities, const TArray<FTransform>& CurrentTransforms,
                                                    const TArray<FTransform>& PrevTransforms, const float TimeInterval)
{
	Velocities.Empty();
	if (CurrentTransforms.Num() != PrevTransforms.Num())
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"CalculatePoseVelocities\": There are %d bones for the current frame's transforms and %d bone transforms being from the previous frame. "
									 "The number of bone transforms between the current and previous frames must be the same."), CurrentTransforms.Num(), PrevTransforms.Num());
		return;
	}
	
	for (int32 i = 0; i < CurrentTransforms.Num(); ++i)
	{
		Velocities.Add((CurrentTransforms[i].GetTranslation() - PrevTransforms[i].GetTranslation()) / TimeInterval); 
	}

	return;
}

int32 UAnimSuiteMathLibrary::FindNormalizedMinCostIndex(TArray<float>& CostArrayX, const float MinX, const float MaxX, TArray<float>& CostArrayY,
	const float MinY, const float MaxY)
{
	if (CostArrayX.Num() != CostArrayY.Num())
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"FindNormalizedMinCost\": There are %d entries in the X array and %d entries in the Y array. "
									 "The number of entries must be the same. Returning 0."), CostArrayX.Num(), CostArrayY.Num());
		return 0;
	}

	if (CostArrayX.Num() == 0)
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"FindNormalizedMinCost\": There are %d entries in the input arrays. "
									 "The number of entries must be positive. Returning 0."), CostArrayX.Num());
		return 0;
	}
	
	TArray<float> NormalizedCostArray;
	for (int32 i = 0; i < CostArrayX.Num(); ++i)
	{
		CostArrayX[i] = (CostArrayX[i] - MinX) / (MaxX - MinX);
		CostArrayY[i] = (CostArrayY[i] - MinY) / (MaxY - MinY);
		NormalizedCostArray.Add(CostArrayX[i] + CostArrayY[i]);
	}
	int32 MinCostIndex;
	FMath::Min(NormalizedCostArray, &MinCostIndex);

	return MinCostIndex;
}

FBoneContainer UAnimSuiteMathLibrary::SetBoneContainer(USkeleton* Skeleton)
{
	if (Skeleton == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"SetBoneContainer\": The input Skeleton is invalid. Returning default contructor of FBoneContainer."));
		return FBoneContainer();
	}

	const int32 TotalBones = Skeleton->GetReferenceSkeleton().GetNum();
	TArray<FBoneIndexType> RequiredBoneIndexArray;
	RequiredBoneIndexArray.Reserve(TotalBones);
	for (int32 Idx = 0; Idx < TotalBones; Idx++)
	{
		RequiredBoneIndexArray.Add(Idx);
	}

	FBoneContainer BoneContainer(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(), *Skeleton);
	
	return BoneContainer;
}

const FTransform& UAnimSuiteMathLibrary::GetBonePose(const FAMSPose& Pose, FName BoneName, EAMSPoseSpaces Space)
{
	if (!Pose.IsValid())
	{
		UE_LOG(LogAnimation, Error, TEXT("\"GetBonePose\": The input pose is not valid."));
		return FTransform::Identity;
	}

	const int32 BoneIndex = Pose.BoneNames.IndexOfByKey(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogAnimation, Error, TEXT("\"GetBonePose\": No bone with name %s was found."), *BoneName.ToString());
		return FTransform::Identity;
	}

	return Space == EAMSPoseSpaces::Local ? Pose.LocalSpacePoses[BoneIndex] : Pose.ComponentSpacePoses[BoneIndex];	
}

void UAnimSuiteMathLibrary::GetAnimPoseAtTime(const UAnimSequenceBase* AnimationSequenceBase, double Time,
                                              FAMSPoseEvaluationOptions EvaluationOptions, FAMSPose& Pose)
{
	TArray<FAMSPose> InOutPoses;
	GetAnimPoseAtTimeIntervals(AnimationSequenceBase, { Time }, EvaluationOptions, InOutPoses);

	if (InOutPoses.Num())
	{
		ensure(InOutPoses.Num() == 1);
		Pose = InOutPoses[0];
	}
}

void UAnimSuiteMathLibrary::GetAnimPoseAtTimeIntervals(const UAnimSequenceBase* AnimationSequenceBase,
                                                       TArray<double> TimeIntervals, FAMSPoseEvaluationOptions EvaluationOptions, TArray<FAMSPose>& InOutPoses)
{
	if (!(AnimationSequenceBase && AnimationSequenceBase->GetSkeleton()))
	{
		UE_LOG(LogAnimation, Error, TEXT("\"GetAnimPoseAtTimeIntervals\": An invalid animation Sequence was supplied."))
		return;
	}
	
	/** To prevent the crash with error !bShouldEnforceAllocMarks || NumMarks > 0, add the below line. */
	FMemMark Mark(FMemStack::Get());
	
	/** Select the asset to use for initializing the bone container. */
	UObject* AssetToUse = CastChecked<UObject>(AnimationSequenceBase->GetSkeleton());
	int32 NumRequiredBones = AnimationSequenceBase->GetSkeleton()->GetReferenceSkeleton().GetNum();

	/** Set the array of bone indices. */
	TArray<FBoneIndexType> RequiredBoneIndexArray;
	RequiredBoneIndexArray.AddUninitialized(NumRequiredBones);
	for (int32 BoneIndex = 0; BoneIndex < RequiredBoneIndexArray.Num(); ++BoneIndex)
	{
		RequiredBoneIndexArray[BoneIndex] = static_cast<FBoneIndexType>(BoneIndex);
	}

	/** Set the bone container. */
	FBoneContainer RequiredBones;
	RequiredBones.InitializeTo(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(), *AssetToUse);
	RequiredBones.SetUseRAWData(EvaluationOptions.EvaluationType == EAMSAnimDataEvalType::Raw);
	RequiredBones.SetUseSourceData(EvaluationOptions.EvaluationType == EAMSAnimDataEvalType::Source);
	RequiredBones.SetDisableRetargeting(false);

	FCompactPose CompactPose;
	FBlendedCurve Curve;
	UE::Anim::FStackAttributeContainer Attributes;

	FAnimationPoseData PoseData(CompactPose, Curve, Attributes);
	FAnimExtractContext Context(0.0, EvaluationOptions.bExtractRootMotion);

	FCompactPose BasePose;
	BasePose.SetBoneContainer(&RequiredBones);
    
	CompactPose.SetBoneContainer(&RequiredBones);
	Curve.InitFrom(RequiredBones);

	FAMSPose Pose;
	Pose.Init(RequiredBones);

	for (int32 Index = 0; Index < TimeIntervals.Num(); ++Index)
	{
		/** Ensure the time is within the animation range. */
		const double EvalInterval = TimeIntervals[Index];
		bool bValidTime = FMath::IsWithinInclusive(EvalInterval, 0.0f, AnimationSequenceBase->GetPlayLength());
		ensure(bValidTime);

		/** Set the Context's time to the validated time. */
		Context.CurrentTime = EvalInterval;

		FAMSPose& FramePose = InOutPoses.AddDefaulted_GetRef();
		FramePose = Pose;

		Curve.InitFrom(RequiredBones);
		
		if (bValidTime)
		{
			if (AnimationSequenceBase->IsValidAdditive())
			{
				UE_LOG(LogAnimation, Error, TEXT("\"GetAnimPoseAtTimeIntervals\": Additive animations are not (yet) supported."))
				return;
			}
			
			CompactPose.ResetToRefPose();
			AnimationSequenceBase->GetAnimationPose(PoseData, Context);
			FramePose.SetPose(PoseData);
		}
		else
		{
			UE_LOG(LogAnimation, Error, TEXT("\"GetAnimPoseAtTimeIntervals\": An invalid time value of %f for Animation Sequence %s was supplied."), EvalInterval, *AnimationSequenceBase->GetName());
		}
	}
}


//-------------------------------------
// Distance Matching 
//-------------------------------------

float UAnimSuiteMathLibrary::CalculateNormalizedTime(const UBlendSpace* BlendSpace, const float MatchingDistance,
                                                       const float PrevNormalizedTime, TArray<FBlendSampleData>& BlendSampleData, const float DeltaTime,
                                                       const FName DistanceCurveName, const FVector2D PlayRateClamp, const bool bAdvanceTimeNaturally,
                                                       const bool bUseHighestWeightedSample, const bool bIsLooping)
{
	float BlendAnimLength = 0.0f;
	float NormalizedMatchingTime = 0.0f;
	float NormalizedDeltaTime = 0.0f;

	/** If we're to use only the highest weighted sample, get that sample; otherwise, use all relevant samples. */
	if (bUseHighestWeightedSample)
	{
		int32 HighestWeightSampleIdx = 0;
		const FBlendSampleData* HighestSample = &BlendSampleData[HighestWeightSampleIdx];
		for(int32 I = 1; I < BlendSampleData.Num(); ++I)
		{
			if(BlendSampleData[I].TotalWeight > HighestSample->TotalWeight)
			{
				HighestSample = &BlendSampleData[I];
				HighestWeightSampleIdx = I;
			}
		}

		TObjectPtr<UAnimSequenceBase> HighestWeightedAnimSequence = Cast<UAnimSequenceBase>(HighestSample->Animation);
		if (HighestWeightedAnimSequence == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("\"CalculateNormalizedTime\": The highest weighted sample is null. Return 0.0f."))
			return 0.0f;
		}

		if (bAdvanceTimeNaturally == true) // don't distance match
		{
			/** Get the actual playback time of the animation Sequence (itafter being modified by the BlendSpace. When playing
				naturally, the playrate and rate scale should be taken into account. */
			float MultipliedSampleRateScale = HighestWeightedAnimSequence->RateScale * BlendSampleData[HighestWeightSampleIdx].SamplePlayRate;
			MultipliedSampleRateScale = (MultipliedSampleRateScale != 0.0f) ? FMath::Abs(MultipliedSampleRateScale) : 1.0f;
			BlendAnimLength = HighestWeightedAnimSequence->GetPlayLength() / MultipliedSampleRateScale;
		}
		else // distance match
		{
			/** Get the normalized time. */
			const float MatchingTime = GetAnimTimeFromCurveValue(HighestWeightedAnimSequence, MatchingDistance, DistanceCurveName);
			NormalizedMatchingTime = MatchingTime / HighestWeightedAnimSequence->GetPlayLength();

			/** Get the normalized delta time. @TODO: should I use this or (DeltaTime / BlendAnimLength) outside of loop? */
			NormalizedDeltaTime = DeltaTime / HighestWeightedAnimSequence->GetPlayLength();
		}
		
	}
	else // Use all relevant Blend Samples
	{
		for (int32 I = 0; I < BlendSampleData.Num(); ++I)
		{
			const int32 Index = BlendSampleData[I].SampleDataIndex;
			if (Index < 0  /*!BlendSpace->IsValidBlendSampleIndex(Index)*/) // check that Index is valid (must be >= 0)
			{
				continue;
			}
		
			const FBlendSample& Sample = BlendSpace->GetBlendSample(Index);
			if (Sample.Animation == nullptr)
			{
				continue;
			}

			TObjectPtr<UAnimSequenceBase> SampleAnimSequenceBase = Cast<UAnimSequenceBase>(Sample.Animation);
			if (SampleAnimSequenceBase == nullptr)
			{
				continue;
			}
		
			if (bAdvanceTimeNaturally == true) // don't distance match
			{
				/** Get the actual playback time of the animation Sequence after being modified by the BlendSpace. When playing
					naturally, the playrate and rate scale should be taken into account. */
				float MultipliedSampleRateScale = SampleAnimSequenceBase->RateScale /*Sample.Animation->RateScale*/ * BlendSampleData[I].SamplePlayRate;
				MultipliedSampleRateScale = (MultipliedSampleRateScale != 0.0f) ? FMath::Abs(MultipliedSampleRateScale) : 1.0f;
				BlendAnimLength += SampleAnimSequenceBase->GetPlayLength() /*Sample.Animation->GetPlayLength()*/ / MultipliedSampleRateScale * BlendSampleData[I].GetClampedWeight();
			}
			else // distance match
			{
				/** Get the normalized time. */
				const float MatchingTime = GetAnimTimeFromCurveValue(SampleAnimSequenceBase /*Sample.Animation*/, MatchingDistance, DistanceCurveName);
				NormalizedMatchingTime += MatchingTime / SampleAnimSequenceBase->GetPlayLength() /*Sample.Animation->GetPlayLength()*/ * BlendSampleData[I].GetClampedWeight();

				/** Get the normalized delta time. @TODO: should I use this or (DeltaTime / BlendAnimLength) outside of loop? */
				NormalizedDeltaTime += DeltaTime / SampleAnimSequenceBase->GetPlayLength() /*Sample.Animation->GetPlayLength()*/ * BlendSampleData[I].GetClampedWeight();
			}
		}
	}
	
	if (bAdvanceTimeNaturally == true)
	{
		/** Return the clamped normalized time in a range [0,1]. */
		NormalizedDeltaTime = DeltaTime / BlendAnimLength;
		return NormalizedMatchingTime = FMath::Clamp(PrevNormalizedTime + NormalizedDeltaTime, 0.0f, 1.0f);
	}
	else
	{
		/** Clamp the effective play rate based on the PlayRateClamp. */
		const float DesiredNormalizedDeltaTime = NormalizedMatchingTime - PrevNormalizedTime;
		float EffectivePlayRate = DesiredNormalizedDeltaTime / NormalizedDeltaTime;
		if (PlayRateClamp.X >= 0.0f && PlayRateClamp.X < PlayRateClamp.Y)
		{
			EffectivePlayRate = FMath::Clamp(EffectivePlayRate, PlayRateClamp.X, PlayRateClamp.Y);
		}

		/** Return the clamped normalized time in a range [0,1]. */
		//return NormalizedMatchingTime = FMath::Clamp(PrevNormalizedTime + EffectivePlayRate * NormalizedDeltaTime,0.0f, 1.0f) ;

		/** Find the normalized time. */
		NormalizedMatchingTime = PrevNormalizedTime + EffectivePlayRate * NormalizedDeltaTime;

		/** Deal explicitly with the case of a looping animation. */
		if (NormalizedMatchingTime < 0.0f || NormalizedMatchingTime > 1.0f)
		{
			if (bIsLooping)
			{
				NormalizedMatchingTime = FMath::Fmod(NormalizedMatchingTime, 1.0f);

				/** Fmod does not give a result that falls into [0, AnimLength] but one that falls into [-AnimLength, AnimLength];
					hence, negative values need to be addressed in a special way. */
				if (NormalizedMatchingTime < 0.0f)
				{
					NormalizedMatchingTime += 1.0f;
				}
			}
		}
		else
		{
			NormalizedMatchingTime = FMath::Clamp(NormalizedMatchingTime,0.0f, 1.0f);
		}

		return NormalizedMatchingTime;
	}
}

float UAnimSuiteMathLibrary::CalculateExplicitTime(const UAnimSequenceBase* Sequence, const float MatchingDistance,
	const float PrevExplicitTime, const float DeltaTime, const FName DistanceCurveName, const FVector2D PlayRateClamp,
	const bool bAdvanceTimeNaturally, const bool bIsLooping)
{
	if (bAdvanceTimeNaturally)
	{
		return PrevExplicitTime + DeltaTime;
	}

	/** Distance match to find the time. */
	float ExplicitTime = GetAnimTimeFromCurveValue(Sequence, MatchingDistance, DistanceCurveName);

	/** Get the effective playrate. */
	float EffectivePlayRate = (ExplicitTime - PrevExplicitTime) / DeltaTime;
	if (PlayRateClamp.X >= 0.0f && PlayRateClamp.X < PlayRateClamp.Y )
	{
		EffectivePlayRate = FMath::Clamp(EffectivePlayRate, PlayRateClamp.X, PlayRateClamp.Y);
	}

	/** Find the explicit time. */
	ExplicitTime = PrevExplicitTime + EffectivePlayRate * DeltaTime;

	/** Deal explicitly with the case of a looping animation. */
	const float AnimLength = Sequence->GetPlayLength();
	if (ExplicitTime < 0.0f || ExplicitTime > AnimLength)
	{
		if (bIsLooping)
		{
			if (AnimLength != 0)
			{
				ExplicitTime = FMath::Fmod(ExplicitTime, AnimLength);

				/** Fmod does not give a result that falls into [0, AnimLength] but one that falls into [-AnimLength, AnimLength];
				    hence, negative values need to be addressed in a special way. */
				if (ExplicitTime < 0.0f)
				{
					ExplicitTime += AnimLength;
				}
			}
			else // The AnimLength is 0.0f
			{
				ExplicitTime = 0.0f;
			}
		}
	}
	else
	{
		ExplicitTime = FMath::Clamp(ExplicitTime, 0.0f, Sequence->GetPlayLength());
	}
	
	return ExplicitTime;
	
	//return FMath::Clamp(ExplicitTime, 0.0f, Sequence->GetPlayLength());
}

float UAnimSuiteMathLibrary::GetAnimTimeFromCurveValue(const UAnimSequenceBase* InAnimSequence, const float InValue,
                                                         const FName CurveName) 
{
	const FAnimCurveBufferAccess BufferCurveAccess(InAnimSequence, CurveName);
	
	if (BufferCurveAccess.IsValid())
	{
		const int32 NumKeys = BufferCurveAccess.GetNumSamples();
		if (NumKeys < 2)
		{
			return 0.f;
		}

		// Some assumptions: 
		// - keys have unique values, so for a given value, it maps to a single position on the timeline of the animation.
		// - key values are sorted in increasing order.

		int32 First = 1;
		const int32 Last = NumKeys - 1;
		int32 Count = Last - First;

		while (Count > 0)
		{
			const int32 Step = Count / 2;
			const int32 Middle = First + Step;

			if (InValue > BufferCurveAccess.GetValue(Middle))
			{
				First = Middle + 1;
				Count -= Step + 1;
			}
			else
			{
				Count = Step;
			}
		}

		const float KeyAValue = BufferCurveAccess.GetValue(First - 1);
		const float KeyBValue = BufferCurveAccess.GetValue(First);
		const float Diff = KeyBValue - KeyAValue;
		const float Alpha = !FMath::IsNearlyZero(Diff) ? ((InValue - KeyAValue) / Diff) : 0.0f;

		const float KeyATime = BufferCurveAccess.GetTime(First - 1);
		const float KeyBTime = BufferCurveAccess.GetTime(First);
		return FMath::Lerp(KeyATime, KeyBTime, Alpha);
	}

	return 0.0f;
}

float UAnimSuiteMathLibrary::GetCurveValueFromAnimTime(const UAnimSequenceBase* InAnimSequence, const float& InTime,
															const FName CurveName)
{
	const FAnimCurveBufferAccess BufferCurveAccess(InAnimSequence, CurveName);
	
	if (BufferCurveAccess.IsValid())
	{
		const int32 NumKeys = BufferCurveAccess.GetNumSamples();
		if (NumKeys < 2)
		{
			return 0.f;
		}
		
		int32 First = 1;
		const int32 Last = NumKeys - 1;
		int32 Count = Last - First;

		while (Count > 0)
		{
			const int32 Step = Count / 2;
			const int32 Middle = First + Step;

			if (InTime > BufferCurveAccess.GetTime(Middle))
			{
				First = Middle + 1;
				Count -= Step + 1;
			}
			else
			{
				Count = Step;
			}
		}

		const float KeyATime = BufferCurveAccess.GetTime(First - 1);
		const float KeyBTime = BufferCurveAccess.GetTime(First);
		const float Diff = KeyBTime - KeyATime;
		const float Alpha = !FMath::IsNearlyZero(Diff) ? ((InTime - KeyATime) / Diff) : 0.0f;

		const float KeyAValue = BufferCurveAccess.GetValue(First - 1);
		const float KeyBValue = BufferCurveAccess.GetValue(First);
		return FMath::Lerp(KeyAValue, KeyBValue, Alpha);
	}

	return 0.0f;
}

float UAnimSuiteMathLibrary::GetDistanceRange(const UAnimSequenceBase* InAnimSequence, const FName CurveName)
{
	const FAnimCurveBufferAccess BufferCurveAccess(InAnimSequence, CurveName);
	
	if (BufferCurveAccess.IsValid())
	{
		const int32 NumSamples = BufferCurveAccess.GetNumSamples();
		if (NumSamples >= 2)
		{
			return (BufferCurveAccess.GetValue(NumSamples - 1) - BufferCurveAccess.GetValue(0));
		}
	}
	return 0.f;
}

float UAnimSuiteMathLibrary::GetTimeAfterDistanceTraveled(const UAnimSequenceBase* AnimSequence, const float CurrentTime,
                                                            const float DistanceTraveled, const FName DistanceCurveName, const int32 StuckLoopThreshold, const bool bAllowLooping)
{
	float NewTime = CurrentTime;
	if (AnimSequence != nullptr)
	{
		/** Avoid infinite loops if the animation doesn't cover any distance. */
		if (!FMath::IsNearlyZero(GetDistanceRange(AnimSequence, DistanceCurveName)))
		{
			float AccumulatedDistance = 0.0f;

			const float SequenceLength = AnimSequence->GetPlayLength();
			const float StepTime = 1.0f / 30.0f;

			/** Distance Matching expects the distance curve on the animation to increase monotonically. If the curve fails to increase in value
			    after a certain number of iterations, we abandon the algorithm to avoid an infinite loop. */
			//const int32 StuckLoopThreshold = 5;
			int32 StuckLoopCounter = 0;

			/** Traverse the distance curve, accumulating animated distance until the desired distance is reached. */
			while ((AccumulatedDistance < DistanceTraveled) && (bAllowLooping || (NewTime + StepTime < SequenceLength)))
			{
				const float CurrentDistance = AnimSequence->EvaluateCurveData(DistanceCurveName, NewTime);
				const float DistanceAfterStep = AnimSequence->EvaluateCurveData(DistanceCurveName, NewTime + StepTime);
				const float AnimationDistanceThisStep = DistanceAfterStep - CurrentDistance;

				if (!FMath::IsNearlyZero(AnimationDistanceThisStep))
				{
					/** Keep advancing if the desired distance hasn't been reached. */
					if (AccumulatedDistance + AnimationDistanceThisStep < DistanceTraveled)
					{
						FAnimationRuntime::AdvanceTime(bAllowLooping, StepTime, NewTime, SequenceLength);
						AccumulatedDistance += AnimationDistanceThisStep;
					}
					/** Once the desired distance is passed, find the approximate time between samples where the distance will be reached. */
					else
					{
						const float DistanceAlpha = (DistanceTraveled - AccumulatedDistance) / AnimationDistanceThisStep;
						FAnimationRuntime::AdvanceTime(bAllowLooping, DistanceAlpha * StepTime, NewTime, SequenceLength);
						AccumulatedDistance = DistanceTraveled;
						break;
					}

					StuckLoopCounter = 0;
				}
				else
				{
					++StuckLoopCounter;
					if (StuckLoopCounter >= StuckLoopThreshold)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to advance any distance after %d loops in GetTimeAfterDistanceTraveled on anim sequence (%s). Aborting."), StuckLoopThreshold, *GetNameSafe(AnimSequence));
						break;
					}
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Anim sequence (%s) is missing a distance curve or doesn't cover enough distance for GetTimeAfterDistanceTraveled."), *GetNameSafe(AnimSequence));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid AnimSequence passed to GetTimeAfterDistanceTraveled."));
	}

	return NewTime;
}

