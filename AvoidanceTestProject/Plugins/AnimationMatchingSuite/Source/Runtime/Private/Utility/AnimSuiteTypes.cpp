// Copyright MuuKnighted Games 2024. All rights reserved.

#include "Utility/AnimSuiteTypes.h"

#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"


bool FAMSPose::IsValid() const
{
	const int32 ExpectedNumBones = BoneNames.Num();
	bool bIsValid = ExpectedNumBones != 0;
	
	bIsValid &= BoneIndices.Num() == ExpectedNumBones;
	bIsValid &= ParentBoneIndices.Num() == ExpectedNumBones;
	bIsValid &= LocalSpacePoses.Num() == ExpectedNumBones;
	bIsValid &= ComponentSpacePoses.Num() == ExpectedNumBones;
	bIsValid &= RefLocalSpacePoses.Num() == ExpectedNumBones;
	bIsValid &= RefComponentSpacePoses.Num() == ExpectedNumBones;
	
	return bIsValid;
}

void FAMSPose::Init(const FBoneContainer& InBoneContainer)
{
	Reset();

	const FReferenceSkeleton& RefSkeleton = InBoneContainer.GetSkeletonAsset()->GetReferenceSkeleton();

	for (const FBoneIndexType BoneIndex : InBoneContainer.GetBoneIndicesArray())
	{			
		const FCompactPoseBoneIndex CompactIndex(BoneIndex);
		const FCompactPoseBoneIndex CompactParentIndex = InBoneContainer.GetParentBoneIndex(CompactIndex);

		const int32 SkeletonBoneIndex = InBoneContainer.GetSkeletonIndex(CompactIndex);
		if (SkeletonBoneIndex != INDEX_NONE)
		{
			const int32 ParentBoneIndex = CompactParentIndex.GetInt() != INDEX_NONE ? InBoneContainer.GetSkeletonIndex(CompactParentIndex) : INDEX_NONE;

			BoneIndices.Add(SkeletonBoneIndex);
			ParentBoneIndices.Add(ParentBoneIndex);

			BoneNames.Add(RefSkeleton.GetBoneName(SkeletonBoneIndex));

			RefLocalSpacePoses.Add(InBoneContainer.GetRefPoseTransform(FCompactPoseBoneIndex(BoneIndex)));
		}
	} 

	TArray<bool> Processed;
	Processed.SetNumZeroed(BoneNames.Num());
	RefComponentSpacePoses.SetNum(BoneNames.Num());
	for (int32 EntryIndex = 0; EntryIndex < BoneNames.Num(); ++EntryIndex)
	{
		const int32 ParentIndex = ParentBoneIndices[EntryIndex];
		const int32 TransformIndex = BoneIndices.IndexOfByKey(ParentIndex);

		if (TransformIndex != INDEX_NONE)
		{
			ensure(Processed[TransformIndex]);
			RefComponentSpacePoses[EntryIndex] = RefLocalSpacePoses[EntryIndex] * RefComponentSpacePoses[TransformIndex];
		}
		else
		{
			RefComponentSpacePoses[EntryIndex] = RefLocalSpacePoses[EntryIndex];
		}

		Processed[EntryIndex] = true;
	}
}

void FAMSPose::GetPose(FCompactPose& InOutCompactPose) const
{
	if (IsValid())
	{
		for (int32 Index = 0; Index < BoneNames.Num(); ++Index)
		{
			const FName& BoneName = BoneNames[Index];
			const FCompactPoseBoneIndex PoseBoneIndex = FCompactPoseBoneIndex(InOutCompactPose.GetBoneContainer().GetPoseBoneIndexForBoneName(BoneName));
			if (PoseBoneIndex != INDEX_NONE)
			{
				InOutCompactPose[PoseBoneIndex] = LocalSpacePoses[Index];
			}
		}
	}
}

void FAMSPose::SetPose(USkeletalMeshComponent* Component)
{
	if (!IsInitialized())
	{
		return;
	}
		
	const FBoneContainer& ContextBoneContainer = Component->GetAnimInstance()->GetRequiredBones();

	LocalSpacePoses.SetNum(RefLocalSpacePoses.Num());

	TArray<FTransform> BoneSpaceTransforms = Component->GetBoneSpaceTransforms();
	for (const FBoneIndexType BoneIndex : ContextBoneContainer.GetBoneIndicesArray())
	{
		const int32 SkeletonBoneIndex = ContextBoneContainer.GetSkeletonIndex(FCompactPoseBoneIndex(BoneIndex));
		LocalSpacePoses[BoneIndices.IndexOfByKey(SkeletonBoneIndex)] = BoneSpaceTransforms[BoneIndex];
	}

	ensure(LocalSpacePoses.Num() == RefLocalSpacePoses.Num());	
	GenerateComponentSpaceTransforms();
}

void FAMSPose::SetPose(const FAnimationPoseData& PoseData)
{
	const FCompactPose& CompactPose = PoseData.GetPose();
	
	if (!IsInitialized())
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"SetPose\": The animation pose was not previously initialized."));
		return;
	}

	const FBoneContainer& ContextBoneContainer = CompactPose.GetBoneContainer();
			
	LocalSpacePoses.SetNum(RefLocalSpacePoses.Num());
	for (const FCompactPoseBoneIndex BoneIndex : CompactPose.ForEachBoneIndex())
	{
		const int32 SkeletonBoneIndex = ContextBoneContainer.GetSkeletonIndex(BoneIndex);
		LocalSpacePoses[BoneIndices.IndexOfByKey(SkeletonBoneIndex)] = CompactPose[BoneIndex];
	}

	ensure(LocalSpacePoses.Num() == RefLocalSpacePoses.Num());
	GenerateComponentSpaceTransforms();

	const FBlendedCurve& Curve = PoseData.GetCurve();

	Curve.ForEachElement([&CurveNames = CurveNames, &CurveValues = CurveValues](const UE::Anim::FCurveElement& InElement)
						{
							CurveNames.Add(InElement.Name);
							CurveValues.Add(InElement.Value);
						});
	TArray<USkeletalMeshSocket*> Sockets;
	const USkeleton* Skeleton = ContextBoneContainer.GetSkeletonAsset();
	const USkeletalMesh* SkeletalMesh = ContextBoneContainer.GetSkeletalMeshAsset();
	if (SkeletalMesh)
	{
		Sockets = SkeletalMesh->GetActiveSocketList();
	}
	else if (Skeleton)
	{
		Sockets = Skeleton->Sockets;
	}

	for (const USkeletalMeshSocket* Socket : Sockets)
	{
		const int32 PoseBoneIndex = ContextBoneContainer.GetPoseBoneIndexForBoneName(Socket->BoneName);
		if (PoseBoneIndex != INDEX_NONE)
		{
			SocketNames.Add(Socket->SocketName);
			SocketParentBoneNames.Add(Socket->BoneName);
			SocketTransforms.Add(Socket->GetSocketLocalTransform());
		}
	}		
}

void FAMSPose::SetToRefPose()
{
	if (!IsInitialized())
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"SetToRefPose\": The animation pose was not previously initialized."));
		return;
	}

	LocalSpacePoses = RefLocalSpacePoses;
	ComponentSpacePoses = RefComponentSpacePoses;
}

void FAMSPose::GenerateComponentSpaceTransforms()
{
	if (!IsPopulated())
	{
		UE_LOG(LogAnimation, Warning, TEXT("\"GenerateComponentSpaceTransforms\": The animation pose was not previously populated."));
		return;
	}

	TArray<bool> Processed;
	Processed.SetNumZeroed(BoneNames.Num());
	ComponentSpacePoses.SetNum(BoneNames.Num());
	for (int32 EntryIndex = 0; EntryIndex < BoneNames.Num(); ++EntryIndex)
	{
		const int32 ParentIndex = ParentBoneIndices[EntryIndex];
		const int32 TransformIndex = BoneIndices.IndexOfByKey(ParentIndex);
		if (TransformIndex != INDEX_NONE)
		{
			ensure(Processed[TransformIndex]);
			ComponentSpacePoses[EntryIndex] = LocalSpacePoses[EntryIndex] * ComponentSpacePoses[TransformIndex];
		}
		else
		{
			ComponentSpacePoses[EntryIndex] = LocalSpacePoses[EntryIndex];
		}

		Processed[EntryIndex] = true;
	}
}

void FAMSPose::Reset()
{
	BoneNames.Empty();
	BoneIndices.Empty();
	ParentBoneIndices.Empty(); 
	LocalSpacePoses.Empty();
	ComponentSpacePoses.Empty();
	RefLocalSpacePoses.Empty();
	RefComponentSpacePoses.Empty();
}

FAMSDebugData::FAMSDebugData()
{
}

FAMSDebugData::FAMSDebugData(TArray<FTransform> InDebugBoneTransforms)
	: DebugBoneTransforms(InDebugBoneTransforms)
{
	const int32 NumOfEntries = DebugBoneTransforms.Num();
	for (int32 i = 0; i < NumOfEntries; ++i)
	{
		DebugBoneVelocities.Add(FVector::ZeroVector);
	}
}

FAMSDebugData::FAMSDebugData(TArray<FTransform> InDebugBoneTransforms, TArray<FVector> InDebugBoneVelocities)
	: DebugBoneTransforms(InDebugBoneTransforms)
	, DebugBoneVelocities(InDebugBoneVelocities)
{
}

bool FAMSDebugData::AreTransformsEmpty() const
{
	return DebugBoneTransforms.IsEmpty();
}

bool FAMSDebugData::AreVelocitiesEmpty() const
{
	return DebugBoneVelocities.IsEmpty();
}

bool FAMSDebugData::IsStructEmpty() const
{
	const bool bTransformsEmpty = DebugBoneTransforms.IsEmpty();
	const bool bVelocitiesEmpty = DebugBoneVelocities.IsEmpty();

	return bTransformsEmpty || bVelocitiesEmpty;
}

FPoseBoneData::FPoseBoneData()
	: Transform(FTransform::Identity)
	, Velocity(FVector::ZeroVector)
	, BoneID(0)
{
}

FPoseBoneData::FPoseBoneData(FTransform InTransform, FVector InVelocity, int32 InBoneID)
	: Transform(InTransform)
	, Velocity(InVelocity)
	, BoneID(InBoneID)
{
}

FPoseMatchData::FPoseMatchData()
	: PoseID(-1)
	, Time(0.0f)
{
}

FPoseMatchData::FPoseMatchData(int32 InPoseID, float InTime)
	: PoseID(InPoseID)
	, Time(InTime)
{
}

FPoseExtrapolationParameters::FPoseExtrapolationParameters()
	: RotationalSpeedThreshold(1.0f)
	, LinearSpeedThreshold(1.0f)
	, SampleTime(0.05f)
{
}

