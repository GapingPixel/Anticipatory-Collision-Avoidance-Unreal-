// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCPlayerController.h"
#include "GMCPawn.h"
#include "GMCReplicationComponent.h"
#include "WorldTime.h"
#include "GMCAggregator.h"
#include "GMCLog.h"
#include "GMCPlayerController_DBG.h"

namespace GMCCVars
{
#if ALLOW_CONSOLE && !NO_LOGGING

  int32 StatPing = 0;
  FAutoConsoleVariableRef CVarStatPing(
    TEXT("gmc.StatPing"),
    StatPing,
    TEXT("Display the current ping of the local machine to the server. Only applicable on clients. 0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogNetWorldTime = 0;
  FAutoConsoleVariableRef CVarLogNetWorldTime(
    TEXT("gmc.LogNetWorldTime"),
    LogNetWorldTime,
    TEXT("Log the current world time on server and client with a UTC-timestamp. 0: Disable, 1: Enable"),
    ECVF_Default
  );

#endif
}

AGMC_PlayerController::AGMC_PlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  bAllowTickBeforeBeginPlay = false;
  bReplicates = true;
}

void AGMC_PlayerController::BeginPlay()
{
  Super::BeginPlay();

  gmc_ck(PrimaryActorTick.IsTickFunctionRegistered())

  GMCAggregator = AGMC_Aggregator::GetGMCAggregator(this);

  if (IsValid(GMCAggregator))
  {
    GMCAggregator->RegisterController(this);
  }

#if !NO_LOGGING

  // Only check this on the client since it is the most relevant there and the server potentially has many player controllers (so it could spam the log).
  if (IsNetMode(NM_Client))
  {
    TArray<AActor*> Actors{};
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGMC_WorldTimeReplicator::StaticClass(), Actors);
    gmc_ck(Actors.Num() == 1)

    GMC_CLOG(
      Actors.Num() == 0,
      LogGMCReplication,
      GetPawn(),
      Error,
      TEXT("No actor of class %s found in the current world."),
      TO_STR(AGMC_WorldTimeReplicator)
    )

    GMC_CLOG(
      Actors.Num() > 1,
      LogGMCReplication,
      GetPawn(),
      Error,
      TEXT("More than one actor of class %s found in the current world."),
      TO_STR(AGMC_WorldTimeReplicator)
    )
  }

#endif
}

void AGMC_PlayerController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
  if (GetLocalRole() == ROLE_AutonomousProxy)
  {
    // Update the client world time before input actions are called from the parent tick.
    CL_UpdateWorldTime();
  }
  else if (GetLocalRole() == ROLE_Authority)
  {
    SV_AdaptiveDelayAux.SendAdaptiveDelayParams(this);
  }

  if (!bCullNonPlayerServerPawnParams)
  {
    RefNonPlayerPawn = nullptr;
  }

  gmc_ck(SV_AdaptiveDelayAux.AdaptiveDelayPackets.Num() == 0)

  Super::TickActor(DeltaTime, TickType, ThisTickFunction);

  DEBUG_STAT_PING
  DEBUG_LOG_NET_WORLD_TIME
}

void AGMC_PlayerController::PlayerTick(float DeltaTime)
{
  UGMC_ReplicationCmp* ReplicationComponent{nullptr};
  if (const auto& GMCPawn = Cast<AGMC_Pawn>(GetPawn()))
  {
    ReplicationComponent = GMCPawn->GetReplicationComponent();
  }

  if (IsValid(ReplicationComponent))
  {
    CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, PrePlayerTick, DeltaTime);
  }

  Super::PlayerTick(DeltaTime);

  if (IsValid(ReplicationComponent))
  {
    CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, PostPlayerTick, DeltaTime);
  }
}

void AGMC_PlayerController::PawnLeavingGame()
{
  gmc_ck(GetLocalRole() == ROLE_Authority)
  CALL_NATIVE_EVENT_ALWAYS(this, SV_OnPawnLeavingGame);
  SetPawn(nullptr);
}

void AGMC_PlayerController::SV_OnPawnLeavingGame_Implementation()
{
  Super::PawnLeavingGame();
}

void AGMC_PlayerController::UpdateCameraManager(float DeltaSeconds)
{
  bDeferredCameraManagerUpdate = false;

  const auto& GMCPawn = Cast<AGMC_Pawn>(GetPawn());
  if (!IsValid(GMCPawn))
  {
    Super::UpdateCameraManager(DeltaSeconds);
    return;
  }

  const auto& ReplicationComponent = Cast<UGMC_ReplicationCmp>(GMCPawn->GetMovementComponent());
  if (!IsValid(ReplicationComponent))
  {
    Super::UpdateCameraManager(DeltaSeconds);
    return;
  }

  if (!ReplicationComponent->ComponentStatus.bOnWorldTickStartExecuted)
  {
    Super::UpdateCameraManager(DeltaSeconds);
    return;
  }

  if (ReplicationComponent->ShouldDeferCameraManagerUpdate())
  {
    bDeferredCameraManagerUpdate = true;
    return;
  }

  Super::UpdateCameraManager(DeltaSeconds);
}

void AGMC_PlayerController::ManualUpdateCamera_Implementation(float DeltaTime)
{
  gmc_ck(WasCameraManagerUpdateDeferred())

  if (const auto& ControlledPawn = GetPawn())
  {
    if (const auto& SpringArm = ControlledPawn->FindComponentByClass<USpringArmComponent>())
    {
      // If the pawn has a spring arm component, it must be ticked after all other spring arm components (but before the camera manager update) to get the
      // correct camera view for the player in all cases. The easiest way to ensure this is to tick the spring arm component manually here.
      SpringArm->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All/*not used*/, nullptr/*not used*/);

      // Ensure the spring arm component won't be ticked twice each frame.
      if (SpringArm->IsComponentTickEnabled())
      {
        SpringArm->SetComponentTickEnabled(false);
      }
    }
  }

  // The UpdateCameraManager function is overridden to defer the camera manager update so make sure that the correct super implementation is called here.
  Super::UpdateCameraManager(DeltaTime);
}

bool AGMC_PlayerController::WasCameraManagerUpdateDeferred() const
{
  return bDeferredCameraManagerUpdate;
}

double AGMC_PlayerController::CL_GetSyncedWorldTimeSeconds() const
{
  gmc_ck(GetLocalRole() < ROLE_Authority)
  return CL_WorldTimeAux.SyncedWorldTime;
}

double AGMC_PlayerController::CL_GetAverageTimeDiscrepancy() const
{
  gmc_ck(GetLocalRole() < ROLE_Authority)
  return CL_WorldTimeAux.AvgSignedTimeDiscrepancy;
}

float AGMC_PlayerController::GetPingInMilliseconds() const
{
  if (!PlayerState)
  {
    return 0.f;
  }

  return PlayerState->GetPingInMilliseconds();
}

float AGMC_PlayerController::CL_GetTimeSyncAdjustment() const
{
  return CL_WorldTimeAux.GetTimeSyncAdjustment(this);
}

void AGMC_PlayerController::CL_UpdateWorldTime()
{
  CL_WorldTimeAux.UpdateWorldTime(this);
}

void AGMC_PlayerController::CL_SyncWithServerTime(double LastReplicatedServerWorldTime)
{
  CL_WorldTimeAux.SyncWithServerTime(LastReplicatedServerWorldTime, this);
}

void AGMC_PlayerController::CL_SendAdaptiveDelayParams_Implementation(const TArray<FGMC_AdaptiveDelayServerPacket>& AdaptiveDelayParams)
{
  gmc_ckc(
    if (bCullNonPlayerServerPawnParams)
    {
      bool bNonPlayerServerPawnParamsFound = false;
      for (const auto& Params : AdaptiveDelayParams)
      {
        if (IsValid(Params.TargetComponent))
        {
          if (bNonPlayerServerPawnParamsFound)
          {
            // The params for non-player server pawns should have been sent only once.
            gmc_ck(!Params.TargetComponent->IsSimulatedProxy() || Params.TargetComponent->IsPlayerControlledPawn())
          }

          if (Params.TargetComponent->IsNonPlayerControlledSimulatedProxy())
          {
            bNonPlayerServerPawnParamsFound = true;
          }
        }
      }
    }
  )

  for (FGMC_AdaptiveDelayServerPacket Params : AdaptiveDelayParams)
  {
    // The target component may not exist on the client yet (or it may have been destroyed already).
    if (IsValid(Params.TargetComponent))
    {
      Params.TargetComponent->CL_SendAdaptiveDelayParams(Params);

      if (bCullNonPlayerServerPawnParams && Params.TargetComponent->IsNonPlayerControlledSimulatedProxy())
      {
        RefNonPlayerPawn = Params.TargetComponent->GetPawnOwner();

        TArray<UMovementComponent*> MovementComponents{};

        if (IsValid(GMCAggregator))
        {
          MovementComponents = GMCAggregator->GetMovementComponents();
        }
        else
        {
          TArray<AActor*> Actors{};

          UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGMC_Pawn::StaticClass(), Actors);

          for (const auto& Actor : Actors)
          {
            if (!IsValid(Actor))
            {
              gmc_ckne()
              continue;
            }

            const auto& GMCPawn = Cast<AGMC_Pawn>(Actor);
            gmc_ck(GMCPawn)

            if (const auto& MovementComponent = GMCPawn->GetMovementComponent())
            {
              MovementComponents.Emplace(MovementComponent);
            }
          }
        }

        // Distribute the parameters to all other non-player controlled simulated proxies as well.
        for (const auto& MovementComponent : MovementComponents)
        {
          const auto& TargetComponent = Cast<UGMC_ReplicationCmp>(MovementComponent);

          if (!IsValid(TargetComponent))
          {
             continue;
          }

          if (TargetComponent == Params.TargetComponent)
          {
            // Was already processed above.
            continue;
          }

          if (TargetComponent->IsNonPlayerControlledSimulatedProxy())
          {
            (Params.TargetComponent = TargetComponent)->CL_SendAdaptiveDelayParams(Params);
          }
        }
      }
    }
  }
}

void AGMC_PlayerController::SV_EnqueueAdaptiveDelay(const UGMC_ReplicationCmp* ReplicationComponent, const FGMC_AdaptiveDelayServerPacket& DelayPacket)
{
  gmc_ckc(
    for (const auto& Packet : SV_AdaptiveDelayAux.AdaptiveDelayPackets)
    {
      // New delay parameters should only be enqueued once per tick at most from each replication component.
      gmc_ck(Packet.TargetComponent != DelayPacket.TargetComponent)
    }
  )

  const auto& CallingPawn = ReplicationComponent->GetPawnOwner();
  if (!IsValid(CallingPawn))
  {
    return;
  }

  if (bCullNonPlayerServerPawnParams)
  {
    if (CallingPawn == RefNonPlayerPawn)
    {
      // Only enqueue the adaptive delay for the non-player reference pawn.
      gmc_ck(!CallingPawn->IsPlayerControlled())
      SV_AdaptiveDelayAux.NonPlayerServerPawnDelayIndex = SV_AdaptiveDelayAux.AdaptiveDelayPackets.Emplace(DelayPacket);
      SV_AdaptiveDelayAux.LastRefNonPlayerPawnAdaptiveDelayPacket = DelayPacket;
      return;
    }
    else if (!CallingPawn->IsPlayerControlled())
    {
      // Other non-player pawns should generally not be calling this function (can happen during transient occurrences like beginning play).
      return;
    }
  }

  SV_AdaptiveDelayAux.AdaptiveDelayPackets.Emplace(DelayPacket);
}

bool AGMC_PlayerController::SV_ShouldEnqueueNonPlayerPawnAdaptiveDelay(
  UGMC_ReplicationCmp* ReplicationComponent,
  FGMC_AdaptiveDelayServerPacket& OutNonPlayerPawnParams
)
{
  gmc_ck(bCullNonPlayerServerPawnParams)
  gmc_ck(!ReplicationComponent->IsPlayerControlledPawn())

  OutNonPlayerPawnParams = FGMC_AdaptiveDelayServerPacket{};

  const auto& CallingPawn = ReplicationComponent->GetPawnOwner();
  if (!IsValid(CallingPawn))
  {
    return false;
  }

  if (!RefNonPlayerPawn.Get())
  {
    // No reference pawn was chosen yet or it may have been destroyed.

    if (SV_AdaptiveDelayAux.HasNonPlayerServerPawnParams())
    {
      // Remove the saved parameters from the old reference pawn if present.
      SV_AdaptiveDelayAux.AdaptiveDelayPackets.RemoveAt(SV_AdaptiveDelayAux.NonPlayerServerPawnDelayIndex, 1, false);
      SV_AdaptiveDelayAux.NonPlayerServerPawnDelayIndex = -1;
    }

    // Set the new reference pawn to be the owner of the calling replication component.
    RefNonPlayerPawn = CallingPawn;

    // Force an immediate update of the adaptive delay parameters when a new reference pawn was chosen.
    auto& AdaptiveDelayParams = ReplicationComponent->AdaptiveDelayParams;
    auto& ClientData = AdaptiveDelayParams.SV_PerClientParams.FindOrAdd(this);
    ClientData.UpdateTimer = AdaptiveDelayParams.SyncInterval + AdaptiveDelayParams.SYNC_INTERVAL_VARIANCE;

    return true;
  }

  // There's already a reference pawn to calculate the adaptive delay for all non-player pawns.

  if (CallingPawn == RefNonPlayerPawn)
  {
    // The calling pawn is the reference pawn.
    return true;
  }

  // The calling pawn is not the reference pawn, it should simply copy the calculated adaptive delay parameters from the reference pawn.
  const bool bHasAdaptiveDelayPacket = static_cast<bool>(SV_AdaptiveDelayAux.LastRefNonPlayerPawnAdaptiveDelayPacket.TargetComponent);
  if (bHasAdaptiveDelayPacket)
  {
    OutNonPlayerPawnParams = SV_AdaptiveDelayAux.LastRefNonPlayerPawnAdaptiveDelayPacket;
  }

  return false;
}

APawn* AGMC_PlayerController::GetRefNonPlayerPawn() const
{
  return RefNonPlayerPawn.Get();
}

void AGMC_PlayerController::SV_IncrementNumRejectedBufferTimeRequests()
{
  gmc_ck(GetLocalRole() == ROLE_Authority)
  ++SV_AdaptiveDelayAux.NumRejectedBufferTimeRequests;
}

void AGMC_PlayerController::SV_ResetNumRejectedBufferTimeRequests()
{
  gmc_ck(GetLocalRole() == ROLE_Authority)
  SV_AdaptiveDelayAux.NumRejectedBufferTimeRequests = 0;
}

bool AGMC_PlayerController::SV_CanRejectClientBufferTimeRequest() const
{
  return SV_AdaptiveDelayAux.NumRejectedBufferTimeRequests < SV_AdaptiveDelayAux.MAX_NUM_REJECTED_BUFFER_TIME_REQUESTS;
}

void AGMC_PlayerController::SV_RequestAdaptiveDelayBufferTime_Implementation(const FGMC_AdaptiveDelayClientPacket& NewBufferTime) const
{
  // The target component may have been destroyed already on the server.
  if (IsValid(NewBufferTime.TargetComponent))
  {
    NewBufferTime.TargetComponent->SV_UpdateAdaptiveDelayBufferTime(const_cast<AGMC_PlayerController*>(this), NewBufferTime.DynamicBufferTime);
  }
}

bool AGMC_PlayerController::SV_RequestAdaptiveDelayBufferTime_Validate(const FGMC_AdaptiveDelayClientPacket& NewBufferTime) const
{
  // The buffer time should have been clamped during net deserialization.
  gmc_ck(NewBufferTime.DynamicBufferTime >= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
  gmc_ck(NewBufferTime.DynamicBufferTime <= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)
  return true;
}

void AGMC_PlayerController::FClientWorldTimeAux::UpdateWorldTime(AGMC_PlayerController* Outer)
{
  gmc_ck(Outer->GetLocalRole() == ROLE_AutonomousProxy)

  const auto& World = Outer->GetWorld();
  if (!World)
  {
    return;
  }

  float DeltaTime = World->DeltaRealTimeSeconds;

  if (bDoNotUpdateWorldTime)
  {
    // Do not add to the world time this frame because we already set the updated value from the server.
    bDoNotUpdateWorldTime = false;
  }
  else if (bSlowWorldTime)
  {
    // Only add a fraction of the delta time to bring the client time closer to the server time. This will effectively speed up movement for one frame (which is
    // usually imperceptible with low adjustment values).
    const float Adjustment = Outer->CL_GetTimeSyncAdjustment();
    SyncedWorldTime += DeltaTime * (1.f - Adjustment);
    bSlowWorldTime = false;
    GMC_LOG(LogGMCController, Outer->GetPawn(), VeryVerbose, TEXT("Client world time was slowed down for this frame (adjustment = %f)."), Adjustment)
  }
  else if (bSpeedUpWorldTime)
  {
    // Add a higher delta time value to bring the client time closer to the server time. This will effectively slow down movement for one frame (which is
    // usually imperceptible with low adjustment values).
    const float Adjustment = Outer->CL_GetTimeSyncAdjustment();
    SyncedWorldTime += DeltaTime * (1.f + Adjustment);
    bSpeedUpWorldTime = false;
    GMC_LOG(LogGMCController, Outer->GetPawn(), VeryVerbose, TEXT("Client world time was sped up for this frame (adjustment = %f)."), Adjustment)
  }
  else
  {
    SyncedWorldTime += DeltaTime;
  }

  gmc_ck(!bDoNotUpdateWorldTime)
  gmc_ck(!bSlowWorldTime)
  gmc_ck(!bSpeedUpWorldTime)

  GMC_CLOG(
    SyncedWorldTime <= SyncedTimeLastFrame,
    LogGMCController,
    Outer->GetPawn(),
    Verbose,
    TEXT("Client has world time inconsistency: Timestamp current frame (%9.3f) <= Timestamp last frame (%9.3f)"),
    SyncedWorldTime,
    SyncedTimeLastFrame
  )

  SyncedTimeLastFrame = SyncedWorldTime;
}

void AGMC_PlayerController::FClientWorldTimeAux::SyncWithServerTime(double LastReplicatedServerWorldTime, AGMC_PlayerController* Outer)
{
  gmc_ck(Outer->GetLocalRole() == ROLE_AutonomousProxy)

  // Always reset these in the beginning in case this function is called more than once during one frame.
  bDoNotUpdateWorldTime = false;
  bSlowWorldTime = false;
  bSpeedUpWorldTime = false;

  const auto& World = Outer->GetWorld();
  if (!World)
  {
    GMC_LOG(LogGMCController, Outer->GetPawn(), Warning, TEXT("Client time could not not be synced with the server."))
    gmc_ckne()
    return;
  }

  // The client ping (round-trip time) in milliseconds.
  float AvgRTTInMs = 0.f;
  if (USE_PLAYER_STATE_PING)
  {
    AvgRTTInMs = Outer->GetPingInMilliseconds();
  }
  else if (const auto& Connection = Outer->GetNetConnection())
  {
    AvgRTTInMs = Connection->AvgLag * 1000.f;
  }
  GMC_CLOG(AvgRTTInMs <= 0.f, LogGMCController, Outer->GetPawn(), Verbose, TEXT("Client ping could not be retrieved."))

  const float EstimatedLatency = FMath::Min(AvgRTTInMs / 1000.f / 2.f, Outer->MaxExpectedPing / 2.f);
  const double NewWorldTime = LastReplicatedServerWorldTime + EstimatedLatency;
  const float DeltaTime = World->DeltaRealTimeSeconds;
  const double SignedTimeDiscrepancy = NewWorldTime - (SyncedWorldTime + DeltaTime/*the time will be updated afterwards*/);
  const double TimeDiscrepancy = FMath::Abs(SignedTimeDiscrepancy);
  if (TimeDiscrepancy > Outer->MaxClientTimeDifferenceHardLimit)
  {
    // If the client time deviates by more than the set limit at any point we sync the local time with the server time directly.
    GMC_LOG(
      LogGMCController,
      Outer->GetPawn(),
      VeryVerbose,
      TEXT("Synced client world time with server, time discrepancy was %f s (max allowed is %f s): ")
      TEXT("new world time = %f s | old world time = %f s | ping = %.0f ms | jitter = %.0f ms"),
      TimeDiscrepancy,
      Outer->MaxClientTimeDifferenceHardLimit,
      NewWorldTime,
      SyncedWorldTime,
      AvgRTTInMs,
      Outer->GetNetConnection() ? Outer->GetNetConnection()->GetAverageJitterInMS() : 0.f
    )
    SyncedWorldTime = NewWorldTime;

    // Do not add to the time locally this frame, the received server world time is already the updated time for this frame. The flag is processed and reset in
    // CL_UpdateWorldTime.
    bDoNotUpdateWorldTime = true;

    // Clear the buffer after a hard re-sync of the client time.
    TimeDiscrepancyBuffer.Reset();

    GMC_LOG(
      LogGMCController,
      Outer->GetPawn(),
      Verbose,
      TEXT("Corrected client time discrepancy of %f seconds."),
      TimeDiscrepancy
    )
  }
  else
  {
    TimeDiscrepancyBuffer.Add(SignedTimeDiscrepancy);
    AvgSignedTimeDiscrepancy = TimeDiscrepancyBuffer.GetMean();
    const double AvgTimeDiscrepancy = FMath::Abs(AvgSignedTimeDiscrepancy);

    GMC_LOG(
      LogGMCController,
      Outer->GetPawn(),
      VeryVerbose,
      TEXT("Queried server time, client is %f seconds %s."),
      AvgTimeDiscrepancy,
      AvgSignedTimeDiscrepancy > 0. ? TEXT("behind") : TEXT("ahead")
    )

    // We can never assure more accurate time synchronisation than the current delta time value because the packet may arrive at any point during the processing
    // of a frame.
    constexpr float DeltaTimeTolerance = 0.001f;
    const bool bIsExceedingSoftLimit = AvgTimeDiscrepancy > FMath::Max(DeltaTime + DeltaTimeTolerance, Outer->MaxClientTimeDifferenceSoftLimit);
    if (AvgSignedTimeDiscrepancy < 0. && bIsExceedingSoftLimit)
    {
      // The client is ahead of the server, set the flag to slow time down (processed and reset in CL_UpdateWorldTime).
      bSlowWorldTime = true;
    }
    else if (AvgSignedTimeDiscrepancy > 0. && bIsExceedingSoftLimit)
    {
      // The client is lagging behind the server, set the flag to speed up time (processed and reset in CL_UpdateWorldTime).
      bSpeedUpWorldTime = true;
    }
  }
}

float AGMC_PlayerController::FClientWorldTimeAux::GetTimeSyncAdjustment(const AGMC_PlayerController* Outer) const
{
  gmc_ck(Outer->GetLocalRole() == ROLE_AutonomousProxy)

  const auto& World = Outer->GetWorld();
  if (!World)
  {
    return 0.f;
  }

  // The in/out parameters can be adjusted as desired.
  int32 constexpr MinFrameRate = 30;
  int32 constexpr MaxFrameRate = 120;
  float constexpr MinAdjustment = 0.1f;
  float constexpr MaxAdjustment = 0.5f;

  // Static asserts for safety.
  static_assert(MinFrameRate > 0);
  static_assert(MaxFrameRate > 0);
  static_assert(MinAdjustment > 0.f);
  static_assert(MaxAdjustment > 0.f);
  static_assert(MinFrameRate != MaxFrameRate);
  static_assert(MinAdjustment != MaxAdjustment);

  // Calculate the adjustment for this frame.
  float constexpr MinFrameTime = 1.f / MinFrameRate;
  float constexpr MaxFrameTime = 1.f / MaxFrameRate;
  float constexpr Slope = (MaxAdjustment - MinAdjustment) / (MaxFrameTime - MinFrameTime);
  return FMath::Clamp(MinAdjustment + Slope * (World->DeltaRealTimeSeconds - MinFrameTime), MinAdjustment, MaxAdjustment);
}

void AGMC_PlayerController::FServerAdaptiveDelayAux::SendAdaptiveDelayParams(AGMC_PlayerController* Outer)
{
  // For resetting it is important that the player controller ticks before the replication component.
  NonPlayerServerPawnDelayIndex = -1;

  if (AdaptiveDelayPackets.Num() > 0)
  {
    if (UGMC_ReplicationCmp::CheckReliableBuffer(Outer, SEND_ADAPTIVE_DELAY_OVERFLOW_PROTECTION, UNUSED(int32)))
    {
      Outer->CL_SendAdaptiveDelayParams(AdaptiveDelayPackets);
    }

    AdaptiveDelayPackets.Reset();
  }
}

bool AGMC_PlayerController::FServerAdaptiveDelayAux::HasNonPlayerServerPawnParams() const
{
  return NonPlayerServerPawnDelayIndex >= 0;
}
