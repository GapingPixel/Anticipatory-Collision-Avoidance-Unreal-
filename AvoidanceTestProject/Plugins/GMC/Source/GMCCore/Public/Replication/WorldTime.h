// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "WorldTime.generated.h"

/// Used in connection with AGMC_PlayerController to ensure accurate synchronisation of the server world time on the client. There should only ever be exactly
/// one actor of this type per world.
UCLASS(BlueprintType, Blueprintable)
class GMCCORE_API AGMC_WorldTimeReplicator : public AInfo
{
  GENERATED_BODY()

public:

  AGMC_WorldTimeReplicator();
  void BeginPlay() override;
  void EndPlay(EEndPlayReason::Type EndPlayReason) override;
  void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Time Sync", meta = (ClampMin = "0", UIMin = "0.02", UIMax = "0.2"))
  /// The interval in seconds at which the server world time should be replicated to the client. If changed at runtime, the update timer must be reset.
  float WorldTimeUpdateInterval{0.01f};

  /// Returns the value of the last replicated world time value received from the server.
  ///
  /// @returns      double    The replicated server world time in seconds (not dilated or paused).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetRealWorldTimeSecondsReplicated() const;

  /// Starts the timer for periodically updating the world time on the server.
  ///
  /// @param        UpdateInterval    The interval at which the timer should run.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_StartUpdateTimer(float UpdateInterval);

  /// Clears the timer for periodically updating the world time on the server.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_StopUpdateTimer();

  /// Restarts the timer for periodically updating the world time on the server.
  ///
  /// @param        UpdateInterval    The interval at which the timer should run.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_ResetUpdateTimer(float UpdateInterval);

protected:

  /// Called when the client receives an updated value for RealWorldTimeSecondsReplicated which triggers the time synchronisation process.
  ///
  /// @returns      void
  UFUNCTION()
  virtual void CL_OnRepRealWorldTimeSecondsReplicated();

  /// Timer function which updates the replicated world time periodically on the server according to the set interval.
  ///
  /// @returns      void
  virtual void SV_UpdateRealWorldTimeSecondsReplicated();

private:

  FTimerHandle SV_TimerHandleUpdateTime{};

  UPROPERTY(Transient, ReplicatedUsing = CL_OnRepRealWorldTimeSecondsReplicated)
  double RealWorldTimeSecondsReplicated{0.};
};
