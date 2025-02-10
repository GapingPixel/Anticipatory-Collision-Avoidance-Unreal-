// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CircularContainer.h"
#include "Smoothing.h"
#include "GameFramework/PlayerController.h"
#include "GMCPlayerController.generated.h"

/// Player controller class intended to be used with UGMC_MovementReplicationComponent.
UCLASS(BlueprintType, Blueprintable)
class GMCCORE_API AGMC_PlayerController : public APlayerController
{
  GENERATED_BODY()

public:

  AGMC_PlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void BeginPlay() override;
  void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
  void PlayerTick(float DeltaTime) override;
  void UpdateCameraManager(float DeltaSeconds) override;
  void PawnLeavingGame() override;

  /// Called on the server when a pawn's controlling player is leaving the game. The base implementation destroys the pawn.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void SV_OnPawnLeavingGame();
  virtual void SV_OnPawnLeavingGame_Implementation();

  /// Returns exact ping from the player state if available (for local players or when running on the server) or the replicated compressed ping (converted back
  /// to milliseconds) otherwise. Make sure APlayerState::bShouldUpdateReplicatedPing is set to true.
  ///
  /// @returns      float    The current ping in milliseconds.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float GetPingInMilliseconds() const;

  /// Queries and synchronises the client world time with the server world time if necessary (accounts for network delay).
  ///
  /// @param        LastReplicatedServerWorldTime    The latest world time value that was received from the server (not dilated or paused).
  /// @returns      void
  void CL_SyncWithServerTime(double LastReplicatedServerWorldTime);

  /// Returns the synchronised server world time for a client. Considers network delay meaning the returned value will be nearly the same as the world time on
  /// the server at any given moment.
  ///
  /// @returns      double    The current synchronised server world time.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double CL_GetSyncedWorldTimeSeconds() const;

  /// Returns the average time discrepancy of the client that was calculated during the last time sync. If the value is greater/smaller than 0 the client is
  /// estimated to be behind/ahead.
  ///
  /// @returns      double    The current estimated time discrepancy of the client.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double CL_GetAverageTimeDiscrepancy() const;

  /// The camera manager update is usually deferred to ensure the correct camera view for the player in all situations.
  ///
  /// @returns      bool    True if the camera manager update was deferred, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool WasCameraManagerUpdateDeferred() const;

  /// Called to perform the camera manager update if it was deferred previously.
  ///
  /// @param        DeltaTime    The current frame delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void ManualUpdateCamera(float DeltaTime);
  virtual void ManualUpdateCamera_Implementation(float DeltaTime);

  /// Enqueues the passed delay packet for transmission to the client on the next tick.
  ///
  /// @param        ReplicationComponent    The calling replication component.
  /// @param        DelayPacket             The packet to enqueue.
  /// @returns      void
  void SV_EnqueueAdaptiveDelay(const class UGMC_ReplicationCmp* ReplicationComponent, const FGMC_AdaptiveDelayServerPacket& DelayPacket);

  /// Whether an adaptive delay packet for the calling non-player server pawn should be calculated and enqueued. Only relevant when
  /// bCullNonPlayerServerPawnParams is enabled.
  ///
  /// @param        ReplicationComponent      The calling replication component.
  /// @param        OutNonPlayerPawnParams    The parameters of the last enqueued adaptive delay packet of the reference non-player pawn (if present). Only
  ///                                         relevant when "false" is returned.
  /// @returns      bool                      True if an adaptive delay packet for the caller should be calculated and enqueued, false otherwise.
  bool SV_ShouldEnqueueNonPlayerPawnAdaptiveDelay(UGMC_ReplicationCmp* ReplicationComponent, FGMC_AdaptiveDelayServerPacket& OutNonPlayerPawnParams);

  /// Returns the non-player pawn that is currently being used to calculate the adaptive delay parameters. Only relevant when bCullNonPlayerServerPawnParams is
  /// enabled.
  ///
  /// @returns      APawn*    The pawn that is being used to calculate the adaptive delay parameters for all non-player pawns.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  APawn* GetRefNonPlayerPawn() const;

  /// Increments the number of rejected buffer time requests for this controller by one.
  ///
  /// @returns      void
  void SV_IncrementNumRejectedBufferTimeRequests();

  /// Resets the number of rejected buffer time requests for this controller to zero.
  ///
  /// @returns      void
  void SV_ResetNumRejectedBufferTimeRequests();

  /// Whether the server is allowed to reject a buffer time request by the client if necessary.
  ///
  /// @returns      bool    True if the server is allowed to reject the buffer time request, false otherwise.
  bool SV_CanRejectClientBufferTimeRequest() const;

  /// Informs the client about new parameters when using an adaptive simulation delay for interpolation.
  ///
  /// @param        AdaptiveDelayParams    All new adaptive delay parameters that should be applied.
  /// @returns      void
  UFUNCTION(Client, Reliable)
  void CL_SendAdaptiveDelayParams(const TArray<FGMC_AdaptiveDelayServerPacket>& AdaptiveDelayParams);
  void CL_SendAdaptiveDelayParams_Implementation(const TArray<FGMC_AdaptiveDelayServerPacket>& AdaptiveDelayParams);

  /// Requests a new buffer time for the client's adaptive delay.
  ///
  /// @param        NewBufferTime    The new buffer time that the client is requesting.
  /// @returns      void
  UFUNCTION(Server, Unreliable, WithValidation)
  void SV_RequestAdaptiveDelayBufferTime(const FGMC_AdaptiveDelayClientPacket& NewBufferTime) const;
  void SV_RequestAdaptiveDelayBufferTime_Implementation(const FGMC_AdaptiveDelayClientPacket& NewBufferTime) const;
  bool SV_RequestAdaptiveDelayBufferTime_Validate(const FGMC_AdaptiveDelayClientPacket& NewBufferTime) const;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Time Sync", meta =
    (ClampMin = "0", ClampMax = "1", UIMin = "0.05", UIMax = "1"))
  /// The max ping that a client is expected to have (in seconds). This is not enforced but if a client has a higher ping than this the local world time will
  /// desync which can create all sorts of problems. Do not set this arbitrarily high or low, it is recommended to leave this at the default value.
  float MaxExpectedPing{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Time Sync", meta =
    (ClampMin = "0", UIMin = "0.1", UIMax = "0.5"))
  /// The maximum acceptable difference in seconds between the local client world time and the authoritative server world time. Syncing (i.e. overwriting) the
  /// local time with the server time can create inconsistencies for consecutive timestamps which can cause stutter for the client.
  float MaxClientTimeDifferenceHardLimit{0.5f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Time Sync", meta =
    (ClampMin = "0", UIMin = "0.01", UIMax = "0.1"))
  /// If the difference in seconds between the local client world time and the authoritative server world time exceeds this threshold the client time will be
  /// adjusted towards the server time by increasing or shortening the local delta time value for the next update. This will prevent inconsistencies in the
  /// client time while still reducing the time deviation. Regardless of the configured value, the lower bound for the soft limit is always the current frame
  /// delta time.
  float MaxClientTimeDifferenceSoftLimit{0.02f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Delay")
  /// If true, one adaptive delay value will be used collectively for all non-player controlled pawns (experimental).
  bool bCullNonPlayerServerPawnParams{false};

protected:

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// Cached reference to the GMC aggregator (if present).
  TObjectPtr<class AGMC_Aggregator> GMCAggregator{nullptr};

  /// Calculates the time sync adjustment value to use.
  ///
  /// @returns      float    The time sync adjustment value to use.
  virtual float CL_GetTimeSyncAdjustment() const;

  /// Updates the client world time every frame.
  ///
  /// @returns      void
  virtual void CL_UpdateWorldTime();

private:

  bool bDeferredCameraManagerUpdate{false};

  struct FClientWorldTimeAux
  {
    // How many past values should be saved in the discrepancy buffer for the client world time.
    static constexpr int32 TIME_DISCREPANCY_BUFFER_SIZE = 16;

    // Whether we want to retrieve the ping from the player state or from the net connection. If enabled, make sure APlayerState::bShouldUpdateReplicatedPing is
    // set to true.
    static constexpr bool USE_PLAYER_STATE_PING = true;

    // The time in seconds since the world was brought up (synchronised with the server time).
    double SyncedWorldTime{0.};

    // What the time was during the last frame so we can identify inconsistent timestamps (can happen after the time was synced).
    double SyncedTimeLastFrame{0.};

    // If set only a fraction of the delta time will be added to the client world time during the next client time update. This is done to preemptively bring
    // the client time closer to the server time.
    bool bSlowWorldTime{false};

    // If set a larger delta time value will be added to the client world time during the next client time update. This is done to preemptively bring the client
    // time closer to the server time.
    bool bSpeedUpWorldTime{false};

    // Flag to signal to the controller that the delta time should not be added to the synchronised world time this frame. Set when the server world time was
    // received in which case AGMC_WorldTimeRep::CL_OnRepRealWorldTimeSecondsReplicated is called. OnRep functions are processed before actors are ticked, and
    // the received server world time is already the updated time for this frame.
    bool bDoNotUpdateWorldTime{false};

    // The average time discrepancy of the client calculated during the last time sync. If greater/smaller than 0 the client is estimated to be behind/ahead.
    float AvgSignedTimeDiscrepancy{0.};

    // Saves past discrepancies of the client time from the server time to generate an average deviation.
    TGMC_CircularNumericBuffer<double, TIME_DISCREPANCY_BUFFER_SIZE> TimeDiscrepancyBuffer{};

    void UpdateWorldTime(AGMC_PlayerController* Outer);

    void SyncWithServerTime(double LastReplicatedServerWorldTime, AGMC_PlayerController* Outer);

    float GetTimeSyncAdjustment(const AGMC_PlayerController* Outer) const;
  };

  FClientWorldTimeAux CL_WorldTimeAux{};

  struct FServerAdaptiveDelayAux
  {
    // How many reliable RPCs should remain for the actor at the least when attempting to send adaptive delay parameters to the client.
    static constexpr int32 SEND_ADAPTIVE_DELAY_OVERFLOW_PROTECTION = 50;

    // How many consecutive buffer time requests by the client should be rejected at most.
    static constexpr int32 MAX_NUM_REJECTED_BUFFER_TIME_REQUESTS = 10;

    // How many times a new buffer time request by the client was rejected since the last accepted buffer time request.
    int32 NumRejectedBufferTimeRequests{0};

    // The adaptive delay parameters that are sent to the client at regular intervals.
    TArray<FGMC_AdaptiveDelayServerPacket> AdaptiveDelayPackets{};

    // The current index at which the adaptive delay packet for non-player controlled server pawns is located in the array (only relevant when
    // bCullNonPlayerServerPawnParams is enabled).
    int32 NonPlayerServerPawnDelayIndex{-1};

    // Some non-player pawns may tick before the reference pawn, so we need to save the last enqueued adaptive delay packet separately.
    FGMC_AdaptiveDelayServerPacket LastRefNonPlayerPawnAdaptiveDelayPacket{};

    void SendAdaptiveDelayParams(AGMC_PlayerController* Outer);

    bool HasNonPlayerServerPawnParams() const;
  };

  FServerAdaptiveDelayAux SV_AdaptiveDelayAux{};

  TWeakObjectPtr<APawn> RefNonPlayerPawn{nullptr};
};
