// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "BonePose.h"
#include "BoneContainer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimInstance.h"
#include "AnimSuiteTypes.generated.h"

UENUM(BlueprintType)
enum class EMatchingRange : uint8
{
	/** The entire animation will be searched for the best matching pose. */
	FullRange,
	
	/** Only the time interval in the custom range will be searched for the best matching pose. */
	CustomRange		
};


UENUM(BlueprintType)
enum class EAMSPoseSpaces : uint8
{
	/** Parent-bone-relative space */ 
	Local,
	
	/** Root-relative space */
	Component		
};

UENUM(BlueprintType)
enum class EAMSAnimDataEvalType : uint8
{
	/** Evaluates the original Animation Source data */ 
	Source,
	
	/** Evaluates the original Animation Source data with additive animation layers */
	Raw,
	
	/** Evaluates the compressed Animation data - matching runtime (cooked) */
	Compressed
};

USTRUCT(BlueprintType)
struct ANIMATIONMATCHINGSUITE_API FAMSPoseEvaluationOptions
{
	GENERATED_BODY()

	/** Type of evaluation which should be used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation|Pose")
	EAMSAnimDataEvalType EvaluationType = EAMSAnimDataEvalType::Raw;

	/** Indicates whether to extract root motion values. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category="Animation|Pose")
	bool bExtractRootMotion = false;

	/** Optional skeletal mesh with proportions to use when evaluating a pose. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category="Animation|Pose")
	TObjectPtr<USkeletalMesh> OptionalSkeletalMesh = nullptr;
	
	/** Indicates whether additive animations should be applied to their base-pose. */ 
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category="Animation|Pose")
	bool bRetrieveAdditiveAsFullPose = true;

	/** Indicates whether to evaluate Animation Curves. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category="Animation|Pose")
	bool bEvaluateCurves = true;
	
};

USTRUCT(BlueprintType)
struct ANIMATIONMATCHINGSUITE_API FAMSPose
{
	GENERATED_BODY()

public: 
	
	/** Returns whether the pose data was correctly initialized and populated. */
	bool IsValid() const;
	
	/** Initializes the various arrays, using and copying the provided bone container */
	void Init(const FBoneContainer& InBoneContainer);
	
	/** Populates an FCompactPose using the contained bone data */
	void GetPose(FCompactPose& InOutCompactPose) const;
	
	/** Generates the contained bone data using the provided Component and its AnimInstance */
	void SetPose(USkeletalMeshComponent* Component);
	
	/** Generates the contained bone data using the provided CompactPose */
	void SetPose(const FAnimationPoseData& PoseData);
	
	/** Copies the reference pose to animated pose data. */
	void SetToRefPose();

	/** (Re-)Generates the world space transforms using populated local space data */
	void GenerateComponentSpaceTransforms();
	
	/** Resets all contained data, rendering the instance invalid */
	void Reset();

	/** Indicates whether the contained data was initialized and can be used to store a pose. */
	bool IsInitialized() const { return BoneNames.Num() != 0; }
	
	/** Indicates whether local space pose data has been populated. */
	bool IsPopulated() const { return LocalSpacePoses.Num() != 0; }

public:
	
	UPROPERTY()
	TArray<FName> BoneNames;
	
	UPROPERTY()
	TArray<int32> BoneIndices;

	UPROPERTY()
	TArray<int32> ParentBoneIndices;

	UPROPERTY()
	TArray<FTransform> LocalSpacePoses;
	
	UPROPERTY()
	TArray<FTransform> ComponentSpacePoses;

	UPROPERTY()
	TArray<FTransform> RefLocalSpacePoses;
	
	UPROPERTY()
	TArray<FTransform> RefComponentSpacePoses;

	UPROPERTY()
	TArray<FName> CurveNames;

	UPROPERTY()
	TArray<float> CurveValues;

	UPROPERTY()
	TArray<FName> SocketNames;

	UPROPERTY()
	TArray<FName> SocketParentBoneNames;
	
	UPROPERTY()
	TArray<FTransform> SocketTransforms;
	
	//friend class UAnimPoseExtensions;
	
};

UENUM(BlueprintType)
enum class EMatchingType : uint8
{
	None					UMETA(DisplayName = "No Matching", Tooltip = "The asset player exhibits standard behavior."),
	PoseMatch				UMETA(DisplayName = "Pose Match", Tooltip = "The asset player performs pose matching upon initialization (and optionally upon asset change)."),
	DistanceMatch			UMETA(DisplayName = "Distance Match", Tooltip = "The asset player performs distance matching, meaning play time is driven by an input distance. Note: bLoop is set to false internally."),
	PoseAndDistanceMatch	UMETA(DisplayName = "Pose and Distance Match", Tooltip = "The asset player performs pose matching upon initialization (and optionally upon asset change) and distance matching at other times. Note: bLoop is set to false internally.")
};

UENUM(BlueprintType)
enum class EDebugPoseMatchLevel : uint8
{
	ShowSelectedPosePosition				UMETA(DisplayName = "Show Bone Pose Positions"),
	ShowSelectedPosePositionAndVelocity		UMETA(DisplayName = "Show Bone Pose Positions and Velocities")
};

enum class EPoseExtrapolationType : uint8
{
	NoExtrapolation,	
	ExtrapolateNegative,
	ExtrapolatePositive		
};

USTRUCT(BlueprintInternalUseOnly)
struct ANIMATIONMATCHINGSUITE_API FAMSDebugData
{
	GENERATED_BODY()

public:

	/** Default constructor */
	FAMSDebugData();

	/** Non-default constructor that sets the transforms to the given values and the velocities to zero. */
	FAMSDebugData(TArray<FTransform> InDebugBoneTransforms);
	
	/** Non-default constructor that sets the transforms and velocities to the input values. */
	FAMSDebugData(TArray<FTransform> InDebugBoneTransforms, TArray<FVector> InDebugBoneVelocities);

	bool AreTransformsEmpty() const;

	bool AreVelocitiesEmpty() const;
	
	bool IsStructEmpty() const;

public:

	/** The positions of the pose-matched bones in Component Space (i.e. root-relative space). */
	UPROPERTY(BlueprintReadWrite, Category = "Debug Bone Data")
	TArray<FTransform> DebugBoneTransforms;

	/** The velocities of the pose-matched bones in Component Space (i.e. root-relative space). */
	UPROPERTY(BlueprintReadWrite, Category = "Debug Bone Data")
	TArray<FVector> DebugBoneVelocities;
	
};

USTRUCT(BlueprintInternalUseOnly)
struct ANIMATIONMATCHINGSUITE_API FPoseBoneData
{
	GENERATED_BODY()

public:

	/** Default constructor sets Position and Velocity to zero. */
	FPoseBoneData();

	/** Non-default constructor that sets the Position and Velocity to the input values. */
	FPoseBoneData(FTransform InTransform, FVector InVelocity, int32 InBoneID);

public:

	/** The position of the bone in Component Space (i.e. root-relative space). */
	UPROPERTY(BlueprintReadWrite, Category = "Pose Bone Data")
	FTransform Transform;

	/** The velocity of the bone in Component Space (i.e. root-relative space). */
	UPROPERTY(BlueprintReadWrite, Category = "Pose Bone Data")
	FVector Velocity;

	/** The FCompactPoseBoneIndex (converted to an int32) of the bone. */
	UPROPERTY(BlueprintReadWrite, Category = "Pose Bone Data")
	int32 BoneID;
	
};

USTRUCT(BlueprintInternalUseOnly)
struct ANIMATIONMATCHINGSUITE_API FPoseMatchData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 PoseID;

	// @TODO: add this if I allow more than one Sequence or Blend Space asset in the Pose Matching nodes.
	// UPROPERTY()
	// int32 AssetID;

	UPROPERTY()
	float Time;

	UPROPERTY()
	TArray<FPoseBoneData> BoneData;

public:
	FPoseMatchData();
	FPoseMatchData(int32 InPoseID, float InTime);
};

USTRUCT(BlueprintType)
struct ANIMATIONMATCHINGSUITE_API FPoseExtrapolationParameters
{
	GENERATED_BODY()

	FPoseExtrapolationParameters();
	
	/** If the angular root motion speed in degrees is below this value, it will be treated as zero. */
	UPROPERTY(EditAnywhere, Category = "Settings")
	float RotationalSpeedThreshold;

	/** If the root motion linear speed is below this value, it will be treated as zero. */
	UPROPERTY(EditAnywhere, Category = "Settings")
	float LinearSpeedThreshold;

	/** Time from sequence start/end used to extrapolate the trajectory. (@TODO: NOT USED.) */
	UPROPERTY(EditAnywhere, Category = "Settings")
	float SampleTime;
};
