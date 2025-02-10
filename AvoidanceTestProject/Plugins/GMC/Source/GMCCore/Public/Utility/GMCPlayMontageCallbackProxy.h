// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCOrganicMovementComponent.h"
#include "UObject/Object.h"
#include "GMCPlayMontageCallbackProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGMC_OnMontagePlayDelegate, FName, NotifyName);

UCLASS()
class GMCCORE_API UGMC_PlayMontageCallbackProxy : public UObject
{
  GENERATED_BODY()

public:

  virtual void BeginDestroy() override;

  UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Target", BlueprintInternalUseOnly = true))
  static UGMC_PlayMontageCallbackProxy* CreateProxyObjectForPlayMontage(
    class UGMC_OrganicMovementCmp* Target,
    class UAnimMontage* MontageToPlay,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    FName StartSection = NAME_None,
    bool bInterruptOther = true,
    bool bCancelOnBlendIn = false,
    bool bCancelOnBlendOut = true
  );

  UPROPERTY(BlueprintAssignable)
  // Called when the montage started playing. Triggered from the movement component, use for predicted gameplay logic.
  FGMC_OnMontagePlayDelegate OnStarted{};

  UPROPERTY(BlueprintAssignable)
  // Called when the montage finished playing to the end and was not interrupted. Triggered from the movement component, use for predicted gameplay logic.
  FGMC_OnMontagePlayDelegate OnCompleted{};

  UPROPERTY(BlueprintAssignable)
  // Called whenever the montage passes the blend in position at the default blend in time. Triggered from the movement component, use for predicted gameplay
  // logic.
  FGMC_OnMontagePlayDelegate OnBlendIn{};

  UPROPERTY(BlueprintAssignable)
  // Called whenever the montage passes the blend out position at the default blend out time (from the end of the last non-looping montage section). Triggered
  // from the movement component, use for predicted gameplay logic.
  FGMC_OnMontagePlayDelegate OnBlendOut{};

  UPROPERTY(BlueprintAssignable)
  // Called when a montage notify has begun. Triggered from the movement component, use for predicted gameplay logic.
  FGMC_OnMontagePlayDelegate OnNotifyBegin{};

  UPROPERTY(BlueprintAssignable)
  // Called when a montage notify has ended. Triggered from the movement component, use for predicted gameplay logic.
  FGMC_OnMontagePlayDelegate OnNotifyEnd{};

  UPROPERTY(BlueprintAssignable)
  // Called when the montage finished playing to the end and was not interrupted. Triggered from the anim instance, use for local effects and animations.
  FGMC_OnMontagePlayDelegate OnCompleted_Cosmetic{};

  UPROPERTY(BlueprintAssignable)
  // Called when the montage started blending out. Triggered from the anim instance, use for local effects and animations.
  FGMC_OnMontagePlayDelegate OnBlendOut_Cosmetic{};

  UPROPERTY(BlueprintAssignable)
  // Called when the montage has been interrupted. Triggered from the anim instance, use for local effects and animations.
  FGMC_OnMontagePlayDelegate OnInterrupted_Cosmetic{};

  UPROPERTY(BlueprintAssignable)
  // Called when a montage notify has begun. Triggered from the anim instance, use for local effects and animations.
  FGMC_OnMontagePlayDelegate OnNotifyBegin_Cosmetic{};

  UPROPERTY(BlueprintAssignable)
  // Called when a montage notify has ended. Triggered from the anim instance, use for local effects and animations.
  FGMC_OnMontagePlayDelegate OnNotifyEnd_Cosmetic{};

protected:

  virtual void BindDelegates(UGMC_OrganicMovementCmp* Target, USkeletalMeshComponent* Mesh, UAnimInstance* AnimInstance, FName StartSection);

  UFUNCTION()
  void OnMontageStart();

  UFUNCTION()
  void OnMontageBlendIn();

  UFUNCTION()
  void OnMontageBlendOut();

  UFUNCTION()
  void OnNotifyBeginReceived(FName NotifyName);

  UFUNCTION()
  void OnNotifyEndReceived(FName NotifyName);

  UFUNCTION()
  void OnMontageComplete();

  UFUNCTION()
  void OnMontageBlendOut_Cosmetic(UAnimMontage* Montage, bool bInterrupted);

  UFUNCTION()
  void OnMontageEnd_Cosmetic(UAnimMontage* Montage, bool bInterrupted);

  UFUNCTION()
  void OnNotifyBeginReceived_Cosmetic(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);

  UFUNCTION()
  void OnNotifyEndReceived_Cosmetic(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);

private:

  TWeakObjectPtr<UGMC_OrganicMovementCmp> OrganicMovementCmpPtr{nullptr};
  TWeakObjectPtr<UAnimInstance> AnimInstancePtr{nullptr};

  FGMC_OnMontageStart MontageStartDelegate{};
  FGMC_OnMontageBlendInComplete MontageBlendInDelegate{};
  FGMC_OnMontageBlendOutBegin MontageBlendOutDelegate{};
  FGMC_OnMontageNotifyBegin MontageNotifyBeginDelegate{};
  FGMC_OnMontageNotifyEnd MontageNotifyEndDelegate{};
  FGMC_OnMontageComplete MontageCompleteDelegate{};

  int32 MontageInstanceID_Cosmetic{INDEX_NONE};
  bool bInterruptedCalledBeforeMontageBlendOut_Cosmetic{false};
  FOnMontageBlendingOutStarted MontageBlendOutDelegate_Cosmetic{};
  FOnMontageEnded MontageEndDelegate_Cosmetic{};

  bool IsNotifyValid_Cosmetic(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const;
  void UnbindDelegates_Cosmetic();
};
