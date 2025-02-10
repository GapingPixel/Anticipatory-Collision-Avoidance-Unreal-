// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCPlayMontageCallbackProxy.h"
#include "GMCOrganicMovementComponent.h"
#include "GMCLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GMCPlayMontageCallbackProxy)

void UGMC_PlayMontageCallbackProxy::BeginDestroy()
{
  UnbindDelegates_Cosmetic();
  Super::BeginDestroy();
}

UGMC_PlayMontageCallbackProxy* UGMC_PlayMontageCallbackProxy::CreateProxyObjectForPlayMontage(
  UGMC_OrganicMovementCmp* Target,
  UAnimMontage* MontageToPlay,
  float StartPosition,
  float PlayRate,
  FName StartSection,
  bool bInterruptOther,
  bool bCancelOnBlendIn,
  bool bCancelOnBlendOut
)
{
  if (!IsValid(Target) || !IsValid(MontageToPlay))
  {
    return nullptr;
  }

  const auto& Mesh = Target->GetSkeletalMeshReference();
  if (!IsValid(Mesh))
  {
    return nullptr;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!IsValid(AnimInstance))
  {
    return nullptr;
  }

  const bool bPlayedSuccessfully = Target->PlayMontage_Blocking(
    Mesh,
    Target->MontageTracker,
    MontageToPlay,
    StartPosition,
    PlayRate,
    bInterruptOther,
    bCancelOnBlendIn,
    bCancelOnBlendOut,
    false,
    EMontagePlayReturnType::MontageLength
  ) > 0.f;

  if (bPlayedSuccessfully)
  {
    if (UGMC_PlayMontageCallbackProxy* Proxy = NewObject<UGMC_PlayMontageCallbackProxy>())
    {
      Proxy->SetFlags(RF_StrongRefOnFrame);
      Proxy->BindDelegates(Target, Mesh, AnimInstance, StartSection);
      Target->MontageTracker.CallbackProxyRef = Proxy;
      return Proxy;
    }
    else
    {
      gmc_ckne()
    }
  }

  return nullptr;
}

void UGMC_PlayMontageCallbackProxy::BindDelegates(
  UGMC_OrganicMovementCmp* Target,
  USkeletalMeshComponent* Mesh,
  UAnimInstance* AnimInstance,
  FName StartSection
)
{
  gmc_ck(IsValid(Target))
  gmc_ck(IsValid(Mesh))
  gmc_ck(IsValid(AnimInstance))

  OrganicMovementCmpPtr = Target;
  AnimInstancePtr = AnimInstance;

  gmc_ck(OrganicMovementCmpPtr->MontageTracker.HasActiveMontage())
  const auto& Montage = OrganicMovementCmpPtr->MontageTracker.Montage;

  if (StartSection != NAME_None)
  {
    OrganicMovementCmpPtr->JumpToMontageSection(Mesh, OrganicMovementCmpPtr->MontageTracker, StartSection, false);
  }

  // Networked delegates.

  const auto& MontageTracker = OrganicMovementCmpPtr->MontageTracker;

  MontageStartDelegate.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnMontageStart);
  OrganicMovementCmpPtr->SetMontageStartDelegate(MontageStartDelegate, MontageTracker);

  MontageBlendInDelegate.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnMontageBlendIn);
  OrganicMovementCmpPtr->SetMontageBlendInDelegate(MontageBlendInDelegate, MontageTracker);

  MontageBlendOutDelegate.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnMontageBlendOut);
  OrganicMovementCmpPtr->SetMontageBlendOutDelegate(MontageBlendOutDelegate, MontageTracker);

  MontageNotifyBeginDelegate.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnNotifyBeginReceived);
  OrganicMovementCmpPtr->SetMontageNotifyBeginDelegate(MontageNotifyBeginDelegate, MontageTracker);

  MontageNotifyEndDelegate.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnNotifyEndReceived);
  OrganicMovementCmpPtr->SetMontageNotifyEndDelegate(MontageNotifyEndDelegate, MontageTracker);

  MontageCompleteDelegate.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnMontageComplete);
  OrganicMovementCmpPtr->SetMontageCompleteDelegate(MontageCompleteDelegate, MontageTracker);

  // Default cosmetic delegates.

  if (FAnimMontageInstance* MontageInstance = AnimInstancePtr->GetActiveInstanceForMontage(Montage))
  {
    MontageInstanceID_Cosmetic = MontageInstance->GetInstanceID();
  }

  MontageBlendOutDelegate_Cosmetic.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnMontageBlendOut_Cosmetic);
  AnimInstancePtr->Montage_SetBlendingOutDelegate(MontageBlendOutDelegate_Cosmetic, Montage);

  MontageEndDelegate_Cosmetic.BindUObject(this, &UGMC_PlayMontageCallbackProxy::OnMontageEnd_Cosmetic);
  AnimInstancePtr->Montage_SetEndDelegate(MontageEndDelegate_Cosmetic, Montage);

  AnimInstancePtr->OnPlayMontageNotifyBegin.AddDynamic(this, &UGMC_PlayMontageCallbackProxy::OnNotifyBeginReceived_Cosmetic);
  AnimInstancePtr->OnPlayMontageNotifyEnd.AddDynamic(this, &UGMC_PlayMontageCallbackProxy::OnNotifyEndReceived_Cosmetic);
}

void UGMC_PlayMontageCallbackProxy::OnMontageStart()
{
  OnStarted.Broadcast(NAME_None);
}

void UGMC_PlayMontageCallbackProxy::OnMontageBlendIn()
{
  OnBlendIn.Broadcast(NAME_None);
}

void UGMC_PlayMontageCallbackProxy::OnMontageBlendOut()
{
  OnBlendOut.Broadcast(NAME_None);
}

void UGMC_PlayMontageCallbackProxy::OnNotifyBeginReceived(FName NotifyName)
{
  OnNotifyBegin.Broadcast(NotifyName);
}

void UGMC_PlayMontageCallbackProxy::OnNotifyEndReceived(FName NotifyName)
{
  OnNotifyEnd.Broadcast(NotifyName);
}

void UGMC_PlayMontageCallbackProxy::OnMontageComplete()
{
  OnCompleted.Broadcast(NAME_None);
}

void UGMC_PlayMontageCallbackProxy::OnMontageBlendOut_Cosmetic(UAnimMontage* Montage, bool bInterrupted)
{
  if (bInterrupted)
  {
    OnInterrupted_Cosmetic.Broadcast(NAME_None);
    bInterruptedCalledBeforeMontageBlendOut_Cosmetic = true;
  }
  else
  {
    OnBlendOut_Cosmetic.Broadcast(NAME_None);
  }
}

void UGMC_PlayMontageCallbackProxy::OnMontageEnd_Cosmetic(UAnimMontage* Montage, bool bInterrupted)
{
  if (!bInterrupted)
  {
    OnCompleted_Cosmetic.Broadcast(NAME_None);
  }
  else if (!bInterruptedCalledBeforeMontageBlendOut_Cosmetic)
  {
    OnInterrupted_Cosmetic.Broadcast(NAME_None);
  }

  UnbindDelegates_Cosmetic();
}

void UGMC_PlayMontageCallbackProxy::OnNotifyBeginReceived_Cosmetic(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload)
{
  if (IsNotifyValid_Cosmetic(NotifyName, BranchingPointNotifyPayload))
  {
    OnNotifyBegin_Cosmetic.Broadcast(NotifyName);
  }
}

void UGMC_PlayMontageCallbackProxy::OnNotifyEndReceived_Cosmetic(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload)
{
  if (IsNotifyValid_Cosmetic(NotifyName, BranchingPointNotifyPayload))
  {
    OnNotifyEnd_Cosmetic.Broadcast(NotifyName);
  }
}

bool UGMC_PlayMontageCallbackProxy::IsNotifyValid_Cosmetic(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const
{
  return MontageInstanceID_Cosmetic != INDEX_NONE && BranchingPointNotifyPayload.MontageInstanceID == MontageInstanceID_Cosmetic;
}

void UGMC_PlayMontageCallbackProxy::UnbindDelegates_Cosmetic()
{
  if (UAnimInstance* AnimInstance = AnimInstancePtr.Get())
  {
    AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UGMC_PlayMontageCallbackProxy::OnNotifyBeginReceived_Cosmetic);
    AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UGMC_PlayMontageCallbackProxy::OnNotifyEndReceived_Cosmetic);
  }
}
