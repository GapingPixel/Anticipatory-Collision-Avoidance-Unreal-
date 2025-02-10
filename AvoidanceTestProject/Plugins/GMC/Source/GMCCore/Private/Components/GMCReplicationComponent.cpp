// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCReplicationComponent.h"
#include "GMCPlayerController.h"
#include "GMCPawn.h"
#include "GMCFlatCapsuleComponent.h"
#include "GMCRollbackActor.h"
#include "Compression.h"
#include "GMCAggregator.h"
#include "GMCLog.h"
#include "GMCReplicationComponent_DBG.h"

DECLARE_CYCLE_STAT(TEXT("TickComponent"), STAT_TickComponent, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("OnWorldTickStart"), STAT_OnWorldTickStart, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("OnWorldTickEnd"), STAT_OnWorldTickEnd, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateLocallyControlledServerPawn"), STAT_UpdateLocallyControlledServerPawn, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateAutonomousProxyPawn"), STAT_UpdateAutonomousProxyPawn, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateRemotelyControlledServerPawn"), STAT_UpdateRemotelyControlledServerPawn, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SimulatePawn"), STAT_SimulatePawn, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ManagePrerequisiteTicks"), STAT_ManagePrerequisiteTicks, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ClearTransientData"), STAT_ClearTransientData, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ManageAdaptiveSimulationDelay"), STAT_ManageAdaptiveSimulationDelay, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SV_VerifyTimestamps"), STAT_SV_VerifyTimestamps, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SV_ProcessProxyMove"), STAT_SV_ProcessProxyMove, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SV_AuditClientMoves"), STAT_SV_AuditClientMoves, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SV_ExecuteClientMoves"), STAT_SV_ExecuteClientMoves, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_OnRepAPMove"), STAT_CL_OnRepAPMove, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_OnRepSPMove"), STAT_CL_OnRepSPMove, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_MaintainPredictionHistory"), STAT_CL_MaintainPredictionHistory, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_MaintainNonPredictedMoves"), STAT_CL_MaintainNonPredictedMoves, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_ShouldEnqueueMove"), STAT_CL_ShouldEnqueueMove, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_AddToPredictionHistory"), STAT_CL_AddToPredictionHistory, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_ExecuteMove"), STAT_CL_ExecuteMove, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_ClearAcknowledgedMoves"), STAT_CL_ClearAcknowledgedMoves, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_ShouldReplay"), STAT_CL_ShouldReplay, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("CL_ReplayMoves"), STAT_CL_ReplayMoves, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("GetTime"), STAT_GetTime, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("AddToSimulationHistory"), STAT_AddToSimulationHistory, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ExecuteMove"), STAT_ExecuteMove, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SetSimulatedPawnState"), STAT_SetSimulatedPawnState, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SmoothMovement"), STAT_SmoothMovement, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ComputeSmoothingParams"), STAT_ComputeSmoothingParams, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("DetermineSkippedStates"), STAT_DetermineSkippedStates, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("GatherRollbackPawns"), STAT_GatherRollbackPawns, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ShouldRollBackGMCPawn"), STAT_ShouldRollBackGMCPawn, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ShouldRollBackGMCActor"), STAT_ShouldRollBackGMCActor, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("SaveLocalStateBeforeRollback"), STAT_SaveLocalStateBeforeRollback, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("RollBackPawns"), STAT_RollBackPawns, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("ComputeRollbackParams"), STAT_ComputeRollbackParams, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("RestoreRolledBackPawns"), STAT_RestoreRolledBackPawns, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("RestoreOriginalStateAfterRollback"), STAT_RestoreOriginalStateAfterRollback, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("TickGenericRollbackActors"), STAT_TickGenericRollbackActors, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("GatherGenericRollbackActors"), STAT_GatherGenericRollbackActors, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("RollbackGenericActors"), STAT_RollbackGenericActors, STATGROUP_UGMC_ReplicationCmp)
DECLARE_CYCLE_STAT(TEXT("RestoreRolledBackGenericActors"), STAT_RestoreRolledBackGenericActors, STATGROUP_UGMC_ReplicationCmp)

namespace GMCCVars
{
#if ALLOW_CONSOLE && !NO_LOGGING

  int32 StatNetMovementValues = 0;
  FAutoConsoleVariableRef CVarStatNetMovementValues(
    TEXT("gmc.StatNetMovementValues"),
    StatNetMovementValues,
    TEXT("Display realtime values of movement variables for pawns with the specified role on the screen. ")
    TEXT("0: Disable, 1: ROLE_Authority, 2: ROLE_AutonomousProxy, 3: ROLE_SimulatedProxy"),
    ECVF_Default
  );

  int32 StatNetContextValues = 0;
  FAutoConsoleVariableRef CVarStatNetContextValues(
    TEXT("gmc.StatNetContextValues"),
    StatNetContextValues,
    TEXT("Display realtime values of network related context variables for pawns with the specified role on the screen. ")
    TEXT("0: Disable, 1: ROLE_Authority, 2: ROLE_AutonomousProxy, 3: ROLE_SimulatedProxy"),
    ECVF_Default
  );

  int32 ShowNetRole = 0;
  FAutoConsoleVariableRef CVarShowNetRole(
    TEXT("gmc.ShowNetRole"),
    ShowNetRole,
    TEXT("Display the net roles of pawns as floating text above them. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 ShowSimulationDelay = 0;
  FAutoConsoleVariableRef CVarShowSimulationDelay(
    TEXT("gmc.ShowSimulationDelay"),
    ShowSimulationDelay,
    TEXT("Display each pawn's current simulation delay value as floating text above them. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 ShowClientCorrections = 0;
  FAutoConsoleVariableRef CVarShowClientCorrections(
    TEXT("gmc.ShowClientCorrections"),
    ShowClientCorrections,
    TEXT("Visualize corrections of the autonomous proxy state on the client. The original state is displayed in red, the corrected state in green. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 ShowClientErrors = 0;
  FAutoConsoleVariableRef CVarShowClientErrors(
    TEXT("gmc.ShowClientErrors"),
    ShowClientErrors,
    TEXT("Visualize errors in the state of remotely controlled pawns on the server. The client state is displayed in red, the server state in green. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogClientReplay = 0;
  FAutoConsoleVariableRef CVarLogClientReplay(
    TEXT("gmc.LogClientReplay"),
    LogClientReplay,
    TEXT("Log more detailed information when a client replay happens. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogSmoothing = 0;
  FAutoConsoleVariableRef CVarLogSmoothing(
    TEXT("gmc.LogSmoothing"),
    LogSmoothing,
    TEXT("Log all data relating to the smoothing of simulated pawns. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogExtrapolation = 0;
  FAutoConsoleVariableRef CVarLogExtrapolation(
    TEXT("gmc.LogExtrapolation"),
    LogExtrapolation,
    TEXT("Log when extrapolation is being used for displaying smoothed simulated pawns. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogClientMoveTrace = 0;
  FAutoConsoleVariableRef CVarLogClientMoveTrace(
    TEXT("gmc.LogClientMoveTrace"),
    LogClientMoveTrace,
    TEXT("Trace the execution and replication of a move from the client to the server and back to the client. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogNumExecutedRemoteMoves = 0;
  FAutoConsoleVariableRef CVarLogNumExecutedRemoteMoves(
    TEXT("gmc.LogNumExecutedRemoteMoves"),
    LogNumExecutedRemoteMoves,
    TEXT("Log how many client moves are executed on the server each frame. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogDynamicBufferTime = 0;
  FAutoConsoleVariableRef CVarLogDynamicBufferTime(
    TEXT("gmc.LogDynamicBufferTime"),
    LogDynamicBufferTime,
    TEXT("Monitor the dynamic buffer time (if enabled) when using an adaptive simulation delay. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogForcedNetUpdates = 0;
  FAutoConsoleVariableRef CVarLogForcedNetUpdates(
    TEXT("gmc.LogForcedNetUpdates"),
    LogForcedNetUpdates,
    TEXT("Log when a forced net update from the server to the clients is triggered. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 ForceFullSyncDataValidation = 0;
  FAutoConsoleVariableRef CVarForceFullSyncDataValidation(
    TEXT("gmc.ForceFullSyncDataValidation"),
    ForceFullSyncDataValidation,
    TEXT("Validate all bound data every time (instead of stopping when the first deviating variable is encountered). ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Default
  );

#endif
}

UGMC_ReplicationCmp::UGMC_ReplicationCmp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  SetIsReplicatedByDefault(false);
}

void UGMC_ReplicationCmp::InitializeComponent()
{
  Super::InitializeComponent();
}

void UGMC_ReplicationCmp::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime)
{
  SCOPE_CYCLE_COUNTER(STAT_OnWorldTickStart)

  gmc_ck(!ComponentStatus.bOnWorldTickStartExecuted)

  if (!World || !World->IsGameWorld() || World != GetWorld() || !IsReadyForPlay())
  {
    return;
  }

  // Only continue execution if actors will be ticked this frame.
  const auto& NetDriver = World->GetNetDriver();
  ComponentStatus.bOnWorldTickStartExecuted =
    PawnOwner &&
    TickType != LEVELTICK_TimeOnly &&
    !World->IsPaused() &&
    (!NetDriver || !NetDriver->ServerConnection || NetDriver->ServerConnection->GetConnectionState() == USOCK_Open);

  if (!ComponentStatus.bOnWorldTickStartExecuted)
  {
    return;
  }

  if (CL_ShouldUseSmoothCorrections() && CL_SmoothCorrection.HasBegunCorrection())
  {
    CL_HandleSmoothCorrectionOnWorldTickStart();
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, WorldTickStart, DeltaTime);
}

void UGMC_ReplicationCmp::OnWorldTickEnd(UWorld* World, ELevelTick TickType, float DeltaTime)
{
  SCOPE_CYCLE_COUNTER(STAT_OnWorldTickEnd)

  if (!World || !World->IsGameWorld() || World != GetWorld())
  {
    return;
  }

  // Only continue if OnWorldTickStart was also executed fully.
  if (!ComponentStatus.bOnWorldTickStartExecuted)
  {
    return;
  }

  ComponentStatus.bOnWorldTickStartExecuted = false;

  if (CL_ShouldUseSmoothCorrections() && CL_SmoothCorrection.HasData())
  {
    CL_HandleSmoothCorrectionOnWorldTickEnd(DeltaTime, TickType);
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, WorldTickEnd, DeltaTime);

  // PawnOwner might be null at this point.
  const auto& OwningPawn = Cast<APawn>(GetOwner());
  if (!IsValid(OwningPawn))
  {
    return;
  }

  // Call after WorldTickEnd event in case the pawn state was changed in it.
  if (const auto& Controller = Cast<AGMC_PlayerController>(OwningPawn->GetController()))
  {
    if (Controller->WasCameraManagerUpdateDeferred())
    {
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, Controller, ManualUpdateCamera, DeltaTime);
    }
  }
}

void UGMC_ReplicationCmp::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
  // The parent implementation may enable the tick function automatically again.
  const auto DisableTickIfAggregating = [this]()
  {
    if (IsValid(GMCAggregator) && GMCAggregator->bEnableAggregateTick && GMCAggregator->bAggregateMovementComponents)
    {
      PrimaryComponentTick.SetTickFunctionEnable(false);
    }
  };

  if (!IsValid(NewUpdatedComponent))
  {
    Super::SetUpdatedComponent(NewUpdatedComponent);
    DisableTickIfAggregating();
    return;
  }

  const auto& NewGMCPawnOwner = Cast<AGMC_Pawn>(NewUpdatedComponent->GetOwner());
  if (!NewGMCPawnOwner)
  {
    GMC_CLOG(
      GetWorld() && GetWorld()->IsGameWorld(),
      LogGMCReplication,
      PawnOwner,
      Error,
      TEXT("New updated component must be owned by a pawn of type \"%s\"."),
      TO_STR(AGMC_Pawn)
    )
    Super::SetUpdatedComponent(NewUpdatedComponent);
    DisableTickIfAggregating();
    return;
  }

  Super::SetUpdatedComponent(NewUpdatedComponent);
  DisableTickIfAggregating();
  gmc_ck(IsValid(UpdatedComponent))
  gmc_ck(IsValid(UpdatedPrimitive))
  gmc_ck(IsValid(PawnOwner))
}

#if WITH_EDITORONLY_DATA && WITH_EDITOR

void UGMC_ReplicationCmp::HideProperty(UClass* Class, const FName& PropertyName)
{
  if (FProperty* Property = Class->FindPropertyByName(PropertyName))
  {
    const FString& EditCondition = Property->GetMetaData("EditCondition");
    const FString NewEditCondition = EditCondition.IsEmpty() ? TEXT("!bHideProperties") : EditCondition + TEXT(" && !bHideProperties");
    Property->SetMetaData("EditCondition", *NewEditCondition);
    Property->SetMetaData("EditConditionHides", "true");
  }
}

#endif

void UGMC_ReplicationCmp::BeginPlay()
{
  gmc_ck(PrimaryComponentTick.IsTickFunctionRegistered())

  GMCAggregator = AGMC_Aggregator::GetGMCAggregator(this);

  if (IsValid(GMCAggregator))
  {
    GMCAggregator->RegisterMovementComponent(this);
  }

  if (!IsValid(PawnOwner))
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Error, TEXT("Owning pawn is not valid."))
    Super::BeginPlay();
    return;
  }

  if (!Cast<AGMC_Pawn>(PawnOwner))
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Error, TEXT("Owning pawn has to be of type \"%s\"."), TO_STR(AGMC_Pawn))
    Super::BeginPlay();
    return;
  }

  if (!IsValid(PawnOwner->GetRootComponent()))
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Error, TEXT("No valid root component."))
    Super::BeginPlay();
    return;
  }

  if (!IsValid(UpdatedComponent))
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Error, TEXT("No valid updated component."))
    Super::BeginPlay();
    return;
  }

  FWorldDelegates::OnWorldTickStart.AddUObject(this, &UGMC_ReplicationCmp::OnWorldTickStart);
  FWorldDelegates::OnWorldTickEnd.AddUObject(this, &UGMC_ReplicationCmp::OnWorldTickEnd);

  gmc_ck(GetElapsedTimeSinceSpawn() == 0.)

  if (!IsNetMode(NM_Standalone))
  {
    BeginPlayNetworked();
  }

  // Call the Blueprint event after the replication component has been initialized.
  Super::BeginPlay();
}

void UGMC_ReplicationCmp::BeginPlayNetworked()
{
  gmc_ck(!IsNetMode(NM_Standalone))
  gmc_ck(IsValid(PawnOwner))
  gmc_ck(PawnOwner->GetRootComponent())

  // Save the default settings for toggling server-side physics simulation.
  SavedSettingsServerAuthPhysics.SaveSettings(this);

  // Save the default settings for toggling client-side physics simulation.
  SavedSettingsClientAuthPhysics.SaveSettings(this);

  // Avoid overhead from resizing when adding or removing elements.
  MoveHistory.Reset(MoveHistoryMaxSize);
  CL_MoveExecutionAux.NonPredictedMoves.Reset(NUM_SAVED_NO_PREDICTION_MOVES);
  static_assert(NUM_SAVED_NO_PREDICTION_MOVES >= 3);

  GMC_CLOG(
    !PawnOwner->GetIsReplicated(),
    LogGMCReplication,
    PawnOwner,
    Warning,
    TEXT("The owning pawn does not replicate. Enable replication on the actor to use GMC networking features.")
  )

  GMC_CLOG(
    PawnOwner->IsReplicatingMovement(),
    LogGMCReplication,
    PawnOwner,
    Warning,
    TEXT("Built-in actor movement replication is being used. Disable to use GMC networking features.")
  )

  GMC_CLOG(
    PawnOwner->GetRootComponent()->GetIsReplicated(),
    LogGMCReplication,
    PawnOwner,
    Log,
    TEXT("The pawn's root component is set to replicate - some replicated properties may interfere with GMC functionality.")
  )

  if (ClientSendRate > 0)
  {
    GMC_CLOG(
      1.f / ClientSendRate > MaxMoveDeltaTime,
      LogGMCReplication,
      PawnOwner,
      Warning,
      TEXT("1 / \"%s\" (%f) > \"%s\" (%f), increasing \"%s\" to %f s."),
      TO_STR(ClientSendRate),
      1.f / ClientSendRate,
      TO_STR(MaxMoveDeltaTime),
      MaxMoveDeltaTime,
      TO_STR(MaxMoveDeltaTime),
      1.f / ClientSendRate
    )
    MaxMoveDeltaTime = FMath::Max(MaxMoveDeltaTime, 1.f / ClientSendRate);
  }

  GMC_CLOG(
    MaxCombinedDeltaTime > MaxMoveDeltaTime,
    LogGMCReplication,
    PawnOwner,
    Warning,
    TEXT("\"%s\" (%f) > \"%s\" (%f), reducing \"%s\" to match \"%s\"."),
    TO_STR(MaxCombinedDeltaTime),
    MaxCombinedDeltaTime,
    TO_STR(MaxMoveDeltaTime),
    MaxMoveDeltaTime,
    TO_STR(MaxCombinedDeltaTime),
    TO_STR(MaxMoveDeltaTime)
  )
  MaxCombinedDeltaTime = FMath::Min(MaxCombinedDeltaTime, MaxMoveDeltaTime);

  if (ServerMinUpdateRate > 0)
  {
    GMC_CLOG(
      ServerMinUpdateRate > ClientSendRate,
      LogGMCReplication,
      PawnOwner,
      Warning,
      TEXT("\"%s\" (%d) > \"%s\" (%d), lowering \"%s\" to match \"%s\"."),
      TO_STR(ServerMinUpdateRate),
      ServerMinUpdateRate,
      TO_STR(ClientSendRate),
      ClientSendRate,
      TO_STR(ServerMinUpdateRate),
      TO_STR(ClientSendRate)
    )
    ServerMinUpdateRate = FMath::Min(ServerMinUpdateRate, ClientSendRate);
  }

  if (IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer))
  {
    gmc_ck(IsServerPawn())
    // We set the timers for all server pawns because 1) any pawn may be possessed by a client at some point 2) the pawn may not have a controller assigned yet
    // when BeginPlay runs (so we can't check the net role at this point).
    SV_NetReserializationAux.SetTimer(this);
    SV_TimestampVerificationAux.SetTimer(this);
  }
}

void UGMC_ReplicationCmp::EndPlay(EEndPlayReason::Type EndPlayReason)
{
  SV_NetReserializationAux.ClearTimer(this);
  SV_TimestampVerificationAux.ClearTimer(this);

  // Call the Blueprint event after the replication component has been deinitialized.
  Super::EndPlay(EndPlayReason);
}

bool UGMC_ReplicationCmp::IsReadyForPlay() const
{
  if (const auto& GMCPawnOwner = GetGMCPawnOwner())
  {
    return GMCPawnOwner->HasActorBegunPlay() && HasBegunPlay();
  }

  return false;
}

FGMC_Move& UGMC_ReplicationCmp::LocalMove()
{
  gmc_ck(Cast<AGMC_Pawn>(PawnOwner))
  return GetGMCPawnOwner()->LocalMove;
}

const FGMC_Move& UGMC_ReplicationCmp::LocalMove() const
{
  gmc_ck(Cast<AGMC_Pawn>(PawnOwner))
  return GetGMCPawnOwner()->LocalMove;
}

FGMC_Move& UGMC_ReplicationCmp::APMove()
{
  gmc_ck(Cast<AGMC_Pawn>(PawnOwner))
  return GetGMCPawnOwner()->APMove;
}

const FGMC_Move& UGMC_ReplicationCmp::APMove() const
{
  gmc_ck(Cast<AGMC_Pawn>(PawnOwner))
  return GetGMCPawnOwner()->APMove;
}

FGMC_Move& UGMC_ReplicationCmp::SPMove()
{
  gmc_ck(Cast<AGMC_Pawn>(PawnOwner))
  return GetGMCPawnOwner()->SPMove;
}

const FGMC_Move& UGMC_ReplicationCmp::SPMove() const
{
  gmc_ck(Cast<AGMC_Pawn>(PawnOwner))
  return GetGMCPawnOwner()->SPMove;
}

void UGMC_ReplicationCmp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
  SCOPE_CYCLE_COUNTER(STAT_TickComponent)

  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  PreTick();

  if (!ShouldRunGMCUpdates())
  {
    ComponentStatus.SaveFrameInfo(false, this);
    return;
  }

  if (ComponentStatus.ShouldClearTransientData(this))
  {
    ClearTransientData(true);
  }

  ComponentStatus.SaveFrameInfo(true, this);

  ManagePrerequisiteTicks();

  ManageAdaptiveSimulationDelay();

  if (IsServerPawn() && (PawnOwner->IsLocallyControlled() || !PawnOwner->Controller))
  {
    UpdateLocallyControlledServerPawn();
  }
  else if (IsAutonomousProxy())
  {
    UpdateAutonomousProxyPawn();
  }
  else if (IsRemotelyControlledServerPawn())
  {
    UpdateRemotelyControlledServerPawn();
  }

  if (IsSimulatedPawn())
  {
    SimulatePawn();
    ManageDynamicBufferTime();
  }

  DEBUG_SHOW_PAWN_NET_ROLE
  DEBUG_SHOW_PAWN_SIMULATION_DELAY
  DEBUG_DISPLAY_LOCAL_STATS
  DEBUG_DISPLAY_NET_STATS
}

void UGMC_ReplicationCmp::PreTick()
{
  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  // Use real-time for updating the timers.
  const float DeltaTime = World->DeltaRealTimeSeconds;

  ComponentStatus.PreTick(DeltaTime);

  if (IsNetMode(NM_Standalone))
  {
    return;
  }

  SimulationAux.PreTick(this);

  if (IsAutonomousProxy())
  {
    CL_MoveExecutionAux.PreTick(this, DeltaTime);
    return;
  }

  if (IsServerPawn())
  {
    SV_NetReserializationAux.PreTick(this);

    SV_PredictedClientNetSerializationAux.PreTick(this, DeltaTime);

    if (InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay)
    {
      if (IsSmoothedListenServerPawn())
      {
        AdaptiveDelayParams.SV_PreTickSmoothedRemoteServerPawn(DeltaTime);
      }

      AdaptiveDelayParams.SV_PreTickServerPawn(DeltaTime);
    }
  }
}

bool UGMC_ReplicationCmp::ShouldRunGMCUpdates() const
{
  if (!IsGMCEnabled())
  {
    return false;
  }

  if (!IsReadyForPlay())
  {
    return false;
  }

  gmc_ck(GetGMCPawnOwner())

  if (!IsValid(UpdatedComponent) || UpdatedComponent != PawnOwner->GetRootComponent())
  {
    return false;
  }

  if (IsServerPawn() && !PawnOwner->CheckStillInWorld())
  {
    return false;
  }

  if (!IsNetMode(NM_Client))
  {
    return true;
  }

  // The following checks are only relevant for clients.

  if (!PawnOwner->GetIsReplicated())
  {
    // The owning pawn must be set to replicate.
    return false;
  }

  if (PawnOwner->IsReplicatingMovement())
  {
    // No GMC updates when using built-in movement replication.
    return false;
  }

  return true;
}

void UGMC_ReplicationCmp::UpdateLocallyControlledServerPawn()
{
  SCOPE_CYCLE_COUNTER(STAT_UpdateLocallyControlledServerPawn)

  gmc_ck(IsLocallyControlledServerPawn() || (IsServerPawn() && !PawnOwner->Controller))
  gmc_ck(!(IsUsingServerAuthPhysicsReplication() && IsUsingClientAuthPhysicsReplication()))

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  float MoveDeltaTime = World->DeltaRealTimeSeconds;

  if (!ComponentStatus.ShouldExecuteMove(MoveDeltaTime, this))
  {
    return;
  }

  LocalMove().MetaData.Timestamp = GetTime();
  LocalMove().MetaData.DeltaTime = MoveDeltaTime;
  LocalMove().MetaData.ServerAuthPhysicsTimestamp = -1.;
  LocalMove().MetaData.bIsUsingServerAuthPhysics = IsUsingServerAuthPhysicsReplication();
  LocalMove().MetaData.bIsUsingClientAuthPhysics = IsUsingClientAuthPhysicsReplication();
  LocalMove().MetaData.bPredictedClientMove = false;
  LocalMove().MetaData.bValidClientMove = false;
  LocalMove().MetaData.bPlayerOwned = static_cast<bool>(Cast<APlayerController>(GetController()));

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreLocalMoveExecution, LocalMove());

  ProcessSyncData(LocalMove().InputState, {DataOp::AllDataOps}, AliasData, bUseRelativeValuesForSimulation, this);
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, LocalMove().InputState, EGMC_NetContext::LocalServerPawn_PreMoveExecution);

  if (Cast<AGMC_PlayerController>(PawnOwner->GetController()))
  {
    TickGenericRollbackActors(
      LocalMove().MetaData.Timestamp,
      LocalMove().MetaData.DeltaTime,
      LocalMove(),
      EGMC_NetContext::LocalServerPawn_PreMoveExecution
    );
  }

  ExecuteMove(
    LocalMove().InputState,
    LocalMove().MetaData.DeltaTime,
    LocalMove().MetaData.Timestamp,
    MaxTimeStep,
    MaxIterations,
    bUseRelativeValuesForSimulation,
    true,
    false,
    false
  );

  AncillaryTick(LocalMove(), LocalMove().MetaData.DeltaTime, LocalMove().MetaData.Timestamp, true, false, UNUSED(bool));

  ProcessSyncData(LocalMove().OutputState, {DataOp::AllDataOps}, AliasData, bUseRelativeValuesForSimulation, this);
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, LocalMove().OutputState, EGMC_NetContext::LocalServerPawn_PostMoveExecution);

  if (LocalMove().MetaData.bIsUsingServerAuthPhysics)
  {
    LocalMove().MetaData.ServerAuthPhysicsTimestamp = GetTime();
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostLocalMoveExecution, LocalMove());

  if (IsNetworkedServer() && ShouldAddToSimulationHistory(LocalMove().MetaData.Timestamp))
  {
    LocalMove().CopyDataValuesTo(SPMove(), this);
    LocalMove().CopyMetaDataTo(SPMove(), this);

    SV_CheckForcedNetUpdate(SPMove(), true);

    AddToSimulationHistory(SPMove());

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_OnSPMoveSaved, SPMove());
  }
}

void UGMC_ReplicationCmp::UpdateRemotelyControlledServerPawn()
{
  SCOPE_CYCLE_COUNTER(STAT_UpdateRemotelyControlledServerPawn)

  gmc_ck(IsRemotelyControlledServerPawn())
  gmc_ck(!SV_RemoteMoveExecutionAux.bIsExecutingRemoteMoves)

  const bool bUsesMinUpdateRate = ServerMinUpdateRate > 0;
  const int32 NumPendingMoves = SV_RemoteMoveExecutionAux.PendingMoves.Num();
  const bool bHasPendingMoves = NumPendingMoves > 0;
  if (bUsesMinUpdateRate && !bHasPendingMoves)
  {
    if (SV_ProcessProxyMove())
    {
      DEBUG_LOG_NUM_EXECUTED_REMOTE_PACKETS_PROXY_MOVE
      return;
    }
  }

  gmc_ck(!SV_RemoteMoveExecutionAux.bIsExecutingRemoteMoves)

  if (!bHasPendingMoves)
  {
    return;
  }

  gmc_ck(NumPendingMoves > 0)
  gmc_ck(RemoteMoveProcessingChunkSize > 0)
  gmc_ck(MaxNumBufferedRemoteMoves >= 0)

  const int32 NumExceedingMoves = FMath::Clamp(NumPendingMoves - MaxNumBufferedRemoteMoves, 0, MAX_int32);
  const int32 NumMovesToExecute = FMath::Clamp(
    NumExceedingMoves + FMath::Clamp((NumPendingMoves - NumExceedingMoves) / RemoteMoveProcessingChunkSize, 1, MAX_int32), 1, NumPendingMoves
  );

  gmc_ck(NumMovesToExecute > 0)
  gmc_ck(NumMovesToExecute <= NumPendingMoves)

  TArray<FGMC_Move> MovesToExecute{SV_RemoteMoveExecutionAux.PendingMoves.GetData(), NumMovesToExecute};

  SV_ExecuteClientMoves(MovesToExecute);

  if (SV_RemoteMoveExecutionAux.PendingMoves.Num() >= NumMovesToExecute)
  {
    SV_RemoteMoveExecutionAux.PendingMoves.RemoveAt(0, NumMovesToExecute, false);
  }
  else
  {
    // The pending moves array could have been cleared during move execution if ClearTransientData was called from somewhere.
    gmc_ck(SV_RemoteMoveExecutionAux.PendingMoves.IsEmpty())
  }

  gmc_ck(SV_RemoteMoveExecutionAux.PendingMoves.Num() <= MaxNumBufferedRemoteMoves)

  DEBUG_LOG_NUM_EXECUTED_REMOTE_PACKETS
}

void UGMC_ReplicationCmp::UpdateAutonomousProxyPawn()
{
  SCOPE_CYCLE_COUNTER(STAT_UpdateAutonomousProxyPawn)

  gmc_ck(IsAutonomousProxy())

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  if (!Cast<APlayerController>(PawnOwner->GetController()))
  {
    // Can happen during possession change.
    return;
  }

  LocalMove().MetaData.Timestamp = GetTime();
  LocalMove().MetaData.DeltaTime = FMath::Clamp(World->DeltaRealTimeSeconds, MIN_DELTA_TIME, MaxMoveDeltaTime);
  LocalMove().MetaData.ServerAuthPhysicsTimestamp = -1.;
  LocalMove().MetaData.bIsUsingServerAuthPhysics = false;
  LocalMove().MetaData.bIsUsingClientAuthPhysics = false;
  LocalMove().MetaData.bPredictedClientMove = false;
  LocalMove().MetaData.bValidClientMove = false;
  LocalMove().MetaData.bPlayerOwned = false;

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreLocalMoveExecution, LocalMove());

  const bool bUseRelative = bUseClientPrediction ? bUseRelativeValuesForPrediction : bUseRelativeValuesForSimulation;
  ProcessSyncData(LocalMove().InputState, {DataOp::Save | DataOp::Quantize}, AliasData, bUseRelative, this);

  bool bStartedNewMove{false};
  bool bPredictionHistoryFull{false};
  const bool bMoveDiscarded = !(
    bUseClientPrediction ?
    CL_MaintainPredictionHistory(LocalMove(), bStartedNewMove, bPredictionHistoryFull) :
    CL_MaintainNonPredictedMoves(LocalMove(), bStartedNewMove)
  );
  gmc_ck((bUseClientPrediction ? MoveHistory.Num() > 0 : CL_MoveExecutionAux.NonPredictedMoves.Num() > 0 && !bPredictionHistoryFull) || bMoveDiscarded)

  // Always reset the flag after the new move has been processed.
  CL_MoveExecutionAux.bDoNotCombineNextMove = false;

  auto& CurrentMove = CL_MoveExecutionAux.GetCurrentMoveForExecution(this);
  const auto& PreviousMove = CL_MoveExecutionAux.GetPreviousMoveForExecution(this);

  if (!PreviousMove.NetInfo.OwningComponent.IsValid())
  {
    // Can sometimes be invalidated when modifying things in the editor at runtime.
    return;
  }

  if (!bMoveDiscarded && (bUseClientPrediction ? MoveHistory.Num() > 1 : CL_MoveExecutionAux.NonPredictedMoves.Num() > 1))
  {
    gmc_ck(CurrentMove.HasValidTimestamp())
    gmc_ck(PreviousMove.HasValidTimestamp())
    gmc_ck(CurrentMove.NetInfo.OwningComponent.IsValid())
    gmc_ck(PreviousMove.NetInfo.OwningComponent.IsValid())

    // Use a delta time value that is calculated the same way as on the server.
    CurrentMove.MetaData.DeltaTime = FMath::Clamp(CurrentMove.MetaData.Timestamp - PreviousMove.MetaData.Timestamp, MIN_DELTA_TIME, MaxMoveDeltaTime);

    CL_ExecuteMove(CurrentMove, bUseClientPrediction, bStartedNewMove);

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostLocalMoveExecution, CurrentMove);

    if (bStartedNewMove)
    {
      // Sync data should only be suspended on the local move.
      LocalMove().CopySuspendedFlagsTo(const_cast<FGMC_Move&>(PreviousMove), this);

      // If we added a new move to the history this tick we can send the previous one (which is now complete). This is the only chance a move gets to be sent to
      // the server, afterwards it will soon be cleared from the history when the acknowledgement was received.
      CL_MoveExecutionAux.PendingMoves.Emplace(PreviousMove);

      gmc_ckc(
        double PreviousTimestamp = -1.;
        for (const auto& Move : CL_MoveExecutionAux.PendingMoves)
        {
          gmc_ck(Move.MetaData.Timestamp > PreviousTimestamp)
          PreviousTimestamp = Move.MetaData.Timestamp;
        }
      )
    }
  }
  else
  {
    // This shouldn't happen too often, if it does there is most likely a problem with the time synchronisation process.

    const bool bHasMove = bUseClientPrediction ? MoveHistory.Num() == 1 : CL_MoveExecutionAux.NonPredictedMoves.Num() == 1;
    gmc_ck(bHasMove || bMoveDiscarded)

    CL_ExecuteMove(bHasMove && !bMoveDiscarded ? CurrentMove : LocalMove(), bUseClientPrediction, true);

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostLocalMoveExecution, CurrentMove);

    GMC_CLOG(
      bMoveDiscarded,
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Client move was discarded.")
    )

    GMC_CLOG(
      bHasMove && !bMoveDiscarded,
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Client move was executed with the local delta time because there is only one move available.")
    )
  }

  if (CL_ShouldSendMoves())
  {
    CL_SendMoves();
    CL_MoveExecutionAux.TimeSinceLastMoveBatchWasSent = 0.;
    CL_MoveExecutionAux.PendingMoves.Reset();
  }
}

void UGMC_ReplicationCmp::SimulatePawn()
{
  SCOPE_CYCLE_COUNTER(STAT_SimulatePawn)

  gmc_ck(IsSimulatedProxy() || IsSmoothedListenServerPawn() || IsNonPredictedAutonomousProxy())
  gmc_ck(!IsNetMode(NM_DedicatedServer))
  gmc_ck(!SimulationAux.bIsSimulating)

  ++SimulationAux.NumFramesSinceLastSimulation;

  if (!ShouldSimulatePawn(SimulationThrottle.MaxSmoothingDistance, SimulationThrottle.SmoothingFallOffDistance, SimulationThrottle.MaxSkippedSmoothingFrames))
  {
    return;
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  const bool bIsSmoothedListenServerPawn = IsSmoothedListenServerPawn();

  if (bIsSmoothedListenServerPawn && !SV_RemoteServerPawnSmoothingSwapBuffer.bInitialized)
  {
    // On a listen server, if the buffer is not initialized yet we should not apply any smoothing as it could get initialized to the smoothed state otherwise.
    return;
  }

  SimulationAux.bIsSimulating = true;
  SimulationAux.NumFramesSinceLastSimulation = 0;

  const int32 MoveHistoryNum = MoveHistory.Num();
  const float DeltaTime = World->DeltaRealTimeSeconds;

  double SimTime{-1.};
  int32 TargetIdx{-1};
  TArray<int32> SkippedStateIndices{};

  const bool bHasEnoughMovesForSmoothing = MoveHistoryNum >= 2;
  if (!bNoSmoothing && bHasEnoughMovesForSmoothing)
  {
    SmoothMovement(DeltaTime, SimTime, TargetIdx, SkippedStateIndices);
  }
  else
  {
    SimulationAux.Reset();

    // Do not reset the simulation flag.
    SimulationAux.bIsSimulating = true;
  }

  gmc_ck(SimulationAux.bIsSimulating)

  SimulationTick(DeltaTime, SimTime, TargetIdx, SkippedStateIndices);

  SimulationAux.bIsSimulating = false;

  gmc_ck(MoveHistory.Num() == MoveHistoryNum)
}

void UGMC_ReplicationCmp::CL_OnRepAPMove()
{
  SCOPE_CYCLE_COUNTER(STAT_CL_OnRepAPMove)

  if (!IsAutonomousProxy())
  {
    // Can happen during possession change.
    return;
  }

  if (!APMove().HasValidTimestamp() || APMove().MetaData.Timestamp <= CL_MoveExecutionAux.LastReceivedMoveTimestamp)
  {
    // This is a throttled autonomous proxy update with no information.
    return;
  }

  gmc_ck(APMove().MetaData.Timestamp > CL_MoveExecutionAux.LastReceivedMoveTimestamp)

  CL_MoveExecutionAux.LastReceivedMoveTimestamp = APMove().MetaData.Timestamp;

  gmc_ck(APMove().MetaData.bPlayerOwned)

  ComponentStatus.bIsPlayerOwned = true;

  DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_RECEIVED_APMOVE

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_OnAPMoveReceived, APMove());

  if (!IsGMCEnabled() || !IsReadyForPlay())
  {
    return;
  }

  const bool bReceivedPredictedMove = APMove().MetaData.bPredictedClientMove;
  const bool bPredictionSettingsSynced = bReceivedPredictedMove == bUseClientPrediction;

  if (!bPredictionSettingsSynced || !CL_CheckPhysicsSettingsSynced(APMove()))
  {
    // This may happen briefly when toggling something via RPC and the settings are not synchronised yet or when older packets that were already in transit are
    // received. In this case just ignore all AP moves until those that are aligned with the current local settings start coming in. However, if this branch is
    // entered repeatedly for an extended period of time the server and client settings are out of sync.
    GMC_LOG(LogGMCReplication, PawnOwner, VeryVerbose, TEXT("AP move discarded. The replicated settings were not in sync with the local client settings."))
    return;
  }

  if (!bReceivedPredictedMove)
  {
    if (ShouldAddToSimulationHistory(APMove().MetaData.Timestamp))
    {
      // The AP move contains the same data simulated proxies receive in this case.
      AddToSimulationHistory(APMove());
    }

    return;
  }

  gmc_ck(!APMove().MetaData.bIsUsingServerAuthPhysics)
  gmc_ck(APMove().MetaData.bPredictedClientMove)

  FGMC_Move SourceMove = CL_ClearAcknowledgedMoves(APMove().MetaData.Timestamp);

  const double CurrentTime = GetTime();

  // Data marked "SV_ReplicateForPrediction" and/or "SV_ReplicateForCorrection" must also have "CL_Validate" set. Data marked "SV_UseClientValue" must not be
  // replicated back to predicted clients.
  const int32 DataFilterMask = DataFilter::SV_ReplicateForPrediction | (!APMove().MetaData.bValidClientMove ? DataFilter::SV_ReplicateForCorrection : 0);

  if (!SourceMove.HasValidTimestamp())
  {
    const float ElapsedTimeSinceLastValidUpdate = CurrentTime - CL_MoveExecutionAux.LastValidRepUpdateTime;
    GMC_LOG(LogGMCReplication, PawnOwner, Verbose, TEXT("Elapsed time since last valid server state update is %f s."), ElapsedTimeSinceLastValidUpdate)

    // Only reset the client if we are exceeding the max wait time (do nothing otherwise).
    if (MaxClientUpdateWaitTime <= 0.f || ElapsedTimeSinceLastValidUpdate > MaxClientUpdateWaitTime)
    {
      // When no source move was found we cannot check the client state or execute a replay, so we have to set the pawn state directly to the received server
      // state (which will almost certainly cause a teleport for the client). This should only occur very rarely e.g. when the client wasn't able to meet the
      // server's min required update rate.
      ProcessSyncData(APMove().OutputState, {DataOp::Apply, DataFilterMask, DataFilterMode::Exclusive}, AliasData, bUseRelativeValuesForPrediction, this);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, APMove().OutputState, EGMC_NetContext::LocalClientPawn_ReplayAborted);

      // Do not combine the adopted server state with the previous move.
      CL_DoNotCombineNextMove();

      GMC_LOG(
        LogGMCReplication,
        PawnOwner,
        Verbose,
        TEXT("No valid source move for %f s > %s (%f s), client adopted server state directly."),
        ElapsedTimeSinceLastValidUpdate,
        TO_STR(MaxClientUpdateWaitTime),
        MaxClientUpdateWaitTime
      )

      // Do not set CL_MoveExecutionAux.LastValidRepUpdateTime here. It makes sense to keep updating the pawn directly after the wait time was exceeded until a
      // valid source move is available again.
    }

    return;
  }

  CL_MoveExecutionAux.LastValidRepUpdateTime = CurrentTime;

  // Copy any data that was not replicated by the server from the original source move to the AP move.
  CopySyncDataValues(SourceMove.InputState, APMove().InputState, DataFilterMask, DataFilterMode::Inclusive, this);
  CopySyncDataValues(SourceMove.OutputState, APMove().OutputState, DataFilterMask, DataFilterMode::Inclusive, this);

  DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_COMPLEMENTED_APMOVE

  if (CL_ShouldReplay(APMove(), SourceMove))
  {
    CL_HandleSmoothCorrectionOnReplayStart();

    DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_LOCATION
    DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_ROTATION
    DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_SCALE
    DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_CONTROL_ROTATION

    DEBUG_LOG_REPLAY_APMOVE
    DEBUG_LOG_REPLAY_CLIENT_STATE_INITIAL

    CL_PreAdoptStateForReplay(APMove(), SourceMove);
    ProcessSyncData(APMove().OutputState, {DataOp::Apply}, AliasData, bUseRelativeValuesForPrediction, this);
    CALL_NATIVE_EVENT_CONDITIONAL(
      bNoBlueprintEvents,
      this,
      OnSyncDataApplied,
      APMove().OutputState,
      EGMC_NetContext::LocalClientPawn_ServerStateAdoptedForReplay
    );
    CL_PostAdoptStateForReplay(APMove(), SourceMove);

    DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_LOCATION
    DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_ROTATION
    DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_SCALE
    DEBUG_NET_CORRECTION_UPDATED_CLIENT_CONTROL_ROTATION

    CL_ReplayMoves();

    DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_LOCATION
    DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_ROTATION
    DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_SCALE
    DEBUG_NET_CORRECTION_REPLAYED_CLIENT_CONTROL_ROTATION

    DEBUG_SHOW_CLIENT_CORRECTIONS

    GMC_CLOG(
      !bAlwaysReplay,
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Replayed %d moves."),
      MoveHistory.Num()
    )

    GMC_CLOG(
      ReplicationSettings.DefaultCompressionSettings.ActorLocation == EGMC_FloatPrecisionBlueprint::FullPrecision ?
        Debug_OriginalActorLocation != Debug_ReplayedActorLocation :
        !Debug_OriginalActorLocation.Equals(
          Debug_ReplayedActorLocation,
          1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ActorLocation)
        ),
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Replayed client actor location differs by %.*f cm from the original location."),
      (uint8)ReplicationSettings.DefaultCompressionSettings.ActorLocation,
      (Debug_ReplayedActorLocation - Debug_OriginalActorLocation).Size()
    )
    GMC_CLOG(
      ReplicationSettings.DefaultCompressionSettings.ActorRotation == EGMC_FloatPrecisionBlueprint::FullPrecision ?
        Debug_OriginalActorRotation != Debug_ReplayedActorRotation :
        !Debug_OriginalActorRotation.Equals(
          Debug_ReplayedActorRotation,
          1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ActorRotation)
        ),
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Replayed client actor rotation differs by %.*f deg from the original rotation."),
      (uint8)ReplicationSettings.DefaultCompressionSettings.ActorRotation,
      FMath::RadiansToDegrees(Debug_ReplayedActorRotation.AngularDistance(Debug_OriginalActorRotation))
    )
    GMC_CLOG(
      ReplicationSettings.DefaultCompressionSettings.ActorScale == EGMC_FloatPrecisionBlueprint::FullPrecision ?
        Debug_OriginalActorScale != Debug_ReplayedActorScale :
        !Debug_OriginalActorScale.Equals(
          Debug_ReplayedActorScale,
          1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ActorScale)
        ),
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Replayed client actor scale differs by %.*f cm from the original scale."),
      (uint8)ReplicationSettings.DefaultCompressionSettings.ActorScale,
      (Debug_ReplayedActorScale - Debug_OriginalActorScale).Size()
    )
    GMC_CLOG(
      ReplicationSettings.DefaultCompressionSettings.ControlRotation == EGMC_FloatPrecisionBlueprint::FullPrecision ?
        Debug_OriginalControlRotation != Debug_ReplayedControlRotation :
        !Debug_OriginalControlRotation.Equals(
          Debug_ReplayedControlRotation,
          1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ControlRotation)
        ),
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Replayed client control rotation differs by %.*f deg from the original control rotation."),
      (uint8)ReplicationSettings.DefaultCompressionSettings.ControlRotation,
      FMath::RadiansToDegrees(Debug_ReplayedControlRotation.AngularDistance(Debug_OriginalControlRotation))
    )

    CL_HandleSmoothCorrectionOnReplayEnd();
  }
}

void UGMC_ReplicationCmp::CL_OnRepSPMove()
{
  SCOPE_CYCLE_COUNTER(STAT_CL_OnRepSPMove)

  if (!IsSimulatedProxy())
  {
    // Can happen during possession change.
    return;
  }

  if (!SPMove().HasValidTimestamp() || SPMove().MetaData.Timestamp <= CL_MoveExecutionAux.LastReceivedMoveTimestamp)
  {
    // Can happen in bad network conditions.
    return;
  }

  gmc_ck(SPMove().MetaData.Timestamp > CL_MoveExecutionAux.LastReceivedMoveTimestamp)

  CL_MoveExecutionAux.LastReceivedMoveTimestamp = SPMove().MetaData.Timestamp;

  ComponentStatus.bIsPlayerOwned = SPMove().MetaData.bPlayerOwned;

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_OnSPMoveReceived, SPMove());

  if (!IsGMCEnabled() || !IsReadyForPlay())
  {
    return;
  }

  const bool bReceivedUsingServerAuthPhysics = SPMove().MetaData.bIsUsingServerAuthPhysics;
  const bool bReceivedUsingClientAuthPhysics = SPMove().MetaData.bIsUsingClientAuthPhysics;
  const bool bServerAuthPhysicsSettingChanged = IsUsingServerAuthPhysicsReplication() != bReceivedUsingServerAuthPhysics;
  const bool bClientAuthPhysicsSettingChanged = IsUsingClientAuthPhysicsReplication() != bReceivedUsingClientAuthPhysics;

  gmc_ck(!(bReceivedUsingServerAuthPhysics && bReceivedUsingClientAuthPhysics))

  if (bServerAuthPhysicsSettingChanged || bClientAuthPhysicsSettingChanged)
  {
    if (bServerAuthPhysicsSettingChanged)
    {
      EnableServerAuthPhysicsSimulationLocal(
        bReceivedUsingServerAuthPhysics,
        FGMC_ServerAuthPhysicsSettings{ToPhysicsInterpolationMode(InterpolationMode), SimulatedProxyPhysicsInterp}
      );
      gmc_ck(IsUsingServerAuthPhysicsReplication() == bReceivedUsingServerAuthPhysics)
    }

    if (bClientAuthPhysicsSettingChanged)
    {
      EnableClientAuthPhysicsSimulationLocal(
        bReceivedUsingClientAuthPhysics,
        FGMC_ClientAuthPhysicsSettings{ToPhysicsInterpolationMode(InterpolationMode), SimulatedProxyPhysicsInterp, false, false, false, false}
      );
      gmc_ck(IsUsingClientAuthPhysicsReplication() == bReceivedUsingClientAuthPhysics)
    }
  }
  else if (ShouldAddToSimulationHistory(SPMove().MetaData.Timestamp))
  {
    AddToSimulationHistory(SPMove());
  }
}

void UGMC_ReplicationCmp::SV_PreReplication()
{
  gmc_ck(IsServerPawn())

  SV_NetReserializationAux.PreReplication(this);
}

void UGMC_ReplicationCmp::EnableGMC(bool bEnable)
{
  const auto& GMCPawnOwner = GetGMCPawnOwner();
  if (!GMCPawnOwner)
  {
    return;
  }

  if (IsGMCEnabled() == bEnable)
  {
    return;
  }

  if (bEnable)
  {
    ComponentStatus.bGMCEnabled = true;

    if (IsServerPawn())
    {
      GMCPawnOwner->SV_MarkForNetReserialization();
    }

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnGMCEnabled);

    ClearTransientData(true);
  }
  else
  {
    ComponentStatus.bGMCEnabled = false;

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnGMCDisabled);

    ClearTransientData(true);
  }
}

double UGMC_ReplicationCmp::GetElapsedTimeSinceSpawn() const
{
  return ComponentStatus.ElapsedRealTimeSinceSpawn;
}

void UGMC_ReplicationCmp::CL_PreAdoptStateForReplay(const FGMC_Move& APMove, const FGMC_Move& SourceMove)
{
  if (!bUseRelativeValuesForPrediction || !bRestoreClientAuthValuesAfterReplay)
  {
    return;
  }

  // Save the pre-correction state so we can restore client-auth values that may have been altered during the replay due to a base change.
  ProcessSyncData_IntegratedOnly(CL_MoveExecutionAux.TempMove.OutputState, {DataOp::Save, DataFilter::None, DataFilterMode::Inclusive}, AliasData, false, this);
}

void UGMC_ReplicationCmp::CL_PostReplay_Implementation()
{
  if (!bUseRelativeValuesForPrediction || !bRestoreClientAuthValuesAfterReplay)
  {
    return;
  }

  const auto& PredictionSettings = ReplicationSettings.DefaultPredictionSettings;
  const auto& PreCorrectionState = CL_MoveExecutionAux.TempMove.OutputState;

  if (IsClientAuth(PredictionSettings.LinearVelocity))
  {
    SetLinearVelocity_GMC(PreCorrectionState.LinearVelocity.Read());
  }
  if (IsClientAuth(PredictionSettings.AngularVelocity))
  {
    SetAngularVelocity_GMC(PreCorrectionState.AngularVelocity.Read());
  }
  if (IsClientAuth(PredictionSettings.ActorLocation))
  {
    SetActorLocation_GMC(PreCorrectionState.ActorLocation.Read(), true);
  }
  if (IsClientAuth(PredictionSettings.ActorRotation))
  {
    SetActorRotation_GMC(PreCorrectionState.ActorRotation.Read(), true);
  }
  if (IsClientAuth(PredictionSettings.ActorScale))
  {
    SetActorScale_GMC(PreCorrectionState.ActorScale.Read());
  }
  if (IsClientAuth(PredictionSettings.ControlRotation))
  {
    SetControllerRotation_GMC(PreCorrectionState.ControlRotation.Read());
  }
}

void UGMC_ReplicationCmp::CL_SwapNoPredictionBuffer(EGMC_NetContext Context)
{
  if (IsNonPredictedAutonomousProxy() && CL_NoPredictionSwapBuffer.bInitialized)
  {
    CL_NoPredictionSwapBuffer.Swap(false, this, Context);
    CL_OnSwapNoPredictionBuffer(CL_NoPredictionSwapBuffer.Buffer, Context);
  }
}

void UGMC_ReplicationCmp::PreLocalMoveExecution_Implementation(const FGMC_Move& LocalMove)
{
  CL_SwapNoPredictionBuffer(EGMC_NetContext::LocalClientPawn_PreMoveExecutionNoPrediction);
}

void UGMC_ReplicationCmp::PostLocalMoveExecution_Implementation(const FGMC_Move& ExecutedMove)
{
  CL_SwapNoPredictionBuffer(EGMC_NetContext::LocalClientPawn_PostMoveExecutionNoPrediction);
}

void UGMC_ReplicationCmp::PrePlayerTick_Implementation(float DeltaTime)
{
  if (IsNonPredictedAutonomousProxy())
  {
    if (!CL_NoPredictionSwapBuffer.bInitialized)
    {
      CL_NoPredictionSwapBuffer.Initialize(false, this);
      CL_OnInitNoPredictionBuffer(CL_NoPredictionSwapBuffer.Buffer);
    }

    CL_SwapNoPredictionBuffer(EGMC_NetContext::LocalClientPawn_PrePlayerTickNoPrediction);
  }
}

void UGMC_ReplicationCmp::PostPlayerTick_Implementation(float DeltaTime)
{
  CL_SwapNoPredictionBuffer(EGMC_NetContext::LocalClientPawn_PostPlayerTickNoPrediction);
}

void UGMC_ReplicationCmp::OnServerAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings)
{
  gmc_ck(!(IsUsingServerAuthPhysicsReplication() && IsUsingClientAuthPhysicsReplication()))

  ComponentStatus.bIsUsingServerAuthPhysicsReplication = bEnabled;

  if (IsNetMode(NM_Standalone))
  {
    return;
  }

  // Do not reset the moves for simulated proxies.
  ClearTransientData(!IsSimulatedProxy());

  if (bEnabled)
  {
    SavedSettingsServerAuthPhysics.SaveSettings(this);

    Settings.ApplySettings(this);
    InterpolationMode = ToDefaultInterpolationMode(PhysicsInterpolationMode);
    ExtrapolationMode = EGMC_ExtrapolationMode::DeadReckoning;

    // These must be false regardless of the values in Settings.
    bUseClientPrediction = false;
    bSmoothRemoteListenServerPawn = false;

    SV_RemoteServerPawnSmoothingSwapBuffer.Reset();
  }
  else
  {
    // Restore the original settings when physics simulation is toggled off.
    SavedSettingsServerAuthPhysics.ApplySettings(this);
  }
}

void UGMC_ReplicationCmp::EnableServerAuthPhysicsSimulationLocal(bool bEnable, FGMC_ServerAuthPhysicsSettings Settings)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return;
  }

  if (IsUsingClientAuthPhysicsReplication())
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Warning,
      TEXT("Client-authoritative physics replication is already active, cannot enable server-authoritative physics replication.")
    )
    gmc_ckne()
    return;
  }

  if (IsUsingServerAuthPhysicsReplication() == bEnable)
  {
    return;
  }

  // Physics are only simulated on the server, clients will just display the replicated transform.
  RootComponent->SetSimulatePhysics(bEnable && IsServerPawn());

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnServerAuthPhysicsSimulationToggled, bEnable, Settings);
}

bool UGMC_ReplicationCmp::IsUsingServerAuthPhysicsReplication() const
{
  gmc_ck(!(ComponentStatus.bIsUsingServerAuthPhysicsReplication && ComponentStatus.bIsUsingClientAuthPhysicsReplication))
  return ComponentStatus.bIsUsingServerAuthPhysicsReplication;
}

void UGMC_ReplicationCmp::OnClientAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings)
{
  gmc_ck(!(IsUsingServerAuthPhysicsReplication() && IsUsingClientAuthPhysicsReplication()))

  ComponentStatus.bIsUsingClientAuthPhysicsReplication = bEnabled;

  if (IsNetMode(NM_Standalone))
  {
    return;
  }

  // Do not reset the moves for simulated proxies.
  ClearTransientData(!IsSimulatedProxy());

  if (bEnabled)
  {
    SavedSettingsClientAuthPhysics.SaveSettings(this);

    Settings.ApplySettings(this);
    InterpolationMode = ToDefaultInterpolationMode(PhysicsInterpolationMode);
    ExtrapolationMode = EGMC_ExtrapolationMode::DeadReckoning;

    // Must be true regardless of the values in Settings. Client-auth movement is still considered predicted internally.
    bUseClientPrediction = true;
    bAssumeClientState = true;

    SV_RemoteServerPawnSmoothingSwapBuffer.Reset();
  }
  else
  {
    // Restore the original settings when physics simulation is toggled off.
    SavedSettingsClientAuthPhysics.ApplySettings(this);
  }
}

void UGMC_ReplicationCmp::EnableClientAuthPhysicsSimulationLocal(bool bEnable, FGMC_ClientAuthPhysicsSettings Settings)
{
  if (!IsRemotelyControlledServerPawn() && !IsAutonomousProxy() && !IsSimulatedProxy())
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return;
  }

  if (IsUsingServerAuthPhysicsReplication())
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Warning,
      TEXT("Server-authoritative physics replication is already active, cannot enable client-authoritative physics replication.")
    )
    gmc_ckne()
    return;
  }

  if (IsUsingClientAuthPhysicsReplication() == bEnable)
  {
    return;
  }

  // Each pawn simulates their own physics. This means that any synchronised physics state values must be client-authoritative.
  RootComponent->SetSimulatePhysics(bEnable && PawnOwner->IsLocallyControlled());

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnClientAuthPhysicsSimulationToggled, bEnable, Settings);
}

bool UGMC_ReplicationCmp::IsUsingClientAuthPhysicsReplication() const
{
  gmc_ck(!(ComponentStatus.bIsUsingServerAuthPhysicsReplication && ComponentStatus.bIsUsingClientAuthPhysicsReplication))
  return ComponentStatus.bIsUsingClientAuthPhysicsReplication;
}

void UGMC_ReplicationCmp::ClearTransientData(bool bResetMoves)
{
  SCOPE_CYCLE_COUNTER(STAT_ClearTransientData)

  if (const auto& GMCPawnOwner = GetGMCPawnOwner())
  {
    if (bResetMoves)
    {
      GMCPawnOwner->InitializeMoves();
    }

    if (IsServerPawn())
    {
      GMCPawnOwner->SV_MarkForNetReserialization();
    }
  }

  MoveHistory.Reset(MoveHistoryMaxSize);

  SV_RemoteServerPawnSmoothingSwapBuffer.Reset();
  SV_RemoteMoveExecutionAux.Reset();
  SV_NetReserializationAux.Reset();
  SV_PredictedClientNetSerializationAux.Reset();
  SV_TimestampVerificationAux.Reset();

  DynamicBufferTimeAux.Reset();
  CL_MoveExecutionAux.Reset();
  CL_NoPredictionSwapBuffer.Reset();
  CL_SmoothCorrection.Reset();

  RollbackRestoreBuffer.Reset();
  RollbackSwapBuffer.Reset();

  SimulationAux.Reset();
  MatchLatestParams.Reset();
  MatchLatestParamsAux.Reset();
  FixedDelayParams.Reset();
  AdaptiveDelayParams.Reset();
  UniformSimulationParams.Reset();
  CumulativeSimulationParams.Reset();

  // Keep the value of SimulatedControlRotation for consistency during controller changes.
}

void UGMC_ReplicationCmp::ManagePrerequisiteTicks()
{
  SCOPE_CYCLE_COUNTER(STAT_ManagePrerequisiteTicks)

  const bool bAggregates = IsValid(GMCAggregator) && GMCAggregator->bEnableAggregateTick;

  if (!bAggregates || !GMCAggregator->bAggregateControllers)
  {
    // The player controller should tick before the replication component to update input.
    if (const auto& GMCPlayerController = Cast<AGMC_PlayerController>(PawnOwner->GetController()))
    {
      if (GMCPlayerController->IsLocalController())
      {
        AddTickPrerequisiteActor(GMCPlayerController);
      }
    }
  }

  if (!IsClientPawn())
  {
    return;
  }

  if (!bAggregates || !GMCAggregator->bAggregateControllers)
  {
    // On the client the local player controller must tick before the replication components of all pawns to update the world time.
    if (const auto& LocalPC = GEngine ? Cast<AGMC_PlayerController>(GEngine->GetFirstLocalPlayerController(GetWorld())) : nullptr)
    {
      AddTickPrerequisiteActor(LocalPC);
    }
  }

  if (bAggregates && GMCAggregator->bAggregateMovementComponents)
  {
    return;
  }

  TArray<AActor*> Actors{};

  if (IsValid(GMCAggregator))
  {
    Actors = static_cast<const TArray<AActor*>>(GMCAggregator->GetPawns());
  }
  else
  {
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGMC_Pawn::StaticClass(), Actors);
  }

  if (IsSimulatedProxy())
  {
    // A simulated proxy could have been previously possessed by the client in which case we need to remove the prerequisite ticks from the unpossessed pawn as
    // they would otherwise form a cycle (AController::OnUnPossess is not called on clients so we need to do it like this).
    for (const auto& Actor : Actors)
    {
      const auto& GMCPawn = Cast<AGMC_Pawn>(Actor);
      gmc_ck(GMCPawn)

      if (const auto& ReplicationComponent = GMCPawn->GetReplicationComponent())
      {
        RemoveTickPrerequisiteComponent(ReplicationComponent);
      }
    }
  }
  else if (IsAutonomousProxy())
  {
    // All simulated proxy replication components must tick before the replication component of the autonomous proxy. This guarantees that the interpolation
    // data of smoothed pawns is up to date when accessed from the locally controlled pawn which is particularly important for pawn rollback.
    for (const auto& Actor : Actors)
    {
      const auto& GMCPawn = Cast<AGMC_Pawn>(Actor);
      gmc_ck(GMCPawn)

      if (GMCPawn->GetLocalRole() == ROLE_SimulatedProxy)
      {
        if (const auto& ReplicationComponent = GMCPawn->GetReplicationComponent())
        {
          AddTickPrerequisiteComponent(ReplicationComponent);
        }
      }
    }
  }
}

void UGMC_ReplicationCmp::ManageDynamicBufferTime()
{
  gmc_ck(!IsNetMode(NM_Standalone))
  gmc_ck(IsSimulatedProxy() || IsSmoothedListenServerPawn() || IsNonPredictedAutonomousProxy())

  if (bNoSmoothing || InterpolationMode != EGMC_InterpolationMode::AdaptiveDelay || !AdaptiveDelayParams.bUseDynamicBufferTime)
  {
    DynamicBufferTimeAux.Reset();
    return;
  }

  DynamicBufferTimeAux.Update(this);

  DEBUG_LOG_DYNAMIC_BUFFER_TIME_VALUATION

  const auto& GameInstance = PawnOwner->GetGameInstance();
  if (!GameInstance)
  {
    gmc_ckne()
    return;
  }

  // A listen server will also have a local controller.
  const auto& LocalController = Cast<AGMC_PlayerController>(GameInstance->GetFirstLocalPlayerController());
  if (!LocalController)
  {
    gmc_ckne()
    return;
  }

  if (!DynamicBufferTimeAux.CanRequest(this, LocalController))
  {
    return;
  }

  if (DynamicBufferTimeAux.Request(this))
  {
    gmc_ck(DynamicBufferTimeAux.HasPendingBufferTime())

    if (IsSmoothedListenServerPawn())
    {
      // Listen server pawns do not need to be notified of a new requested buffer time, they just read the local value directly.
      AdaptiveDelayParams.SV_SmoothedRemoteServerPawnUpdateTimer = AdaptiveDelayParams.SyncInterval + AdaptiveDelayParams.SYNC_INTERVAL_VARIANCE;
      gmc_ck(AdaptiveDelayParams.BufferTime != DynamicBufferTimeAux.RequestedBufferTime)
    }
    else
    {
      LocalController->SV_RequestAdaptiveDelayBufferTime(FGMC_AdaptiveDelayClientPacket{this, DynamicBufferTimeAux.RequestedBufferTime});
    }

    DEBUG_LOG_DYNAMIC_BUFFER_TIME_REQUEST
  }

  gmc_ck(DynamicBufferTimeAux.RequestTimer == 0.)
}

void UGMC_ReplicationCmp::ManageAdaptiveSimulationDelay()
{
  SCOPE_CYCLE_COUNTER(STAT_ManageAdaptiveSimulationDelay)

  if (IsNetMode(NM_Standalone) || bNoSmoothing || InterpolationMode != EGMC_InterpolationMode::AdaptiveDelay)
  {
    return;
  }

  if (IsClientPawn())
  {
    CL_UpdateLocalAdaptiveDelay();

    return;
  }

  if (IsServerPawn())
  {
    if (IsSmoothedListenServerPawn())
    {
      SV_UpdateLocalAdaptiveDelay();
    }

    SV_UpdateClientAdaptiveDelays();

    return;
  }
}

FVector UGMC_ReplicationCmp::ExecuteMove(
  const FGMC_PawnState& InputState,
  float DeltaTime,
  double Timestamp,
  float InMaxTimestep,
  int32 InMaxIterations,
  bool bUseRelative,
  bool bLocal,
  bool bSimulated,
  bool bCombined
)
{
  SCOPE_CYCLE_COUNTER(STAT_ExecuteMove)

  gmc_ck(!ComponentStatus.bIsExecutingLocalMove)
  gmc_ck(!ComponentStatus.bIsExecutingNonSimulatedMove)
  gmc_ck(!ComponentStatus.bIsExecutingSimulatedMove)
  gmc_ck(bLocal ? !bSimulated : true)

  if (DeltaTime < MIN_DELTA_TIME)
  {
    return FVector{0.};
  }

  ComponentStatus.bIsExecutingLocalMove = bLocal;
  ComponentStatus.bIsExecutingNonSimulatedMove = !bSimulated;
  ComponentStatus.bIsExecutingSimulatedMove = bSimulated;

  FGMC_PawnState IterationInputState = InputState;
  int32 Iterations = 0;
  float RemainingTime = DeltaTime;
  float SubDeltaTime = 0.f;
  while (RemainingTime >= MIN_DELTA_TIME)
  {
    ++Iterations;
    SubDeltaTime = CalculateSubDeltaTime(Iterations, RemainingTime, InMaxTimestep, InMaxIterations);
    RemainingTime -= SubDeltaTime;
    const bool bIsSubSteppedIteration = RemainingTime >= MIN_DELTA_TIME;

    UpdatePhysicsVelocity();

    PredictionTick(IterationInputState, SubDeltaTime, Timestamp, Iterations, bIsSubSteppedIteration, bLocal, bSimulated, bCombined);

    UpdateComponentVelocity();

    if (bIsSubSteppedIteration)
    {
      ProcessSyncData(IterationInputState, {DataOp::Save}, AliasData, bUseRelative, this);
    }
  }

  ComponentStatus.bIsExecutingLocalMove = false;
  ComponentStatus.bIsExecutingNonSimulatedMove = false;
  ComponentStatus.bIsExecutingSimulatedMove = false;

  return GetActorLocation_GMC() - InputState.ActorLocation.Read();
}

float UGMC_ReplicationCmp::CalculateSubDeltaTime(
  int32 Iterations,
  float RemainingTime,
  float InMaxTimeStep,
  int32 InMaxIterations
) const
{
  gmc_ck(Iterations > 0)

  float SubDeltaTime = 0.f;
  if (RemainingTime > InMaxTimeStep)
  {
    if (Iterations < InMaxIterations)
    {
      // The remaining time is larger than the max allowed time step and we have iterations left. Dividing the remaining time by 2 gives us equal time steps for
      // the last two iterations.
      SubDeltaTime = FMath::Min(InMaxTimeStep, RemainingTime / 2.f);
    }
    else
    {
      // This is the last iteration, use all the remaining time. If the max iteration count is 1 this will be the same as the original delta time.
      SubDeltaTime = RemainingTime;
      GMC_CLOG(
        InMaxIterations > 1,
        LogGMCReplication,
        PawnOwner,
        Verbose,
        TEXT("Reached maximum amount of %d iterations while calculating sub-stepping delta time with %f s > max time step (%f s) remaining."),
        InMaxIterations,
        RemainingTime,
        InMaxTimeStep
      );
    }
  }
  else
  {
    // Remaining time is already lower than the max allowed time step.
    SubDeltaTime = RemainingTime;
  }

  return SubDeltaTime;
}

float UGMC_ReplicationCmp::GetClientSendRate() const
{
  gmc_ck(ClientSendRate >= 0)
  gmc_ck(IsReadyForPlay()) // Don't call before BeginPlay has finished.

  return !IsUsingUnreliableClientMoves() ? ClientSendRate : STAND_IN_CLIENT_SEND_RATE_UNRELIABLE;
}

float UGMC_ReplicationCmp::GetMaxCombinedDeltaTime() const
{
  gmc_ck(MaxCombinedDeltaTime >= 0.000001f)
  gmc_ck(IsReadyForPlay()) // Don't call before BeginPlay has finished.

  return !IsUsingUnreliableClientMoves() ? MaxCombinedDeltaTime : CLIENT_MAX_MOVE_COMBINE_DELTA_TIME_UNRELIABLE;
}

void UGMC_ReplicationCmp::UpdatePhysicsVelocity()
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return;
  }

  if (RootComponent->IsSimulatingPhysics())
  {
    gmc_ck(RootComponent->GetPhysicsLinearVelocity() == RootComponent->GetComponentVelocity())
    Velocity = RootComponent->GetPhysicsLinearVelocity();
    AngularVelocity = RootComponent->GetPhysicsAngularVelocityInRadians();
  }
}

void UGMC_ReplicationCmp::SetSimulatedPawnState(const FGMC_PawnState& State, bool bSkipBoneUpdate, bool bUseRelative, EGMC_NetContext Context)
{
  SCOPE_CYCLE_COUNTER(STAT_SetSimulatedPawnState)

  gmc_ck(MoveHistory.Num() >= 2)

  // If the pawn has a skeletal mesh component we may want to skip updating kinematic bones as it can be expensive.
  auto SkeletalMesh = PawnOwner->FindComponentByClass<USkeletalMeshComponent>();
  EKinematicBonesUpdateToPhysics::Type SavedBoneUpdateSetting{EKinematicBonesUpdateToPhysics::Type::SkipSimulatingBones};
  const bool bShouldSkipBoneUpdate = bSkipBoneUpdate && SkeletalMesh;
  if (bShouldSkipBoneUpdate)
  {
    SavedBoneUpdateSetting = SkeletalMesh->KinematicBonesUpdateType;
    SkeletalMesh->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::Type::SkipAllBones;
  }

  ProcessSyncData(
    const_cast<FGMC_PawnState&>(State),
    {DataOp::Apply, DataFilter::SV_ReplicateForSimulation, DataFilterMode::Exclusive},
    AliasData,
    bUseRelative,
    this
  );
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, State, Context);

  if (bShouldSkipBoneUpdate)
  {
    // Restore the original flag.
    SkeletalMesh->KinematicBonesUpdateType = SavedBoneUpdateSetting;
  }
}

void UGMC_ReplicationCmp::SV_ReceiveClientSendStatus_Implementation(bool bIsUsingUnreliableMoves)
{
  gmc_ck(IsRemotelyControlledServerPawn())
  gmc_ck(ComponentStatus.bUseUnreliableClientMoves != bIsUsingUnreliableMoves)

  ComponentStatus.bUseUnreliableClientMoves = bIsUsingUnreliableMoves;

  if (bIsUsingUnreliableMoves)
  {
    GMC_LOG(LogGMCReplication, GetPawnOwner(), VeryVerbose, TEXT("Unreliable client move transmission enabled."))
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnUnreliableClientMovesActivated);
  }
  else
  {
    GMC_LOG(LogGMCReplication, GetPawnOwner(), VeryVerbose, TEXT("Unreliable client move transmission disabled."))
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnUnreliableClientMovesDeactivated);
  }
}

bool UGMC_ReplicationCmp::SV_ReceiveClientSendStatus_Validate(bool bIsUsingUnreliableMoves)
{
  return true;
}

void UGMC_ReplicationCmp::SV_ReceiveMoves_Implementation()
{
  DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_RECEIVED_MOVES

  if (SV_AuditClientMoves(SV_RemoteMoveExecutionAux.DeserializedMoves))
  {
    SV_RemoteMoveExecutionAux.PendingMoves.Append(MoveTemp(SV_RemoteMoveExecutionAux.DeserializedMoves));
  }
  else
  {
    SV_RemoteMoveExecutionAux.DeserializedMoves.Reset();
  }

  gmc_ck(SV_RemoteMoveExecutionAux.DeserializedMoves.Num() == 0)
}

bool UGMC_ReplicationCmp::SV_ReceiveMoves_Validate()
{
  return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_ValidateRemoteMoves, SV_RemoteMoveExecutionAux.DeserializedMoves);
};

bool UGMC_ReplicationCmp::SV_AuditClientMoves(TArray<FGMC_Move>& RemoteMoves)
{
  SCOPE_CYCLE_COUNTER(STAT_SV_AuditClientMoves)

  gmc_ck(RemoteMoves.Num() > 0)

  if (MoveHistory.Num() > 0)
  {
    for (int32 Index = 0; Index < RemoteMoves.Num(); ++Index)
    {
      if (RemoteMoves[Index].MetaData.Timestamp > SV_RemoteMoveExecutionAux.LastAcceptedClientTimestamp)
      {
        // The client timestamps are always in ascending order so the following moves should have valid timestamps as well.
        gmc_ckc(
          double PreviousTimestamp = -1.;
          for (const auto& Move : RemoteMoves)
          {
            gmc_ck(Move.MetaData.Timestamp > PreviousTimestamp)
            PreviousTimestamp = Move.MetaData.Timestamp;
          }
        )

        break;
      }

      // Discard moves that are older than the latest timestamp in the move history. This can happen if proxy moves with a newer timestamp were created by the
      // server because the client wasn't able to reach the min required update rate.
      RemoteMoves.RemoveAt(Index, 1, false);
      --Index;
    }
  }

  if (RemoteMoves.Num() == 0)
  {
    // Also needs to be updated here, otherwise the remote pawn could get permanently blocked by proxy moves due to timestamp discrepancies.
    SV_RemoteMoveExecutionAux.LastRemotePawnUpdateTime = GetTime();

    return false;
  }

  bool bClientCredible = true;
  if (bVerifyClientTimestamps)
  {
    if (!SV_VerifyTimestamps(RemoteMoves))
    {
      bClientCredible = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents, this, SV_HandleConspicuousClient, ++SV_TimestampVerificationAux.InfractionsThisPeriod
      );

      GMC_CLOG(
        !bClientCredible,
        LogGMCReplication,
        PawnOwner,
        Warning,
        TEXT("Remote timestamps were not valid (infractions this period = %d), moves will not be executed."),
        SV_TimestampVerificationAux.InfractionsThisPeriod
      )
    }
  }

  if (bClientCredible)
  {
    // The last accepted client move timestamp may not always be the same as the last received client timestamp or the last one in the move history. The
    // distinction is important when unreliably sent client moves are involved.
    SV_RemoteMoveExecutionAux.LastAcceptedClientTimestamp = RemoteMoves[RemoteMoves.Num() - 1].MetaData.Timestamp;
  }

  return bClientCredible;
}

void UGMC_ReplicationCmp::SV_ExecuteClientMoves(TArray<FGMC_Move>& ClientMoves, bool bIsProxyMove)
{
  SCOPE_CYCLE_COUNTER(STAT_SV_ExecuteClientMoves)

  gmc_ck(!SV_RemoteMoveExecutionAux.bIsExecutingRemoteMoves)
  gmc_ck(ClientMoves.Num() > 0)

  // Set the timestamp to enforce ServerMinUpdateRate.
  SV_RemoteMoveExecutionAux.LastRemotePawnUpdateTime = GetTime();

  if (!IsGMCEnabled())
  {
    // Moves can still arrive after the GMC was disabled locally.
    return;
  }

  if (!Cast<APlayerController>(PawnOwner->GetController()))
  {
    // Do not execute moves on pawns that do not have a player controller (i.e. owning connection) anymore. This can happen when switching pawns as the remote
    // moves can arrive after the pawn was already unpossessed on the server.
    return;
  }

  // No more early returns after this point.

  if (IsSmoothedListenServerPawn())
  {
    if (!SV_RemoteServerPawnSmoothingSwapBuffer.bInitialized)
    {
      SV_RemoteServerPawnSmoothingSwapBuffer.Initialize(false, this);
      SV_OnInitRemoteServerPawnSmoothingBuffer(SV_RemoteServerPawnSmoothingSwapBuffer.Buffer);
    }

    SV_RemoteServerPawnSmoothingSwapBuffer.Swap(false, this, EGMC_NetContext::RemoteServerPawn_UnSmoothing);
    SV_OnSwapRemoteServerPawnSmoothingBuffer(SV_RemoteServerPawnSmoothingSwapBuffer.Buffer, EGMC_NetContext::RemoteServerPawn_UnSmoothing);
    gmc_ck(!SV_RemoteServerPawnSmoothingSwapBuffer.HasEvenNumberOfSwaps())
  }

  SV_RemoteMoveExecutionAux.bIsExecutingRemoteMoves = true;

  // Rollback only makes sense for predicted clients and actual client moves (not proxy moves).
  const bool bShouldRollBackServerPawns = bRollBackServerPawns && bUseClientPrediction && !bIsProxyMove;
  const bool bShouldRollBackGenericServerActors = bRollBackGenericServerActors && bUseClientPrediction && !bIsProxyMove;

  TArray<AGMC_Pawn*> RollbackPawnList{};
  if (bShouldRollBackServerPawns)
  {
    RollbackPawnList = GatherRollbackPawns();
  }

  TArray<AGMC_RollbackActor*> GenericRollbackActorList{};
  if (bShouldRollBackGenericServerActors)
  {
    GenericRollbackActorList = GatherGenericRollbackActors();
    BufferGenericRollbackActorStates(GenericRollbackActorList, EGMC_NetContext::RemoteServerPawn_PreMoveProcessing);
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_PreRemoteMovesProcessing);

  // Move validation is only relevant for predicted clients with actual move data.
  const bool bShouldValidate = bUseClientPrediction && !bIsProxyMove;
  gmc_ck(IsUsingServerAuthPhysicsReplication() ? !bUseClientPrediction : true)

  // Move execution happens based on the settings of the owning client, SP move data is transformed to relative values after the move was processed if required.
  const bool bUseRelative = bUseClientPrediction ? bUseRelativeValuesForPrediction : bUseRelativeValuesForSimulation;

  for (int32 Index = 0; Index < ClientMoves.Num(); ++Index)
  {
    auto& ClientMove = ClientMoves[Index];

    SV_RemoteMoveExecutionAux.LastRawMove = ClientMove;
    SV_RemoteMoveExecutionAux.bLastClientMoveWasValid = true;

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_PreRemoteMoveExecution, ClientMove);

    // The input state must be saved every loop iteration because the output of the current move will be the input for the next move.
    if (bShouldValidate)
    {
      gmc_ck(SV_RemoteMoveExecutionAux.bLastClientMoveWasValid)
      gmc_ck(bUseClientPrediction)
      SV_ApplyValidatedState(
        ClientMove.InputState,
        bAssumeClientState,
        SV_RemoteMoveExecutionAux.bLastClientMoveWasValid,
        bUseRelative,
        EGMC_NetContext::RemoteServerPawn_PreMoveExecution
      );
      GMC_CLOG(
        !SV_RemoteMoveExecutionAux.bLastClientMoveWasValid,
        LogGMCReplication,
        PawnOwner,
        Verbose,
        TEXT("One or more values of the client input state failed server-side validation (timestamp = %f)."),
        ClientMove.MetaData.Timestamp
      )
    }
    else if (!bUseClientPrediction && !bIsProxyMove)
    {
      GMCReplication::FDataOpDirective Directive{DataOp::AllDataOps};
      Directive.SetSaveFilters(DataFilter::SV_UseClientValue, DataFilterMode::Inclusive);
      Directive.SetQuantizeFilters(DataFilter::SV_UseClientValue, DataFilterMode::Inclusive);
      Directive.SetApplyFilters(DataFilter::None, DataFilterMode::Inclusive);
      ProcessSyncData(ClientMove.InputState, Directive, AliasData, bUseRelative, this);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, ClientMove.InputState, EGMC_NetContext::RemoteServerPawn_PreMoveExecution);
    }
    else
    {
      gmc_ck(bIsProxyMove)
      // The input for the proxy move is already fully set up at this point.
      ProcessSyncData(ClientMove.InputState, {DataOp::Apply}, AliasData, bUseRelative, this);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, ClientMove.InputState, EGMC_NetContext::RemoteServerPawn_PreMoveExecution);
    }

    if (bShouldRollBackServerPawns)
    {
      RollBackPawns(ClientMove.MetaData.Timestamp, RollbackPawnList, EGMC_NetContext::RemoteServerPawn_PreMoveExecution);
    }

    if (bShouldRollBackGenericServerActors)
    {
      RollbackGenericActors(
        ClientMove.MetaData.Timestamp,
        ClientMove.MetaData.DeltaTime,
        ClientMove,
        GenericRollbackActorList,
        EGMC_NetContext::RemoteServerPawn_PreMoveExecution
      );
    }

    ExecuteMove(
      ClientMove.InputState,
      ClientMove.MetaData.DeltaTime,
      ClientMove.MetaData.Timestamp,
      MaxTimeStep,
      MaxIterations,
      bUseRelative,
      false,
      false,
      false
    );

    AncillaryTick(ClientMove, ClientMove.MetaData.DeltaTime, ClientMove.MetaData.Timestamp, false, false, UNUSED(bool));

    DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_EXECUTED_MOVE

    if (bShouldValidate)
    {
      gmc_ck(bUseClientPrediction)
#if !NO_LOGGING
      const bool bInputStateValid = SV_RemoteMoveExecutionAux.bLastClientMoveWasValid;
#endif
      SV_ApplyValidatedState(
        ClientMove.OutputState,
        bAssumeClientState,
        SV_RemoteMoveExecutionAux.bLastClientMoveWasValid,
        bUseRelative,
        EGMC_NetContext::RemoteServerPawn_PostMoveExecution
      );
      GMC_CLOG(
        bInputStateValid && !SV_RemoteMoveExecutionAux.bLastClientMoveWasValid,
        LogGMCReplication,
        PawnOwner,
        Verbose,
        TEXT("One or more values of the client output state failed server-side validation (timestamp = %f)."),
        ClientMove.MetaData.Timestamp
      )
    }
    else if (!bUseClientPrediction && !bIsProxyMove)
    {
      GMCReplication::FDataOpDirective Directive{DataOp::AllDataOps};
      Directive.SetSaveFilters(DataFilter::SV_UseClientValue, DataFilterMode::Inclusive);
      Directive.SetQuantizeFilters(DataFilter::SV_UseClientValue, DataFilterMode::Inclusive);
      Directive.SetApplyFilters(DataFilter::None, DataFilterMode::Inclusive);
      ProcessSyncData(ClientMove.OutputState, Directive, AliasData, bUseRelative, this);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, ClientMove.OutputState, EGMC_NetContext::RemoteServerPawn_PostMoveExecution);
    }
    else
    {
      gmc_ck(bIsProxyMove)
      ProcessSyncData(ClientMove.OutputState, {DataOp::AllDataOps}, AliasData, bUseRelative, this);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, ClientMove.OutputState, EGMC_NetContext::RemoteServerPawn_PostMoveExecution);
    }

    DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_VALIDATED_MOVE

    gmc_ck(!bUseClientPrediction ? SV_RemoteMoveExecutionAux.bLastClientMoveWasValid : true)

    if (bIsProxyMove || bAlwaysReplay)
    {
      // Always force replication of correction data in this case.
      SV_RemoteMoveExecutionAux.bLastClientMoveWasValid = false;
    }

    const bool bIsUsingServerAuthPhysics = IsUsingServerAuthPhysicsReplication();
    const bool bIsUsingClientAuthPhysics = IsUsingClientAuthPhysicsReplication();

    gmc_ck(ClientMove.MetaData.Timestamp >= 0.)
    gmc_ck(ClientMove.MetaData.DeltaTime >= MIN_DELTA_TIME)
    gmc_ck(ClientMove.MetaData.ServerAuthPhysicsTimestamp == -1.)
    gmc_ck(!(bIsUsingServerAuthPhysics && bIsUsingClientAuthPhysics))
    gmc_ck(bIsUsingClientAuthPhysics ? bUseClientPrediction : true)
    ClientMove.MetaData.bIsUsingServerAuthPhysics = bIsUsingServerAuthPhysics;
    ClientMove.MetaData.bIsUsingClientAuthPhysics = bIsUsingClientAuthPhysics;
    ClientMove.MetaData.bPredictedClientMove = bUseClientPrediction;
    ClientMove.MetaData.bValidClientMove = SV_RemoteMoveExecutionAux.bLastClientMoveWasValid;
    ClientMove.MetaData.bPlayerOwned = true;

    if (bIsUsingServerAuthPhysics)
    {
      gmc_ck(!bUseClientPrediction)

      // Since the physics are calculated on the server we need to use the server time for accurate simulation, but we still need the original timestamp for
      // identifying acknowledged moves on the client.
      ClientMove.MetaData.ServerAuthPhysicsTimestamp = GetTime();
    }

    ClientMove.CopyDataValuesTo(APMove(), this);
    ClientMove.CopyMetaDataTo(APMove(), this);

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_OnAPMoveSaved, APMove());

    DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_SAVED_APMOVE

    if (ShouldAddToSimulationHistory(ClientMove.MetaData.Timestamp))
    {
      ClientMove.CopyDataValuesTo(SPMove(), this);
      ClientMove.CopyMetaDataTo(SPMove(), this);

      // Not relevant for simulated proxy data.
      SPMove().MetaData.bPredictedClientMove = false;
      SPMove().MetaData.bValidClientMove = false;

      // The move execution happened based on the configuration of the client-owned pawn so we need to convert SP move data to relative values in some cases.
      if (bUseClientPrediction && !bUseRelativeValuesForPrediction && bUseRelativeValuesForSimulation)
      {
        ToRelativeSyncData(SPMove().InputState, SPMove().InputState.ActorBase.Read(), DataFilter::None, DataFilterMode::Inclusive, this);
        ToRelativeSyncData(SPMove().OutputState, SPMove().OutputState.ActorBase.Read(), DataFilter::None, DataFilterMode::Inclusive, this);
      }

      SV_CheckForcedNetUpdate(SPMove(), APMove().MetaData.bValidClientMove);

      AddToSimulationHistory(SPMove());

      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_OnSPMoveSaved, SPMove());
    }

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_PostRemoteMoveExecution, ClientMove);
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_OnRemoteMovesProcessed);

  if (bShouldRollBackServerPawns)
  {
    // Restore the states of all pawns that were rolled back for move execution.
    RestoreRolledBackPawns(RollbackPawnList, EGMC_NetContext::RemoteServerPawn_PostMoveProcessing);
  }

  if (bShouldRollBackGenericServerActors)
  {
    // Restore the states of all generic actors that were rolled back for move execution.
    RestoreRolledBackGenericActors(GenericRollbackActorList, EGMC_NetContext::RemoteServerPawn_PostMoveProcessing);
  }

  SV_RemoteMoveExecutionAux.bIsExecutingRemoteMoves = false;

  if (IsSmoothedListenServerPawn())
  {
    gmc_ck(SV_RemoteServerPawnSmoothingSwapBuffer.bInitialized)
    SV_RemoteServerPawnSmoothingSwapBuffer.Swap(false, this, EGMC_NetContext::RemoteServerPawn_Smoothing);
    SV_OnSwapRemoteServerPawnSmoothingBuffer(SV_RemoteServerPawnSmoothingSwapBuffer.Buffer, EGMC_NetContext::RemoteServerPawn_Smoothing);
    gmc_ck(SV_RemoteServerPawnSmoothingSwapBuffer.HasEvenNumberOfSwaps())
  }
}

bool UGMC_ReplicationCmp::SV_CheckForcedNetUpdate(const FGMC_Move& Move, bool bValidMove)
{
  gmc_ck(PawnOwner->GetLocalRole() == ROLE_Authority)

  if (SimLocationUpdateThreshold >= 0.)
  {
    if (!bValidMove && PawnOwner->NetUpdateFrequency <= 0.f)
    {
      // We have to force an update in this case, otherwise the client may never get corrected.
      PawnOwner->ForceNetUpdate();
      DEBUG_LOG_FORCED_NET_UPDATES_CLIENT_CORRECTION
      return true;
    }

    const FVector CurrentLocation = GetActorLocation_GMC();
    const double RepLocationDelta = FVector::Dist(CurrentLocation, SV_RemoteMoveExecutionAux.LastReplicatedLocation);
    if (RepLocationDelta > SimLocationUpdateThreshold)
    {
      PawnOwner->ForceNetUpdate();
      DEBUG_LOG_FORCED_NET_UPDATES_SIM_LOCATION_THRESHOLD
      return true;
    }
  }

  const int32 LastIdx = MoveHistory.Num() - 1;
  if (LastIdx >= 0)
  {
    const auto& PreviousMove = MoveHistory[LastIdx];

    if (
      CheckSyncDataForcedNetUpdate(Move.OutputState, PreviousMove.OutputState, this) ||
      CheckSyncDataForcedNetUpdate(Move.InputState, PreviousMove.InputState, this)
    )
    {
      PawnOwner->ForceNetUpdate();
      DEBUG_LOG_FORCED_NET_UPDATES_SYNC_DATA
      return true;
    }

    if (CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_CheckForcedNetUpdate_Custom, Move, PreviousMove, bValidMove))
    {
      PawnOwner->ForceNetUpdate();
      DEBUG_LOG_FORCED_NET_UPDATES_CUSTOM
      return true;
    }
  }

  return false;
}

void UGMC_ReplicationCmp::SV_ApplyValidatedState(
  FGMC_PawnState& State,
  bool bInAssumeClientState,
  bool& bInOutValidState,
  bool bUseRelative,
  EGMC_NetContext Context
)
{
  // "CL_SendToServer" must be set for any data that has the "SV_UseClientValue" or "SV_Validate" flag (which must be mutually exclusive).

  ProcessSyncData(
    State,
    {DataOp::Save | DataOp::Quantize, DataFilter::SV_UseClientValue | DataFilter::SV_Validate, DataFilterMode::Inclusive},
    AliasData,
    bUseRelative,
    this
  );

  bool bDebugForceFullSyncDataValidation = false;

#if ALLOW_CONSOLE && !NO_LOGGING

  bDebugForceFullSyncDataValidation = GMCCVars::ForceFullSyncDataValidation != 0;

#endif

  bInOutValidState = bInOutValidState && ValidateSyncDataAgainstCurrentServerState(
    State,
    DataFilter::SV_UseClientValue,
    DataFilterMode::Inclusive,
    AliasData,
    bUseRelative,
    UNUSED(EGMC_SyncType),
    UNUSED(int32),
    this,
    bDebugForceFullSyncDataValidation
  );

  DEBUG_SHOW_CLIENT_ERRORS

  if (!bInOutValidState || !bInAssumeClientState)
  {
    // Overwrite client data that was sent for validation (which we skipped before), either because the state failed validation or because be don't want to
    // assume the exact client state.
    ProcessSyncData(
      State,
      {DataOp::Save | DataOp::Quantize, DataFilter::SV_Validate, DataFilterMode::Exclusive},
      AliasData,
      bUseRelative,
      this
    );
  }

  ProcessSyncData(State, {DataOp::Apply}, AliasData, bUseRelative, this);
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, State, Context);
}

FGMC_Move UGMC_ReplicationCmp::SV_CreateProxyMove(double Timestamp, float DeltaTime)
{
  FGMC_NetInfo ProxyMoveNetInfo;
  ProxyMoveNetInfo.NetType = GMCReplication::ESimType::LocalMove;
  ProxyMoveNetInfo.OwningComponent = this;

  FGMC_MetaData ProxyMoveMetaData;
  ProxyMoveMetaData.Timestamp = Timestamp;
  ProxyMoveMetaData.DeltaTime = DeltaTime;
  ProxyMoveMetaData.bIsUsingServerAuthPhysics = IsUsingServerAuthPhysicsReplication();
  ProxyMoveMetaData.bIsUsingClientAuthPhysics = IsUsingClientAuthPhysicsReplication();
  ProxyMoveMetaData.bPredictedClientMove = bUseClientPrediction;

  FGMC_Move ProxyMove{};
  InitializeSyncData(
    ProxyMove.InputState,
    ReplicationSettings,
    GetSyncTagsData(),
    AliasData,
    GMCReplication::ESimState::Input,
    GMCReplication::ESimType::LocalMove,
    this
  );
  InitializeSyncData(
    ProxyMove.OutputState,
    ReplicationSettings,
    GetSyncTagsData(),
    AliasData,
    GMCReplication::ESimState::Output,
    GMCReplication::ESimType::LocalMove,
    this
  );
  ProxyMove.NetInfo = ProxyMoveNetInfo;
  ProxyMove.MetaData = ProxyMoveMetaData;

  const bool bUseRelative = bUseClientPrediction ? bUseRelativeValuesForPrediction : bUseRelativeValuesForSimulation;
  ProcessSyncData(ProxyMove.InputState, {DataOp::Save | DataOp::Quantize}, AliasData, bUseRelative, this);

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, SV_OnProxyMoveInitialized, ProxyMove, ProxyMove.MetaData.DeltaTime, ProxyMove.MetaData.Timestamp);

  return ProxyMove;
}

bool UGMC_ReplicationCmp::SV_VerifyTimestamps(const TArray<FGMC_Move>& ClientMoves)
{
  SCOPE_CYCLE_COUNTER(STAT_SV_VerifyTimestamps)

  gmc_ck(ClientMoves.Num() > 0)

  if (GetClientSendRate() <= 0)
  {
    return false;
  }

  const auto& Controller = Cast<AGMC_PlayerController>(PawnOwner->GetController());
  if (!IsValid(Controller))
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      VeryVerbose,
      TEXT("Verification failed, pawn has no player controller.")
    )
    return false;
  }

  // The difference between the timestamps of the newest and oldest received move should not be greater than the max move delta time (the max move delta time
  // will never be lower than the client send interval, we check this when beginning play).
  if (ClientMoves[0].MetaData.Timestamp < ClientMoves.Last().MetaData.Timestamp - MaxMoveDeltaTime)
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      VeryVerbose,
      TEXT("Verification failed, the oldest move timestamp (%f s) was lower than the min expected time (%f s)."),
      ClientMoves[0].MetaData.Timestamp,
      ClientMoves.Last().MetaData.Timestamp - MaxMoveDeltaTime
    )
    return false;
  }

  // The accumulated delta time of all client moves should not be greater than the max move delta time.
  double TimestampPreviousMove{0.};
  float CalculatedDeltaTime{0.f};
  float AccumulatedClientDeltaTime{0.f};
  for (int32 Index = 0; Index < ClientMoves.Num(); ++Index)
  {
    // The accumulation buffer holds a certain number of past received timestamps.
    SV_TimestampVerificationAux.TimestampBuffer.Add(ClientMoves[Index].MetaData.Timestamp);

    if (Index == 0)
    {
      // We can't really rely on a timestamp from the previous batch for verification so we skip the delta time of the first move. This will make the
      // accumulated delta time a little less than it should be but it's better than not checking at all.
      TimestampPreviousMove = ClientMoves[Index].MetaData.Timestamp;
      continue;
    }
    if (ClientMoves[Index].MetaData.Timestamp <= TimestampPreviousMove)
    {
      GMC_LOG(
        LogGMCReplication,
        PawnOwner,
        VeryVerbose,
        TEXT("Verification failed, received batch of client moves had inconsistent timestamps.")
      )
      return false;
    }
    CalculatedDeltaTime = ClientMoves[Index].MetaData.Timestamp - TimestampPreviousMove;
    TimestampPreviousMove = ClientMoves[Index].MetaData.Timestamp;
    AccumulatedClientDeltaTime += CalculatedDeltaTime;
  }

  if (AccumulatedClientDeltaTime > MaxMoveDeltaTime)
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      VeryVerbose,
      TEXT("Verification failed, the received batch of client moves had an accumulated delta time (%f s) greater than \"%s\" (%f s)."),
      AccumulatedClientDeltaTime,
      TO_STR(MaxMoveDeltaTime),
      MaxMoveDeltaTime
    )
    return false;
  }

  // We calculate the client time based on the client's ping. This is a very rough estimation that depends on a lot of variable factors (first and foremost on
  // the amount of packet loss and jitter in the client connection).
  const double ServerTime = GetTime();
  const float EstimatedClientLatency = Controller->GetPingInMilliseconds() / 1000.f / 2.f;
  const float EstimatedSendDelay = 1.f / GetClientSendRate();
  const float TotalEstimatedClientDelay = EstimatedSendDelay + EstimatedClientLatency;

  if (SV_TimestampVerificationAux.VERIFY_WORLD_TIME)
  {
    double EstimatedClientWorldTime{0.};
    if (SV_TimestampVerificationAux.ComputeAccumulatedClientTime(EstimatedClientWorldTime))
    {
      EstimatedClientWorldTime += TotalEstimatedClientDelay;
      SV_TimestampVerificationAux.TimeDiffBuffer.Add(ServerTime - EstimatedClientWorldTime);
    }
    else
    {
      GMC_LOG(
        LogGMCReplication,
        PawnOwner,
        VeryVerbose,
        TEXT("Verification failed, entries in the timestamp buffer are inconsistent.")
      )
      return false;
    }
  }
  else
  {
    const double EstimatedTimestampLatestMove = ServerTime - TotalEstimatedClientDelay;
    SV_TimestampVerificationAux.TimeDiffBuffer.Add(EstimatedTimestampLatestMove - ClientMoves.Last().MetaData.Timestamp);
  }

  const double AvgDiff = SV_TimestampVerificationAux.TimeDiffBuffer.GetMean();

  if (AvgDiff > MaxAllowedTimeDeviation)
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      VeryVerbose,
      TEXT("Verification failed, the estimated client time differs by more than \"%s\" (%f): ")
      TEXT("avg diff = %f [ (-) client ahead <~> server ahead (+) ]"),
      TO_STR(MaxAllowedTimeDeviation),
      MaxAllowedTimeDeviation,
      AvgDiff
    )
    return false;
  }

  // The client data passed all verification checks.
  return true;
}

void UGMC_ReplicationCmp::SV_ResetClientCredibility()
{
  SV_TimestampVerificationAux.ResetClientCredibility_Impl(this);
}

FGMC_PawnState& UGMC_ReplicationCmp::SV_RemoteServerPawnSmoothingBuffer()
{
  return SV_RemoteServerPawnSmoothingSwapBuffer.Buffer;
}

const FGMC_PawnState& UGMC_ReplicationCmp::SV_RemoteServerPawnSmoothingBuffer() const
{
  return SV_RemoteServerPawnSmoothingSwapBuffer.Buffer;
}

void UGMC_ReplicationCmp::SV_SwapServerState()
{
  if (IsSmoothedListenServerPawn() && SV_RemoteServerPawnSmoothingSwapBuffer.bInitialized)
  {
    SV_RemoteServerPawnSmoothingSwapBuffer.Swap(false, this, EGMC_NetContext::NoContextInformation);
  }
}

void UGMC_ReplicationCmp::SV_UpdateLocalAdaptiveDelay()
{
  gmc_ck(IsSmoothedListenServerPawn())
  gmc_ck(InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay)

  const auto& ClientController = Cast<AGMC_PlayerController>(PawnOwner->GetController());
  if (!ClientController)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Error, TEXT("All player controllers must be of type \"%s\"."), TO_STR(AGMC_PlayerController))
    gmc_ckne()
    return;
  }

  // For the local simulation delay on a listen server we only need to consider the latency of the controlling client.
  const float PlayerPing = ClientController->GetPingInMilliseconds() / 1000.f;
  const float EstimatedPlayerLatency = PlayerPing / 2.f;
  float ProposedDelay = EstimatedPlayerLatency;

  bool bAppliedNewDynamicBufferTime = false;

  if (!AdaptiveDelayParams.bUseDynamicBufferTime || !DynamicBufferTimeAux.HasPendingBufferTime())
  {
    ProposedDelay += AdaptiveDelayParams.BufferTime;
  }
  else
  {
    gmc_ck(DynamicBufferTimeAux.RequestedBufferTime >= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
    gmc_ck(DynamicBufferTimeAux.RequestedBufferTime <= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)
    ProposedDelay += DynamicBufferTimeAux.RequestedBufferTime;
    bAppliedNewDynamicBufferTime = true;
  }

  ProposedDelay = FMath::Clamp(ProposedDelay, AdaptiveDelayParams.MIN_TOTAL_DELAY, AdaptiveDelayParams.MAX_TOTAL_DELAY);

  // We could just set the simulation delay directly every frame, but to prevent the value from changing too frequently and to get consistent behaviour for all
  // simulated pawns we'll use the same mechanism that we use for the client pawns here.
  if (
    !bAppliedNewDynamicBufferTime &&
    AdaptiveDelayParams.CurrentDelay > 0.f &&
    AdaptiveDelayParams.SV_SmoothedRemoteServerPawnUpdateTimer < AdaptiveDelayParams.SV_GetInternalSyncInterval()
  )
  {
    if (FMath::IsNearlyEqual(ProposedDelay, AdaptiveDelayParams.CurrentDelay, AdaptiveDelayParams.Tolerance))
    {
      return;
    }
  }

  AdaptiveDelayParams.CurrentDelay = ProposedDelay;
  GMCCompression::Round(AdaptiveDelayParams.CurrentDelay, FGMC_AdaptiveDelayPersistentParams::DELAY_VALUE_COMPRESSION);

  if (bAppliedNewDynamicBufferTime)
  {
    AdaptiveDelayParams.BufferTime = DynamicBufferTimeAux.RequestedBufferTime;
    GMCCompression::Round(AdaptiveDelayParams.BufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION);
    DynamicBufferTimeAux.Apply(this);
  }

  AdaptiveDelayParams.SV_SmoothedRemoteServerPawnUpdateTimer = 0.f;
}

void UGMC_ReplicationCmp::SV_UpdateClientAdaptiveDelays()
{
  gmc_ck(IsServerPawn())
  gmc_ck(InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay)

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  for (auto PCIterator = World->GetPlayerControllerIterator(); PCIterator; ++PCIterator)
  {
    gmc_ck(PCIterator->IsValid())
    static_assert(FGMC_AdaptiveDelayPersistentParams::NUM_SAVED_PARAMS > 1);

    const auto& ClientController = Cast<AGMC_PlayerController>(*PCIterator);
    if (!IsValid(ClientController) || ClientController->IsLocalController())
    {
      // We are only interested in the client controllers.
      continue;
    }

    // The controller can be null.
    const auto& PawnController = PawnOwner->GetController();

    const bool bIsClientControllerPawn = ClientController == PawnController;
    if (bIsClientControllerPawn && bUseClientPrediction)
    {
      // Predicted clients don't use a simulation delay.
      continue;
    }

    // We just leave controllers that are not valid anymore in the map. Deleting them shouldn't be necessary as the client pawn is usually destroyed with the
    // client controller.
    auto& ClientData = AdaptiveDelayParams.SV_PerClientParams.FindOrAdd(ClientController);
    auto& ClientParameters = ClientData.Parameters;
    auto& NewestClientParameters = ClientParameters[AdaptiveDelayParams.NUM_SAVED_PARAMS - 1];

    // Never skip on the first run (when no delay value has been set yet) and always re-sync when a full update interval has passed.
    const bool bIsTimeToUpdate = NewestClientParameters.DelayValue <= 0.f ||  ClientData.UpdateTimer >= AdaptiveDelayParams.SV_GetInternalSyncInterval();

    if (ClientController->bCullNonPlayerServerPawnParams && !IsPlayerControlledPawn())
    {
      FGMC_AdaptiveDelayServerPacket NonPlayerPawnParams{};
      if (!ClientController->SV_ShouldEnqueueNonPlayerPawnAdaptiveDelay(this, NonPlayerPawnParams))
      {
        const bool bNonPlayerPawnParamsValid = static_cast<bool>(NonPlayerPawnParams.TargetComponent);

        if (!bNonPlayerPawnParamsValid || !bIsTimeToUpdate)
        {
          return;
        }

        // Copy the parameters from the reference non-player pawn.

        FMemory::Memmove(
          &ClientParameters[0], &ClientParameters[1], sizeof(FGMC_AdaptiveDelayPersistentParams::FAux) * (AdaptiveDelayParams.NUM_SAVED_PARAMS - 1)
        );

        NewestClientParameters.DelayValue = NonPlayerPawnParams.DelayValue;
        NewestClientParameters.DynamicBufferTime = NonPlayerPawnParams.DynamicBufferTime;
        NewestClientParameters.SyncTime = NonPlayerPawnParams.SyncTime;

        ClientData.RequestedDynamicBufferTime = 0.f;
        ClientData.AppliedDynamicBufferTime = 0.f;
        ClientData.UpdateTimer = 0.f;

        return;
      }
    }

    const double CurrentTime = GetTime();
    if (CurrentTime < NewestClientParameters.SyncTime)
    {
      // We don't bother calculating a new simulation delay when the current one hasn't even been committed yet.
      continue;
    }

    const float PlayerPing = ClientController->GetPingInMilliseconds() / 1000.f;
    const float EstimatedPlayerLatency = PlayerPing / 2.f;

    // For locally controlled server pawns we only need to consider the time a packet takes to get from the server to the client. This may
    // be a listen server player or an AI-controlled pawn.
    float ProposedDelay = EstimatedPlayerLatency;
    if (bIsClientControllerPawn)
    {
      // If this is the pawn of the current client controller we also need to consider the way to the server from the client.
      ProposedDelay += EstimatedPlayerLatency;
    }
    else if (IsRemotelyControlledServerPawn())
    {
      // If this is another client pawn we need to consider the latency of the other client.
      if (const auto& PawnGenController = Cast<AGMC_PlayerController>(PawnController))
      {
        const float OtherPlayerPing = PawnGenController->GetPingInMilliseconds() / 1000.f;
        ProposedDelay += OtherPlayerPing / 2.f;
      }
    }
    else
    {
      gmc_ck(!PawnController || PawnController->IsLocalController())
    }

    bool bAppliedNewDynamicBufferTime = false;

    // Add the buffer time to the proposed delay. This may be a value requested by the client when using a dynamic buffer time.
    if (!AdaptiveDelayParams.bUseDynamicBufferTime)
    {
      ProposedDelay += AdaptiveDelayParams.BufferTime;

      // Reset in case the option was enabled before.
      ClientData.RequestedDynamicBufferTime = 0.f;
      ClientData.AppliedDynamicBufferTime = 0.f;

      gmc_ck(!bAppliedNewDynamicBufferTime)
    }
    else
    {
      if (ClientData.RequestedDynamicBufferTime != 0.f)
      {
        gmc_ck(ClientData.RequestedDynamicBufferTime >= AdaptiveDelayParams.MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
        gmc_ck(ClientData.RequestedDynamicBufferTime <= AdaptiveDelayParams.MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)

        // A new buffer time value was requested by the client.
        ProposedDelay += ClientData.RequestedDynamicBufferTime;
        ClientData.AppliedDynamicBufferTime = ClientData.RequestedDynamicBufferTime;
        bAppliedNewDynamicBufferTime = true;
      }
      else
      {
        // No new buffer time value was requested by the client.
        gmc_ck(ClientData.RequestedDynamicBufferTime == 0.f)

        if (ClientData.AppliedDynamicBufferTime != 0.f)
        {
          gmc_ck(ClientData.AppliedDynamicBufferTime >= AdaptiveDelayParams.MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
          gmc_ck(ClientData.AppliedDynamicBufferTime <= AdaptiveDelayParams.MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)

          // If a requested buffer time value was previously applied keep using that one.
          ProposedDelay += ClientData.AppliedDynamicBufferTime;
        }
        else
        {
          gmc_ck(ClientData.AppliedDynamicBufferTime == 0.f)

          // If no requested buffer time value was previously applied yet use the default one.
          ProposedDelay += AdaptiveDelayParams.BufferTime;
        }

        gmc_ck(!bAppliedNewDynamicBufferTime)
      }
    }

    ProposedDelay = FMath::Clamp(ProposedDelay, AdaptiveDelayParams.MIN_TOTAL_DELAY, AdaptiveDelayParams.MAX_TOTAL_DELAY);

    if (!bAppliedNewDynamicBufferTime /*always inform the client that the requested buffer time was applied*/ && !bIsTimeToUpdate)
    {
      if (FMath::IsNearlyEqual(ProposedDelay, NewestClientParameters.DelayValue, AdaptiveDelayParams.Tolerance))
      {
        // The proposed delay is still within the configured tolerance.
        continue;
      }
    }

    // Shift the whole array to the left to enqueue the new values. This will discard the oldest parameters.
    FMemory::Memmove(&ClientParameters[0], &ClientParameters[1], sizeof(FGMC_AdaptiveDelayPersistentParams::FAux) * (AdaptiveDelayParams.NUM_SAVED_PARAMS - 1));

    NewestClientParameters.DelayValue = ProposedDelay;
    GMCCompression::Round(NewestClientParameters.DelayValue, FGMC_AdaptiveDelayPersistentParams::DELAY_VALUE_COMPRESSION);

    if (bAppliedNewDynamicBufferTime)
    {
      gmc_ck(AdaptiveDelayParams.bUseDynamicBufferTime)
      gmc_ck(ClientData.RequestedDynamicBufferTime >= AdaptiveDelayParams.MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
      gmc_ck(ClientData.RequestedDynamicBufferTime <= AdaptiveDelayParams.MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)

      NewestClientParameters.DynamicBufferTime = ClientData.RequestedDynamicBufferTime;
      GMCCompression::Round(NewestClientParameters.DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION);

      // Set the requested buffer time to 0 again to indicate to the client (with the next update) that the value was processed and to keep bandwidth usage
      // minimal. This will remain 0 until the client requests another value.
      ClientData.RequestedDynamicBufferTime = 0.f;
    }
    else
    {
      gmc_ck(ClientData.RequestedDynamicBufferTime == 0.f)
      NewestClientParameters.DynamicBufferTime = 0.f;
    }

    // We can't immediately set the new delay because then the values would deviate while the packet is in transit to the client. Instead we choose a time in
    // the future at which the client has almost certainly received the packet to set the new delay. The tolerance is supposed to account for additional
    // processing times, possible replication retries and client world time discrepancy.
    constexpr double Tolerance = 0.6;
    NewestClientParameters.SyncTime = CurrentTime + PlayerPing / 2. + Tolerance;
    GMCCompression::Round(NewestClientParameters.SyncTime, FGMC_AdaptiveDelayPersistentParams::SYNC_TIME_COMPRESSION);

    // Enqueue the new parameters to have them transmitted to the client on the next tick.
    ClientController->SV_EnqueueAdaptiveDelay(
      this,
      FGMC_AdaptiveDelayServerPacket{
        this,
        NewestClientParameters.DelayValue,
        NewestClientParameters.DynamicBufferTime,
        NewestClientParameters.SyncTime
      }
    );

    ClientData.UpdateTimer = 0.f;
  }
}

float UGMC_ReplicationCmp::GetAdaptiveDelayAt(double Time, APlayerController* ClientController)
{
  if (!PawnOwner || (!IsSimulatedProxy() && !IsValid(ClientController)))
  {
    return 0.f;
  }

  const auto& ClientParameters =
    PawnOwner->GetLocalRole() == ROLE_Authority ?
    AdaptiveDelayParams.SV_PerClientParams.FindOrAdd(ClientController).Parameters :
    AdaptiveDelayParams.CL_LocalParams;

  for (int32 Index = AdaptiveDelayParams.NUM_SAVED_PARAMS - 1; Index >= 0; --Index)
  {
    if (Time >= ClientParameters[Index].SyncTime)
    {
      return ClientParameters[Index].DelayValue;
    }
  }

  GMC_LOG(LogGMCReplication, PawnOwner, Verbose, TEXT("No adaptive delay value found for time %f (possibly too few saved entries)."), Time)

  return 0.f;
}

float UGMC_ReplicationCmp::GetDynamicBufferTimeAt(double Time, APlayerController* ClientController)
{
  if (!PawnOwner || (!IsSimulatedProxy() && !IsValid(ClientController)))
  {
    return 0.f;
  }

  const auto& ClientParameters =
    PawnOwner->GetLocalRole() == ROLE_Authority ?
    AdaptiveDelayParams.SV_PerClientParams.FindOrAdd(ClientController).Parameters :
    AdaptiveDelayParams.CL_LocalParams;

  for (int32 Index = AdaptiveDelayParams.NUM_SAVED_PARAMS - 1; Index >= 0; --Index)
  {
    if (Time >= ClientParameters[Index].SyncTime)
    {
      return ClientParameters[Index].DynamicBufferTime;
    }
  }

  GMC_LOG(LogGMCReplication, PawnOwner, Verbose, TEXT("No dynamic buffer time value found for time %f (possibly too few saved entries)."), Time)

  return 0.f;
}

bool UGMC_ReplicationCmp::SV_ProcessProxyMove()
{
  SCOPE_CYCLE_COUNTER(STAT_SV_ProcessProxyMove)

  gmc_ck(ServerMinUpdateRate > 0)

  // Remotely controlled server pawns should maintain a certain min update rate so they don't completely "freeze" in case of a temporary connection issue.

  if (SV_RemoteMoveExecutionAux.LastRemotePawnUpdateTime == 0.)
  {
    // The remote pawn must have been updated at least once before a meaningful delta time value can be calculated for a proxy move.
    return false;
  }

  gmc_ck(SV_RemoteMoveExecutionAux.LastRemotePawnUpdateTime > 0.)

  const double CurrentTime = GetTime();
  const double ElapsedTime = CurrentTime - SV_RemoteMoveExecutionAux.LastRemotePawnUpdateTime;
  const float MinUpdateInterval = 1.f / ServerMinUpdateRate;
  if (ElapsedTime >= MinUpdateInterval)
  {
    // The proxy move is a "fake" client move which will be executed instead of a real one because we haven't received any move data from the client within the
    // required time frame (based on the configured min update rate). This will ensure that if one client experiences a lag spike the game can continue
    // relatively smoothly for the other connected players. It is also possible to produce such lag spikes artificially (lag switching) so this functionality
    // also increases security.
    TArray<FGMC_Move> ProxyMove{};

    // The timestamp and delta time of the proxy move is based off the elapsed time since the last update (which is the best approximation we can make).
    double ProxyMoveTimestamp = MoveHistory.Num() > 0 ? MoveHistory.Last().MetaData.Timestamp + ElapsedTime : CurrentTime;
    float ProxyMoveDeltaTime = FMath::Clamp(ElapsedTime, (double)MIN_DELTA_TIME, UE_DOUBLE_BIG_NUMBER); // Do not clamp to the max move delta time here.
    ProxyMove.Emplace(SV_CreateProxyMove(ProxyMoveTimestamp, ProxyMoveDeltaTime));

    SV_ExecuteClientMoves(ProxyMove, true);

    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("The server executed a proxy move because the client did not meet the configured min update rate: ")
      TEXT("%s = %d (interval: %f s), time since last remote move execution = %f s"),
      TO_STR(ServerMinUpdateRate),
      ServerMinUpdateRate,
      MinUpdateInterval,
      ElapsedTime
    )

    return true;
  }

  return false;
}

void UGMC_ReplicationCmp::SV_UpdateAdaptiveDelayBufferTime(APlayerController* ClientController, float NewBufferTime)
{
  const auto& GMCClientController = Cast<AGMC_PlayerController>(ClientController);

  if (!IsGMCEnabled() || !AdaptiveDelayParams.bUseDynamicBufferTime)
  {
    GMCClientController->SV_ResetNumRejectedBufferTimeRequests();
    return;
  }

  gmc_ck(NewBufferTime > 0.f)

  auto& ClientData = AdaptiveDelayParams.SV_PerClientParams.FindOrAdd(ClientController);

  gmc_ck(GMCClientController)

  // In rare cases the server and client may be out of sync regarding the last applied dynamic buffer time (e.g. if a delay value was not sent due to too many
  // pending reliable RPCs). If this happens, the client may continuously request a new buffer time, which is then repeatedly rejected by the server. Therefore,
  // we limit the amount of times that the server can reject the request to give the client a chance a get back in sync if necessary.
  if (GMCClientController->SV_CanRejectClientBufferTimeRequest())
  {
    if (
      FMath::IsNearlyEqual(
        ClientData.RequestedDynamicBufferTime, NewBufferTime, 1.f / (int32)FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION
      ) ||
      FMath::IsNearlyEqual(
        ClientData.AppliedDynamicBufferTime, NewBufferTime, 1.f / (int32)FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION
      )
    )
    {
      // The requested buffer time was probably already received and/or applied.

      GMCClientController->SV_IncrementNumRejectedBufferTimeRequests();

      GMC_LOG(
        LogGMCReplication,
        PawnOwner,
        VeryVerbose,
        TEXT("Client buffer time request rejected (requested = %f, pending = %f, applied = %f)."),
        NewBufferTime,
        ClientData.RequestedDynamicBufferTime,
        ClientData.AppliedDynamicBufferTime
      )

      return;
    }
  }
  else
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Client exceeded the maximum amount of allowed buffer time rejections (requested = %f, pending = %f, applied = %f)."),
      NewBufferTime,
      ClientData.RequestedDynamicBufferTime,
      ClientData.AppliedDynamicBufferTime
    )
  }

  GMCClientController->SV_ResetNumRejectedBufferTimeRequests();

  GMCCompression::Round(NewBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION);
  ClientData.RequestedDynamicBufferTime = NewBufferTime;

  // Force an update of the adaptive delay when a new buffer time was requested so the client can apply the change as quickly as possible.
  ClientData.UpdateTimer = AdaptiveDelayParams.SyncInterval + AdaptiveDelayParams.SYNC_INTERVAL_VARIANCE;
}

FVector UGMC_ReplicationCmp::CL_ExecuteMove(FGMC_Move& Move, bool bPredicted, bool bStartedNewMove)
{
  SCOPE_CYCLE_COUNTER(STAT_CL_ExecuteMove)

  FVector MovedDistance{0.};

  if (bPredicted)
  {
    ProcessSyncData(Move.InputState, {DataOp::Apply}, AliasData, bUseRelativeValuesForPrediction, this);
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, Move.InputState, EGMC_NetContext::LocalClientPawn_PreMoveExecution);
  }

  TickGenericRollbackActors(
    Move.MetaData.Timestamp,
    Move.MetaData.DeltaTime,
    Move,
    EGMC_NetContext::LocalClientPawn_PreMoveExecution
  );

  if (bPredicted)
  {
    const bool bIsCombinedMove = !bStartedNewMove;

    MovedDistance = ExecuteMove(
      Move.InputState,
      Move.MetaData.DeltaTime,
      Move.MetaData.Timestamp,
      MaxTimeStep,
      MaxIterations,
      bUseRelativeValuesForPrediction,
      true,
      false,
      bIsCombinedMove
    );

    CL_AncillaryTick(Move, true, bIsCombinedMove);

    ProcessSyncData(Move.OutputState, {DataOp::AllDataOps}, AliasData, bUseRelativeValuesForPrediction, this);
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, Move.OutputState, EGMC_NetContext::LocalClientPawn_PostMoveExecution);
  }
  else
  {
    ProcessSyncData(Move.OutputState, {DataOp::Save | DataOp::Quantize}, AliasData, bUseRelativeValuesForSimulation, this);
  }

  DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_EXECUTED_MOVE

  return MovedDistance;
}

void UGMC_ReplicationCmp::CL_AncillaryTick(FGMC_Move& Move, bool bLocalMove, bool bCombinedMove)
{
  bool bDoNotCombineNextMove = false;
  AncillaryTick(Move, Move.MetaData.DeltaTime, Move.MetaData.Timestamp, bLocalMove, bCombinedMove, bDoNotCombineNextMove);
  if (bDoNotCombineNextMove)
  {
    CL_DoNotCombineNextMove();
  }
}

void UGMC_ReplicationCmp::SV_NetReserializationTimer() const
{
  gmc_ck(IsServerPawn())

  SV_NetReserializationAux.ReserializationTimer_Impl(this);
}

void UGMC_ReplicationCmp::SV_EnableServerAuthPhysicsSimulation(bool bEnable, FGMC_ServerAuthPhysicsSettings Settings)
{
  if (!IsServerPawn())
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return;
  }

  if (IsUsingClientAuthPhysicsReplication())
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Warning,
      TEXT("Client-authoritative physics replication is already active, cannot enable server-authoritative physics replication.")
    )
    gmc_ckne()
    return;
  }

  if (IsUsingServerAuthPhysicsReplication() == bEnable)
  {
    return;
  }

  RootComponent->SetSimulatePhysics(bEnable);

  if (IsRemotelyControlledServerPawn())
  {
    CL_OnServerAuthPhysicsSimulationToggled(bEnable, Settings);
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnServerAuthPhysicsSimulationToggled, bEnable, Settings);
}

void UGMC_ReplicationCmp::SV_EnableClientAuthPhysicsSimulation(bool bEnable, FGMC_ClientAuthPhysicsSettings Settings)
{
  if (!IsRemotelyControlledServerPawn())
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return;
  }

  if (IsUsingServerAuthPhysicsReplication())
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Warning,
      TEXT("Server-authoritative physics replication is already active, cannot enable client-authoritative physics replication.")
    )
    gmc_ckne()
    return;
  }

  if (IsUsingClientAuthPhysicsReplication() == bEnable)
  {
    return;
  }

  // Server pawns do not simulate the physics, they just assume the received client transform.
  RootComponent->SetSimulatePhysics(false);

  CL_OnClientAuthPhysicsSimulationToggled(bEnable, Settings);

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnClientAuthPhysicsSimulationToggled, bEnable, Settings);
}

void UGMC_ReplicationCmp::CL_OnServerAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings)
{
  gmc_ck(IsAutonomousProxy())

  if (!IsValid(PawnOwner))
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return;
  }

  // Client pawns do not simulate the physics, they just display the replicated transform.
  RootComponent->SetSimulatePhysics(false);

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnServerAuthPhysicsSimulationToggled, bEnabled, Settings);
}

void UGMC_ReplicationCmp::CL_OnClientAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings)
{
  gmc_ck(IsAutonomousProxy())

  if (!IsValid(PawnOwner))
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return;
  }

  RootComponent->SetSimulatePhysics(bEnabled);

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnClientAuthPhysicsSimulationToggled, bEnabled, Settings);
}

bool UGMC_ReplicationCmp::CheckReliableBuffer(AActor* Owner, int32 ProtectionMargin, int32& OutNumPendingPackets)
{
  OutNumPendingPackets = 0;

  if (!Owner)
  {
    return false;
  }

  const auto& Connection = Owner->GetNetConnection();
  if (!Connection)
  {
    return false;
  }

  const auto& ActorChannel = Connection->FindActorChannelRef(Owner);
  if (!ActorChannel)
  {
    return false;
  }

  OutNumPendingPackets = ActorChannel->NumOutRec;

  // Check the number of out packets waiting to be acknowledged. The margin should not be too small so that other reliable client RPCs that may exist outside of
  // the replication component won't cause a disconnect.
  if (OutNumPendingPackets < (RELIABLE_BUFFER - 1) - FMath::Clamp(ProtectionMargin, 0, RELIABLE_BUFFER - 1))
  {
    return true;
  }

  return false;
}

FGMC_SyncTagsData UGMC_ReplicationCmp::GetSyncTagsData() const
{
  return FGMC_SyncTagsData{};
}

bool UGMC_ReplicationCmp::CL_MaintainPredictionHistory(const FGMC_Move& NewMove, bool& bOutStartedNewMove, bool& bOutPredictionHistoryFull)
{
  SCOPE_CYCLE_COUNTER(STAT_CL_MaintainPredictionHistory)

  gmc_ck(bUseClientPrediction)

  const int32 MoveHistoryNum = MoveHistory.Num();

  // A full prediction history doesn't necessarily mean that the new move will be discarded as it can still be combined with the last move.
  bOutPredictionHistoryFull = MoveHistoryNum == MoveHistoryMaxSize;

  auto& PreviousMove = MoveHistoryNum > 0 ? MoveHistory[MoveHistoryNum - 1] : CL_MoveExecutionAux.GetDefaultMove(this);

  bool bCombineMove = false;

  if ((bOutStartedNewMove = CL_ShouldEnqueueMove(NewMove, PreviousMove, bCombineMove)) == true)
  {
    gmc_ck(bCombineMove == false)
    return CL_AddToPredictionHistory(NewMove, bOutPredictionHistoryFull);
  }

  if (bCombineMove)
  {
    gmc_ck(MoveHistory.Num() > 0)
    gmc_ck(NewMove.MetaData.Timestamp > PreviousMove.MetaData.Timestamp)

    PreviousMove.MetaData.Timestamp = NewMove.MetaData.Timestamp;
    PreviousMove.MetaData.DeltaTime += NewMove.MetaData.DeltaTime;

    SetSyncDataCombinedMove(PreviousMove.InputState, NewMove.InputState, this);
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_SetSyncDataCombinedMove_Custom, PreviousMove.InputState, NewMove.InputState);

    return true;
  }

  return false;
}

bool UGMC_ReplicationCmp::CL_MaintainNonPredictedMoves(const FGMC_Move& NewMove, bool& bOutStartedNewMove)
{
  SCOPE_CYCLE_COUNTER(STAT_CL_MaintainNonPredictedMoves)

  gmc_ck(!bUseClientPrediction)

  auto& NonPredictedMoves = CL_MoveExecutionAux.NonPredictedMoves;
  const int32 NonPredictedMovesNum = NonPredictedMoves.Num();

  auto& PreviousMove = NonPredictedMovesNum > 0 ? NonPredictedMoves[NonPredictedMovesNum - 1] : CL_MoveExecutionAux.GetDefaultMove(this);

  bool bCombineMove = false;

  if ((bOutStartedNewMove = CL_ShouldEnqueueMove(NewMove, PreviousMove, bCombineMove)) == true)
  {
    gmc_ck(bCombineMove == false)
    gmc_ck(NonPredictedMoves.Num() < NUM_SAVED_NO_PREDICTION_MOVES)

    NonPredictedMoves.Emplace(NewMove);

    if (NonPredictedMoves.Num() == NUM_SAVED_NO_PREDICTION_MOVES)
    {
      NonPredictedMoves.RemoveAt(0, 1, false);
    }

    return true;
  }

  if (bCombineMove)
  {
    gmc_ck(NonPredictedMoves.Num() > 0)
    gmc_ck(NewMove.MetaData.Timestamp > PreviousMove.MetaData.Timestamp)

    PreviousMove.MetaData.Timestamp = NewMove.MetaData.Timestamp;
    PreviousMove.MetaData.DeltaTime += NewMove.MetaData.DeltaTime;

    SetSyncDataCombinedMove(PreviousMove.InputState, NewMove.InputState, this);
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_SetSyncDataCombinedMove_Custom, PreviousMove.InputState, NewMove.InputState);

    return true;
  }

  return false;
}

bool UGMC_ReplicationCmp::CL_ShouldEnqueueMove(const FGMC_Move& CurrentMove, const FGMC_Move& PreviousMove, bool& bOutCombineMove)
{
  SCOPE_CYCLE_COUNTER(STAT_CL_ShouldEnqueueMove)

  bOutCombineMove = false;

  int32 NumPendingPackets = 0;

  const bool bReliableBufferFull = !CheckReliableBuffer(PawnOwner, SEND_CLIENT_MOVES_OVERFLOW_PROTECTION, NumPendingPackets);

  ComponentStatus.CL_SetUseUnreliableClientMoves(NumPendingPackets, this);

  if (bReliableBufferFull)
  {
    // The reliable buffer is about to overflow, do not accept any more moves until the buffer has more capacity again.
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Client move was discarded to prevent a reliable buffer overflow (protection margin = %d)."),
      SEND_CLIENT_MOVES_OVERFLOW_PROTECTION
    )
    gmc_ck(!bOutCombineMove)
    return false;
  }

  if (!PreviousMove.HasValidTimestamp())
  {
    // The move history should never be empty except when just spawned, but it can still happen sometimes if the timestamps were inconsistent after a client
    // world time update.
    gmc_ck(bUseClientPrediction ? MoveHistory.Num() == 0 : CL_MoveExecutionAux.NonPredictedMoves.Num() == 0)
    gmc_ck(!bOutCombineMove)
    return true;
  }

  if (CurrentMove.MetaData.Timestamp <= PreviousMove.MetaData.Timestamp)
  {
    // If the move has an inconsistent timestamp we don't enqueue because the delta time of a move is calculated from the timestamp difference with the previous
    // move (which would end up being either 0 or negative in this case).
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      VeryVerbose,
      TEXT("Current client move has an inconsistent timestamp: timestamp current move = %f | timestamp previous move = %f"),
      CurrentMove.MetaData.Timestamp,
      PreviousMove.MetaData.Timestamp
    )
    GMC_LOG(LogGMCReplication, PawnOwner, Verbose, TEXT("Client move was discarded due to an inconsistent timestamp."))
    gmc_ck(!bOutCombineMove)
    return false;
  }

  if (bNoMoveCombining)
  {
    // Move combining is disabled.
    gmc_ck(!bOutCombineMove)
    return true;
  }

  if (CL_MoveExecutionAux.bDoNotCombineNextMove)
  {
    // The user forced this move to be enqueued.
    gmc_ck(!bOutCombineMove)
    return true;
  }

  const int32 HistoryNum = bUseClientPrediction ? MoveHistory.Num() : CL_MoveExecutionAux.NonPredictedMoves.Num();
  if (HistoryNum == 1)
  {
    // We should have at least 2 entries in the history for move execution. In very low latency situations it is possible that we frequently only have one move
    // in the history after moves were cleared so we enqueue another one now.
    gmc_ck(!bOutCombineMove)
    return true;
  }

  if (PreviousMove.MetaData.DeltaTime + CurrentMove.MetaData.DeltaTime > GetMaxCombinedDeltaTime())
  {
    // Enqueue if we would exceed the max combined delta time.
    gmc_ck(!bOutCombineMove)
    return true;
  }

  // Implementation specific checks. We continue with the default sync data checks if this returns 0.
  const int32 ResultCustom =
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_CheckSyncDataShouldCombineMove_Custom, CurrentMove.InputState, PreviousMove.InputState);
  if (ResultCustom < 0)
  {
    // Custom checking forced this move to be enqueued.
    gmc_ck(!bOutCombineMove)
    return true;
  }
  else if (ResultCustom > 0)
  {
    // Custom checking forced this move to be combined.
    bOutCombineMove = true;
    return false;
  }

  if (CheckSyncDataShouldCombineMove(CurrentMove.InputState, PreviousMove.InputState, this))
  {
    bOutCombineMove = true;
    return false;
  }

  gmc_ck(!bOutCombineMove)
  return true;
}

bool UGMC_ReplicationCmp::CL_ShouldSendMoves() const
{
  if (CL_MoveExecutionAux.PendingMoves.Num() == 0)
  {
    return false;
  }

  if (IsUsingUnreliableClientMoves())
  {
    // Get the client moves out as fast as possible when using unreliable RPCs.
    return true;
  }

  if (GetClientSendRate() <= 0)
  {
    return false;
  }

  if (CL_MoveExecutionAux.TimeSinceLastMoveBatchWasSent > 1.f / GetClientSendRate())
  {
    return true;
  }

  return false;
}

void UGMC_ReplicationCmp::CL_SendMoves()
{
  gmc_ck(GetGMCPawnOwner())

  if (IsUsingUnreliableClientMoves())
  {
    GetGMCPawnOwner()->SV_ReceiveMovesUnreliable(CL_MoveExecutionAux.PendingMoves);
  }
  else
  {
    GetGMCPawnOwner()->SV_ReceiveMovesReliable(CL_MoveExecutionAux.PendingMoves);
  }

  DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_SENT_MOVES
}

void UGMC_ReplicationCmp::CL_SendAdaptiveDelayParams(const FGMC_AdaptiveDelayServerPacket& NextDelay)
{
  static_assert(FGMC_AdaptiveDelayPersistentParams::NUM_SAVED_PARAMS > 1);
  gmc_ck(NextDelay.TargetComponent == this)

  // Shift the whole array to the left to enqueue the new values. This will discard the oldest parameters.
  FMemory::Memmove(
    &AdaptiveDelayParams.CL_LocalParams[0],
    &AdaptiveDelayParams.CL_LocalParams[1],
    sizeof(FGMC_AdaptiveDelayPersistentParams::FAux) * (AdaptiveDelayParams.NUM_SAVED_PARAMS - 1)
  );

  // Save the new values to the highest index.
  auto& NewestParameters = AdaptiveDelayParams.CL_LocalParams[AdaptiveDelayParams.NUM_SAVED_PARAMS - 1];
  NewestParameters.DelayValue = NextDelay.DelayValue;
  NewestParameters.DynamicBufferTime = NextDelay.DynamicBufferTime;
  NewestParameters.SyncTime = NextDelay.SyncTime;
  GMCCompression::Round(NewestParameters.DelayValue, FGMC_AdaptiveDelayPersistentParams::DELAY_VALUE_COMPRESSION);
  GMCCompression::Round(NewestParameters.DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION);
  GMCCompression::Round(NewestParameters.SyncTime, FGMC_AdaptiveDelayPersistentParams::SYNC_TIME_COMPRESSION);

  gmc_ck(NewestParameters.DynamicBufferTime != 0.f ? NewestParameters.DynamicBufferTime >= AdaptiveDelayParams.MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER : true)
  gmc_ck(NewestParameters.DynamicBufferTime != 0.f ? NewestParameters.DynamicBufferTime <= AdaptiveDelayParams.MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER : true)

  GMC_CLOG(
    NewestParameters.SyncTime < GetTime(),
    LogGMCReplication,
    PawnOwner,
    Verbose,
    TEXT("New adaptive delay parameters were received too late (sync time = %f, current time = %f)."),
    NewestParameters.SyncTime,
    GetTime()
  )
}

void UGMC_ReplicationCmp::CL_UpdateLocalAdaptiveDelay()
{
  gmc_ck(IsClientPawn())
  gmc_ck(InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay)
  static_assert(FGMC_AdaptiveDelayPersistentParams::NUM_SAVED_PARAMS > 1);

  const auto& NewestParameters = AdaptiveDelayParams.CL_LocalParams[AdaptiveDelayParams.NUM_SAVED_PARAMS - 1];
  if (GetTime() >= NewestParameters.SyncTime)
  {
    AdaptiveDelayParams.CurrentDelay = NewestParameters.DelayValue;

    if (NewestParameters.DynamicBufferTime != 0.f)
    {
      gmc_ck(NewestParameters.DynamicBufferTime >= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
      gmc_ck(NewestParameters.DynamicBufferTime <= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)

      // A value greater than zero means that the parameters contain a dynamic buffer time received from the server (which we previously requested).
      AdaptiveDelayParams.BufferTime = NewestParameters.DynamicBufferTime;

      const auto& GameInstance = PawnOwner->GetGameInstance();
      if (!GameInstance)
      {
        gmc_ckne()
        return;
      }

      const auto& LocalController = Cast<AGMC_PlayerController>(GameInstance->GetFirstLocalPlayerController());
      if (!LocalController)
      {
        gmc_ckne()
        return;
      }

      if (LocalController->bCullNonPlayerServerPawnParams && !IsPlayerControlledPawn())
      {
        if (const auto& RefPawn = Cast<AGMC_Pawn>(LocalController->GetRefNonPlayerPawn()))
        {
          if (GetPawnOwner() != RefPawn)
          {
            // If this is not the ref pawn we never requested a buffer time for this pawn to begin with, we need to check whether the ref pawn received the
            // requested buffer time.

            const auto& RefComponent = RefPawn->GetReplicationComponent();
            if (!RefComponent)
            {
              return;
            }

            const bool bRefPawnReceivedRequestedBufferTime = FMath::IsNearlyEqual(
              RefComponent->AdaptiveDelayParams.BufferTime,
              RefComponent->DynamicBufferTimeAux.RequestedBufferTime,
              1.f / (int32)FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION
            );

            if (bRefPawnReceivedRequestedBufferTime)
            {
              DynamicBufferTimeAux.Apply(this);

              DEBUG_LOG_DYNAMIC_BUFFER_TIME_APPLY
            }

            return;
          }
        }
      }

      const bool bReceivedRequestedBufferTime = FMath::IsNearlyEqual(
        AdaptiveDelayParams.BufferTime,
        DynamicBufferTimeAux.RequestedBufferTime,
        1.f / (int32)FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION
      );

      if (bReceivedRequestedBufferTime)
      {
        DynamicBufferTimeAux.Apply(this);

        DEBUG_LOG_DYNAMIC_BUFFER_TIME_APPLY
      }
    }
  }
}

void UGMC_ReplicationCmp::CL_ReplayMoves()
{
  SCOPE_CYCLE_COUNTER(STAT_CL_ReplayMoves)

  gmc_ck(IsAutonomousProxy())
  gmc_ck(bUseClientPrediction)
  gmc_ck(!CL_MoveExecutionAux.bIsReplaying)
  gmc_ck(CL_MoveExecutionAux.ReplayMoveIdx == -1)

  if (MoveHistory.Num() > 0)
  {
    DEBUG_LOG_REPLAY_CLIENT_STATE_BEFORE_REPLAY

    CL_MoveExecutionAux.bIsReplaying = true;

    TArray<AGMC_Pawn*> RollbackPawnList{};
    if (bRollBackClientPawns)
    {
      RollbackPawnList = GatherRollbackPawns();
    }

    TArray<AGMC_RollbackActor*> GenericRollbackActorList{};
    if (bRollBackGenericClientActors)
    {
      GenericRollbackActorList = GatherGenericRollbackActors();
      BufferGenericRollbackActorStates(GenericRollbackActorList, EGMC_NetContext::LocalClientPawn_PreReplay);
    }

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_PreReplay);

    GMCReplication::FDataOpDirective ReplayDirective{DataOp::AllDataOps};
    ReplayDirective.SetSaveFilters(DataFilter::SV_UseClientValue, DataFilterMode::Inclusive);
    ReplayDirective.SetQuantizeFilters(DataFilter::SV_UseClientValue, DataFilterMode::Inclusive);
    ReplayDirective.SetApplyFilters(DataFilter::None, DataFilterMode::Inclusive);

    for (int32 Index = 0; Index < MoveHistory.Num(); ++Index)
    {
      CL_MoveExecutionAux.ReplayMoveIdx = Index;

      auto& Move = MoveHistory[Index];

      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_PreReplayMoveExecution, Move);

      if (bRollBackClientPawns)
      {
        RollBackPawns(Move.MetaData.Timestamp, RollbackPawnList, EGMC_NetContext::LocalClientPawn_PreReplayMoveExecution);
      }

      if (bRollBackGenericClientActors)
      {
        RollbackGenericActors(
          Move.MetaData.Timestamp,
          Move.MetaData.DeltaTime,
          Move,
          GenericRollbackActorList,
          EGMC_NetContext::LocalClientPawn_PreReplayMoveExecution
        );
      }

      ProcessSyncData(Move.InputState, ReplayDirective, AliasData, bUseRelativeValuesForPrediction, this);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, Move.InputState, EGMC_NetContext::LocalClientPawn_PreReplayMoveExecution);

      ExecuteMove(
        Move.InputState,
        Move.MetaData.DeltaTime,
        Move.MetaData.Timestamp,
        MaxTimeStep,
        MaxIterations,
        bUseRelativeValuesForPrediction,
        false,
        false,
        false
      );

      ProcessSyncData(Move.OutputState, ReplayDirective, AliasData, bUseRelativeValuesForPrediction, this);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, Move.OutputState, EGMC_NetContext::LocalClientPawn_PostReplayMoveExecution);

      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_PostReplayMoveExecution, Move);
    }

    CL_MoveExecutionAux.ReplayMoveIdx = -1;

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_PostReplay);

    if (bRollBackClientPawns)
    {
      RestoreRolledBackPawns(RollbackPawnList, EGMC_NetContext::LocalClientPawn_PostReplay);
    }

    if (bRollBackGenericClientActors)
    {
      RestoreRolledBackGenericActors(GenericRollbackActorList, EGMC_NetContext::LocalClientPawn_PostReplay);
    }

    CL_MoveExecutionAux.bIsReplaying = false;

    DEBUG_LOG_REPLAY_CLIENT_STATE_AFTER_REPLAY
  }
}

bool UGMC_ReplicationCmp::CL_ShouldReplay(const FGMC_Move& ReceivedMove, const FGMC_Move& SourceMove)
{
  SCOPE_CYCLE_COUNTER(STAT_CL_ShouldReplay)

  gmc_ck(bUseClientPrediction)
  gmc_ck(IsGMCEnabled())
  gmc_ck(ReceivedMove.HasValidTimestamp())
  gmc_ck(SourceMove.HasValidTimestamp())
  gmc_ck(FMath::IsNearlyEqual(ReceivedMove.MetaData.Timestamp, SourceMove.MetaData.Timestamp, 0.000001))

  if (bAlwaysReplay)
  {
    gmc_ck(!ReceivedMove.MetaData.bValidClientMove)
    return true;
  }

  // Any data that is not replicated for prediction or correction must not have the "CL_Validate" flag.

  const int32 ValidateMask = DataFilter::SV_ReplicateForPrediction | (!ReceivedMove.MetaData.bValidClientMove ? DataFilter::SV_ReplicateForCorrection : 0);

  EGMC_SyncType DeviatingSyncType{EGMC_SyncType::MAX};
  int32 DeviatingSyncTypeIndex{-1};

  bool bDebugForceFullSyncDataValidation = false;

#if ALLOW_CONSOLE && !NO_LOGGING

  bDebugForceFullSyncDataValidation = GMCCVars::ForceFullSyncDataValidation != 0;

#endif

  const bool bOutputStateValid = ValidateSyncDataAgainstOtherClientState(
    ReceivedMove.OutputState,
    SourceMove.OutputState,
    ValidateMask,
    DataFilterMode::Exclusive,
    DeviatingSyncType,
    DeviatingSyncTypeIndex,
    const_cast<UGMC_ReplicationCmp*>(this),
    bDebugForceFullSyncDataValidation
  );

  bool bInputStateValid = true;
  if (bOutputStateValid)
  {
    bInputStateValid = ValidateSyncDataAgainstOtherClientState(
      ReceivedMove.InputState,
      SourceMove.InputState,
      ValidateMask,
      DataFilterMode::Exclusive,
      DeviatingSyncType,
      DeviatingSyncTypeIndex,
      const_cast<UGMC_ReplicationCmp*>(this),
      bDebugForceFullSyncDataValidation
    );
  }

  const bool bClientStateValid = bOutputStateValid && bInputStateValid;

  gmc_ck(bClientStateValid == (DeviatingSyncType == EGMC_SyncType::MAX))

  gmc_ck(bOutputStateValid || bInputStateValid)
  GMC_CLOG(
    !bOutputStateValid,
    LogGMCReplication,
    PawnOwner,
    Verbose,
    TEXT("One or more values of the client output state failed client-side validation (timestamp = %f)."),
    ReceivedMove.MetaData.Timestamp
  )
  GMC_CLOG(
    !bInputStateValid,
    LogGMCReplication,
    PawnOwner,
    Verbose,
    TEXT("One or more values of the client input state failed client-side validation (timestamp = %f)."),
    ReceivedMove.MetaData.Timestamp
  )

  if (bClientStateValid)
  {
    return false;
  }

  gmc_ck(DeviatingSyncType != EGMC_SyncType::MAX)
  gmc_ck((uint8)DeviatingSyncType >= (uint8)EGMC_SyncType::Bool ? DeviatingSyncTypeIndex >= 0 : DeviatingSyncTypeIndex == -1)

  GMC_LOG(LogGMCReplication, PawnOwner, Verbose, TEXT("Source move with timestamp %f s was not valid."), SourceMove.MetaData.Timestamp)

  gmc_ck(bOutputStateValid ^ bInputStateValid)
  const auto& DeviatingState = !bOutputStateValid ? SourceMove.OutputState : SourceMove.InputState;
  const auto& ServerState = !bOutputStateValid ? ReceivedMove.OutputState : ReceivedMove.InputState;

  if (!CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_IsAllowedToReplay, DeviatingSyncType, DeviatingSyncTypeIndex, DeviatingState, ServerState))
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Verbose, TEXT("Replay not allowed."))
    return false;
  }

  return true;
}

bool UGMC_ReplicationCmp::CL_ShouldUseSmoothCorrections() const
{
  return SmoothCorrection.bEnable && !bAlwaysReplay && IsPredictedAutonomousProxy();
}

bool UGMC_ReplicationCmp::ShouldDeferCameraManagerUpdate() const
{
  return true;
}

void UGMC_ReplicationCmp::CL_HandleSmoothCorrectionOnReplayStart()
{
  if (CL_ShouldUseSmoothCorrections())
  {
    CL_SmoothCorrection.SavePreReplayTransform(this);
  }
}

void UGMC_ReplicationCmp::CL_HandleSmoothCorrectionOnReplayEnd()
{
  if (CL_ShouldUseSmoothCorrections())
  {
    CL_SmoothCorrection.SavePostReplayTransform(this);
    CL_SmoothCorrection.SetTargetTransformReached(this);

    if (!CL_SmoothCorrection.HasData())
    {
      // Save the current deviating transform of the pawn as the starting point for interpolating towards the corrected state.
      CL_SmoothCorrection.StartTransform = CL_SmoothCorrection.PreReplayTransform;
    }
    else
    {
      // If we are already performing a smooth correction, take the current smoothed state as the starting point for the new interpolation.
      CL_SmoothCorrection.StartTransform = CL_SmoothCorrection.Buffer.SwapTransform;
    }

    CL_SmoothCorrection.InitCorrection();

    gmc_ck(CL_SmoothCorrection.HasData())
  }
}

void UGMC_ReplicationCmp::CL_HandleSmoothCorrectionOnWorldTickStart()
{
  gmc_ck(IsPredictedAutonomousProxy())

  // Restore the actual pawn transform, the smoothed transform is only for display.
  CL_SmoothCorrection.SwapTransform(this);

  if (CL_SmoothCorrection.HasFinishedCorrection(SmoothCorrection.InterpolationTime) || !bUseClientPrediction)
  {
    // If the target transform has been reached (i.e. the interpolation ratio is 1), reset all correction data. We also do this when client prediction is turned
    // off as we want to abort any ongoing correction in that case.
    CL_SmoothCorrection.Reset();
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CL_OnSmoothCorrectionCompleted);
  }
}

void UGMC_ReplicationCmp::CL_HandleSmoothCorrectionOnWorldTickEnd(float DeltaTime, ELevelTick TickType)
{
  gmc_ck(IsPredictedAutonomousProxy())

  gmc_ck(DeltaTime >= MIN_DELTA_TIME)
  CL_SmoothCorrection.AddCorrectionTime(DeltaTime);

  // The target transform is the actual current transform of the pawn.
  CL_SmoothCorrection.SaveTargetTransform(this);

  // Calculate the interpolated values.
  CL_SmoothCorrection.Interpolate(this, SmoothCorrection.InterpolationTime);

  // Set the pawn to the smoothed transform for display.
  CL_SmoothCorrection.SwapTransform(this);
}

FGMC_PawnState& UGMC_ReplicationCmp::CL_NoPredictionBuffer()
{
  return CL_NoPredictionSwapBuffer.Buffer;
}

const FGMC_PawnState& UGMC_ReplicationCmp::CL_NoPredictionBuffer() const
{
  return CL_NoPredictionSwapBuffer.Buffer;
}

FGMC_Move UGMC_ReplicationCmp::CL_ClearAcknowledgedMoves(double ReceivedTimestamp)
{
  SCOPE_CYCLE_COUNTER(STAT_CL_ClearAcknowledgedMoves)

  TGMC_CircularArray<FGMC_Move> TempMoves{};
  TempMoves.Reset(MoveHistoryMaxSize);
  FGMC_Move SourceMove{};
  for (int32 Index = 0; Index < MoveHistory.Num(); ++Index)
  {
    const auto& Move = MoveHistory[Index];
    if (Move.MetaData.Timestamp > ReceivedTimestamp)
    {
      TempMoves.Add(Move);
    }
    else if (Move.MetaData.Timestamp == ReceivedTimestamp)
    {
      SourceMove = Move;
    }
  }

  MoveHistory = MoveTemp(TempMoves);

  // An empty move history after clearing can happen due to inconsistent timestamps.
  GMC_CLOG(MoveHistory.Num() == 0, LogGMCReplication, PawnOwner,Verbose, TEXT("Client move history is empty after clearing acknowledged moves."))

  if (!SourceMove.HasValidTimestamp())
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("No source move found while clearing acknowledged moves: ")
      TEXT("oldest timestamp in the history is %s, received server timestamp is %f ")
      TEXT("(the received server move may have been based on a proxy move)."),
      MoveHistory.Num() > 0 ? *FString::SanitizeFloat(MoveHistory[0].MetaData.Timestamp) : TEXT("<history is empty>"),
      ReceivedTimestamp
    )
  }

  return SourceMove;
}

bool UGMC_ReplicationCmp::CL_CheckPhysicsSettingsSynced(const FGMC_Move& ReplicatedMove) const
{
  const bool bReceivedServerAuthPhysicsMove = ReplicatedMove.MetaData.bIsUsingServerAuthPhysics;
  const bool bServerAuthPhysicsSettingsSynced = bReceivedServerAuthPhysicsMove == IsUsingServerAuthPhysicsReplication();

  const bool bReceivedClientAuthPhysicsMove = ReplicatedMove.MetaData.bIsUsingClientAuthPhysics;
  const bool bClientAuthPhysicsSettingsSynced = bReceivedClientAuthPhysicsMove == IsUsingClientAuthPhysicsReplication();

  return bServerAuthPhysicsSettingsSynced && bClientAuthPhysicsSettingsSynced;
}

bool UGMC_ReplicationCmp::CL_AddToPredictionHistory(const FGMC_Move& NewMove, bool& bOutPredictionHistoryFull)
{
  SCOPE_CYCLE_COUNTER(STAT_CL_AddToPredictionHistory)

  gmc_ck(bUseClientPrediction)

  bOutPredictionHistoryFull = false;

  if (MoveHistory.Num() == MoveHistoryMaxSize)
  {
    GMC_LOG(
      LogGMCReplication,
      PawnOwner,
      Verbose,
      TEXT("Prediction history limit of %d moves reached, \"%s\" may need to be increased if this occurs repeatedly."),
      MoveHistoryMaxSize, TO_STR(MoveHistoryMaxSize)
    )
    bOutPredictionHistoryFull = true;
    return false;
  }

  // We don't need to check for things like a valid timestamp here anymore because those things have been checked already when it was determined that the move
  // should be added.
  MoveHistory.Add(NewMove);

  if (MoveHistory.Num() == MoveHistoryMaxSize)
  {
    // The prediction history might be full after enqueueing the new move, which is not a problem (yet) if the history gets cleared before the next local move
    // is created that cannot be combined.
    bOutPredictionHistoryFull = true;
  }

  return true;
}

void UGMC_ReplicationCmp::SmoothNone(int32 LastIdx, bool bRollback, EGMC_InterpolationStates InterpStates, bool bSkipBoneUpdate, EGMC_NetContext Context)
{
  gmc_ck(MoveHistory.Num() >= 2)
  gmc_ck(IsValidMoveHistoryIndex(LastIdx))

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreSmoothing, LastIdx, false, bRollback, InterpStates);

  const auto& InterpState = InterpStates == EGMC_InterpolationStates::Input ? MoveHistory[LastIdx].InputState : MoveHistory[LastIdx].OutputState;
  SetSimulatedPawnState(InterpState, bSkipBoneUpdate, bUseRelativeValuesForSimulation, Context);

  DEBUG_LOG_SMOOTHING_NONE
}

void UGMC_ReplicationCmp::SmoothMatchLatest(
  int32 LastIdx,
  int32 PrevIdx,
  float Alpha,
  bool bInterpolating,
  bool bRollback,
  float TeleportThreshold,
  EGMC_InterpolationStates InterpStates,
  bool bSkipBoneUpdate,
  EGMC_NetContext Context
)
{
  gmc_ck(MoveHistory.Num() >= 2)
  gmc_ck(IsValidMoveHistoryIndex(PrevIdx))
  gmc_ck(IsValidMoveHistoryIndex(LastIdx))
  gmc_ck(!IsUsingServerAuthPhysicsReplication())
  gmc_ck(!IsUsingClientAuthPhysicsReplication())
  gmc_ck(MatchLatestParamsAux.bSimulationStartStateInitialized)

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreSmoothing, LastIdx, !bInterpolating, bRollback, InterpStates);

  const GMCReplication::ESimState InterpStatesNative = ToNativeState(InterpStates);
  const bool bUseInputStates = InterpStatesNative == GMCReplication::ESimState::Input;

  const auto& LastMove = MoveHistory[LastIdx];
  const auto& PreviousMove = MoveHistory[PrevIdx];

  bool bTeleport = false;
  if (TeleportThreshold >= 0.f)
  {
    const auto& StartState = bUseInputStates ? PreviousMove.InputState : PreviousMove.OutputState;
    const auto& TargetState = bUseInputStates ? LastMove.InputState : LastMove.OutputState;
    const auto& StartActorBase = StartState.ActorBase.Read();
    const auto& TargetActorBase = TargetState.ActorBase.Read();
    const auto& StartWorldActorLocation = StartActorBase ?
      GetWorldActorLocationFor(StartState.ActorLocation.Read(), StartActorBase->GetComponentTransform()) : StartState.ActorLocation.Read();
    const auto& TargetWorldActorLocation = TargetActorBase ?
      GetWorldActorLocationFor(TargetState.ActorLocation.Read(), TargetActorBase->GetComponentTransform()) : TargetState.ActorLocation.Read();
    bTeleport = FVector::Distance(StartWorldActorLocation, TargetWorldActorLocation) >= TeleportThreshold;
  }

  FGMC_PawnState InterpolatedState{};

  if (bTeleport)
  {
    InterpolatedState = bUseInputStates ? LastMove.InputState : LastMove.OutputState;
    if (InterpolatedState.ActorBase.Read())
    {
      ToAbsoluteSyncData(InterpolatedState, InterpolatedState.ActorBase.Read(), DataFilter::None, DataFilterMode::Inclusive, this);
    }
  }
  else
  {
    InterpolatedState = InterpolateSyncData(
      MatchLatestParamsAux.SimulationStartState,
      bUseInputStates ? LastMove.InputState : LastMove.OutputState,
      bUseInputStates ? PreviousMove.InputState : PreviousMove.OutputState,
      bUseInputStates ? LastMove.InputState : LastMove.OutputState,
      Alpha,
      InterpStatesNative,
      bUseRelativeValuesForSimulation,
      DataFilter::SV_ReplicateForSimulation,
      DataFilterMode::Exclusive,
      ReplicationSettings,
      GetSyncTagsData(),
      AliasData,
      this
    );
  }

  SetSimulatedPawnState(InterpolatedState, bSkipBoneUpdate, false/*the interpolated values are always in world space*/, Context);

  DEBUG_LOG_SMOOTHING_MATCH_LATEST
}

void UGMC_ReplicationCmp::SmoothComponent(
  double Time,
  float DeltaTime,
  const FTransform& AbsoluteStartTransform,
  const FTransform& AbsoluteTargetTransform,
  FGMC_SmoothComponentParams& InOutSmoothComponentParams
)
{
  gmc_ck(Time >= 0.)

  USceneComponent* Component = InOutSmoothComponentParams.Component;

  if (!IsValid(Component))
  {
    return;
  }

  const auto& Owner = GetOwner();
  if (!IsValid(Owner))
  {
    return;
  }

  if (Component->GetAttachParent() != Owner->GetRootComponent())
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("PostSmoothingParams.Component must be a direct child of the actor's root component."))
    SetComponentToSmooth(nullptr);
    gmc_ckne()
    return;
  }

  const FTransform AbsStartTransform = AbsoluteStartTransform;

  // We must consider the base offset of the component that is being smoothed.
  const FTransform AbsTargetTransform = InOutSmoothComponentParams.ComponentOffset * AbsoluteTargetTransform;

  const bool bFirstUpdate = InOutSmoothComponentParams.SimTime == 0.;
  if (
    bFirstUpdate ||
    (
      InOutSmoothComponentParams.TeleportThreshold >= 0.f &&
      FVector::Dist(AbsStartTransform.GetLocation(), AbsTargetTransform.GetLocation()) >= InOutSmoothComponentParams.TeleportThreshold
    )
  )
  {
    // Set directly to the target transform on the first update or when teleporting.
    InOutSmoothComponentParams.SimTime = InOutSmoothComponentParams.TargetTransformTimestamp = Time;
    InOutSmoothComponentParams.TargetDelta = 0.f;
  }
  else if (Time > InOutSmoothComponentParams.TargetTransformTimestamp)
  {
    // Calculate the time between the last two updates.
    const float ElapsedTime = Time - InOutSmoothComponentParams.TargetTransformTimestamp;

    // Do not exceed the target and do not fall behind by more than Delta.
    const float MaxDelta = FMath::Min(InOutSmoothComponentParams.MaxDeltaTime, Time);
    const float MinDelta = FMath::Min(InOutSmoothComponentParams.MinDeltaTime, MaxDelta);
    const float Delta = FMath::Clamp(ElapsedTime * InOutSmoothComponentParams.DeltaTimeDilation, MinDelta, MaxDelta);
    const double MinSimTime = Time - Delta;
    InOutSmoothComponentParams.SimTime = FMath::Clamp(InOutSmoothComponentParams.SimTime, MinSimTime, Time);

    const double TargetTime = Time - MinDelta - (MaxDelta - MinDelta) / 2.;
    const double TargetMin = TargetTime - UE_KINDA_SMALL_NUMBER;
    const double TargetMax = TargetTime + UE_KINDA_SMALL_NUMBER;
    if (InOutSmoothComponentParams.SimTime > TargetMax)
    {
      InOutSmoothComponentParams.SimTime = FMath::LerpStable(TargetTime, InOutSmoothComponentParams.SimTime, 0.5);
    }
    else if (InOutSmoothComponentParams.SimTime < TargetMin)
    {
      InOutSmoothComponentParams.SimTime = FMath::LerpStable(InOutSmoothComponentParams.SimTime, TargetTime, 0.5);
    }

    // Determine the delta between the new target and the current simulation.
    InOutSmoothComponentParams.TargetDelta = Time - InOutSmoothComponentParams.SimTime;

    InOutSmoothComponentParams.TargetTransformTimestamp = Time;
  }

  // Advance the simulation time.
  InOutSmoothComponentParams.SimTime = FMath::Clamp(InOutSmoothComponentParams.SimTime, 0., UE_DOUBLE_BIG_NUMBER) + DeltaTime;

  float Alpha = 1.f;
  if (InOutSmoothComponentParams.TargetDelta > UE_KINDA_SMALL_NUMBER)
  {
    // Compute the interpolation ratio for this iteration.
    const float RemainingTime = InOutSmoothComponentParams.TargetTransformTimestamp - InOutSmoothComponentParams.SimTime;
    const float TargetTime = InOutSmoothComponentParams.TargetDelta - RemainingTime;
    Alpha = FMath::Clamp(TargetTime / InOutSmoothComponentParams.TargetDelta, 0.f, 1.f);
  }

  // Calculate the interpolated transform to set.
  FTransform NewTransform{};
  NewTransform.SetLocation(FMath::Lerp(AbsStartTransform.GetLocation(), AbsTargetTransform.GetLocation(), Alpha));
  NewTransform.SetRotation(FQuat::Slerp(AbsStartTransform.GetRotation(), AbsTargetTransform.GetRotation(), Alpha));
  NewTransform.SetScale3D(FMath::Lerp(AbsStartTransform.GetScale3D(), AbsTargetTransform.GetScale3D(), Alpha));

  // If the passed component is a skeletal mesh we may want to skip updating kinematic bones.
  const auto& SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component);
  EKinematicBonesUpdateToPhysics::Type SavedBoneUpdateSetting{EKinematicBonesUpdateToPhysics::Type::SkipSimulatingBones};
  const bool bShouldSkipBoneUpdate = InOutSmoothComponentParams.bSkipBoneUpdate && SkeletalMeshComponent;
  if (bShouldSkipBoneUpdate)
  {
    SavedBoneUpdateSetting = SkeletalMeshComponent->KinematicBonesUpdateType;
    SkeletalMeshComponent->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::Type::SkipAllBones;
  }

  Component->SetWorldTransform(NewTransform, false, nullptr, InOutSmoothComponentParams.TeleportPhysics);

  if (bShouldSkipBoneUpdate)
  {
    // Restore the original flag.
    SkeletalMeshComponent->KinematicBonesUpdateType = SavedBoneUpdateSetting;
  }
}

void UGMC_ReplicationCmp::SmoothDelay(
  int32 StartIdx,
  int32 TargetIdx,
  float Alpha,
  bool bInterpolating,
  bool bRollback,
  float ExtrapolationRecoveryTime,
  float TeleportThreshold,
  EGMC_InterpolationStates InterpStates,
  bool bSkipBoneUpdate,
  EGMC_NetContext Context
)
{
  gmc_ck(IsValidMoveHistoryIndex(StartIdx))
  gmc_ck(IsValidMoveHistoryIndex(TargetIdx))

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreSmoothing, TargetIdx, !bInterpolating, bRollback, InterpStates);

  const FGMC_Move& StartMove = MoveHistory[StartIdx];
  const FGMC_Move& TargetMove = MoveHistory[TargetIdx];

  const int32 PreviousIndex = StartIdx - 1;
  const int32 NextIndex = TargetIdx + 1;
  const FGMC_Move& PreviousMove = IsValidMoveHistoryIndex(PreviousIndex) ? MoveHistory[PreviousIndex] : StartMove;
  const FGMC_Move& NextMove = IsValidMoveHistoryIndex(NextIndex) ? MoveHistory[NextIndex] : TargetMove;

  const GMCReplication::ESimState InterpStatesNative = ToNativeState(InterpStates);
  const bool bUseInputStates = InterpStatesNative == GMCReplication::ESimState::Input;

  bool bTeleport = false;
  if (TeleportThreshold >= 0.f)
  {
    const auto& StartState = bUseInputStates ? StartMove.InputState : StartMove.OutputState;
    const auto& TargetState = bUseInputStates ? TargetMove.InputState : TargetMove.OutputState;
    const auto& StartActorBase = StartState.ActorBase.Read();
    const auto& TargetActorBase = TargetState.ActorBase.Read();
    const auto& StartWorldActorLocation = StartActorBase ?
      GetWorldActorLocationFor(StartState.ActorLocation.Read(), StartActorBase->GetComponentTransform()) : StartState.ActorLocation.Read();
    const auto& TargetWorldActorLocation = TargetActorBase ?
      GetWorldActorLocationFor(TargetState.ActorLocation.Read(), TargetActorBase->GetComponentTransform()) : TargetState.ActorLocation.Read();
    bTeleport = FVector::Distance(StartWorldActorLocation, TargetWorldActorLocation) >= TeleportThreshold;
  }

  FGMC_PawnState InterpolatedState{};

  if (IsUsingServerAuthPhysicsReplication() || IsUsingClientAuthPhysicsReplication())
  {
    if (bTeleport)
    {
      InterpolatedState = bUseInputStates ? TargetMove.InputState : TargetMove.OutputState;
      if (InterpolatedState.ActorBase.Read())
      {
        ToAbsoluteSyncData(InterpolatedState, InterpolatedState.ActorBase.Read(), DataFilter::None, DataFilterMode::Inclusive, this);
      }
    }
    else
    {
      InterpolatedState = InterpolateSyncData_NoPhysics(
        bUseInputStates ? StartMove.InputState : StartMove.OutputState,
        bUseInputStates ? TargetMove.InputState : TargetMove.OutputState,
        bUseInputStates ? PreviousMove.InputState : PreviousMove.OutputState,
        bUseInputStates ? NextMove.InputState : NextMove.OutputState,
        Alpha,
        InterpStatesNative,
        bUseRelativeValuesForSimulation,
        DataFilter::SV_ReplicateForSimulation,
        DataFilterMode::Exclusive,
        ReplicationSettings,
        GetSyncTagsData(),
        AliasData,
        this
      );

      FGMC_PhysicsInterpState InterpolatedPhysicsState = InterpolatePhysicsState(StartMove, TargetMove, PreviousMove, NextMove, Alpha, InterpStates);

      InterpolatedState.LinearVelocity.Write(InterpolatedPhysicsState.LinearVelocity);
      InterpolatedState.AngularVelocity.Write(InterpolatedPhysicsState.AngularVelocity);
      InterpolatedState.ActorLocation.Write(InterpolatedPhysicsState.ActorLocation);
      InterpolatedState.ActorRotation.Write(InterpolatedPhysicsState.ActorRotation);
    }
  }
  else
  {
    if (bTeleport)
    {
      InterpolatedState = bUseInputStates ? TargetMove.InputState : TargetMove.OutputState;
      if (InterpolatedState.ActorBase.Read())
      {
        ToAbsoluteSyncData(InterpolatedState, InterpolatedState.ActorBase.Read(), DataFilter::None, DataFilterMode::Inclusive, this);
      }
    }
    else
    {
      InterpolatedState = InterpolateSyncData(
        bUseInputStates ? StartMove.InputState : StartMove.OutputState,
        bUseInputStates ? TargetMove.InputState : TargetMove.OutputState,
        bUseInputStates ? PreviousMove.InputState : PreviousMove.OutputState,
        bUseInputStates ? NextMove.InputState : NextMove.OutputState,
        Alpha,
        InterpStatesNative,
        bUseRelativeValuesForSimulation,
        DataFilter::SV_ReplicateForSimulation,
        DataFilterMode::Exclusive,
        ReplicationSettings,
        GetSyncTagsData(),
        AliasData,
        this
      );
    }

    if (!bRollback)
    {
      ExtrapolationRecovery(
        bInterpolating,
        ExtrapolationRecoveryTime,
        Alpha,
        InterpStates,
        bUseInputStates ? StartMove.InputState : StartMove.OutputState,
        InterpolatedState
      );
    }
  }

  SetSimulatedPawnState(InterpolatedState, bSkipBoneUpdate, false/*the interpolated values are always in world space*/, Context);

  DEBUG_LOG_SMOOTHING_DELAY
}

void UGMC_ReplicationCmp::ExtrapolationRecovery(
  bool bInterpolating,
  float RecoveryTime,
  float& Alpha,
  EGMC_InterpolationStates InterpStates,
  const FGMC_PawnState& StartState,
  FGMC_PawnState& InterpolatedState
)
{
  if (!bInterpolating || RecoveryTime <= 0.f)
  {
    SimulationAux.ExtrapolationRecoveryTimer = -1.f;
    SimulationAux.ExtrapolationRecoveryStartState = FGMC_PawnState{};
    SimulationAux.bPerformExtrapolationRecoveryInWorldSpace = false;
    return;
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  const float DeltaTime = FMath::Clamp(World->DeltaRealTimeSeconds, MIN_DELTA_TIME, UE_BIG_NUMBER);

  const bool bResumingInterpolation = SimulationAux.bWasExtrapolatingLastUpdate || SimulationAux.bTriggerManualExtrapolationRecovery;
  const bool bRecoveryInProgress = !SimulationAux.bWasExtrapolatingLastUpdate && SimulationAux.ExtrapolationRecoveryTimer > 0.f;

  gmc_ck(SimulationAux.bTriggerManualExtrapolationRecovery ? SimulationAux.ExtrapolationRecoveryTimer == -1.f : true)

  if (bResumingInterpolation)
  {
    // Start recovery.
    gmc_ck(SimulationAux.ExtrapolationRecoveryTimer == -1.f)
    SimulationAux.ExtrapolationRecoveryTimer = DeltaTime;
    SimulationAux.ExtrapolationRecoveryStartState = StartState;

    // Extrapolation recovery uses relative values by default.
    SimulationAux.bPerformExtrapolationRecoveryInWorldSpace = false;

    if (SimulationAux.bTriggerManualExtrapolationRecovery)
    {
      SimulationAux.bPerformExtrapolationRecoveryInWorldSpace = SimulationAux.bManualExtrapolationRecoveryInWorldSpace;
    }

    // Only reset this here, which effectively queues the extrapolation recovery until it can be performed.
    SimulationAux.bTriggerManualExtrapolationRecovery = false;
    SimulationAux.bManualExtrapolationRecoveryInWorldSpace = false;
  }
  else if (bRecoveryInProgress)
  {
    // Recovery has already started in a previous frame.
    gmc_ck(SimulationAux.ExtrapolationRecoveryTimer > 0.f)
    gmc_ck(!SimulationAux.bWasExtrapolatingLastUpdate)
    gmc_ck(!SimulationAux.bTriggerManualExtrapolationRecovery)
    gmc_ck(!SimulationAux.bManualExtrapolationRecoveryInWorldSpace)
    SimulationAux.ExtrapolationRecoveryTimer += DeltaTime;
  }
  else
  {
    // Regular interpolation is currently in progress, no recovery needed.
    gmc_ck(SimulationAux.ExtrapolationRecoveryTimer == -1.f)
    gmc_ck(!SimulationAux.bWasExtrapolatingLastUpdate)
    gmc_ck(!SimulationAux.bTriggerManualExtrapolationRecovery)
    gmc_ck(!SimulationAux.bManualExtrapolationRecoveryInWorldSpace)
    SimulationAux.NumAbortedExtrapolationRecoveries = 0;
    return;
  }

  if (SimulationAux.ExtrapolationRecoveryTimer >= RecoveryTime)
  {
    // Recovery is complete this frame.
    SimulationAux.ExtrapolationRecoveryTimer = -1.f;
    SimulationAux.ExtrapolationRecoveryStartState = FGMC_PawnState{};
    SimulationAux.bPerformExtrapolationRecoveryInWorldSpace = false;
    SimulationAux.NumAbortedExtrapolationRecoveries = 0;
    return;
  }

  // Always use world space values when the actor base differs.
  const bool bHasSameActorBase = SimulationAux.ExtrapolationRecoveryStartState.ActorBase.Read() == InterpolatedState.ActorBase.Read();
  SimulationAux.bPerformExtrapolationRecoveryInWorldSpace |= !bHasSameActorBase;

  gmc_ck(SimulationAux.ExtrapolationRecoveryTimer > 0.f)
  gmc_ck(SimulationAux.ExtrapolationRecoveryTimer < RecoveryTime)
  gmc_ck(!SimulationAux.bPerformExtrapolationRecoveryInWorldSpace ? bHasSameActorBase : true)

  if (SimulationAux.NumAbortedExtrapolationRecoveries > SimulationAux.RECOVERY_JITTER_GUARD_NUM)
  {
    // Stop extrapolation recoveries until the connection has stabilized again.
    return;
  }

  Alpha = SimulationAux.ExtrapolationRecoveryTimer / RecoveryTime;

  gmc_ck(Alpha > 0.f)
  gmc_ck(Alpha < 1.f)

  const bool bTransformValues = !SimulationAux.bPerformExtrapolationRecoveryInWorldSpace && static_cast<bool>(InterpolatedState.ActorBase.Read());

  if (!SimulationAux.bPerformExtrapolationRecoveryInWorldSpace)
  {
    // The interpolated sync data is always in world space, but for extrapolation recovery we want to interpolate relative values.
    ToRelativeSyncData(InterpolatedState, InterpolatedState.ActorBase.Read(), DataFilter::None, DataFilterMode::Inclusive, this);
  }
  else
  {
    const auto& RecoveryStartStateBase = SimulationAux.ExtrapolationRecoveryStartState.ActorBase.Read();
    if (RecoveryStartStateBase)
    {
      ToAbsoluteSyncData(SimulationAux.ExtrapolationRecoveryStartState, RecoveryStartStateBase, DataFilter::None, DataFilterMode::Inclusive, this);

      // Set to null once transformed, the rest of the extrapolation recovery will be in world space.
      SimulationAux.ExtrapolationRecoveryStartState.ActorBase.Write(nullptr);
    }
  }

  FGMC_PawnState RecoveryState = InterpolateSyncData_ExtrapolationRecovery(
    SimulationAux.ExtrapolationRecoveryStartState,
    InterpolatedState,
    SimulationAux.ExtrapolationRecoveryStartState,
    InterpolatedState,
    Alpha,
    ToNativeState(InterpStates),
    false, /*use the values as provided*/
    DataFilter::SV_ReplicateForSimulation,
    DataFilterMode::Exclusive,
    ReplicationSettings,
    GetSyncTagsData(),
    AliasData,
    this
  );

  const FVector& StartStateActorLocation = SimulationAux.ExtrapolationRecoveryStartState.ActorLocation.Read();
  const FVector& InterpolatedStateActorLocation = InterpolatedState.ActorLocation.Read();
  if (
    bRecoverActorLocation &&
    FVector::Distance(StartStateActorLocation, InterpolatedStateActorLocation) >= SimulationAux.RECOVER_ACTOR_LOCATION_THRESHOLD
  )
  {
    InterpolatedState.ActorLocation.Write(RecoveryState.ActorLocation.Read());
  }

  const FQuat& StartStateActorRotation = SimulationAux.ExtrapolationRecoveryStartState.ActorRotation.Read().Quaternion();
  const FQuat& InterpolatedStateActorRotation = InterpolatedState.ActorRotation.Read().Quaternion();
  if (
    bRecoverActorRotation &&
    FMath::RadiansToDegrees(StartStateActorRotation.AngularDistance(InterpolatedStateActorRotation)) >= SimulationAux.RECOVER_ACTOR_ROTATION_THRESHOLD
  )
  {
    InterpolatedState.ActorRotation.Write(RecoveryState.ActorRotation.Read());
  }

  const FVector& StartStateActorScale = SimulationAux.ExtrapolationRecoveryStartState.ActorScale.Read();
  const FVector& InterpolatedStateActorScale = InterpolatedState.ActorScale.Read();
  if (
    bRecoverActorScale &&
    FVector::Distance(StartStateActorScale, InterpolatedStateActorScale) >= SimulationAux.RECOVER_ACTOR_SCALE_THRESHOLD
  )
  {
    InterpolatedState.ActorScale.Write(RecoveryState.ActorScale.Read());
  }

  const FQuat& StartStateControlRotation = SimulationAux.ExtrapolationRecoveryStartState.ControlRotation.Read().Quaternion();
  const FQuat& InterpolatedStateControlRotation = InterpolatedState.ControlRotation.Read().Quaternion();
  if (
    bRecoverControlRotation &&
    FMath::RadiansToDegrees(StartStateControlRotation.AngularDistance(InterpolatedStateControlRotation))
      >= SimulationAux.RECOVER_ACTOR_CONTROL_ROTATION_THRESHOLD
  )
  {
    InterpolatedState.ControlRotation.Write(RecoveryState.ControlRotation.Read());
  }

  if (!SimulationAux.bPerformExtrapolationRecoveryInWorldSpace)
  {
    ToAbsoluteSyncData(InterpolatedState, InterpolatedState.ActorBase.Read(), DataFilter::None, DataFilterMode::Inclusive, this);
  }
}

FGMC_PhysicsInterpState UGMC_ReplicationCmp::InterpolatePhysicsState(
  const FGMC_Move& StartMove,
  const FGMC_Move& TargetMove,
  const FGMC_Move& PreviousMove,
  const FGMC_Move& NextMove,
  float Alpha,
  EGMC_InterpolationStates InterpStates
)
{
  gmc_ck(StartMove.MetaData.bIsUsingServerAuthPhysics || StartMove.MetaData.bIsUsingClientAuthPhysics)
  gmc_ck(StartMove.MetaData.bIsUsingServerAuthPhysics ? StartMove.MetaData.ServerAuthPhysicsTimestamp > 0. : true)
  gmc_ck(TargetMove.MetaData.bIsUsingServerAuthPhysics || TargetMove.MetaData.bIsUsingClientAuthPhysics)
  gmc_ck(TargetMove.MetaData.bIsUsingServerAuthPhysics ? TargetMove.MetaData.ServerAuthPhysicsTimestamp > 0. : true)
  gmc_ck(PreviousMove.MetaData.bIsUsingServerAuthPhysics || PreviousMove.MetaData.bIsUsingClientAuthPhysics)
  gmc_ck(PreviousMove.MetaData.bIsUsingServerAuthPhysics ? PreviousMove.MetaData.ServerAuthPhysicsTimestamp > 0. : true)
  gmc_ck(NextMove.MetaData.bIsUsingServerAuthPhysics || NextMove.MetaData.bIsUsingClientAuthPhysics)
  gmc_ck(NextMove.MetaData.bIsUsingServerAuthPhysics ? NextMove.MetaData.ServerAuthPhysicsTimestamp > 0. : true)

  const bool bUseInputStates = ToNativeState(InterpStates) == GMCReplication::ESimState::Input;

  const auto& StartState = bUseInputStates ? StartMove.InputState : StartMove.OutputState;
  const auto& TargetState = bUseInputStates ? TargetMove.InputState : TargetMove.OutputState;
  const auto& PreviousState = bUseInputStates ? PreviousMove.InputState : PreviousMove.OutputState;
  const auto& NextState = bUseInputStates ? NextMove.InputState : NextMove.OutputState;

  const auto& StartActorBase = StartState.ActorBase.Read();
  const auto& TargetActorBase = TargetState.ActorBase.Read();
  const auto& PreviousActorBase = PreviousState.ActorBase.Read();
  const auto& NextActorBase = NextState.ActorBase.Read();

  const auto& StartActorBaseTransform = StartActorBase ? StartActorBase->GetComponentTransform() : FTransform{};
  const auto& TargetActorBaseTransform = TargetActorBase ? TargetActorBase->GetComponentTransform() : FTransform{};
  const auto& PreviousActorBaseTransform = PreviousActorBase ? PreviousActorBase->GetComponentTransform() : FTransform{};
  const auto& NextActorBaseTransform = NextActorBase ? NextActorBase->GetComponentTransform() : FTransform{};

  FGMC_PhysicsInterpState PhysicsStartState{
    StartActorBase ? GetWorldLinearVelocityFor(StartState.LinearVelocity.Read(), StartActorBaseTransform) : StartState.LinearVelocity.Read(),
    StartActorBase ? GetWorldAngularVelocityFor(StartState.AngularVelocity.Read(), StartActorBaseTransform) : StartState.AngularVelocity.Read(),
    StartActorBase ? GetWorldActorLocationFor(StartState.ActorLocation.Read(), StartActorBaseTransform) : StartState.ActorLocation.Read(),
    StartActorBase ? GetWorldActorRotationFor(StartState.ActorRotation.Read(), StartActorBaseTransform) : StartState.ActorRotation.Read(),
    StartMove.MetaData.bIsUsingServerAuthPhysics ? StartMove.MetaData.ServerAuthPhysicsTimestamp : StartMove.MetaData.Timestamp
  };

  FGMC_PhysicsInterpState PhysicsTargetState{
    TargetActorBase ? GetWorldLinearVelocityFor(TargetState.LinearVelocity.Read(), TargetActorBaseTransform) : TargetState.LinearVelocity.Read(),
    TargetActorBase ? GetWorldAngularVelocityFor(TargetState.AngularVelocity.Read(), TargetActorBaseTransform) : TargetState.AngularVelocity.Read(),
    TargetActorBase ? GetWorldActorLocationFor(TargetState.ActorLocation.Read(), TargetActorBaseTransform) : TargetState.ActorLocation.Read(),
    TargetActorBase ? GetWorldActorRotationFor(TargetState.ActorRotation.Read(), TargetActorBaseTransform) : TargetState.ActorRotation.Read(),
    TargetMove.MetaData.bIsUsingServerAuthPhysics ? TargetMove.MetaData.ServerAuthPhysicsTimestamp : TargetMove.MetaData.Timestamp
  };

  FGMC_PhysicsInterpState PhysicsPreviousState{
    PreviousActorBase ? GetWorldLinearVelocityFor(PreviousState.LinearVelocity.Read(), PreviousActorBaseTransform) : PreviousState.LinearVelocity.Read(),
    PreviousActorBase ? GetWorldAngularVelocityFor(PreviousState.AngularVelocity.Read(), PreviousActorBaseTransform) : PreviousState.AngularVelocity.Read(),
    PreviousActorBase ? GetWorldActorLocationFor(PreviousState.ActorLocation.Read(), PreviousActorBaseTransform) : PreviousState.ActorLocation.Read(),
    PreviousActorBase ? GetWorldActorRotationFor(PreviousState.ActorRotation.Read(), PreviousActorBaseTransform) : PreviousState.ActorRotation.Read(),
    PreviousMove.MetaData.bIsUsingServerAuthPhysics ? PreviousMove.MetaData.ServerAuthPhysicsTimestamp : PreviousMove.MetaData.Timestamp
  };

  FGMC_PhysicsInterpState PhysicsNextState{
    NextActorBase ? GetWorldLinearVelocityFor(NextState.LinearVelocity.Read(), NextActorBaseTransform) : NextState.LinearVelocity.Read(),
    NextActorBase ? GetWorldAngularVelocityFor(NextState.AngularVelocity.Read(), NextActorBaseTransform) : NextState.AngularVelocity.Read(),
    NextActorBase ? GetWorldActorLocationFor(NextState.ActorLocation.Read(), NextActorBaseTransform) : NextState.ActorLocation.Read(),
    NextActorBase ? GetWorldActorRotationFor(NextState.ActorRotation.Read(), NextActorBaseTransform) : NextState.ActorRotation.Read(),
    NextMove.MetaData.bIsUsingServerAuthPhysics ? NextMove.MetaData.ServerAuthPhysicsTimestamp : NextMove.MetaData.Timestamp
  };

  FGMC_PhysicsInterpState InterpolatedPhysicsState{};
  switch (PhysicsInterpolationMethod)
  {
    case EGMC_PhysicsInterpolationMethod::Default:
    {
      InterpolatedPhysicsState = InterpolatePhysicsDefault(
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom1:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom1,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom2:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom2,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom3:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom3,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom4:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom4,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom5:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom5,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom6:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom6,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom7:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom7,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    case EGMC_PhysicsInterpolationMethod::Custom8:
    {
      InterpolatedPhysicsState = CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        InterpolatePhysicsCustom8,
        PhysicsStartState,
        PhysicsTargetState,
        PhysicsPreviousState,
        PhysicsNextState,
        Alpha
      );
      break;
    }
    default:
    {
      gmc_ckne();
    }
  }

  return InterpolatedPhysicsState;
}

void UGMC_ReplicationCmp::SmoothSimulated(
  int32 LastIdx,
  bool bCumulativeUpdate,
  float DeltaTime,
  double Timestamp,
  float InMaxTimeStep,
  int32 InMaxIterations,
  bool bInterpolating,
  bool bRollback,
  EGMC_InterpolationStates SimStates,
  bool bSkipBoneUpdate,
  EGMC_NetContext Context
)
{
  gmc_ck(IsValidMoveHistoryIndex(LastIdx))
  gmc_ck(!IsUsingServerAuthPhysicsReplication())
  gmc_ck(!IsUsingClientAuthPhysicsReplication())
  gmc_ck(SimStates == EGMC_InterpolationStates::Input || SimStates == EGMC_InterpolationStates::Output)

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreSmoothing, LastIdx, !bInterpolating, bRollback, SimStates);

  // Copy the latest move, we don't want to change move history data.
  FGMC_Move Move = MoveHistory[LastIdx];

  // We will also just use this state of the copied move to write to so we don't have to create a new one and initialize it.
  auto& SimState = ToNativeState(SimStates) == GMCReplication::ESimState::Input ? Move.InputState : Move.OutputState;

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreSimulatedMoveExecution, SimState, bCumulativeUpdate, DeltaTime, Timestamp);

  if (bCumulativeUpdate)
  {
    gmc_ck(InterpolationMode == EGMC_InterpolationMode::CumulativeSimulation || ExtrapolationMode == EGMC_ExtrapolationMode::CumulativeSimulation)

    ProcessSyncData(SimState, {DataOp::Save}, AliasData, bUseRelativeValuesForSimulation, this);

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnCumulativeMoveInitialized, SimState, SimStates, DeltaTime, Timestamp);
  }

  ProcessSyncData(
    SimState,
    {DataOp::Apply, DataFilter::SV_ReplicateForSimulation, DataFilterMode::Exclusive},
    AliasData,
    bUseRelativeValuesForSimulation,
    this
  );
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, SimState, Context);

  ExecuteMove(SimState, DeltaTime, Timestamp, InMaxTimeStep, InMaxIterations, bUseRelativeValuesForSimulation, false, true, false);

  DEBUG_LOG_SMOOTHING_SIMULATED_INPUT

  ProcessSyncData(SimState, {DataOp::Save}, AliasData, bUseRelativeValuesForSimulation, this);

  DEBUG_LOG_SMOOTHING_SIMULATED_OUTPUT

  SetSimulatedPawnState(SimState, bSkipBoneUpdate, bUseRelativeValuesForSimulation, Context);

  DEBUG_LOG_SMOOTHING_SIMULATED_NEW_STATE

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostSimulatedMoveExecution, SimState, bCumulativeUpdate, DeltaTime, Timestamp);
}

void UGMC_ReplicationCmp::SmoothMovement(float DeltaTime, double& OutSimTime, int32& OutTargetIdx, TArray<int32>& OutSkippedStateIndices)
{
  SCOPE_CYCLE_COUNTER(STAT_SmoothMovement)

  gmc_ck(IsSimulatedProxy() || IsSmoothedListenServerPawn() || IsNonPredictedAutonomousProxy())
  gmc_ck(MoveHistory.Num() >= 2)
  gmc_ck(OutTargetIdx == -1)
  gmc_ck(OutSimTime == -1.)
  gmc_ck(OutSkippedStateIndices.Num() == 0)
  gmc_ck(!SimulationAux.bIsExtrapolating)
  gmc_ck(SimulationAux.bIsSimulating)

  FGMC_MatchLatestTransientParams MatchLatest{MatchLatestParams};
  FGMC_FixedDelayTransientParams FixedDelay{FixedDelayParams};
  FGMC_AdaptiveDelayTransientParams AdaptiveDelay{AdaptiveDelayParams};
  FGMC_UniformSimulationTransientParams UniformSimulation{UniformSimulationParams};
  FGMC_CumulativeSimulationTransientParams CumulativeSimulation{CumulativeSimulationParams};

  ComputeSmoothingParams(DeltaTime, MatchLatest, FixedDelay, AdaptiveDelay, UniformSimulation, CumulativeSimulation);

  DEBUG_LOG_SMOOTHING_PARAMS

  SimulationAux.PrevLatestTime = MoveHistory.Last().MetaData.Timestamp;

  SimulationAux.PrevSmoothingTime = -1.;

  SimulationAux.bIsCumulativeUpdate = false;

  EGMC_NetContext SmoothingContext = GetSmoothingContext();
  bool bDidInterpolate = false;

  // The start transform for component smoothing must be saved before the pawn is moved through smoothing.
  FTransform PostSmoothComponentSimulationStartTransform{};
  SaveSmoothComponentSimulationStartTransform(PostSmoothingParams.Component, PostSmoothComponentSimulationStartTransform);

  const auto HandleDetermineSkippedStates = [this, &OutSkippedStateIndices](int32 StartIdx, int32 TargetIdx)
  {
    if (bDetermineSkippedSmoothingStates && SimulationAux.PrevInterpTargetTime >= 0.)
    {
      DetermineSkippedStates(OutSkippedStateIndices, StartIdx, TargetIdx, SimulationAux.PrevInterpTargetTime);
    }
  };

  const auto SetInterpParams = [&](int32 TargetIdx, double SimTime, bool bCumulativeUpdate, bool bServerAuthPhysics)
  {
    SimulationAux.PrevInterpTargetTime = bServerAuthPhysics ?
      MoveHistory[TargetIdx].MetaData.ServerAuthPhysicsTimestamp : MoveHistory[TargetIdx].MetaData.Timestamp;
    OutTargetIdx = TargetIdx;
    OutSimTime = SimTime;
    SimulationAux.PrevSmoothingTime = OutSimTime;
    SimulationAux.bIsCumulativeUpdate = bCumulativeUpdate;
    bDidInterpolate = true;
  };

  switch (InterpolationMode)
  {
    case EGMC_InterpolationMode::None:
    {
      int32 LastIdx = MoveHistory.Num() - 1;
      HandleDetermineSkippedStates(LastIdx, LastIdx);

      const double LastTimestamp = MoveHistory[LastIdx].MetaData.Timestamp;
      SetInterpParams(LastIdx, LastTimestamp, false, IsUsingServerAuthPhysicsReplication());

      SmoothNone(LastIdx, false, NoInterpStates, bSkipBoneUpdateForSmoothing, SmoothingContext);

      break;
    }
    case EGMC_InterpolationMode::MatchLatest:
    {
      if (MatchLatest.bInterpThisUpdate)
      {
        HandleDetermineSkippedStates(MatchLatest.PrevIdx, MatchLatest.LastIdx);

        SetInterpParams(MatchLatest.LastIdx, MatchLatest.SimTime, false, IsUsingServerAuthPhysicsReplication());

        SmoothMatchLatest(
          MatchLatest.LastIdx,
          MatchLatest.PrevIdx,
          MatchLatest.Alpha,
          true,
          false,
          MatchLatest.TeleportThreshold,
          MatchLatest.InterpStates,
          bSkipBoneUpdateForSmoothing,
          SmoothingContext
        );
      }
      break;
    }
    case EGMC_InterpolationMode::FixedDelay:
    {
      if (FixedDelay.bInterpThisUpdate)
      {
        HandleDetermineSkippedStates(FixedDelay.StartIdx, FixedDelay.TargetIdx);

        SetInterpParams(FixedDelay.TargetIdx, FixedDelay.SimTime, false, IsUsingServerAuthPhysicsReplication());

        SmoothDelay(
          FixedDelay.StartIdx,
          FixedDelay.TargetIdx,
          FixedDelay.Alpha,
          true,
          false,
          FixedDelay.ExtrapolationRecoveryTime,
          FixedDelay.TeleportThreshold,
          FixedDelay.InterpStates,
          bSkipBoneUpdateForSmoothing,
          SmoothingContext
        );
      }
      break;
    }
    case EGMC_InterpolationMode::AdaptiveDelay:
    {
      if (AdaptiveDelay.bInterpThisUpdate)
      {
        HandleDetermineSkippedStates(AdaptiveDelay.StartIdx, AdaptiveDelay.TargetIdx);

        SetInterpParams(AdaptiveDelay.TargetIdx, AdaptiveDelay.SimTime, false, IsUsingServerAuthPhysicsReplication());

        SmoothDelay(
          AdaptiveDelay.StartIdx,
          AdaptiveDelay.TargetIdx,
          AdaptiveDelay.Alpha,
          true,
          false,
          AdaptiveDelay.ExtrapolationRecoveryTime,
          AdaptiveDelay.TeleportThreshold,
          AdaptiveDelay.InterpStates,
          bSkipBoneUpdateForSmoothing,
          SmoothingContext
        );
      }
      break;
    }
    case EGMC_InterpolationMode::UniformSimulation:
    {
      if (UniformSimulation.bInterpThisUpdate)
      {
        HandleDetermineSkippedStates(UniformSimulation.LastIdx, UniformSimulation.LastIdx);

        SetInterpParams(UniformSimulation.LastIdx, UniformSimulation.SimTime, false, IsUsingServerAuthPhysicsReplication());

        SmoothSimulated(
          UniformSimulation.LastIdx,
          false,
          UniformSimulation.SimDeltaTime,
          UniformSimulation.SimTime,
          UniformSimulation.MaxTimeStep,
          UniformSimulation.MaxIterations,
          true,
          false,
          UniformSimulation.SimStates,
          bSkipBoneUpdateForSmoothing,
          SmoothingContext
        );
      }
      break;
    }
    case EGMC_InterpolationMode::CumulativeSimulation:
    {
      if (CumulativeSimulation.bInterpThisUpdate)
      {
        HandleDetermineSkippedStates(CumulativeSimulation.LastIdx, CumulativeSimulation.LastIdx);

        SetInterpParams(CumulativeSimulation.LastIdx, CumulativeSimulation.SimTime, !CumulativeSimulation.bNewMove, IsUsingServerAuthPhysicsReplication());

        SmoothSimulated(
          CumulativeSimulation.LastIdx,
          !CumulativeSimulation.bNewMove,
          CumulativeSimulation.SimDeltaTime,
          CumulativeSimulation.SimTime,
          CumulativeSimulation.MaxTimeStep,
          CumulativeSimulation.MaxIterations,
          true,
          false,
          CumulativeSimulation.SimStates,
          bSkipBoneUpdateForSmoothing,
          SmoothingContext
        );
      }
      break;
    }
    default:
    {
      gmc_ckne();
    }
  }

  if (bDidInterpolate)
  {
    gmc_ck(SimulationAux.PrevInterpTargetTime >= 0.)
    gmc_ck(OutTargetIdx != -1)
    gmc_ck(OutSimTime >= 0.)
    gmc_ck(SimulationAux.PrevSmoothingTime == OutSimTime)

    SimulationAux.ExtrapolationStartLocation = FVector::ZeroVector;
    SimulationAux.AccExtrapolatedDistance = 0.;
    SimulationAux.AbsoluteExtrapolatedDistance = 0.;
    SimulationAux.bMaxExtrapolationDistanceReached = false;
  }
  else
  {
    // Only set this to -1 after the interpolation logic because it may be used to determine skipped states.
    SimulationAux.PrevInterpTargetTime = -1.;
    gmc_ck(SimulationAux.PrevSmoothingTime == -1.)
    gmc_ck(!SimulationAux.bIsCumulativeUpdate)
  }

  SimulationAux.PrevExtrapTargetTime = -1.;

  const bool bCanExtrapolate = ExtrapolationMode != EGMC_ExtrapolationMode::None && MaxExtrapolationDistance > 0.;
  const bool bShouldExtrapolate = bCanExtrapolate && !bDidInterpolate;

  bool bDidExtrapolate = false;

  if (bShouldExtrapolate)
  {
    if (!SimulationAux.bWasExtrapolatingLastUpdate)
    {
      SimulationAux.ExtrapolationStartLocation = PawnOwner->GetActorLocation();
    }

    FGMC_PawnState PreExtrapolationState{};
    InitializeSyncData(
      PreExtrapolationState,
      ReplicationSettings,
      GetSyncTagsData(),
      AliasData,
      GMCReplication::ESimState::None,
      GMCReplication::ESimType::None,
      this
    );
    ProcessSyncData(PreExtrapolationState, {DataOp::Save}, AliasData, false, this);

    const auto SetExtrapParams = [&](int32 TargetIdx, double SimTime, bool bCumulativeUpdate, bool bServerAuthPhysics)
    {
      // Currently the target index for extrapolation is always the latest state in the move history.
      gmc_ck(TargetIdx == MoveHistory.Num() - 1)

      SimulationAux.PrevExtrapTargetTime = bServerAuthPhysics ?
        MoveHistory[TargetIdx].MetaData.ServerAuthPhysicsTimestamp : MoveHistory[TargetIdx].MetaData.Timestamp;
      OutTargetIdx = TargetIdx;
      OutSimTime = SimTime;
      SimulationAux.PrevSmoothingTime = OutSimTime;
      SimulationAux.bIsCumulativeUpdate = bCumulativeUpdate;
      SimulationAux.bIsExtrapolating = true;
      SimulationAux.NumAbortedExtrapolationRecoveries += SimulationAux.ExtrapolationRecoveryTimer > 0.f;
      SimulationAux.ExtrapolationRecoveryTimer = -1.f;
      SimulationAux.ExtrapolationRecoveryStartState = FGMC_PawnState{};
      bDidExtrapolate = true;
    };

    switch (ExtrapolationMode)
    {
      case EGMC_ExtrapolationMode::None:
      {
        gmc_ckne()
        break;
      }
      case EGMC_ExtrapolationMode::DeadReckoning:
      {
        if (InterpolationMode == EGMC_InterpolationMode::MatchLatest && MatchLatest.bExtrapThisUpdate)
        {
          SetExtrapParams(MatchLatest.LastIdx, MatchLatest.SimTime, false, IsUsingServerAuthPhysicsReplication());

          SmoothMatchLatest(
            MatchLatest.LastIdx,
            MatchLatest.PrevIdx,
            MatchLatest.Alpha,
            false,
            false,
            MatchLatest.TeleportThreshold,
            MatchLatest.InterpStates,
            bSkipBoneUpdateForSmoothing,
            SmoothingContext
          );
        }
        else if (InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay && AdaptiveDelay.bExtrapThisUpdate)
        {
          SetExtrapParams(AdaptiveDelay.TargetIdx, AdaptiveDelay.SimTime, false, IsUsingServerAuthPhysicsReplication());

          SmoothDelay(
            AdaptiveDelay.StartIdx,
            AdaptiveDelay.TargetIdx,
            AdaptiveDelay.Alpha,
            false,
            false,
            AdaptiveDelay.ExtrapolationRecoveryTime,
            AdaptiveDelay.TeleportThreshold,
            AdaptiveDelay.InterpStates,
            bSkipBoneUpdateForSmoothing,
            SmoothingContext
          );
        }
        else if (FixedDelay.bExtrapThisUpdate)
        {
          SetExtrapParams(FixedDelay.TargetIdx, FixedDelay.SimTime, false, IsUsingServerAuthPhysicsReplication());

          SmoothDelay(
            FixedDelay.StartIdx,
            FixedDelay.TargetIdx,
            FixedDelay.Alpha,
            false,
            false,
            FixedDelay.ExtrapolationRecoveryTime,
            FixedDelay.TeleportThreshold,
            FixedDelay.InterpStates,
            bSkipBoneUpdateForSmoothing,
            SmoothingContext
          );
        }
        break;
      }
      case EGMC_ExtrapolationMode::UniformSimulation:
      {
        if (UniformSimulation.bExtrapThisUpdate)
        {
          SetExtrapParams(UniformSimulation.LastIdx, UniformSimulation.SimTime, false, IsUsingServerAuthPhysicsReplication());

          SmoothSimulated(
            UniformSimulation.LastIdx,
            false,
            UniformSimulation.SimDeltaTime,
            UniformSimulation.SimTime,
            UniformSimulation.MaxTimeStep,
            UniformSimulation.MaxIterations,
            false,
            false,
            UniformSimulation.SimStates,
            bSkipBoneUpdateForSmoothing,
            SmoothingContext
          );
        }
        break;
      }
      case EGMC_ExtrapolationMode::CumulativeSimulation:
      {
        if (CumulativeSimulation.bExtrapThisUpdate)
        {
          const bool bCumulativeUpdate = CumulativeSimulation.bUseDissociativeExtrapolation || !CumulativeSimulation.bNewMove;
          SetExtrapParams(CumulativeSimulation.LastIdx, CumulativeSimulation.SimTime, bCumulativeUpdate, IsUsingServerAuthPhysicsReplication());

          SmoothSimulated(
            CumulativeSimulation.LastIdx,
            bCumulativeUpdate,
            CumulativeSimulation.SimDeltaTime,
            CumulativeSimulation.SimTime,
            CumulativeSimulation.MaxTimeStep,
            CumulativeSimulation.MaxIterations,
            false,
            false,
            CumulativeSimulation.SimStates,
            bSkipBoneUpdateForSmoothing,
            SmoothingContext
          );
        }
        break;
      }
      default:
      {
        gmc_ckne();
      }
    }

    if (bDidExtrapolate)
    {
      gmc_ck(SimulationAux.PrevExtrapTargetTime >= 0.)
      gmc_ck(OutTargetIdx != -1)
      gmc_ck(OutSimTime >= 0.)
      gmc_ck(SimulationAux.PrevSmoothingTime == OutSimTime)

      const FVector ExtrapolatedLocation = GetActorLocation_GMC();
      const double ExtrapolatedDistance = (ExtrapolatedLocation - PreExtrapolationState.ActorLocation.Read()).Size();
      const double AbsoluteExtrapolatedDistance = (ExtrapolatedLocation - SimulationAux.ExtrapolationStartLocation).Size();

      SimulationAux.AccExtrapolatedDistance += ExtrapolatedDistance;
      SimulationAux.AbsoluteExtrapolatedDistance = AbsoluteExtrapolatedDistance;

      SimulationAux.bMaxExtrapolationDistanceReached = SimulationAux.AbsoluteExtrapolatedDistance > MaxExtrapolationDistance;

      if (SimulationAux.bMaxExtrapolationDistanceReached)
      {
        const auto GetResetContext = [this]
        {
          if (IsSimulatedProxy())
          {
            return EGMC_NetContext::RemoteClientPawn_ExtrapolationAborted;
          }

          if (IsServerPawn())
          {
            gmc_ck(IsSmoothedListenServerPawn())
            return EGMC_NetContext::RemoteServerPawn_ExtrapolationAborted;
          }

          gmc_ck(IsNonPredictedAutonomousProxy())
          return EGMC_NetContext::LocalClientPawn_ExtrapolationAborted;
        };

        ProcessSyncData(PreExtrapolationState, {DataOp::Apply}, AliasData, false, this);
        CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, PreExtrapolationState, GetResetContext());

        SimulationAux.AccExtrapolatedDistance -= ExtrapolatedDistance;
        SimulationAux.AbsoluteExtrapolatedDistance -= ExtrapolatedDistance;
      }
    }
    else
    {
      gmc_ck(SimulationAux.PrevExtrapTargetTime == -1.)
      gmc_ck(SimulationAux.PrevSmoothingTime == -1.)
      gmc_ck(!SimulationAux.bIsCumulativeUpdate)
    }
  }
  else if (!bDidInterpolate && !bCanExtrapolate)
  {
    gmc_ck(ExtrapolationMode == EGMC_ExtrapolationMode::None || MaxExtrapolationDistance == 0.)

    if (bUseRelativeValuesForSimulation)
    {
      // Still update the pawn when on a potentially moving base.
      int32 LastIdx = MoveHistory.Num() - 1;
      const auto& InterpState = NoInterpStates == EGMC_InterpolationStates::Input ? MoveHistory[LastIdx].InputState : MoveHistory[LastIdx].OutputState;
      if (InterpState.ActorBase.Read())
      {
        SetSimulatedPawnState(InterpState, bSkipBoneUpdateForSmoothing, true, SmoothingContext);
      }
    }
  }

  gmc_ck(!(bDidInterpolate && bDidExtrapolate))
  gmc_ck(SimulationAux.PrevLatestTime == MoveHistory.Last().MetaData.Timestamp)
  gmc_ck(SimulationAux.PrevSmoothingTime == OutSimTime)

  gmc_ckc(
    if (!bDidInterpolate && !bDidExtrapolate)
    {
      gmc_ck(OutTargetIdx == -1)
      gmc_ck(OutSimTime == -1.)
      gmc_ck(OutSkippedStateIndices.Num() == 0)
    }
    else
    {
      gmc_ck(OutTargetIdx != -1)
      gmc_ck(OutSimTime >= 0.)
    }

    if (bDidExtrapolate)
    {
      gmc_ck(SimulationAux.bIsExtrapolating)
      gmc_ck(SimulationAux.ExtrapolationRecoveryTimer == -1.f)
    }
  )

  DEBUG_LOG_EXTRAPOLATION

  if (bDidInterpolate || bDidExtrapolate)
  {
    // Post-process smoothing of the configured component (if any).
    SmoothComponent(OutSimTime, DeltaTime, PostSmoothComponentSimulationStartTransform, GetActorTransform_GMC(), PostSmoothingParams);
  }
}

bool UGMC_ReplicationCmp::ShouldSimulatePawn(double MaxSmoothingDistance, double SmoothingFallOffDistance, int32 MaxSkippedSmoothingFrames) const
{
  if (!SimulationThrottle.bEnable || !GEngine)
  {
    return true;
  }

  const auto& LocalPC = Cast<AGMC_PlayerController>(GEngine->GetFirstLocalPlayerController(GetWorld()));
  if (!IsValid(LocalPC))
  {
    return true;
  }

  FVector LocalPCPosition{0.};

  if (const auto& LocalPCPawn = LocalPC->GetPawn())
  {
    LocalPCPosition = LocalPCPawn->GetActorLocation();
  }
  else
  {
    LocalPC->GetPlayerViewPoint(LocalPCPosition, UNUSED(FRotator));
  }

  const double DistanceToLocalPC = (GetActorLocation_GMC() - LocalPCPosition).Size();

  const double FallOffMin = FMath::Max(0., MaxSmoothingDistance);
  const double FallOffMax = FMath::Max(FallOffMin + UU_MILLIMETER, MaxSmoothingDistance + SmoothingFallOffDistance);

  if (DistanceToLocalPC <= FallOffMin)
  {
    return true;
  }

  gmc_ck(FallOffMax > FallOffMin)

  const float InvRatio = 1. - (DistanceToLocalPC - FallOffMin) / (FallOffMax - FallOffMin);
  const int32 NumFramesToSkip =
    FMath::Clamp(FMath::CeilToInt(1.f / FMath::Clamp(InvRatio, UE_KINDA_SMALL_NUMBER, 1.f)) - 1, 0, FMath::Max(1, MaxSkippedSmoothingFrames));

  if (SimulationAux.NumFramesSinceLastSimulation > NumFramesToSkip)
  {
    return true;
  }

  return false;
}

EGMC_NetContext UGMC_ReplicationCmp::GetSmoothingContext() const
{
  if (IsSimulatedProxy())
  {
    return EGMC_NetContext::RemoteClientPawn_Simulation;
  }

  if (IsServerPawn())
  {
    gmc_ck(IsSmoothedListenServerPawn())
    return EGMC_NetContext::RemoteServerPawn_Simulation;
  }

  gmc_ck(IsNonPredictedAutonomousProxy())
  return EGMC_NetContext::LocalClientPawn_Simulation;
}

void UGMC_ReplicationCmp::SaveMatchLatestSimulationStartState(FGMC_PawnState& InOutSimulationStartState)
{
  ProcessSyncData_IntegratedOnly(InOutSimulationStartState, {DataOp::Save}, AliasData, bUseRelativeValuesForSimulation, this);
}

void UGMC_ReplicationCmp::SaveSmoothComponentSimulationStartTransform(USceneComponent* SmoothComponent, FTransform& OutSimulationStartTransform)
{
  if (!IsValid(SmoothComponent))
  {
    OutSimulationStartTransform = FTransform::Identity;
    return;
  }

  OutSimulationStartTransform = SmoothComponent->GetComponentTransform();
}

void UGMC_ReplicationCmp::ComputeSmoothingParams(
  float DeltaTime,
  FGMC_MatchLatestTransientParams& MatchLatest,
  FGMC_FixedDelayTransientParams& FixedDelay,
  FGMC_AdaptiveDelayTransientParams& AdaptiveDelay,
  FGMC_UniformSimulationTransientParams& UniformSimulation,
  FGMC_CumulativeSimulationTransientParams& CumulativeSimulation
)
{
  SCOPE_CYCLE_COUNTER(STAT_ComputeSmoothingParams)

  gmc_ck(MoveHistory.Num() >= 2)

  const bool bIsUsingPhysics = IsUsingServerAuthPhysicsReplication() || IsUsingClientAuthPhysicsReplication();

  if (!bIsUsingPhysics && (InterpolationMode == EGMC_InterpolationMode::MatchLatest || ExtrapolationMode == EGMC_ExtrapolationMode::DeadReckoning))
  {
    const int32 MoveHistoryNum = MoveHistory.Num();
    MatchLatest.PrevIdx = MoveHistoryNum - 2;
    MatchLatest.LastIdx = MoveHistoryNum - 1;

    const double LatestTimestamp = MoveHistory[MatchLatest.LastIdx].MetaData.Timestamp;

    // The start state should always be initialized.
    if (!MatchLatestParamsAux.bSimulationStartStateInitialized)
    {
      MatchLatestParamsAux.bSimulationStartStateInitialized = true;

      // Make sure that the start state has default values before initializing.
      MatchLatestParamsAux.SimulationStartState = FGMC_PawnState{};

      InitializeSyncData(
        MatchLatestParamsAux.SimulationStartState,
        ReplicationSettings,
        GetSyncTagsData(),
        AliasData,
        GMCReplication::ESimState::None,
        GMCReplication::ESimType::None,
        this
      );
    }

    if (MatchLatest.SimTime == 0.)
    {
      // Set to the current server state for the first update.
      MatchLatest.SimTime = MatchLatest.TargetStateTimestamp = LatestTimestamp;
      MatchLatest.TargetDelta = 0.f;
    }
    else if (LatestTimestamp > MatchLatest.TargetStateTimestamp)
    {
      // Save the persistent start state for the next simulation iteration. We want to retain the generic data from the previous state however to ensure that
      // animations tied to bound data are not skipped.
      const auto& PrevMove = MoveHistory[MatchLatest.PrevIdx];
      MatchLatestParamsAux.SimulationStartState =
        MatchLatest.InterpStates == EGMC_InterpolationStates::Input ? PrevMove.InputState : PrevMove.OutputState;
      SaveMatchLatestSimulationStartState(MatchLatestParamsAux.SimulationStartState);

      // Slow down if already ahead of the previous target.
      if (MatchLatest.SimTime > MatchLatest.TargetStateTimestamp)
      {
        MatchLatest.SimTime = FMath::LerpStable(MatchLatest.TargetStateTimestamp, MatchLatest.SimTime, 0.5);
      }

      // Calculate the time between the last two updates based on the original timestamps (meaning this will be unaffected by network latency fluctuations).
      const float ElapsedTime = LatestTimestamp - MatchLatest.TargetStateTimestamp;

      // Do not exceed the target and do not fall behind by more than Delta.
      const float MaxDelta = FMath::Min(MatchLatest.MaxDeltaTime, LatestTimestamp);
      const float MinDelta = FMath::Min(MatchLatest.MinDeltaTime, MaxDelta);
      const float Delta = FMath::Clamp(ElapsedTime * MatchLatest.DeltaTimeDilation, MinDelta, MaxDelta);
      const double MinSimTime = LatestTimestamp - Delta;
      MatchLatest.SimTime = FMath::Clamp(MatchLatest.SimTime, MinSimTime, LatestTimestamp);

      // Determine the delta between the new target state and the current pawn simulation.
      MatchLatest.TargetDelta = LatestTimestamp - MatchLatest.SimTime;

      MatchLatest.TargetStateTimestamp = LatestTimestamp;
    }

    // Advance the simulation time.
    MatchLatest.SimTime = FMath::Clamp(MatchLatest.SimTime, 0., UE_DOUBLE_BIG_NUMBER) + DeltaTime;

    if (MatchLatest.TargetDelta > UE_KINDA_SMALL_NUMBER)
    {
      // Compute the interpolation ratio for this iteration.
      const float RemainingTime = MatchLatest.TargetStateTimestamp - MatchLatest.SimTime;
      const float TargetTime = MatchLatest.TargetDelta - RemainingTime;
      MatchLatest.Alpha = TargetTime / MatchLatest.TargetDelta;
    }
    else
    {
      MatchLatest.Alpha = 1.f;
    }

    MatchLatest.bInterpThisUpdate = MatchLatest.SimTime <= MatchLatest.TargetStateTimestamp;
    MatchLatest.bExtrapThisUpdate = (uint8)!MatchLatest.bInterpThisUpdate;

    // Update the persistent params.
    MatchLatestParams.TargetStateTimestamp = MatchLatest.TargetStateTimestamp;
    MatchLatestParams.SimTime = MatchLatest.SimTime;
    MatchLatestParams.TargetDelta = MatchLatest.TargetDelta;
  }

  {
    const auto GetStartAndTargetIdx = [this](int32& OutStartIdx, int32& OutTargetIdx, double InSimTime, bool bInServerAuthPhysics)
    {
      const auto& MoveHistoryNum = MoveHistory.Num();
      gmc_ck(MoveHistoryNum >= 2)

      gmc_ck(bInServerAuthPhysics ? MoveHistory.Last().MetaData.ServerAuthPhysicsTimestamp > 0. : true)
      if ((bInServerAuthPhysics ? MoveHistory.Last().MetaData.ServerAuthPhysicsTimestamp : MoveHistory.Last().MetaData.Timestamp) < InSimTime)
      {
        OutStartIdx = MoveHistoryNum - 2;
        OutTargetIdx = MoveHistoryNum - 1;
        return;
      }

      for (int32 Index = MoveHistoryNum - 2; Index >= 0; --Index)
      {
        gmc_ck(bInServerAuthPhysics ? MoveHistory[Index].MetaData.ServerAuthPhysicsTimestamp > 0. : true)
        if ((bInServerAuthPhysics ? MoveHistory[Index].MetaData.ServerAuthPhysicsTimestamp : MoveHistory[Index].MetaData.Timestamp) <= InSimTime)
        {
          OutStartIdx = Index;
          OutTargetIdx = Index + 1;
          return;
        }
      }

      GMC_LOG(
        LogGMCReplication,
        PawnOwner,
        Verbose,
        TEXT("No state old enough to interpolate found. \"%s\" may need to be increased if this occurs repeatedly."),
        TO_STR(MoveHistoryMaxSize)
      )

      GMC_LOG(
        LogGMCReplication,
        PawnOwner,
        VeryVerbose,
        TEXT("Oldest available state has timestamp %f, simulation time is %f, current time is %f."),
        MoveHistory[0].MetaData.Timestamp,
        InSimTime,
        GetTime()
      )

      gmc_ck(OutStartIdx == -1)
      gmc_ck(OutTargetIdx ==  -1)
    };

    const auto SetParamsDelayBased = [this, &GetStartAndTargetIdx](auto& Params, float Delay, bool bServerAuthPhysics, bool bClientAuthPhysics)
    {
      Params.SimTime = FMath::Clamp(GetTime() - Delay, 0., UE_DOUBLE_BIG_NUMBER);
      GetStartAndTargetIdx(Params.StartIdx, Params.TargetIdx, Params.SimTime, bServerAuthPhysics);

      if (Params.StartIdx != -1)
      {
        gmc_ck(Params.StartIdx >= 0)
        gmc_ck(Params.TargetIdx >= 0)

        const double StartTime = bServerAuthPhysics ?
          MoveHistory[Params.StartIdx].MetaData.ServerAuthPhysicsTimestamp : MoveHistory[Params.StartIdx].MetaData.Timestamp;
        const double TargetTime = bServerAuthPhysics ?
          MoveHistory[Params.TargetIdx].MetaData.ServerAuthPhysicsTimestamp : MoveHistory[Params.TargetIdx].MetaData.Timestamp;
        Params.Alpha = FMath::Clamp((Params.SimTime - StartTime) / FMath::Max(TargetTime - StartTime, (double)MIN_DELTA_TIME), 0., UE_DOUBLE_BIG_NUMBER);

        Params.bInterpThisUpdate = Params.Alpha <= 1.f;
        Params.bExtrapThisUpdate = (uint8)!Params.bInterpThisUpdate;

        if (Params.bExtrapThisUpdate && (bServerAuthPhysics || bClientAuthPhysics) && !Params.bAllowPhysicsExtrapolation)
        {
          Params.bExtrapThisUpdate = false;
        }
      }
    };

    if (InterpolationMode == EGMC_InterpolationMode::FixedDelay || ExtrapolationMode == EGMC_ExtrapolationMode::DeadReckoning)
    {
      SetParamsDelayBased(FixedDelay, FixedDelay.FixedDelay, IsUsingServerAuthPhysicsReplication(), IsUsingClientAuthPhysicsReplication());
    }

    if (InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay || ExtrapolationMode == EGMC_ExtrapolationMode::DeadReckoning)
    {
      SetParamsDelayBased(AdaptiveDelay, AdaptiveDelay.CurrentDelay, IsUsingServerAuthPhysicsReplication(), IsUsingClientAuthPhysicsReplication());
    }
  }

  if (!bIsUsingPhysics)
  {
    const auto SetParamsSimulationBased = [this](auto& Params, float DeltaTime, bool bForceExtrap)
    {
      Params.LastIdx = MoveHistory.Num() - 1;
      const double LatestTime = MoveHistory[Params.LastIdx].MetaData.Timestamp;

      Params.bNewMove = LatestTime > Params.PreviousMoveTimestamp;
      gmc_ck(!Params.bNewMove ? LatestTime == Params.PreviousMoveTimestamp : true)
      Params.TimeSinceLastNewMove = Params.bNewMove ? 0.f : Params.TimeSinceLastNewMove + DeltaTime;

      gmc_ck(IsValid(PawnOwner))
      const float UpdateFrequency = IsRemotelyControlledServerPawn() ? GetClientSendRate() : PawnOwner->NetUpdateFrequency;
      const float AvgTimeBetweenUpdates = 1.f / FMath::Clamp(UpdateFrequency, UE_KINDA_SMALL_NUMBER, UE_BIG_NUMBER);
      Params.bInterpThisUpdate = bForceExtrap ? false : Params.TimeSinceLastNewMove <= AvgTimeBetweenUpdates + Params.InterpTolerance;
      Params.bExtrapThisUpdate = (uint8)!Params.bInterpThisUpdate;

      Params.PreviousMoveTimestamp = LatestTime;
    };

    if (InterpolationMode == EGMC_InterpolationMode::UniformSimulation || ExtrapolationMode == EGMC_ExtrapolationMode::UniformSimulation)
    {
      gmc_ckc(
        if (ExtrapolationMode == EGMC_ExtrapolationMode::UniformSimulation)
        {
          gmc_ck(MatchLatest.bExtrapThisUpdate ? InterpolationMode == EGMC_InterpolationMode::MatchLatest : true)
          gmc_ck(FixedDelay.bExtrapThisUpdate ? InterpolationMode == EGMC_InterpolationMode::FixedDelay : true)
          gmc_ck(AdaptiveDelay.bExtrapThisUpdate ? InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay : true)
        }
      )

      const bool bForceExtrap = ExtrapolationMode == EGMC_ExtrapolationMode::UniformSimulation ?
        MatchLatest.bExtrapThisUpdate || FixedDelay.bExtrapThisUpdate || AdaptiveDelay.bExtrapThisUpdate :
        false;

      SetParamsSimulationBased(UniformSimulation, DeltaTime, bForceExtrap);

      const double LatestTime = MoveHistory[UniformSimulation.LastIdx].MetaData.Timestamp;

      const double ClampedTimeDiscrepancy = FMath::Clamp(CL_GetTimeDiscrepancy(), -CLIENT_TIME_DISCREPANCY_CLAMP_ABS, CLIENT_TIME_DISCREPANCY_CLAMP_ABS);
      UniformSimulation.SimDeltaTime = FMath::Clamp(GetTime() + ClampedTimeDiscrepancy - LatestTime, (double)MIN_DELTA_TIME, UE_DOUBLE_BIG_NUMBER);

      UniformSimulation.SimTime = LatestTime + UniformSimulation.SimDeltaTime;

      // Update the persistent params.
      UniformSimulationParams.PreviousMoveTimestamp = UniformSimulation.PreviousMoveTimestamp;
      UniformSimulationParams.TimeSinceLastNewMove = UniformSimulation.TimeSinceLastNewMove;
    }

    if (InterpolationMode == EGMC_InterpolationMode::CumulativeSimulation || ExtrapolationMode == EGMC_ExtrapolationMode::CumulativeSimulation)
    {
      gmc_ckc(
        if (ExtrapolationMode == EGMC_ExtrapolationMode::CumulativeSimulation)
        {
          gmc_ck(MatchLatest.bExtrapThisUpdate ? InterpolationMode == EGMC_InterpolationMode::MatchLatest : true)
          gmc_ck(FixedDelay.bExtrapThisUpdate ? InterpolationMode == EGMC_InterpolationMode::FixedDelay : true)
          gmc_ck(AdaptiveDelay.bExtrapThisUpdate ? InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay : true)
        }
      )

      const bool bForceExtrap = ExtrapolationMode == EGMC_ExtrapolationMode::CumulativeSimulation ?
        MatchLatest.bExtrapThisUpdate || FixedDelay.bExtrapThisUpdate || AdaptiveDelay.bExtrapThisUpdate :
        false;

      SetParamsSimulationBased(CumulativeSimulation, DeltaTime, bForceExtrap);

      CumulativeSimulation.SimDeltaTime = FMath::Clamp(DeltaTime, MIN_DELTA_TIME, UE_BIG_NUMBER);

      if (CumulativeSimulation.bNewMove)
      {
        CumulativeSimulation.AccDeltaTime = CumulativeSimulation.SimDeltaTime;
      }
      else
      {
        CumulativeSimulation.AccDeltaTime += CumulativeSimulation.SimDeltaTime;
      }

      CumulativeSimulation.SimTime = MoveHistory[CumulativeSimulation.LastIdx].MetaData.Timestamp + CumulativeSimulation.AccDeltaTime;

      // Update the persistent params.
      CumulativeSimulationParams.PreviousMoveTimestamp = CumulativeSimulation.PreviousMoveTimestamp;
      CumulativeSimulationParams.TimeSinceLastNewMove = CumulativeSimulation.TimeSinceLastNewMove;
      CumulativeSimulationParams.AccDeltaTime = CumulativeSimulation.AccDeltaTime;
    }
  }
}

FGMC_PhysicsInterpState UGMC_ReplicationCmp::InterpolatePhysicsDefault(
  const FGMC_PhysicsInterpState& StartState,
  const FGMC_PhysicsInterpState& TargetState,
  const FGMC_PhysicsInterpState& PreviousState,
  const FGMC_PhysicsInterpState& NextState,
  double Ratio
)
{
  FGMC_PhysicsInterpState Result{};

  const float CurrentDeltaTime = FMath::Max(TargetState.Timestamp - StartState.Timestamp, MIN_DELTA_TIME);

  Result.LinearVelocity = (TargetState.ActorLocation - StartState.ActorLocation) / CurrentDeltaTime;

  Result.AngularVelocity = (TargetState.AngularVelocity - StartState.AngularVelocity) / CurrentDeltaTime;

  Result.ActorLocation = FMath::CubicInterp(
    StartState.ActorLocation,
    StartState.LinearVelocity * CurrentDeltaTime,
    TargetState.ActorLocation,
    TargetState.LinearVelocity * CurrentDeltaTime,
    Ratio
  );

  Result.ActorRotation = FQuat::Slerp(
    StartState.ActorRotation.Quaternion(),
    TargetState.ActorRotation.Quaternion(),
    Ratio
  ).Rotator();

  Result.Timestamp = FMath::Lerp(StartState.Timestamp, TargetState.Timestamp, Ratio);

  return Result;
}

void UGMC_ReplicationCmp::DetermineSkippedStates(
  TArray<int32>& OutSkippedStateIndices,
  int32 StartIdx,
  int32 TargetIdx,
  double PrevTargetTimestamp
) const
{
  SCOPE_CYCLE_COUNTER(STAT_DetermineSkippedStates)

  gmc_ck(OutSkippedStateIndices.IsEmpty())
  gmc_ck(IsValidMoveHistoryIndex(StartIdx))
  gmc_ck(IsValidMoveHistoryIndex(TargetIdx))
  gmc_ck(MoveHistory.Num() >= 2)
  gmc_ck(PrevTargetTimestamp >= 0.)

  if (MoveHistory[TargetIdx].MetaData.Timestamp == PrevTargetTimestamp)
  {
    // We are still interpolating between the same two states as during the last tick which means we cannot have skipped any states.
    return;
  }
  if (MoveHistory[StartIdx].MetaData.Timestamp != PrevTargetTimestamp)
  {
    // We may have skipped a state.
    if (StartIdx == 0)
    {
      // We don't have any older states available to check (should not happen usually).
      return;
    }
    if (MoveHistory[StartIdx - 1].MetaData.Timestamp == PrevTargetTimestamp)
    {
      // We have not interpolated between the current start state and the previous target state but we didn't skip any state.
      return;
    }
    // We skipped one or more states.
    int32 CheckIndex = StartIdx - 2;
    while (IsValidMoveHistoryIndex(CheckIndex))
    {
      // We skipped the state at index CheckIndex + 1.
      OutSkippedStateIndices.Emplace(CheckIndex + 1);
      if (MoveHistory[CheckIndex].MetaData.Timestamp == PrevTargetTimestamp)
      {
        // We found the last target state and return the indices of the skipped states, which are all states with a timestamp larger than
        // the previous target state but smaller than the current start state.
        GMC_LOG(LogGMCReplication, PawnOwner, VeryVerbose, TEXT("%d states were skipped during interpolation."), OutSkippedStateIndices.Num())
        return;
      }
      --CheckIndex;
    }
    // The oldest state in the history is newer than the previous target state.
    gmc_ck(CheckIndex + 1 == 0)
    OutSkippedStateIndices.Emplace(0);
    GMC_LOG(LogGMCReplication, PawnOwner, VeryVerbose, TEXT("Previous target state was already deleted from the move history."))
  }
}

FRotator UGMC_ReplicationCmp::GetControlRotationLocalSpace() const
{
  return ControlRotationToLocal(GetControlRotationWorldSpace());
}

FRotator UGMC_ReplicationCmp::GetControlRotationWorldSpace() const
{
  return GetControllerRotation_GMC();
}

FRotator UGMC_ReplicationCmp::ControlRotationToLocal(const FRotator& WorldControlRotation) const
{
  if (!IsValid(PawnOwner))
  {
    return FRotator::ZeroRotator;
  }

  const auto& RootComponent = PawnOwner->GetRootComponent();
  if (!IsValid(RootComponent))
  {
    return FRotator::ZeroRotator;
  }

  const FTransform& RootTransform = RootComponent->GetComponentTransform();
  const FQuat WorldControlRotationQuat = WorldControlRotation.Quaternion();
  return RootTransform.InverseTransformRotation(WorldControlRotationQuat).Rotator().GetNormalized();
}

FGMC_PawnState UGMC_ReplicationCmp::GetPawnState(bool bUseRelative) const
{
  FGMC_PawnState CurrentState{};
  UGMC_ReplicationCmp* MutableThis = const_cast<UGMC_ReplicationCmp*>(this);
  InitializeSyncData(
    CurrentState,
    ReplicationSettings,
    GetSyncTagsData(),
    AliasData,
    GMCReplication::ESimState::None,
    GMCReplication::ESimType::None,
    MutableThis
  );
  ProcessSyncData(CurrentState, {DataOp::Save}, AliasData, bUseRelative, MutableThis);
  return CurrentState;
}

void UGMC_ReplicationCmp::ApplyPawnState(const FGMC_PawnState& State)
{
  ProcessSyncData(const_cast<FGMC_PawnState&>(State), {DataOp::Apply}, AliasData, bUseRelativeValuesForSimulation, this);
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSyncDataApplied, State, EGMC_NetContext::NoContextInformation);
}

bool UGMC_ReplicationCmp::ShouldAddToSimulationHistory(double MoveTimestamp) const
{
  return MoveHistory.Num() == 0 || MoveTimestamp > MoveHistory.Last().MetaData.Timestamp;
}

void UGMC_ReplicationCmp::AddToSimulationHistory(const FGMC_Move& Move)
{
  SCOPE_CYCLE_COUNTER(STAT_AddToSimulationHistory)

  gmc_ck(ShouldAddToSimulationHistory(Move.MetaData.Timestamp))

  // If the history has already reached the max size the oldest move will be overwritten.
  MoveHistory.Add(Move);
  gmc_ck(MoveHistory.Num() <= MoveHistoryMaxSize)

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnSimulationMoveEnqueued, MoveHistory.Last());
}

FGMC_PawnState& UGMC_ReplicationCmp::RollbackBuffer()
{
  return RollbackRestoreBuffer.Buffer;
}

const FGMC_PawnState& UGMC_ReplicationCmp::RollbackBuffer() const
{
  return RollbackRestoreBuffer.Buffer;
}

bool UGMC_ReplicationCmp::SV_SwapRollbackState(AGMC_Pawn* Pawn) const
{
  if (!IsValid(Pawn))
  {
    return false;
  }

  if (Pawn->GetLocalRole() != ROLE_Authority)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Unwind failed: not a server pawn."))
    return false;
  }

  if (Pawn == PawnOwner)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Unwind failed: a pawn cannot unwind itself."))
    return false;
  }

  const auto& ReplicationComponent = Pawn->GetReplicationComponent();
  if (!ReplicationComponent)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Unwind failed: the target pawn does not have a replication component."))
    return false;
  }

  if (!ReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack)
  {
    return false;
  }

  if (!ReplicationComponent->RollbackSwapBuffer.bInitialized)
  {
    ReplicationComponent->RollbackSwapBuffer.Initialize(ReplicationComponent->RollbackRestoreBuffer.Buffer, ReplicationComponent);
  }

  // Pawn are only rolled back for predicted clients during remote move execution.
  gmc_ck(bUseClientPrediction)
  gmc_ck(SV_RemoteMoveExecutionAux.bIsExecutingRemoteMoves)

  ReplicationComponent->RollbackSwapBuffer.Swap(false, ReplicationComponent, EGMC_NetContext::RollbackSwap);

  return true;
}

bool UGMC_ReplicationCmp::CL_IsPerformingSmoothCorrection() const
{
  return CL_SmoothCorrection.HasData();
}

void UGMC_ReplicationCmp::SaveLocalStateBeforeRollback(const APawn* PawnToSave, bool bUseRelative) const
{
  SCOPE_CYCLE_COUNTER(STAT_SaveLocalStateBeforeRollback)

  const auto& ReplicationComponent = Cast<UGMC_ReplicationCmp>(PawnToSave->GetMovementComponent());
  gmc_ck(ReplicationComponent)

  if (!ReplicationComponent->RollbackRestoreBuffer.bInitialized)
  {
    ReplicationComponent->RollbackRestoreBuffer.Initialize(ReplicationComponent);
  }

  ReplicationComponent->RollbackRestoreBuffer.Save(bUseRelative, ReplicationComponent);
}

void UGMC_ReplicationCmp::RestoreOriginalStateAfterRollback(const APawn* PawnToRestore, bool bUseRelative, EGMC_NetContext Context) const
{
  SCOPE_CYCLE_COUNTER(STAT_RestoreOriginalStateAfterRollback)

  const auto& ReplicationComponent = Cast<UGMC_ReplicationCmp>(PawnToRestore->GetMovementComponent());
  gmc_ck(ReplicationComponent)

  if (ReplicationComponent->RollbackSwapBuffer.bInitialized)
  {
    GMC_CLOG(
      !ReplicationComponent->RollbackSwapBuffer.HasEvenNumberOfSwaps(),
      LogGMCReplication,
      PawnOwner,
      Warning,
      TEXT("The rollback state was not swapped an even number of times.")
    )

    // If the pawn was unwound during rollback sync data may have been manipulated, but this information is not yet saved in the restore state. It is important
    // that the swap function was called a second time for this work (i.e. the rollback swap buffer must contain the "real" state of the pawn at this point).
    ReplicationComponent->RollbackRestoreBuffer.Buffer = ReplicationComponent->RollbackSwapBuffer.Buffer;

    // The swap buffer must be reset so it can be initialized to the correct restore state again during another remote move execution.
    ReplicationComponent->RollbackSwapBuffer.Reset();
  }

  ReplicationComponent->RollbackRestoreBuffer.Restore(bUseRelative, ReplicationComponent, Context);
}

TArray<AGMC_Pawn*> UGMC_ReplicationCmp::GatherRollbackPawns() const
{
  SCOPE_CYCLE_COUNTER(STAT_GatherRollbackPawns)

  gmc_ck(IsServerPawn() || IsAutonomousProxy())
  gmc_ck(bRollBackServerPawns || bRollBackClientPawns)
  gmc_ck(bUseClientPrediction)

  TArray<AActor*> Actors{};

  if (IsValid(GMCAggregator))
  {
    Actors = static_cast<const TArray<AActor*>>(GMCAggregator->GetPawns());
  }
  else
  {
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGMC_Pawn::StaticClass(), Actors);
  }

  TArray<AGMC_Pawn*> RollbackPawns{};

  for (const auto& Actor : Actors)
  {
    const auto& GMCPawn = Cast<AGMC_Pawn>(Actor);

    if (!IsValid(GMCPawn) || !CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, ShouldRollBackGMCPawn, GMCPawn))
    {
      continue;
    }

    RollbackPawns.Emplace(GMCPawn);

    SaveLocalStateBeforeRollback(GMCPawn, bUseRelativeValuesForPrediction);

    gmc_ck(IsServerPawn() && GMCPawn->GetReplicationComponent() ? !GMCPawn->GetReplicationComponent()->SV_RemoteMoveExecutionAux.bIsRolledBack : true)
    gmc_ck(IsAutonomousProxy() && GMCPawn->GetReplicationComponent() ? !GMCPawn->GetReplicationComponent()->CL_MoveExecutionAux.bIsRolledBack : true)
  }

  return RollbackPawns;
}

bool UGMC_ReplicationCmp::ShouldRollBackGMCPawn_Implementation(const AGMC_Pawn* PawnToTest) const
{
  SCOPE_CYCLE_COUNTER(STAT_ShouldRollBackGMCPawn)

  gmc_ck(bRollBackServerPawns || bRollBackClientPawns)

  if (!IsValid(PawnToTest))
  {
    return false;
  }

  if (PawnToTest == PawnOwner)
  {
    // Do not roll back the pawn we are going to simulate the moves for.
    return false;
  }

  const auto& ReplicationComponent = PawnToTest->GetReplicationComponent();
  if (!ReplicationComponent)
  {
    // The considered pawn needs a replication component.
    return false;
  }

  if (ReplicationComponent->bExcludeFromRollback)
  {
    // The considered pawn is excluded from rollback.
    return false;
  }

  const auto& MoveHistoryOther = ReplicationComponent->MoveHistory;
  const int32 MoveHistoryNumOther = MoveHistoryOther.Num();
  if (MoveHistoryNumOther < 2)
  {
    // The considered pawn's move history is not filled yet.
    return false;
  }

  const float ConsiderationRadius = IsServerPawn() ? ServerPawnRollbackRadius : ClientPawnRollbackRadius;
  if ((PawnToTest->GetActorLocation() - GetActorLocation_GMC()).Size() > ConsiderationRadius)
  {
    // The considered pawn is further away than the set threshold.
    return false;
  }

  if (const auto& Controller = PawnOwner->GetController())
  {
    FVector ViewLocation{0.};
    Controller->GetPlayerViewPoint(ViewLocation, UNUSED(FRotator));
    if (!PawnToTest->IsNetRelevantFor(Controller, PawnOwner, ViewLocation))
    {
      // The considered pawn is not net relevant to this pawn meaning there either won't be any current states in its move history (on the client) or none of
      // the states in the history will have the owning client as receiver (on the server).
      return false;
    }
  }

  // Client rollback specific checks.
  if (IsAutonomousProxy())
  {
    gmc_ck(bRollBackClientPawns)

    if (!ReplicationComponent->IsSimulatedProxy())
    {
      // Only simulated proxies are rolled back for replay.
      return false;
    }
  }

  // The tested pawn should be rolled back.
  return true;
}

void UGMC_ReplicationCmp::RollBackPawns(double Time, const TArray<AGMC_Pawn*>& PawnsToRollBack, EGMC_NetContext Context) const
{
  SCOPE_CYCLE_COUNTER(STAT_RollBackPawns)

  gmc_ck(bRollBackServerPawns || bRollBackClientPawns)

  const auto& World = GetWorld();
  if (!World || PawnsToRollBack.Num() == 0)
  {
    return;
  }

  // We are interested in the states of the connection which owns the pawn that we are currently simulating (i.e. the client controller).
  const auto& OwningConnection = Cast<APlayerController>(PawnOwner->GetController());
  gmc_ck(OwningConnection)

  for (const auto& GMCPawn : PawnsToRollBack)
  {
    if (!IsValid(GMCPawn))
    {
      // Pawn may have been destroyed within the prediction tick.
      continue;
    }

    const auto& ReplicationComponent = GMCPawn->GetReplicationComponent();
    gmc_ck(ReplicationComponent)
    const auto& MoveHistoryOther = ReplicationComponent->MoveHistory;
    const int32 MoveHistoryOtherNum = MoveHistoryOther.Num();
    gmc_ck(MoveHistoryOtherNum > 0)

    const double SimulationTime = ComputeRollbackSimulationTime(Time, OwningConnection, GMCPawn, ReplicationComponent);

    // Find the states that were used during the original move execution.
    int32 StartIdx{-1};
    int32 TargetIdx{-1};
    float Alpha{-1.f};
    if (!ComputeRollbackParams(OwningConnection, SimulationTime, MoveHistoryOther, StartIdx, TargetIdx, Alpha))
    {
      GMC_LOG(
        LogGMCReplication,
        PawnOwner,
        Verbose,
        TEXT("No moves to roll back pawn %s (%s) found."),
        *GMCPawn->GetName(),
        *GetNetRoleAsString(GMCPawn->GetLocalRole())
      )

      continue;
    }

    gmc_ck(ReplicationComponent->IsValidMoveHistoryIndex(StartIdx))
    gmc_ck(ReplicationComponent->IsValidMoveHistoryIndex(TargetIdx))
    gmc_ck(Alpha >= 0.f)
    gmc_ck(Alpha <= 1.f + UE_KINDA_SMALL_NUMBER)

    SetRollbackState(Time, SimulationTime, StartIdx, TargetIdx, Alpha, ReplicationComponent, Context);

    // At this point the target pawn may already be rolled back to a different time if more than one client move is executed/replayed.
    if (IsServerPawn())
    {
      gmc_ck(IsRemotelyControlledServerPawn())
      gmc_ck(SV_IsExecutingRemoteMoves())
      ReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack = true;
    }
    else
    {
      gmc_ck(IsAutonomousProxy())
      gmc_ck(CL_IsReplaying())
      ReplicationComponent->CL_MoveExecutionAux.bIsRolledBack = true;
    }

    CALL_NATIVE_EVENT_CONDITIONAL(
      ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, OnPawnRolledBack, SimulationTime, StartIdx, TargetIdx, Context
    );
  }
}

double UGMC_ReplicationCmp::ComputeRollbackSimulationTime(
  double Time,
  APlayerController* const Connection,
  AGMC_Pawn* const OtherPawn,
  UGMC_ReplicationCmp* const OtherReplicationComponent
) const
{
  gmc_ck(IsValid(Connection))
  gmc_ck(IsValid(OtherPawn))
  gmc_ck(IsValid(OtherReplicationComponent))
  gmc_ck(OtherReplicationComponent ? OtherReplicationComponent->GetPawnOwner() == OtherPawn : false)

  const bool bUsingFixedDelay = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::FixedDelay;
  const bool bUsingAdaptiveDelay = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay;

  double SimulationTime = Time;

  if (bUsingFixedDelay)
  {
    // The delay must have the same fixed value as on client and server.
    SimulationTime -= OtherReplicationComponent->FixedDelayParams.FixedDelay;
  }
  else if (bUsingAdaptiveDelay)
  {
    // When using an adaptive delay we must retrieve the simulation delay value that was used when the move was originally executed.
    SimulationTime -= OtherReplicationComponent->GetAdaptiveDelayAt(Time, Connection);
  }
  else
  {
    // All other simulation modes have to rely on the client's ping (which is less accurate).

    {
      if (const auto& OwningConnectionPlayerState = Connection->PlayerState)
      {
        // Consider the latency of the client that is executing the move.
        SimulationTime -= OwningConnectionPlayerState->GetPingInMilliseconds() / 1000.f / 2.f;
      }

      // Also try to account for the net update frequency.
      const float AvgNetUpdateDelay = 1.f / FMath::Max(OtherReplicationComponent->PawnOwner->NetUpdateFrequency, UE_KINDA_SMALL_NUMBER) / 2.f;
      SimulationTime -= AvgNetUpdateDelay;

      // Try to account for unknown processing times (server + receiving client).
      constexpr float Bias = 0.005f;
      SimulationTime -= Bias;
    }

    if (OtherReplicationComponent->IsRemotelyControlledServerPawn())
    {
      // Consider the ping of the client whose pawn is being rolled back.
      if (const auto& OtherPlayerController = Cast<APlayerController>(OtherPawn->GetController()))
      {
        if (const auto& OtherPlayerState = OtherPlayerController->PlayerState)
        {
          // Consider the latency of the client that is being rolled back.
          SimulationTime -= OtherPlayerState->GetPingInMilliseconds() / 1000.f / 2.f;
        }
      }

      // Try to account for the send rate of the client that is being rolled back.
      const float AvgSendDelay = 1.f / FMath::Max(OtherReplicationComponent->GetClientSendRate(), UE_KINDA_SMALL_NUMBER) / 2.f;
      SimulationTime -= AvgSendDelay;

      // Try to account for unknown processing times (server + sending client).
      constexpr float Bias = 0.005f;
      SimulationTime -= Bias;
    }
  }

  return SimulationTime;
}

bool UGMC_ReplicationCmp::ComputeRollbackParams(
  APlayerController* Connection,
  double Time,
  const TGMC_CircularArray<FGMC_Move>& MoveHistoryToSearch,
  int32& OutStartIdx,
  int32& OutTargetIdx,
  float& OutAlpha
) const
{
  SCOPE_CYCLE_COUNTER(STAT_ComputeRollbackParams)

  gmc_ck(IsServerPawn() || IsAutonomousProxy())
  gmc_ck(OutAlpha == -1.f)
  gmc_ck(OutStartIdx == -1)
  gmc_ck(OutTargetIdx == -1)

  if (!IsValid(Connection))
  {
    return false;
  }

  const int32 HistoryNum = MoveHistoryToSearch.Num();
  int32 Index{0};

  if (IsServerPawn())
  {
    // This branch is usually run for regular move execution on the server (if rollback is enabled) but it may also be entered in another context when the user
    // rewinds a server pawn manually.

    // It is very likely that we need recent moves so we go through the history from newest to oldest.
    for (Index = HistoryNum - 1; Index >= 0; --Index)
    {
      const auto& Move = MoveHistoryToSearch[Index];
      const double Timestamp = Move.MetaData.bIsUsingServerAuthPhysics ? Move.MetaData.ServerAuthPhysicsTimestamp : Move.MetaData.Timestamp;

      if (Timestamp > Time)
      {
        continue;
      }

      if (!Move.NetInfo.ClientReceivers.Contains(Connection))
      {
        continue;
      }

      OutStartIdx = Index;
      break;
    }
  }
  else
  {
    gmc_ck(IsAutonomousProxy())
    gmc_ck(bRollBackClientPawns)

    for (Index = HistoryNum - 1; Index >= 0; --Index)
    {
      const auto& Move = MoveHistoryToSearch[Index];
      const double Timestamp = Move.MetaData.bIsUsingServerAuthPhysics ? Move.MetaData.ServerAuthPhysicsTimestamp : Move.MetaData.Timestamp;

      if (Timestamp > Time)
      {
        continue;
      }

      OutStartIdx = Index;
      break;
    }
  }

  if (Index < 0)
  {
    // Start move was not found in the history.
    gmc_ck(OutStartIdx == -1)
    gmc_ck(OutTargetIdx == -1)
    gmc_ck(OutAlpha == -1.f)
    return false;
  }

  gmc_ck(OutStartIdx >= 0)

  if (IsServerPawn())
  {
    // This branch is usually run for regular move execution on the server (if rollback is enabled) but it may also be entered in another context when the user
    // rewinds a server pawn manually.

    // We have found the start index, go back up through the history and find the target index which is the next move with a greater timestamp that was also
    // replicated to the owning client.
    while (++Index < HistoryNum)
    {
      const auto& TargetMove = MoveHistoryToSearch[Index];

      if (!TargetMove.NetInfo.ClientReceivers.Contains(Connection))
      {
        continue;
      }

      OutTargetIdx = Index;
      const auto& StartMove = MoveHistoryToSearch[OutStartIdx];
      const double StartTimestamp =
        StartMove.MetaData.bIsUsingServerAuthPhysics ? StartMove.MetaData.ServerAuthPhysicsTimestamp : StartMove.MetaData.Timestamp;
      const double TargetTimestamp =
        TargetMove.MetaData.bIsUsingServerAuthPhysics ? TargetMove.MetaData.ServerAuthPhysicsTimestamp : TargetMove.MetaData.Timestamp;
      OutAlpha = (Time - StartTimestamp) / FMath::Max(TargetTimestamp - StartTimestamp, MIN_DELTA_TIME);
      gmc_ck(OutAlpha >= 0.f)
      gmc_ck(OutAlpha <= 1.f + UE_KINDA_SMALL_NUMBER)

      break;
    }
  }
  else
  {
    gmc_ck(IsAutonomousProxy())
    gmc_ck(bRollBackClientPawns)

    // We have found the start index, the target index is the next one in the history.
    if (++Index < HistoryNum)
    {
      OutTargetIdx = Index;
      const auto& StartMove = MoveHistoryToSearch[OutStartIdx];
      const auto& TargetMove = MoveHistoryToSearch[OutTargetIdx];
      const double StartTimestamp =
        StartMove.MetaData.bIsUsingServerAuthPhysics ? StartMove.MetaData.ServerAuthPhysicsTimestamp : StartMove.MetaData.Timestamp;
      const double TargetTimestamp =
        TargetMove.MetaData.bIsUsingServerAuthPhysics ? TargetMove.MetaData.ServerAuthPhysicsTimestamp : TargetMove.MetaData.Timestamp;
      OutAlpha = (Time - StartTimestamp) / FMath::Max(TargetTimestamp - StartTimestamp, MIN_DELTA_TIME);
      gmc_ck(OutAlpha >= 0.f)
      gmc_ck(OutAlpha <= 1.f + UE_KINDA_SMALL_NUMBER)
    }
  }

  if (OutTargetIdx == -1)
  {
    gmc_ck(OutAlpha == -1.f)
    gmc_ck(OutStartIdx >= 0)

    // If no target move was found, we use the same state as start and target.
    OutTargetIdx = OutStartIdx;
    OutAlpha = 1.f;
  }

  return true;
}

void UGMC_ReplicationCmp::SetRollbackState(
  double Time,
  double SimulationTime,
  int32 StartIdx,
  int32 TargetIdx,
  float Alpha,
  UGMC_ReplicationCmp* const OtherReplicationComponent,
  EGMC_NetContext Context
) const
{
  gmc_ck(IsValid(OtherReplicationComponent))
  gmc_ck(OtherReplicationComponent ? OtherReplicationComponent->IsValidMoveHistoryIndex(StartIdx) : false)
  gmc_ck(OtherReplicationComponent ? OtherReplicationComponent->IsValidMoveHistoryIndex(TargetIdx) : false)
  gmc_ck(Alpha >= 0.f)
  gmc_ck(Alpha <= 1.f + UE_KINDA_SMALL_NUMBER)

  const bool bUsingNoSmoothing = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::None;
  const bool bUsingMatchLatest = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::MatchLatest;
  const bool bUsingFixedDelay = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::FixedDelay;
  const bool bUsingAdaptiveDelay = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay;
  const bool bUsingUniformSimulation = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::UniformSimulation;
  const bool bUsingCumulativeSimulation = OtherReplicationComponent->InterpolationMode == EGMC_InterpolationMode::CumulativeSimulation;
  gmc_ck(bUsingNoSmoothing ^ bUsingMatchLatest ^ bUsingFixedDelay ^ bUsingAdaptiveDelay ^ bUsingUniformSimulation ^ bUsingCumulativeSimulation)

  if (bUsingFixedDelay || bUsingAdaptiveDelay)
  {
    const auto& OtherFixedDelayParams = OtherReplicationComponent->FixedDelayParams;
    const auto& OtherAdaptiveDelayParams = OtherReplicationComponent->AdaptiveDelayParams;

    const float TeleportThreshold = bUsingFixedDelay ? OtherFixedDelayParams.TeleportThreshold : OtherAdaptiveDelayParams.TeleportThreshold;
    const EGMC_InterpolationStates InterpStates = bUsingFixedDelay ? OtherFixedDelayParams.InterpStates : OtherAdaptiveDelayParams.InterpStates;

    OtherReplicationComponent->SmoothDelay(
      StartIdx,
      TargetIdx,
      Alpha,
      true,
      true,
      -1.f,
      TeleportThreshold,
      InterpStates,
      bSkipBoneUpdateForRollback,
      Context
    );
  }
  else if (bUsingNoSmoothing || bUsingMatchLatest)
  {
    const EGMC_InterpolationStates InterpStates =
      bUsingNoSmoothing ? OtherReplicationComponent->NoInterpStates : OtherReplicationComponent->MatchLatestParams.InterpStates;

    const double StartTimestamp = OtherReplicationComponent->MoveHistory[StartIdx].MetaData.Timestamp;
    const double TargetTimestamp = OtherReplicationComponent->MoveHistory[TargetIdx].MetaData.Timestamp;
    const double StartStateTimeDiff = FMath::Abs(SimulationTime - StartTimestamp);
    const double TargetStateTimeDiff = FMath::Abs(SimulationTime - TargetTimestamp);
    const int32 RollbackIdx = StartStateTimeDiff < TargetStateTimeDiff ? StartIdx : TargetIdx;

    // Setting the target state directly is the best approximation we can do for these smoothing modes.
    OtherReplicationComponent->SmoothNone(
      RollbackIdx,
      true,
      InterpStates,
      bSkipBoneUpdateForRollback,
      Context
    );
  }
  else
  {
    gmc_ck(bUsingUniformSimulation || bUsingCumulativeSimulation)

    const auto& OtherUniformSimulationParams = OtherReplicationComponent->UniformSimulationParams;
    const auto& OtherCumulativeSimulationParams = OtherReplicationComponent->CumulativeSimulationParams;

    const float MaxSimulationTimeStep =
      bUsingUniformSimulation ? OtherUniformSimulationParams.MaxTimeStep : OtherCumulativeSimulationParams.MaxTimeStep;
    const int32 MaxSimulationIterations =
      bUsingUniformSimulation ? OtherUniformSimulationParams.MaxIterations : OtherCumulativeSimulationParams.MaxIterations;
    const EGMC_InterpolationStates SimulationStates =
      bUsingUniformSimulation ? OtherUniformSimulationParams.SimStates : OtherCumulativeSimulationParams.SimStates;

    const double StartTimestamp = OtherReplicationComponent->MoveHistory[StartIdx].MetaData.Timestamp;
    const double TargetTimestamp = OtherReplicationComponent->MoveHistory[TargetIdx].MetaData.Timestamp;

    const double StartStateTimeDiff = FMath::Abs(SimulationTime - StartTimestamp);
    const double TargetStateTimeDiff = FMath::Abs(SimulationTime - TargetTimestamp);
    const int32 RollbackIdx = StartStateTimeDiff < TargetStateTimeDiff ? StartIdx : TargetIdx;

    const float DeltaTime = (bUsingUniformSimulation ? Time : SimulationTime) - OtherReplicationComponent->MoveHistory[RollbackIdx].MetaData.Timestamp;

    constexpr float DeltaTimeThreshold = 0.01f;
    if (DeltaTime <= DeltaTimeThreshold)
    {
      // Save the simulation if we are very close to the target state anyway or the delta time is negative.
      OtherReplicationComponent->SmoothNone(
        RollbackIdx,
        true,
        SimulationStates,
        bSkipBoneUpdateForRollback,
        Context
      );
    }
    else
    {
      // We do not execute multiple iterations for cumulative simulation here. Instead, we simply treat the delta time value calculated from the simulation time
      // as "uniform" as well. Any integration error caused by this is insignificant compared to how inaccurate guessing the simulation time based on ping
      // already is.
      OtherReplicationComponent->SmoothSimulated(
        RollbackIdx,
        false,
        DeltaTime,
        Time,
        MaxSimulationTimeStep,
        MaxSimulationIterations,
        true,
        true,
        SimulationStates,
        bSkipBoneUpdateForRollback,
        Context
      );
    }
  }
}

void UGMC_ReplicationCmp::RestoreRolledBackPawns(const TArray<AGMC_Pawn*>& PawnsToRestore, EGMC_NetContext Context) const
{
  SCOPE_CYCLE_COUNTER(STAT_RestoreRolledBackPawns)

  gmc_ck(bUseClientPrediction)

  for (const auto& GMCPawn : PawnsToRestore)
  {
    if (!IsValid(GMCPawn))
    {
      // Pawn may have been destroyed within the prediction tick.
      continue;
    }

    const auto& ReplicationComponent = GMCPawn->GetReplicationComponent();

    gmc_ck(ReplicationComponent)
    gmc_ck(GMCPawn != PawnOwner)
    gmc_ck(IsServerPawn() ? true : IsAutonomousProxy() && GMCPawn->GetLocalRole() == ROLE_SimulatedProxy)

    if (IsServerPawn())
    {
      gmc_ck(IsRemotelyControlledServerPawn())
      gmc_ck(SV_IsExecutingRemoteMoves())
      if (ReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack)
      {
        RestoreOriginalStateAfterRollback(GMCPawn, bUseRelativeValuesForPrediction, Context);
        ReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack = false;
        CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, OnPawnRestoredAfterRollback, Context);
      }
    }
    else
    {
      gmc_ck(IsAutonomousProxy())
      gmc_ck(CL_IsReplaying())
      if (ReplicationComponent->CL_MoveExecutionAux.bIsRolledBack)
      {
        RestoreOriginalStateAfterRollback(GMCPawn, bUseRelativeValuesForPrediction, Context);
        ReplicationComponent->CL_MoveExecutionAux.bIsRolledBack = false;
        CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, OnPawnRestoredAfterRollback, Context);
      }
    }
  }
}

FGMC_Move UGMC_ReplicationCmp::FindMove(double Timestamp) const
{
  const int32 Index = FindMoveByIndex(Timestamp);

  if (IsValidMoveHistoryIndex(Index))
  {
    return MoveHistory[Index];
  }

  return FGMC_Move{};
}

int32 UGMC_ReplicationCmp::FindMoveByIndex(double Timestamp) const
{
  if (Timestamp < 0.)
  {
    return -1;
  }

  const int32 MoveHistoryNum = MoveHistory.Num();
  if (MoveHistoryNum == 0)
  {
    return -1;
  }

  // It is more likely that a recent move is requested so we go through the history from newest to oldest.
  double PrevMoveTimestamp = -1.;
  for (int32 Index = MoveHistoryNum - 1; Index >= 0; --Index)
  {
    const auto& Move = MoveHistory[Index];

    gmc_ck(Move.HasValidTimestamp())
    const double MoveTimestamp = Move.MetaData.Timestamp;

    if (MoveTimestamp == Timestamp)
    {
      return Index;
    }

    if (MoveTimestamp > Timestamp)
    {
      PrevMoveTimestamp = MoveTimestamp;
      continue;
    }

    gmc_ck(MoveTimestamp < Timestamp)

    if (PrevMoveTimestamp == -1.)
    {
      return Index;
    }

    gmc_ck(PrevMoveTimestamp > Timestamp)

    const double CurrMoveDiff = Timestamp - MoveTimestamp;
    const double PrevMoveDiff = PrevMoveTimestamp - Timestamp;
    gmc_ck(CurrMoveDiff > 0.)
    gmc_ck(PrevMoveDiff > 0.)

    const int32 PrevIndex = Index + 1;
    gmc_ck(IsValidMoveHistoryIndex(PrevIndex))

    return CurrMoveDiff <= PrevMoveDiff ? Index : PrevIndex;
  }

  // The passed timestamp is farther in the past than the oldest saved move in the move history.
  return -1;
}

bool UGMC_ReplicationCmp::SV_GetRewindParams(
  APlayerController* Connection,
  double Time,
  AGMC_Pawn* Pawn,
  int32& OutStartIdx,
  int32& OutTargetIdx,
  float& OutAlpha
) const
{
  OutStartIdx = -1;
  OutTargetIdx = -1;
  OutAlpha = -1.f;

  if (!IsValid(PawnOwner) || !IsValid(Pawn) || !IsValid(Connection))
  {
    return false;
  }

  if (Pawn->GetLocalRole() != ROLE_Authority)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("GetRewindParams failed: can only be called for server pawns."))
    return false;
  }

  if (Connection->IsLocalController())
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("GetRewindParams failed: the target controller must be a client."))
    return false;
  }

  if (Pawn->GetController() == Connection)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("GetRewindParams failed: cannot get params for the target pawn's owning connection."))
    return false;
  }

  const auto& PawnReplicationComponent = Cast<UGMC_ReplicationCmp>(Pawn->GetMovementComponent());
  if (!PawnReplicationComponent)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("GetRewindParams failed: the target pawn does not have a replication component."))
    return false;
  }

  const auto& PawnMoveHistory = PawnReplicationComponent->MoveHistory;
  if (PawnMoveHistory.Num() == 0)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("GetRewindParams failed: the target pawn's move history is empty."))
    return false;
  }

  const double SimulationTime = ComputeRollbackSimulationTime(Time, Connection, Pawn, PawnReplicationComponent);

  if (!ComputeRollbackParams(Connection, SimulationTime, PawnMoveHistory, OutStartIdx, OutTargetIdx, OutAlpha))
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("GetRewindParams failed: No moves found."))
    return false;
  }

  gmc_ck(PawnReplicationComponent->IsValidMoveHistoryIndex(OutStartIdx))
  gmc_ck(PawnReplicationComponent->IsValidMoveHistoryIndex(OutTargetIdx))
  gmc_ck(OutAlpha >= 0.f)
  gmc_ck(OutAlpha <= 1.f + UE_KINDA_SMALL_NUMBER)

  return true;
}

bool UGMC_ReplicationCmp::SV_RewindPawn(APlayerController* Connection, double Time, AGMC_Pawn* Pawn, bool bSkipBoneUpdate)
{
  if (!IsValid(PawnOwner) || !IsValid(Pawn) || !IsValid(Connection))
  {
    return false;
  }

  if (Pawn->GetLocalRole() != ROLE_Authority)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Rewind failed: only server pawns can be rewound."))
    return false;
  }

  if (Connection->IsLocalController())
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Rewind failed: the target controller must be a client."))
    return false;
  }

  if (Pawn->GetController() == Connection)
  {
    // This does not work because we only save simulated proxy server states and the controller in this case is the autonomous proxy connection that owns the
    // target pawn.
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Rewind failed: cannot rewind a pawn for its owning connection."))
    return false;
  }

  const auto& PawnReplicationComponent = Cast<UGMC_ReplicationCmp>(Pawn->GetMovementComponent());
  if (!PawnReplicationComponent)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Rewind failed: the target pawn does not have a replication component."))
    return false;
  }

  if (PawnReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Rewind failed: the target pawn is already rewound."))
    return false;
  }

  const auto& PawnMoveHistory = PawnReplicationComponent->MoveHistory;
  if (PawnMoveHistory.Num() == 0)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Rewind failed: the target pawn's move history is empty."))
    return false;
  }

  const double SimulationTime = ComputeRollbackSimulationTime(Time, Connection, Pawn, PawnReplicationComponent);

  int32 StartIdx{-1};
  int32 TargetIdx{-1};
  float Alpha{-1.f};
  if (!ComputeRollbackParams(Connection, SimulationTime, PawnMoveHistory, StartIdx, TargetIdx, Alpha))
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Rewind failed: No moves found."))
    return false;
  }

  gmc_ck(PawnReplicationComponent->IsValidMoveHistoryIndex(StartIdx))
  gmc_ck(PawnReplicationComponent->IsValidMoveHistoryIndex(TargetIdx))
  gmc_ck(Alpha >= 0.f)
  gmc_ck(Alpha <= 1.f + UE_KINDA_SMALL_NUMBER)

  SaveLocalStateBeforeRollback(Pawn, false);

  SetRollbackState(Time, SimulationTime, StartIdx, TargetIdx, Alpha, PawnReplicationComponent, EGMC_NetContext::ManualRewind);

  gmc_ck(!PawnReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack)
  PawnReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack = true;

  CALL_NATIVE_EVENT_CONDITIONAL(PawnReplicationComponent->bNoBlueprintEvents, PawnReplicationComponent, SV_OnPawnRewound, SimulationTime, StartIdx, TargetIdx);

  return true;
}

bool UGMC_ReplicationCmp::SV_RestorePawnAfterRewind(AGMC_Pawn* Pawn, bool bSkipBoneUpdate)
{
  if (!IsValid(PawnOwner) || !IsValid(Pawn))
  {
    return false;
  }

  if (Pawn->GetLocalRole() != ROLE_Authority)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Restore after rewind failed: only server pawns can be rewound."))
    return false;
  }

  const auto& PawnReplicationComponent = Cast<UGMC_ReplicationCmp>(Pawn->GetMovementComponent());
  if (!PawnReplicationComponent)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Restore after rewind failed: the target pawn does not have a replication component."))
    return false;
  }

  if (!PawnReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack)
  {
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Restore after rewind failed: the target pawn is not rewound."))
    return false;
  }

  RestoreOriginalStateAfterRollback(Pawn, false, EGMC_NetContext::ManualRewind);

  gmc_ck(PawnReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack)
  PawnReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack = false;

  CALL_NATIVE_EVENT_CONDITIONAL(PawnReplicationComponent->bNoBlueprintEvents, PawnReplicationComponent, SV_OnPawnRestoredAfterRewind);

  return true;
}

bool UGMC_ReplicationCmp::SV_IsRewound(AGMC_Pawn* Pawn) const
{
  if (!IsValid(Pawn))
  {
    return false;
  }

  const auto& PawnReplicationComponent = Pawn->GetReplicationComponent();
  if (!PawnReplicationComponent || !PawnReplicationComponent->IsServerPawn())
  {
    return false;
  }

  if (PawnReplicationComponent->SV_RemoteMoveExecutionAux.bIsRolledBack)
  {
    return true;
  }

  return false;
}

void UGMC_ReplicationCmp::TickGenericRollbackActors(
  double Time,
  float DeltaTime,
  const FGMC_Move& Move,
  EGMC_NetContext Context
) const
{
  SCOPE_CYCLE_COUNTER(STAT_TickGenericRollbackActors)

  gmc_ck(PawnOwner->IsLocallyControlled())
  gmc_ck(PawnOwner->GetController())
  gmc_ck(Cast<AGMC_PlayerController>(PawnOwner->GetController()))

  TArray<AActor*> Actors{};

  if (IsValid(GMCAggregator))
  {
    Actors = static_cast<const TArray<AActor*>>(GMCAggregator->GetRollbackActors());
  }
  else
  {
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGMC_RollbackActor::StaticClass(), Actors);
  }

  for (const auto& Actor : Actors)
  {
    const auto& RollbackActor = Cast<AGMC_RollbackActor>(Actor);

    if (!IsValid(RollbackActor) || !CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, ShouldTickGMCActor, RollbackActor))
    {
      continue;
    }

    CALL_NATIVE_EVENT_CONDITIONAL(RollbackActor->bNoBlueprintEvents, RollbackActor, UpdateState, Time, DeltaTime, Move, Context);
    RollbackActor->SetTicked(true);
  }
}

bool UGMC_ReplicationCmp::ShouldTickGMCActor_Implementation(const AGMC_RollbackActor* RollbackActor) const
{
  return IsValid(RollbackActor) && !RollbackActor->HasTicked() && !RollbackActor->bExcludeFromRollback;
}

TArray<class AGMC_RollbackActor*> UGMC_ReplicationCmp::GatherGenericRollbackActors() const
{
  SCOPE_CYCLE_COUNTER(STAT_GatherGenericRollbackActors)

  gmc_ck(bRollBackGenericServerActors || bRollBackGenericClientActors)

  TArray<AActor*> Actors{};

  if (IsValid(GMCAggregator))
  {
    Actors = static_cast<const TArray<AActor*>>(GMCAggregator->GetRollbackActors());
  }
  else
  {
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGMC_RollbackActor::StaticClass(), Actors);
  }

  TArray<AGMC_RollbackActor*> RollbackActors{};

  for (const auto& Actor : Actors)
  {
    const auto& RollbackActor = Cast<AGMC_RollbackActor>(Actor);

    if (!IsValid(RollbackActor) || !CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, ShouldRollBackGMCActor, RollbackActor))
    {
      continue;
    }

    RollbackActors.Emplace(RollbackActor);
  }

  return RollbackActors;
}

bool UGMC_ReplicationCmp::ShouldRollBackGMCActor_Implementation(const AGMC_RollbackActor* ActorToTest) const
{
  SCOPE_CYCLE_COUNTER(STAT_ShouldRollBackGMCActor)

  gmc_ck(bRollBackGenericServerActors || bRollBackGenericClientActors)

  if (!IsValid(ActorToTest))
  {
    // The considered actor is pending kill.
    return false;
  }

  if (ActorToTest->bExcludeFromRollback)
  {
    // The considered actor is excluded from rollback.
    return false;
  }

  const float ConsiderationRadius = IsServerPawn() ? GenericServerActorRollbackRadius : GenericClientActorRollbackRadius;
  if ((ActorToTest->GetActorLocation() - GetActorLocation_GMC()).Size() > ConsiderationRadius)
  {
    // The considered actor is further away than the set threshold.
    return false;
  }

  // The tested actor should be rolled back.
  return true;
}

void UGMC_ReplicationCmp::BufferGenericRollbackActorStates(
  const TArray<AGMC_RollbackActor*>& RollbackActors,
  EGMC_NetContext Context
) const
{
  gmc_ck(bRollBackGenericServerActors || bRollBackGenericClientActors)

  for (const auto& RollbackActor : RollbackActors)
  {
    gmc_ck(RollbackActor)
    CALL_NATIVE_EVENT_CONDITIONAL(RollbackActor->bNoBlueprintEvents, RollbackActor, SaveState, Context);
  }
}

void UGMC_ReplicationCmp::RollbackGenericActors(
  double Time,
  float DeltaTime,
  const FGMC_Move& Move,
  const TArray<AGMC_RollbackActor*>& ActorsToRollBack,
  EGMC_NetContext Context
) const
{
  SCOPE_CYCLE_COUNTER(STAT_RollbackGenericActors)

  gmc_ck(bRollBackGenericServerActors || bRollBackGenericClientActors)

  for (const auto& RollbackActor : ActorsToRollBack)
  {
    gmc_ck(RollbackActor)
    CALL_NATIVE_EVENT_CONDITIONAL(RollbackActor->bNoBlueprintEvents, RollbackActor, UpdateState, Time, DeltaTime, Move, Context);
  }
}

void UGMC_ReplicationCmp::RestoreRolledBackGenericActors(
  const TArray<AGMC_RollbackActor*>& RollbackActors,
  EGMC_NetContext Context
) const
{
  SCOPE_CYCLE_COUNTER(STAT_RestoreRolledBackGenericActors)

  gmc_ck(bRollBackGenericServerActors || bRollBackGenericClientActors)

  for (const auto& RollbackActor : RollbackActors)
  {
    gmc_ck(RollbackActor)
    CALL_NATIVE_EVENT_CONDITIONAL(RollbackActor->bNoBlueprintEvents, RollbackActor, LoadState, Context);
  }
}

AGMC_Pawn* UGMC_ReplicationCmp::GetGMCPawnOwner() const
{
  return Cast<AGMC_Pawn>(PawnOwner);
}

bool UGMC_ReplicationCmp::CL_IsReplaying() const
{
  return CL_MoveExecutionAux.bIsReplaying;
}

int32 UGMC_ReplicationCmp::CL_GetCurrentReplayMoveIndex() const
{
  gmc_ck(CL_IsReplaying())
  gmc_ck(CL_MoveExecutionAux.ReplayMoveIdx >= 0 && CL_MoveExecutionAux.ReplayMoveIdx < MoveHistory.Num())
  return CL_MoveExecutionAux.ReplayMoveIdx;
}

const FGMC_Move& UGMC_ReplicationCmp::CL_GetCurrentReplayMove() const
{
  gmc_ck(CL_IsReplaying())
  gmc_ck(CL_MoveExecutionAux.ReplayMoveIdx >= 0 && CL_MoveExecutionAux.ReplayMoveIdx < MoveHistory.Num())
  return MoveHistory[CL_MoveExecutionAux.ReplayMoveIdx];
}

bool UGMC_ReplicationCmp::SV_IsExecutingRemoteMoves() const
{
  return SV_RemoteMoveExecutionAux.bIsExecutingRemoteMoves;
}

double UGMC_ReplicationCmp::CL_GetTimeSinceLastMoveBatchWasSent() const
{
  return CL_MoveExecutionAux.TimeSinceLastMoveBatchWasSent;
}

bool UGMC_ReplicationCmp::IsSimulating() const
{
  return SimulationAux.bIsSimulating;
}

double UGMC_ReplicationCmp::GetLastInterpTargetTime() const
{
  return SimulationAux.PrevInterpTargetTime;
}

double UGMC_ReplicationCmp::GetLastExtrapTargetTime() const
{
  return SimulationAux.PrevExtrapTargetTime;
}

bool UGMC_ReplicationCmp::IsCumulativeSmoothingUpdate() const
{
  return SimulationAux.bIsCumulativeUpdate;
}

bool UGMC_ReplicationCmp::IsExtrapolating() const
{
  return SimulationAux.bIsExtrapolating;
}

bool UGMC_ReplicationCmp::HasReachedMaxExtrapolationDistance() const
{
  return SimulationAux.bMaxExtrapolationDistanceReached;
}

bool UGMC_ReplicationCmp::WasExtrapolatingLastUpdate() const
{
  return SimulationAux.bWasExtrapolatingLastUpdate;
}

void UGMC_ReplicationCmp::TriggerExtrapolationRecovery(bool bInWorldSpace)
{
  SimulationAux.ExtrapolationRecoveryTimer = -1.f;
  SimulationAux.ExtrapolationRecoveryStartState = FGMC_PawnState{};
  SimulationAux.bTriggerManualExtrapolationRecovery = true;
  SimulationAux.bManualExtrapolationRecoveryInWorldSpace = bInWorldSpace;
  SimulationAux.NumAbortedExtrapolationRecoveries = 0;
}

bool UGMC_ReplicationCmp::IsPerformingExtrapolationRecovery() const
{
  return SimulationAux.ExtrapolationRecoveryTimer >= 0.f;
}

void UGMC_ReplicationCmp::SetComponentToSmooth(USceneComponent* Component)
{
  const auto& Owner = GetOwner();
  if (IsValid(Component) && IsValid(Owner) && Owner == Component->GetOwner())
  {
    PostSmoothingParams.Component = Component;
    PostSmoothingParams.ComponentOffset = Component->GetComponentTransform().GetRelativeTransform(Owner->GetActorTransform());
  }
  else
  {
    PostSmoothingParams.Component = nullptr;
    PostSmoothingParams.Reset();
  }
}

USceneComponent* UGMC_ReplicationCmp::GetComponentToSmooth() const
{
  return PostSmoothingParams.Component;
}

FTransform UGMC_ReplicationCmp::GetComponentToSmoothOffset() const
{
  return PostSmoothingParams.Component ? PostSmoothingParams.ComponentOffset : FTransform::Identity;
}

void UGMC_ReplicationCmp::SetComponentWorldTransformWithSmoothOffset(
  const FTransform& ComponentWorldTransform,
  bool bSweep,
  FHitResult& OutSweepHitResult,
  bool bTeleportPhysics,
  USceneComponent* SmoothComponent,
  const FTransform& SmoothOffset
)
{
  if (!IsValid(SmoothComponent))
  {
    return;
  }

  SmoothComponent->K2_SetWorldTransform(ComponentWorldTransform, bSweep, OutSweepHitResult, bTeleportPhysics);

  if (!IsSimulatedPawn())
  {
    return;
  }

  const auto& Owner = Cast<AGMC_Pawn>(SmoothComponent->GetOwner());
  if (!IsValid(Owner))
  {
    return;
  }

  const auto& ReplicationComponent = Owner->GetReplicationComponent();
  if (!IsValid(ReplicationComponent))
  {
    return;
  }

  auto& SmoothingParams = ReplicationComponent->PostSmoothingParams;
  if (SmoothingParams.Component != SmoothComponent)
  {
    return;
  }

  SmoothingParams.ComponentOffset = SmoothOffset;
}

void UGMC_ReplicationCmp::SetComponentWorldLocationWithSmoothOffset(
  const FVector& ComponentWorldLocation,
  bool bSweep,
  FHitResult& OutSweepHitResult,
  bool bTeleportPhysics,
  USceneComponent* SmoothComponent,
  const FVector& SmoothOffset
)
{
  if (!IsValid(SmoothComponent))
  {
    return;
  }

  SmoothComponent->K2_SetWorldLocation(ComponentWorldLocation, bSweep, OutSweepHitResult, bTeleportPhysics);

  if (!IsSimulatedPawn())
  {
    return;
  }

  const auto& Owner = Cast<AGMC_Pawn>(SmoothComponent->GetOwner());
  if (!IsValid(Owner))
  {
    return;
  }

  const auto& ReplicationComponent = Owner->GetReplicationComponent();
  if (!IsValid(ReplicationComponent))
  {
    return;
  }

  auto& SmoothingParams = ReplicationComponent->PostSmoothingParams;
  if (SmoothingParams.Component != SmoothComponent)
  {
    return;
  }

  SmoothingParams.ComponentOffset.SetLocation(SmoothOffset);
}

void UGMC_ReplicationCmp::SetComponentWorldRotationWithSmoothOffset(
  const FRotator& ComponentWorldRotation,
  bool bTeleportPhysics,
  USceneComponent* SmoothComponent,
  const FRotator& SmoothOffset
)
{
  if (!IsValid(SmoothComponent))
  {
    return;
  }

  SmoothComponent->K2_SetWorldRotation(ComponentWorldRotation, false, UNUSED(FHitResult), bTeleportPhysics);

  if (!IsSimulatedPawn())
  {
    return;
  }

  const auto& Owner = Cast<AGMC_Pawn>(SmoothComponent->GetOwner());
  if (!IsValid(Owner))
  {
    return;
  }

  const auto& ReplicationComponent = Owner->GetReplicationComponent();
  if (!IsValid(ReplicationComponent))
  {
    return;
  }

  auto& SmoothingParams = ReplicationComponent->PostSmoothingParams;
  if (SmoothingParams.Component != SmoothComponent)
  {
    return;
  }

  SmoothingParams.ComponentOffset.SetRotation(SmoothOffset.Quaternion());
}

void UGMC_ReplicationCmp::SetComponentWorldScaleWithSmoothOffset(
  const FVector& ComponentWorldScale,
  USceneComponent* SmoothComponent,
  const FVector& SmoothOffset
)
{
  if (!IsValid(SmoothComponent))
  {
    return;
  }

  SmoothComponent->SetWorldScale3D(ComponentWorldScale);

  if (!IsSimulatedPawn())
  {
    return;
  }

  const auto& Owner = Cast<AGMC_Pawn>(SmoothComponent->GetOwner());
  if (!IsValid(Owner))
  {
    return;
  }

  const auto& ReplicationComponent = Owner->GetReplicationComponent();
  if (!IsValid(ReplicationComponent))
  {
    return;
  }

  auto& SmoothingParams = ReplicationComponent->PostSmoothingParams;
  if (SmoothingParams.Component != SmoothComponent)
  {
    return;
  }

  SmoothingParams.ComponentOffset.SetScale3D(SmoothOffset);
}

void UGMC_ReplicationCmp::CL_DoNotCombineNextMove()
{
  if (IsAutonomousProxy() && !CL_IsReplaying())
  {
    CL_MoveExecutionAux.bDoNotCombineNextMove = true;
  }
}

const FGMC_Move& UGMC_ReplicationCmp::SV_GetLastClientData() const
{
  return SV_RemoteMoveExecutionAux.LastRawMove;
}

const FGMC_Move& UGMC_ReplicationCmp::AccessMoveHistory(int32 Index) const
{
  return MoveHistory[Index];
}

bool UGMC_ReplicationCmp::IsValidMoveHistoryIndex(int32 Index) const
{
  return Index >= 0 && Index < MoveHistory.Num();
}

int32 UGMC_ReplicationCmp::GetCurrentMoveHistoryNum() const
{
  return MoveHistory.Num();
}

int32 UGMC_ReplicationCmp::GetMoveHistoryMaxSize() const
{
  return MoveHistoryMaxSize;
}

int32 UGMC_ReplicationCmp::CL_GetNumPendingClientMoves() const
{
  return CL_MoveExecutionAux.PendingMoves.Num();
}

FVector UGMC_ReplicationCmp::GetLinearVelocityFromState(const FGMC_PawnState& State) const
{
  return State.LinearVelocity.Read();
}

FVector UGMC_ReplicationCmp::GetAngularVelocityFromState(const FGMC_PawnState& State) const
{
  return State.AngularVelocity.Read();
}

FVector UGMC_ReplicationCmp::GetActorLocationFromState(const FGMC_PawnState& State) const
{
  return State.ActorLocation.Read();
}

FRotator UGMC_ReplicationCmp::GetActorRotationFromState(const FGMC_PawnState& State) const
{
  return State.ActorRotation.Read();
}

FVector UGMC_ReplicationCmp::GetActorScaleFromState(const FGMC_PawnState& State) const
{
  return State.ActorScale.Read();
}

FRotator UGMC_ReplicationCmp::GetControlRotationFromState(const FGMC_PawnState& State) const
{
  return State.ControlRotation.Read();
}

void UGMC_ReplicationCmp::SetLinearVelocityInState(const FVector& Value, FGMC_PawnState& State) const
{
  State.LinearVelocity.Write(Value);
}

void UGMC_ReplicationCmp::SetAngularVelocityInState(const FVector& Value, FGMC_PawnState& State) const
{
  State.AngularVelocity.Write(Value);
}

void UGMC_ReplicationCmp::SetActorLocationInState(const FVector& Value, FGMC_PawnState& State) const
{
  State.ActorLocation.Write(Value);
}

void UGMC_ReplicationCmp::SetActorRotationInState(const FRotator& Value, FGMC_PawnState& State) const
{
  State.ActorRotation.Write(Value);
}

void UGMC_ReplicationCmp::SetActorScaleInState(const FVector& Value, FGMC_PawnState& State) const
{
  State.ActorScale.Write(Value);
}

void UGMC_ReplicationCmp::SetControlRotationInState(const FRotator& Value, FGMC_PawnState& State) const
{
  State.ControlRotation.Write(Value);
}

void UGMC_ReplicationCmp::SetLinearVelocitySuspended(bool bSuspend, FGMC_PawnState& State) const
{
  State.LinearVelocity.SetSuspended(bSuspend);
}

void UGMC_ReplicationCmp::SetAngularVelocitySuspended(bool bSuspend, FGMC_PawnState& State) const
{
  State.AngularVelocity.SetSuspended(bSuspend);
}

void UGMC_ReplicationCmp::SetActorLocationSuspended(bool bSuspend, FGMC_PawnState& State) const
{
  State.ActorLocation.SetSuspended(bSuspend);
}

void UGMC_ReplicationCmp::SetActorRotationSuspended(bool bSuspend, FGMC_PawnState& State) const
{
  State.ActorRotation.SetSuspended(bSuspend);
}

void UGMC_ReplicationCmp::SetActorScaleSuspended(bool bSuspend, FGMC_PawnState& State) const
{
  State.ActorScale.SetSuspended(bSuspend);
}

void UGMC_ReplicationCmp::SetControlRotationSuspended(bool bSuspend, FGMC_PawnState& State) const
{
  State.ControlRotation.SetSuspended(bSuspend);
}

void UGMC_ReplicationCmp::SetVelocity(const FVector& InLinearVelocity)
{
  SetLinearVelocity_GMC(InLinearVelocity);
}

FVector UGMC_ReplicationCmp::GetVelocity() const
{
  return GetLinearVelocity_GMC();
}

void UGMC_ReplicationCmp::SetLinearVelocity_GMC(const FVector& InLinearVelocity)
{
  Velocity = InLinearVelocity;
  UpdateComponentVelocity();
}

FVector UGMC_ReplicationCmp::GetLinearVelocity_GMC() const
{
  return Velocity;
}

void UGMC_ReplicationCmp::SetAngularVelocity_GMC(const FVector& InAngularVelocity)
{
  AngularVelocity = InAngularVelocity;
}

FVector UGMC_ReplicationCmp::GetAngularVelocity_GMC() const
{
  return AngularVelocity;
}

void UGMC_ReplicationCmp::SetActorLocation_GMC(const FVector& Location, bool bTeleportPhysics)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  PawnOwner->SetActorLocation(Location, false, nullptr, bTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None);
}

FVector UGMC_ReplicationCmp::GetActorLocation_GMC() const
{
  if (!IsValid(PawnOwner))
  {
    return FVector::ZeroVector;
  }

  return PawnOwner->GetActorLocation();
}

void UGMC_ReplicationCmp::SetActorRotation_GMC(const FRotator& Rotation, bool bTeleportPhysics)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  PawnOwner->SetActorRotation(Rotation, bTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None);
}

FRotator UGMC_ReplicationCmp::GetActorRotation_GMC() const
{
  if (!IsValid(PawnOwner))
  {
    return FRotator::ZeroRotator;
  }

  return PawnOwner->GetActorRotation();
}

void UGMC_ReplicationCmp::SetActorQuat_GMC(const FQuat& Rotation, bool bTeleportPhysics)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  PawnOwner->SetActorRotation(Rotation, bTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None);
}

FQuat UGMC_ReplicationCmp::GetActorQuat_GMC() const
{
  if (!IsValid(PawnOwner))
  {
    return FQuat::Identity;
  }

  return PawnOwner->GetActorQuat();
}

void UGMC_ReplicationCmp::SetActorLocationAndRotation_GMC(const FVector& Location, const FRotator& Rotation, bool bTeleportPhysics)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  PawnOwner->SetActorLocationAndRotation(Location, Rotation, false, nullptr, bTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None);
}

void UGMC_ReplicationCmp::SetActorLocationAndQuat_GMC(const FVector& Location, const FQuat& Rotation, bool bTeleportPhysics)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  PawnOwner->SetActorLocationAndRotation(Location, Rotation, false, nullptr, bTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None);
}

void UGMC_ReplicationCmp::SetActorScale_GMC(const FVector& Scale)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  PawnOwner->SetActorScale3D(Scale);
}

FVector UGMC_ReplicationCmp::GetActorScale_GMC() const
{
  if (!IsValid(PawnOwner))
  {
    return FVector::OneVector;
  }

  return PawnOwner->GetActorScale3D();
}

void UGMC_ReplicationCmp::SetActorTransform_GMC(const FTransform& Transform, bool bTeleportPhysics)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  PawnOwner->SetActorTransform(Transform, false, nullptr, bTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None);
}

FTransform UGMC_ReplicationCmp::GetActorTransform_GMC() const
{
  if (!IsValid(PawnOwner))
  {
    return FTransform::Identity;
  }

  return PawnOwner->GetActorTransform();
}

void UGMC_ReplicationCmp::SetControllerRotation_GMC(const FRotator& Rotation)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  const auto& Controller = PawnOwner->Controller;

  if (IsValid(Controller))
  {
    Controller->SetControlRotation(Rotation.GetNormalized());
  }
  else
  {
    SimulatedControlRotation = Rotation.GetNormalized();
  }
}

FRotator UGMC_ReplicationCmp::GetControllerRotation_GMC() const
{
  if (!IsValid(PawnOwner))
  {
    return FRotator::ZeroRotator;
  }

  const auto& Controller = PawnOwner->Controller;

  if (IsValid(Controller))
  {
    return Controller->GetControlRotation().GetNormalized();
  }
  else
  {
    return SimulatedControlRotation.GetNormalized();
  }
}

FVector UGMC_ReplicationCmp::GetTangentialVelocity_GMC(const FVector& WorldLocation)
{
  const FVector DistanceToPawn = WorldLocation - GetActorLocation_GMC();
  return GetAngularVelocity_GMC() ^ DistanceToPawn;
}

void UGMC_ReplicationCmp::SetBasedLinearVelocity(const FVector& RelativeLinearVelocity)
{
  if (!IsValid(ActorBase))
  {
    SetLinearVelocity_GMC(RelativeLinearVelocity);
    return;
  }

  SetLinearVelocity_GMC(GetWorldLinearVelocityFor(RelativeLinearVelocity, ActorBase->GetComponentTransform()));
}

FVector UGMC_ReplicationCmp::GetWorldLinearVelocityFor(const FVector& InRelativeLinearVelocity, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = InBaseTransform.GetRotation();
  return RotationDelta.RotateVector(InRelativeLinearVelocity);
}

FVector UGMC_ReplicationCmp::GetBasedLinearVelocity() const
{
  if (!IsValid(ActorBase))
  {
    return GetLinearVelocity_GMC();
  }

  return GetBasedLinearVelocityFor(GetLinearVelocity_GMC(), ActorBase->GetComponentTransform());
}

FVector UGMC_ReplicationCmp::GetBasedLinearVelocityFor(const FVector& InLinearVelocity, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = InBaseTransform.GetRotation().Inverse();
  return RotationDelta.RotateVector(InLinearVelocity);
}

void UGMC_ReplicationCmp::SetBasedAngularVelocity(const FVector& RelativeAngularVelocity)
{
  if (!IsValid(ActorBase))
  {
    SetAngularVelocity_GMC(RelativeAngularVelocity);
    return;
  }

  SetAngularVelocity_GMC(GetWorldAngularVelocityFor(RelativeAngularVelocity, ActorBase->GetComponentTransform()));
}

FVector UGMC_ReplicationCmp::GetWorldAngularVelocityFor(const FVector& InRelativeAngularVelocity, const FTransform& InBaseTransform) const
{
  // Not affected by the base transform.
  return InRelativeAngularVelocity;
}

FVector UGMC_ReplicationCmp::GetBasedAngularVelocity() const
{
  if (!IsValid(ActorBase))
  {
    return GetAngularVelocity_GMC();
  }

  return GetBasedAngularVelocityFor(GetAngularVelocity_GMC(), ActorBase->GetComponentTransform());
}

FVector UGMC_ReplicationCmp::GetBasedAngularVelocityFor(const FVector& InAngularVelocity, const FTransform& InBaseTransform) const
{
  // Not affected by the base transform.
  return InAngularVelocity;
}

void UGMC_ReplicationCmp::SetBasedActorLocation(const FVector& RelativeActorLocation)
{
  if (!IsValid(ActorBase))
  {
    SetActorLocation_GMC(RelativeActorLocation, true);
    return;
  }

  SetActorLocation_GMC(GetWorldActorLocationFor(RelativeActorLocation, ActorBase->GetComponentTransform()), true);
}

FVector UGMC_ReplicationCmp::GetWorldActorLocationFor(const FVector& InRelativeActorLocation, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = InBaseTransform.GetRotation();
  return InBaseTransform.GetLocation() + RotationDelta.RotateVector(InRelativeActorLocation);
}

FVector UGMC_ReplicationCmp::GetBasedActorLocation() const
{
  if (!IsValid(ActorBase))
  {
    return GetActorLocation_GMC();
  }

  return GetBasedActorLocationFor(GetActorLocation_GMC(), ActorBase->GetComponentTransform());
}

FVector UGMC_ReplicationCmp::GetBasedActorLocationFor(const FVector& InActorLocation, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = InBaseTransform.GetRotation().Inverse();
  return RotationDelta.RotateVector(InActorLocation - InBaseTransform.GetLocation());
}

void UGMC_ReplicationCmp::SetBasedActorRotation(const FRotator& RelativeActorRotation)
{
  if (!IsValid(ActorBase))
  {
    SetActorRotation_GMC(RelativeActorRotation, true);
    return;
  }

  SetActorRotation_GMC(GetWorldActorRotationFor(RelativeActorRotation, ActorBase->GetComponentTransform()), true);
}

FRotator UGMC_ReplicationCmp::GetWorldActorRotationFor(const FRotator& InRelativeActorRotation, const FTransform& InBaseTransform) const
{
  return (InBaseTransform.GetRotation() * InRelativeActorRotation.Quaternion()).Rotator();
}

FRotator UGMC_ReplicationCmp::GetBasedActorRotation() const
{
  if (!IsValid(ActorBase))
  {
    return GetActorRotation_GMC();
  }

  return GetBasedActorRotationFor(GetActorRotation_GMC(), ActorBase->GetComponentTransform());
}

FRotator UGMC_ReplicationCmp::GetBasedActorRotationFor(const FRotator& InActorRotation, const FTransform& InBaseTransform) const
{
  return (InBaseTransform.GetRotation().Inverse() * InActorRotation.Quaternion()).Rotator().GetNormalized();
}

void UGMC_ReplicationCmp::SetBasedActorScale(const FVector& RelativeActorScale)
{
  if (!IsValid(ActorBase))
  {
    SetActorScale_GMC(RelativeActorScale);
    return;
  }

  SetActorScale_GMC(GetWorldActorScaleFor(RelativeActorScale, ActorBase->GetComponentTransform()));
}

FVector UGMC_ReplicationCmp::GetWorldActorScaleFor(const FVector& InRelativeActorScale, const FTransform& InBaseTransform) const
{
  // Not affected by the base transform.
  return InRelativeActorScale;
}

FVector UGMC_ReplicationCmp::GetBasedActorScale() const
{
  if (!IsValid(ActorBase))
  {
    return GetActorScale_GMC();
  }

  return GetBasedActorScaleFor(GetActorScale_GMC(), ActorBase->GetComponentTransform());
}

FVector UGMC_ReplicationCmp::GetBasedActorScaleFor(const FVector& InActorScale, const FTransform& InBaseTransform) const
{
  // Not affected by the base transform.
  return InActorScale;
}

void UGMC_ReplicationCmp::SetBasedControlRotation(const FRotator& RelativeControlRotation)
{
  if (!IsValid(ActorBase))
  {
    SetControllerRotation_GMC(RelativeControlRotation);
    return;
  }

  SetControllerRotation_GMC(GetWorldControlRotationFor(RelativeControlRotation, ActorBase->GetComponentTransform()));
}

FRotator UGMC_ReplicationCmp::GetWorldControlRotationFor(const FRotator& InRelativeControlRotation, const FTransform& InBaseTransform) const
{
  return (InBaseTransform.GetRotation() * InRelativeControlRotation.Quaternion()).Rotator();
}

FRotator UGMC_ReplicationCmp::GetBasedControlRotation() const
{
  if (!IsValid(ActorBase))
  {
    return GetControllerRotation_GMC();
  }

  return GetBasedControlRotationFor(GetControllerRotation_GMC(), ActorBase->GetComponentTransform());
}

FRotator UGMC_ReplicationCmp::GetBasedControlRotationFor(const FRotator& InControlRotation, const FTransform& InBaseTransform) const
{
  return (InBaseTransform.GetRotation().Inverse() * InControlRotation.Quaternion()).Rotator().GetNormalized();
}

void UGMC_ReplicationCmp::SetActorBase(USceneComponent* Base)
{
  if (IsValid(Base))
  {
    ActorBase = Base;
    return;
  }

  ActorBase = nullptr;
}

USceneComponent* UGMC_ReplicationCmp::GetActorBase() const
{
  if (IsValid(ActorBase))
  {
    return ActorBase;
  }

  return nullptr;
}

bool UGMC_ReplicationCmp::IsServerBot() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  if (PawnOwner->GetLocalRole() == ROLE_Authority && PawnOwner->IsLocallyControlled() && Cast<AAIController>(PawnOwner->GetController()))
  {
    return true;
  }

  return false;
}

bool UGMC_ReplicationCmp::IsUsingClientPrediction() const
{
  return bUseClientPrediction;
}

void UGMC_ReplicationCmp::EnableClientPrediction(bool bEnable)
{
  const auto& GMCPawnOwner = GetGMCPawnOwner();
  if (!GMCPawnOwner)
  {
    return;
  }

  if (bUseClientPrediction == bEnable)
  {
    return;
  }

  if (bEnable)
  {
    bUseClientPrediction = true;

    if (IsServerPawn())
    {
      GMCPawnOwner->SV_MarkForNetReserialization();
    }

    if (CL_NoPredictionSwapBuffer.bInitialized && !CL_NoPredictionSwapBuffer.HasEvenNumberOfSwaps())
    {
      gmc_ck(IsAutonomousProxy())

      CL_NoPredictionSwapBuffer.Swap(false, this, EGMC_NetContext::LocalClientPawn_PredictionEnabled);
      CL_OnSwapNoPredictionBuffer(CL_NoPredictionSwapBuffer.Buffer, EGMC_NetContext::LocalClientPawn_PredictionEnabled);
    }

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnClientPredictionEnabled);

    ClearTransientData(true);
  }
  else
  {
    bUseClientPrediction = false;

    if (CL_NoPredictionSwapBuffer.bInitialized && !CL_NoPredictionSwapBuffer.HasEvenNumberOfSwaps())
    {
      gmc_ck(IsAutonomousProxy())

      CL_NoPredictionSwapBuffer.Swap(false, this, EGMC_NetContext::LocalClientPawn_PredictionDisabled);
      CL_OnSwapNoPredictionBuffer(CL_NoPredictionSwapBuffer.Buffer, EGMC_NetContext::LocalClientPawn_PredictionDisabled);
    }

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnClientPredictionDisabled);

    ClearTransientData(true);
  }
}

bool UGMC_ReplicationCmp::IsUsingUnreliableClientMoves() const
{
  return ComponentStatus.bUseUnreliableClientMoves;
}

bool UGMC_ReplicationCmp::IsGMCEnabled() const
{
  return ComponentStatus.bGMCEnabled;
}

bool UGMC_ReplicationCmp::IsAutonomousProxy() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  return PawnOwner->GetLocalRole() == ROLE_AutonomousProxy;
}

bool UGMC_ReplicationCmp::IsSimulatedProxy() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  return PawnOwner->GetLocalRole() == ROLE_SimulatedProxy;
}

bool UGMC_ReplicationCmp::IsPlayerControlledSimulatedProxy() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  return PawnOwner->GetLocalRole() == ROLE_SimulatedProxy && IsPlayerControlledPawn();
}

bool UGMC_ReplicationCmp::IsNonPlayerControlledSimulatedProxy() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  return PawnOwner->GetLocalRole() == ROLE_SimulatedProxy && !IsPlayerControlledPawn();
}

bool UGMC_ReplicationCmp::IsLocallyControlledServerPawn() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  return PawnOwner->GetLocalRole() == ROLE_Authority && PawnOwner->IsLocallyControlled();
}

bool UGMC_ReplicationCmp::IsRemotelyControlledServerPawn() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  return PawnOwner->GetLocalRole() == ROLE_Authority && PawnOwner->GetController() && !PawnOwner->IsLocallyControlled();
}

bool UGMC_ReplicationCmp::IsRemotelyControlledPredictedServerPawn() const
{
  return IsRemotelyControlledServerPawn() && bUseClientPrediction;
}

bool UGMC_ReplicationCmp::IsRemotelyControlledNonPredictedServerPawn() const
{
  return IsRemotelyControlledServerPawn() && !bUseClientPrediction;
}

bool UGMC_ReplicationCmp::IsLocallyControlledDedicatedServerPawn() const
{
  return IsNetMode(NM_DedicatedServer) && IsLocallyControlledServerPawn();
}

bool UGMC_ReplicationCmp::IsRemotelyControlledDedicatedServerPawn() const
{
  return IsNetMode(NM_DedicatedServer) && IsRemotelyControlledServerPawn();
}

bool UGMC_ReplicationCmp::IsLocallyControlledListenServerPawn() const
{
  return IsNetMode(NM_ListenServer) && IsLocallyControlledServerPawn();
}

bool UGMC_ReplicationCmp::IsRemotelyControlledListenServerPawn() const
{
  return IsNetMode(NM_ListenServer) && IsRemotelyControlledServerPawn();
}

bool UGMC_ReplicationCmp::IsSmoothedListenServerPawn() const
{
  return bSmoothRemoteListenServerPawn && IsRemotelyControlledListenServerPawn();
}

bool UGMC_ReplicationCmp::IsPlayerControlledPawn() const
{
  return ComponentStatus.bIsPlayerOwned;
}

bool UGMC_ReplicationCmp::IsClientPawn() const
{
  if (!IsValid(PawnOwner)) return false;
  return PawnOwner->GetLocalRole() == ROLE_AutonomousProxy || PawnOwner->GetLocalRole() == ROLE_SimulatedProxy;
}

bool UGMC_ReplicationCmp::IsPredictedAutonomousProxy() const
{
  return IsAutonomousProxy() && bUseClientPrediction;
}

bool UGMC_ReplicationCmp::IsNonPredictedAutonomousProxy() const
{
  return IsAutonomousProxy() && !bUseClientPrediction;
}

bool UGMC_ReplicationCmp::IsServerPawn() const
{
  if (!IsValid(PawnOwner)) return false;
  return PawnOwner->GetLocalRole() == ROLE_Authority;
}

bool UGMC_ReplicationCmp::IsSimulatedPawn() const
{
  return IsSimulatedProxy() || IsSmoothedListenServerPawn() || IsNonPredictedAutonomousProxy();
}

bool UGMC_ReplicationCmp::IsNetworkedServer() const
{
  return IsNetMode(NM_DedicatedServer) || IsNetMode(NM_ListenServer);
}

bool UGMC_ReplicationCmp::IsExecutingMove() const
{
  return ComponentStatus.bIsExecutingNonSimulatedMove || ComponentStatus.bIsExecutingSimulatedMove;
}

bool UGMC_ReplicationCmp::IsExecutingLocalMove() const
{
  return ComponentStatus.bIsExecutingLocalMove;
}

bool UGMC_ReplicationCmp::IsExecutingNonSimulatedMove() const
{
  return ComponentStatus.bIsExecutingNonSimulatedMove;
}

bool UGMC_ReplicationCmp::IsExecutingSimulatedMove() const
{
  return ComponentStatus.bIsExecutingSimulatedMove;
}

double UGMC_ReplicationCmp::GetTime() const
{
  SCOPE_CYCLE_COUNTER(STAT_GetTime)

  if (IsClientPawn())
  {
    if (UGameInstance* GameInstance = PawnOwner->GetGameInstance())
    {
      const auto& LocalController = Cast<AGMC_PlayerController>(GameInstance->GetFirstLocalPlayerController());
      if (LocalController)
      {
        return LocalController->CL_GetSyncedWorldTimeSeconds();
      }
    }
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Client was unable to retrieve the server world time."))
    return 0.;
  }

  if (const auto& World = GetWorld())
  {
    return World->GetRealTimeSeconds();
  }
  GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Server world time could not be retrieved."))
  return 0.;
}

double UGMC_ReplicationCmp::CL_GetTimeDiscrepancy() const
{
  if (IsClientPawn())
  {
    if (UGameInstance* GameInstance = PawnOwner->GetGameInstance())
    {
      const auto& LocalController = Cast<AGMC_PlayerController>(GameInstance->GetFirstLocalPlayerController());
      if (LocalController)
      {
        return LocalController->CL_GetAverageTimeDiscrepancy();
      }
    }
    GMC_LOG(LogGMCReplication, PawnOwner, Warning, TEXT("Client was unable to retrieve the average time discrepancy."))
    return 0.;
  }
  return 0.;
}

FGMC_NumericValue UGMC_ReplicationCmp::InterpolateValue(
  uint8 Function,
  const FGMC_NumericValue& StartValue,
  const FGMC_NumericValue& TargetValue,
  const FGMC_NumericValue& PreviousValue,
  const FGMC_NumericValue& NextValue,
  double Ratio
)
{
  gmc_ck(StartValue.GetType() == TargetValue.GetType())
  gmc_ck(StartValue.GetType() == PreviousValue.GetType())
  gmc_ck(StartValue.GetType() == NextValue.GetType())

  if (StartValue.GetType() == EGMC_NumericType::None)
  {
    gmc_ck(StartValue.GetBare())
    gmc_ck(TargetValue.GetBare())
    gmc_ck(PreviousValue.GetBare())
    gmc_ck(NextValue.GetBare())
    switch (static_cast<EGMC_InterpolationFunction>(Function))
    {
      case EGMC_InterpolationFunction::NearestNeighbour:
        return InterpolateNearestNeighbour(StartValue, TargetValue, Ratio);
      case EGMC_InterpolationFunction::StartValue:
        return InterpolateStartValue(StartValue, TargetValue, Ratio);
      case EGMC_InterpolationFunction::TargetValue:
        return InterpolateTargetValue(StartValue, TargetValue, Ratio);

      // Any other function should simply degrade to nearest neighbour interpolation.
      default:
        return InterpolateNearestNeighbour(StartValue, TargetValue, Ratio);
    }
  }

  switch (static_cast<EGMC_InterpolationFunction>(Function))
  {
    case EGMC_InterpolationFunction::NearestNeighbour:
      return InterpolateNearestNeighbour(StartValue, TargetValue, Ratio);
    case EGMC_InterpolationFunction::StartValue:
      return InterpolateStartValue(StartValue, TargetValue, Ratio);
    case EGMC_InterpolationFunction::TargetValue:
      return InterpolateTargetValue(StartValue, TargetValue, Ratio);
    case EGMC_InterpolationFunction::Linear:
      return InterpolateLinear(StartValue, TargetValue, Ratio);
    case EGMC_InterpolationFunction::Cubic:
      return InterpolateCubic(StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom1:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom1, StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom2:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom2, StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom3:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom3, StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom4:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom4, StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom5:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom5, StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom6:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom6, StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom7:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom7, StartValue, TargetValue, PreviousValue, NextValue, Ratio);
    case EGMC_InterpolationFunction::Custom8:
      return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, InterpolateCustom8, StartValue, TargetValue, PreviousValue, NextValue, Ratio);

    default:
      gmc_ckne();
  }
  return FGMC_NumericValue{EGMC_NumericType::Float, 0};
}

FGMC_NumericValue UGMC_ReplicationCmp::InterpolateNearestNeighbour(
  const FGMC_NumericValue& StartValue,
  const FGMC_NumericValue& TargetValue,
  double Ratio
)
{
  return Ratio < 0.5 ? StartValue : TargetValue;
}

FGMC_NumericValue UGMC_ReplicationCmp::InterpolateStartValue(
  const FGMC_NumericValue& StartValue,
  const FGMC_NumericValue& TargetValue,
  double Ratio
)
{
  return StartValue;
}

FGMC_NumericValue UGMC_ReplicationCmp::InterpolateTargetValue(
  const FGMC_NumericValue& StartValue,
  const FGMC_NumericValue& TargetValue,
  double Ratio
)
{
  return TargetValue;
}

FGMC_NumericValue UGMC_ReplicationCmp::InterpolateLinear(
  const FGMC_NumericValue& StartValue,
  const FGMC_NumericValue& TargetValue,
  double Ratio
)
{
  gmc_ck(StartValue.GetType() == TargetValue.GetType())

  switch (StartValue.GetType())
  {
    case EGMC_NumericType::None:
    {
      gmc_ckne()
      return FGMC_NumericValue{EGMC_NumericType::Float, 0};
    }
    case EGMC_NumericType::Int:
    {
      return FGMC_NumericValue{EGMC_NumericType::Int, FMath::LerpStable(StartValue.GetInt(), TargetValue.GetInt(), Ratio)};
    }
    case EGMC_NumericType::Float:
    {
      return FGMC_NumericValue{EGMC_NumericType::Float, FMath::LerpStable(StartValue.GetFloat(), TargetValue.GetFloat(), Ratio)};
    }
    case EGMC_NumericType::Vector2D:
    {
      return FGMC_NumericValue{EGMC_NumericType::Vector2D, FMath::LerpStable(StartValue.GetVector2D(), TargetValue.GetVector2D(), Ratio)};
    }
    case EGMC_NumericType::Vector:
    {
      return FGMC_NumericValue{EGMC_NumericType::Vector, FMath::LerpStable(StartValue.GetVector(), TargetValue.GetVector(), Ratio)};
    }
    case EGMC_NumericType::Rotator:
    {
      return FGMC_NumericValue{EGMC_NumericType::Rotator, FQuat::Slerp(StartValue.GetRotator().Quaternion(), TargetValue.GetRotator().Quaternion(), Ratio).Rotator()};
    }
    case EGMC_NumericType::Quat:
    {
      return FGMC_NumericValue{EGMC_NumericType::Quat, FQuat::Slerp(StartValue.GetQuat(), TargetValue.GetQuat(), Ratio)};
    }
    default:
    {
      gmc_ckne();
    }
  }
  return FGMC_NumericValue{EGMC_NumericType::Float, 0};
}

FGMC_NumericValue UGMC_ReplicationCmp::InterpolateCubic(
  const FGMC_NumericValue& StartValue,
  const FGMC_NumericValue& TargetValue,
  const FGMC_NumericValue& PreviousValue,
  const FGMC_NumericValue& NextValue,
  double Ratio
)
{
  const auto CalcTangent = [](auto PrevP, auto NextP) { return (NextP - PrevP) / 2.0; };

  switch (StartValue.GetType())
  {
    case EGMC_NumericType::None:
    {
      gmc_ckne()
      return FGMC_NumericValue{EGMC_NumericType::Float, 0};
    }
    case EGMC_NumericType::Int:
    {
      const double P0 = PreviousValue.GetInt();
      const double P1 = StartValue.GetInt();
      const double P2 = TargetValue.GetInt();
      const double P3 = NextValue.GetInt();
      const auto T1 = CalcTangent(P0, P2);
      const auto T2 = CalcTangent(P1, P3);
      return FGMC_NumericValue{EGMC_NumericType::Int, FMath::CubicInterp(P1, T1, P2, T2, Ratio)};
    }
    case EGMC_NumericType::Float:
    {
      const double P0 = PreviousValue.GetFloat();
      const double P1 = StartValue.GetFloat();
      const double P2 = TargetValue.GetFloat();
      const double P3 = NextValue.GetFloat();
      const auto T1 = CalcTangent(P0, P2);
      const auto T2 = CalcTangent(P1, P3);
      return FGMC_NumericValue{EGMC_NumericType::Float, FMath::CubicInterp(P1, T1, P2, T2, Ratio)};
    }
    case EGMC_NumericType::Vector2D:
    {
      const FVector2D P0 = PreviousValue.GetVector2D();
      const FVector2D P1 = StartValue.GetVector2D();
      const FVector2D P2 = TargetValue.GetVector2D();
      const FVector2D P3 = NextValue.GetVector2D();
      const auto T1 = CalcTangent(P0, P2);
      const auto T2 = CalcTangent(P1, P3);
      return FGMC_NumericValue{EGMC_NumericType::Vector2D, FMath::CubicInterp(P1, T1, P2, T2, Ratio)};
    }
    case EGMC_NumericType::Vector:
    {
      const FVector P0 = PreviousValue.GetVector();
      const FVector P1 = StartValue.GetVector();
      const FVector P2 = TargetValue.GetVector();
      const FVector P3 = NextValue.GetVector();
      const auto T1 = CalcTangent(P0, P2);
      const auto T2 = CalcTangent(P1, P3);
      return FGMC_NumericValue{EGMC_NumericType::Vector, FMath::CubicInterp(P1, T1, P2, T2, Ratio)};
    }
    case EGMC_NumericType::Rotator:
    {
      const FQuat P0 = PreviousValue.GetRotator().Quaternion();
      const FQuat P1 = StartValue.GetRotator().Quaternion();
      const FQuat P2 = TargetValue.GetRotator().Quaternion();
      const FQuat P3 = NextValue.GetRotator().Quaternion();
      FQuat T1{};
      FQuat T2{};
      FQuat::CalcTangents(P0, P1, P2, 0., T1);
      FQuat::CalcTangents(P1, P2, P3, 0., T2);
      return FGMC_NumericValue{EGMC_NumericType::Rotator, FQuat::Squad(P1, T1, P2, T2, Ratio).Rotator()};
    }
    case EGMC_NumericType::Quat:
    {
      const FQuat P0 = PreviousValue.GetQuat();
      const FQuat P1 = StartValue.GetQuat();
      const FQuat P2 = TargetValue.GetQuat();
      const FQuat P3 = NextValue.GetQuat();
      FQuat T1{};
      FQuat T2{};
      FQuat::CalcTangents(P0, P1, P2, 0., T1);
      FQuat::CalcTangents(P1, P2, P3, 0., T2);
      return FGMC_NumericValue{EGMC_NumericType::Quat, FQuat::Squad(P1, T1, P2, T2, Ratio)};
    }
    default:
    {
      gmc_ckne();
    }
  }
  return FGMC_NumericValue{EGMC_NumericType::Float, 0};
}

bool FGMC_Move::HasValidTimestamp() const
{
  return MetaData.Timestamp >= 0.;
}

void FGMC_Move::CopyDataValuesTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const
{
  gmc_ck(NetInfo.OwningComponent == Other.NetInfo.OwningComponent)
  gmc_ck(NetInfo.OwningComponent == Component)
  CopySyncDataValues(InputState, Other.InputState, DataFilter::None, DataFilterMode::Inclusive, Component);
  CopySyncDataValues(OutputState, Other.OutputState, DataFilter::None, DataFilterMode::Inclusive, Component);
}

void FGMC_Move::CopyLastSerializedValuesTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const
{
  gmc_ck(NetInfo.OwningComponent == Other.NetInfo.OwningComponent)
  gmc_ck(NetInfo.OwningComponent == Component)
  CopySyncDataLastSerializedValues(InputState, Other.InputState, DataFilter::None, DataFilterMode::Inclusive, Component);
  CopySyncDataLastSerializedValues(OutputState, Other.OutputState, DataFilter::None, DataFilterMode::Inclusive, Component);
}

void FGMC_Move::CopySuspendedFlagsTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const
{
  gmc_ck(NetInfo.OwningComponent == Other.NetInfo.OwningComponent)
  gmc_ck(NetInfo.OwningComponent == Component)
  CopySyncDataSuspendedFlags(InputState, Other.InputState, DataFilter::None, DataFilterMode::Inclusive, Component);
  CopySyncDataSuspendedFlags(OutputState, Other.OutputState, DataFilter::None, DataFilterMode::Inclusive, Component);
}

void FGMC_Move::CopyMetaDataTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const
{
  gmc_ck(NetInfo.OwningComponent == Other.NetInfo.OwningComponent)
  gmc_ck(NetInfo.OwningComponent == Component)
  Other.MetaData = MetaData;
}

bool FGMC_Move::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
  gmc_ck(Ar.IsSaving() ^ Ar.IsLoading())
  const bool bArIsSaving = Ar.IsSaving();
  const bool bArIsLoading = !bArIsSaving;

  bOutSuccess = true;

  // The target connection can be null in some cases.
  const AActor* const TargetConnection = Cast<UPackageMapClient>(Map)->GetConnection()->OwningActor;

  bool bHasData = true;

  if (bArIsSaving && IsValid(TargetConnection))
  {
    // Should never be nullptr when saving for valid target connection.
    gmc_ck(NetInfo.OwningComponent.Get())

    // Don't check the pawn role here, it can be inaccurate during possession changes.
    if (NetInfo.OwningComponent->IsNetMode(NM_ListenServer) || NetInfo.OwningComponent->IsNetMode(NM_DedicatedServer))
    {
      bHasData = SV_ShouldReplicate();

      if (NetInfo.NetType == GMCReplication::ESimType::APMove || NetInfo.NetType == GMCReplication::ESimType::SPMove)
      {
        // Net updates should always run for both AP and SP moves in the same frame. Do not base this on the value of "bHasData", we cannot manually distinguish
        // between AP and SP updates for property replication.
        NetInfo.OwningComponent->SV_RemoteMoveExecutionAux.LastReplicatedLocation = NetInfo.OwningComponent->GetActorLocation_GMC();
      }
      else
      {
        gmc_ckne()
      }
    }
  }

  Ar.SerializeBits(&bHasData, 1);
  if (!bHasData)
  {
    return true;
  }

  Ar << NetInfo.OwningComponent;
  gmc_ck(NetInfo.OwningComponent.Get())

  if (!NetInfo.OwningComponent->LocalMove().NetInfo.OwningComponent.IsValid())
  {
    gmc_ckne()
    return true;
  }

  const bool bServerReadingClientMove = bArIsLoading && NetInfo.NetType == GMCReplication::ESimType::LocalMove;
  if (bServerReadingClientMove)
  {
    gmc_ck(NetInfo.OwningComponent == NetInfo.OwningComponent->LocalMove().NetInfo.OwningComponent)
    *this = NetInfo.OwningComponent->LocalMove();
  }

  const bool bClientWritingServerMove = bArIsSaving && NetInfo.NetType == GMCReplication::ESimType::LocalMove;
  if (bClientWritingServerMove)
  {
    gmc_ck(NetInfo.OwningComponent == NetInfo.OwningComponent->LocalMove().NetInfo.OwningComponent)
    NetInfo.OwningComponent->LocalMove().CopyLastSerializedValuesTo(*this, NetInfo.OwningComponent.Get());
  }

  if (bArIsSaving && NetInfo.NetType == GMCReplication::ESimType::SPMove)
  {
    gmc_ck(IsValid(TargetConnection))
    gmc_ck(Cast<APlayerController>(TargetConnection))

    // Add the receiver to the latest move in the history (which is the one that is currently being serialized).
    auto& MoveHistory = NetInfo.OwningComponent->MoveHistory;
    const int32 LastIdx = MoveHistory.Num() - 1;
    gmc_ck(LastIdx >= 0)

    // It is possible that the receiver is already in the map (in case of a replication retry).
    MoveHistory[LastIdx].NetInfo.ClientReceivers.FindOrAdd(TargetConnection);
    gmc_ck(MoveHistory[LastIdx].MetaData.Timestamp == MetaData.Timestamp)
  }

  Ar << MetaData.Timestamp;

  switch (NetInfo.NetType)
  {
    case GMCReplication::ESimType::LocalMove:
    {
      NetSerializeClientMove(Ar, Map, TargetConnection);
      break;
    }
    case GMCReplication::ESimType::APMove:
    {
      NetSerializeAutonomousProxyState(Ar, Map, TargetConnection);
      break;
    }
    case GMCReplication::ESimType::SPMove:
    {
      NetSerializeSimulatedProxyState(Ar, Map, TargetConnection);
      break;
    }

    default:
    gmc_ckne();
  }

  if (bServerReadingClientMove)
  {
    gmc_ck(NetInfo.OwningComponent == NetInfo.OwningComponent->LocalMove().NetInfo.OwningComponent)
    NetInfo.OwningComponent->LocalMove() = *this;

    NetInfo.OwningComponent->SV_RemoteMoveExecutionAux.DeserializedMoves.Add(*this);

    gmc_ckc(
      const auto& DeserializedMoves = NetInfo.OwningComponent->SV_RemoteMoveExecutionAux.DeserializedMoves;
      const int32 DeserializedMovesNum = DeserializedMoves.Num();
      if (DeserializedMovesNum >= 2)
      {
        gmc_ck(DeserializedMoves[DeserializedMovesNum - 1].MetaData.Timestamp > DeserializedMoves[DeserializedMovesNum - 2].MetaData.Timestamp)
      }
    )
  }

  if (bClientWritingServerMove)
  {
    gmc_ck(NetInfo.OwningComponent == NetInfo.OwningComponent->LocalMove().NetInfo.OwningComponent)
    CopyLastSerializedValuesTo(NetInfo.OwningComponent->LocalMove(), NetInfo.OwningComponent.Get());
  }

  return true;
}

bool FGMC_Move::SV_ShouldReplicate()
{
  gmc_ck(NetInfo.OwningComponent.Get())

  const bool bAPMove = NetInfo.NetType == GMCReplication::ESimType::APMove;
  const bool bSPMove = NetInfo.NetType == GMCReplication::ESimType::SPMove;
  gmc_ck(bAPMove ^ bSPMove)

  // Only replicate if the pawn is ready for play and at least one move was saved.
  if (!NetInfo.OwningComponent->IsReadyForPlay() || NetInfo.OwningComponent->MoveHistory.Num() == 0)
  {
    return false;
  }

  if (bAPMove && NetInfo.OwningComponent->bUseClientPrediction)
  {
    if (NetInfo.OwningComponent->SV_PredictedClientNetSerializationAux.ShouldUpdate(MetaData.bValidClientMove))
    {
      NetInfo.OwningComponent->SV_PredictedClientNetSerializationAux.ResetTimer();
      return true;
    }

    return false;
  }

  return true;
}

bool FGMC_Move::SV_IsNetRelevantFor(const AActor* const TargetConnection)
{
  gmc_ck(IsValid(TargetConnection))

  const auto& Owner = NetInfo.OwningComponent->GetOwner();
  gmc_ck(Owner)

  const auto& TargetPlayerController = Cast<APlayerController>(TargetConnection);
  gmc_ck(TargetPlayerController)

  const auto& TargetPawn = TargetPlayerController->GetPawn();
  if (!TargetPawn)
  {
    // Can happen during a possession change.
    return true;
  }

  FVector ViewLocation{0.};
  TargetPlayerController->GetPlayerViewPoint(ViewLocation, UNUSED(FRotator));

  return Owner->IsNetRelevantFor(TargetConnection, TargetPawn, ViewLocation);
}

bool FGMC_Move::SV_BecameNetRelevant(bool bIsNetRelevant, const TGMC_CircularArray<FGMC_Move>& MoveHistory, const AActor* const TargetConnection)
{
  gmc_ck(IsValid(TargetConnection))

  const int32 MoveHistoryNum = MoveHistory.Num();
  if (MoveHistoryNum < 2)
  {
    // When in doubt return true to guarantee a full reserialization.
    return true;
  }

  // Find the latest move previously serialized to the target connection and check if the net relevancy has changed since then. The move that is currently being
  // serialized is the latest one in the move history so we must skip it.
  const int32 PreviousMoveIndex = MoveHistoryNum - 2;
  for (int32 Index = PreviousMoveIndex; Index >= 0; --Index)
  {
    const auto& Move = MoveHistory[Index];
    if (Move.NetInfo.ClientReceivers.Contains(TargetConnection))
    {
      const bool bWasNetRelevantPreviously = Move.NetInfo.ClientReceivers[TargetConnection].bWasNetRelevant;
      return !bWasNetRelevantPreviously && bIsNetRelevant;
    }
  }

  return true;
}

void FGMC_Move::NetSerializeClientMove(FArchive& Ar, UPackageMap* Map, const AActor* const TargetConnection)
{
  gmc_ck(NetInfo.NetType == GMCReplication::ESimType::LocalMove)
  gmc_ck(NetInfo.OwningComponent.IsValid())
  gmc_ck(MetaData.Timestamp > 0.)
  gmc_ck(MetaData.ServerAuthPhysicsTimestamp == -1.)
  gmc_ck(!MetaData.bIsUsingServerAuthPhysics)
  gmc_ck(!MetaData.bIsUsingClientAuthPhysics)
  gmc_ck(!MetaData.bPredictedClientMove)
  gmc_ck(!MetaData.bValidClientMove)
  gmc_ck(!MetaData.bPlayerOwned)
  gmc_ckc(
    if (Ar.IsSaving())
    {
      gmc_ck(IsValid(TargetConnection))
      gmc_ck(MetaData.DeltaTime > 0.f)
    }
    else
    {
      gmc_ck(Ar.IsLoading())
    }
  )

  if (Ar.IsLoading())
  {
    auto& LastReceivedClientTimestamp = NetInfo.OwningComponent->SV_RemoteMoveExecutionAux.LastReceivedClientTimestamp;
    MetaData.DeltaTime = MetaData.Timestamp - LastReceivedClientTimestamp;
    LastReceivedClientTimestamp = MetaData.Timestamp;

    GMC_CLOG(
      MetaData.DeltaTime < UGMC_ReplicationCmp::MIN_DELTA_TIME || MetaData.DeltaTime > NetInfo.OwningComponent->MaxMoveDeltaTime,
      LogGMCReplication,
      NetInfo.OwningComponent->PawnOwner,
      VeryVerbose,
      TEXT("Calculated server delta time of %f s clamped during deserialization (min: %f s, max: %f s)."),
      MetaData.DeltaTime,
      UGMC_ReplicationCmp::MIN_DELTA_TIME,
      NetInfo.OwningComponent->MaxMoveDeltaTime
    )
    MetaData.DeltaTime = FMath::Clamp(MetaData.DeltaTime, UGMC_ReplicationCmp::MIN_DELTA_TIME, NetInfo.OwningComponent->MaxMoveDeltaTime);
  }

  // Reserialization is only needed for autonomous proxy moves if the data is sent to the server via unreliable RPCs.
  bool bForceFullSerialization = NetInfo.OwningComponent->ComponentStatus.bUseUnreliableClientMoves;

  bool& JustDeactivatedUnreliableClientMoves = NetInfo.OwningComponent->ComponentStatus.CL_bJustDeactivatedUnreliableClientMoves;
  if (JustDeactivatedUnreliableClientMoves)
  {
    // When we just switched to reliable client moves again, we need to force full reserialization for the first reliable RPCs because there's no guarantee the
    // last unreliable RPC was actually received by the server.
    bForceFullSerialization = true;
    JustDeactivatedUnreliableClientMoves = false;
  }

  NetSerializeSyncData(
    InputState,
    Ar,
    Map,
    TargetConnection,
    NetInfo,
    MetaData,
    bForceFullSerialization
  );

  NetSerializeSyncData(
    OutputState,
    Ar,
    Map,
    TargetConnection,
    NetInfo,
    MetaData,
    bForceFullSerialization
  );
}

void FGMC_Move::NetSerializeAutonomousProxyState(FArchive& Ar, UPackageMap* Map, const AActor* const TargetConnection)
{
  gmc_ck(NetInfo.NetType == GMCReplication::ESimType::APMove)
  gmc_ck(NetInfo.OwningComponent.IsValid())
  gmc_ck(MetaData.Timestamp > 0.)
  gmc_ckc(
    if (Ar.IsSaving())
    {
      gmc_ck(IsValid(TargetConnection))
      gmc_ck(MetaData.DeltaTime > 0.f)
      gmc_ck(MetaData.bPlayerOwned)
      if (MetaData.bIsUsingServerAuthPhysics)
      {
        gmc_ck(MetaData.ServerAuthPhysicsTimestamp > 0.)
        gmc_ck(!MetaData.bIsUsingClientAuthPhysics)
        gmc_ck(!MetaData.bPredictedClientMove)
        gmc_ck(MetaData.bValidClientMove)
      }
      else if (MetaData.bIsUsingClientAuthPhysics)
      {
        gmc_ck(MetaData.ServerAuthPhysicsTimestamp == -1.)
        gmc_ck(!MetaData.bIsUsingServerAuthPhysics)
        gmc_ck(MetaData.bPredictedClientMove)
      }
      else
      {
        gmc_ck(MetaData.ServerAuthPhysicsTimestamp == -1.)
      }
    }
    else
    {
      gmc_ck(Ar.IsLoading())
      gmc_ck(MetaData.DeltaTime == -1.f)
    }
  )

  Ar.SerializeBits(&MetaData.bIsUsingServerAuthPhysics, 1);
  if (MetaData.bIsUsingServerAuthPhysics)
  {
    Ar << MetaData.ServerAuthPhysicsTimestamp;
    gmc_ck(MetaData.bIsUsingServerAuthPhysics)
    MetaData.bIsUsingClientAuthPhysics = false;
    MetaData.bPredictedClientMove = false;
    MetaData.bValidClientMove = true;
  }
  else
  {
    Ar.SerializeBits(&MetaData.bIsUsingClientAuthPhysics, 1);
    if (MetaData.bIsUsingClientAuthPhysics)
    {
      MetaData.ServerAuthPhysicsTimestamp = -1.;
      gmc_ck(!MetaData.bIsUsingServerAuthPhysics)
      gmc_ck(MetaData.bIsUsingClientAuthPhysics)
      MetaData.bPredictedClientMove = true;
      Ar.SerializeBits(&MetaData.bValidClientMove, 1);
    }
    else
    {
      MetaData.ServerAuthPhysicsTimestamp = -1.;
      gmc_ck(!MetaData.bIsUsingServerAuthPhysics)
      gmc_ck(!MetaData.bIsUsingClientAuthPhysics)
      Ar.SerializeBits(&MetaData.bPredictedClientMove, 1);
      Ar.SerializeBits(&MetaData.bValidClientMove, 1);
    }
  }

  // The autonomous proxy is always player-owned.
  MetaData.bPlayerOwned = true;

  // Always serialize fully for autonomous proxies to avoid erroneous replays due to lost packets.
  constexpr bool bForceFullSerialization = true;

  NetSerializeSyncData(
    InputState,
    Ar,
    Map,
    TargetConnection,
    NetInfo,
    MetaData,
    bForceFullSerialization
  );

  NetSerializeSyncData(
    OutputState,
    Ar,
    Map,
    TargetConnection,
    NetInfo,
    MetaData,
    bForceFullSerialization
  );
}

void FGMC_Move::NetSerializeSimulatedProxyState(FArchive& Ar, UPackageMap* Map, const AActor* const TargetConnection)
{
  gmc_ck(NetInfo.NetType == GMCReplication::ESimType::SPMove)
  gmc_ck(NetInfo.OwningComponent.IsValid())
  gmc_ck(MetaData.Timestamp > 0.)
  gmc_ck(!MetaData.bPredictedClientMove)
  gmc_ck(!MetaData.bValidClientMove)
  gmc_ckc(
    if (Ar.IsSaving())
    {
      gmc_ck(IsValid(TargetConnection))
      gmc_ck(MetaData.DeltaTime > 0.f)
      if (MetaData.bIsUsingServerAuthPhysics)
      {
        gmc_ck(MetaData.ServerAuthPhysicsTimestamp > 0.)
        gmc_ck(!MetaData.bIsUsingClientAuthPhysics)
      }
      else if (MetaData.bIsUsingClientAuthPhysics)
      {
        gmc_ck(MetaData.ServerAuthPhysicsTimestamp == -1.)
        gmc_ck(!MetaData.bIsUsingServerAuthPhysics)
      }
      else
      {
        gmc_ck(MetaData.ServerAuthPhysicsTimestamp == -1.)
      }
    }
    else
    {
      gmc_ck(Ar.IsLoading())
      gmc_ck(MetaData.DeltaTime == -1.f)
    }
  )

  Ar.SerializeBits(&MetaData.bIsUsingServerAuthPhysics, 1);
  if (MetaData.bIsUsingServerAuthPhysics)
  {
    Ar << MetaData.ServerAuthPhysicsTimestamp;
    gmc_ck(MetaData.bIsUsingServerAuthPhysics)
    MetaData.bIsUsingClientAuthPhysics = false;
  }
  else
  {
    Ar.SerializeBits(&MetaData.bIsUsingClientAuthPhysics, 1);
    if (MetaData.bIsUsingClientAuthPhysics)
    {
      MetaData.ServerAuthPhysicsTimestamp = -1.;
      gmc_ck(!MetaData.bIsUsingServerAuthPhysics)
      gmc_ck(MetaData.bIsUsingClientAuthPhysics)
    }
    else
    {
      MetaData.ServerAuthPhysicsTimestamp = -1.;
      gmc_ck(!MetaData.bIsUsingServerAuthPhysics)
      gmc_ck(!MetaData.bIsUsingClientAuthPhysics)
    }
  }

  Ar.SerializeBits(&MetaData.bPlayerOwned, 1);

  bool bForceFullSerialization =
    !NetInfo.OwningComponent->bUseConditionalNetSerialization ||
    NetInfo.OwningComponent->SV_NetReserializationAux.bOwnerNeedsReserialization;

  if (Ar.IsSaving())
  {
    const bool bIsNetRelevant = SV_IsNetRelevantFor(TargetConnection);

    // Set net relevancy in the move history for the last saved move (which is the one that's being serialized right now).
    auto& MoveHistory = NetInfo.OwningComponent->MoveHistory;
    gmc_ck(MoveHistory.Num() > 0)
    gmc_ck(MoveHistory.Last().NetInfo.ClientReceivers.Contains(TargetConnection))
    MoveHistory.Last().NetInfo.ClientReceivers[TargetConnection].bWasNetRelevant = bIsNetRelevant;

    if (!bForceFullSerialization)
    {
      bForceFullSerialization |= SV_BecameNetRelevant(bIsNetRelevant, MoveHistory, TargetConnection);
    }
  }

  NetSerializeSyncData(
    InputState,
    Ar,
    Map,
    TargetConnection,
    NetInfo,
    MetaData,
    bForceFullSerialization
  );

  NetSerializeSyncData(
    OutputState,
    Ar,
    Map,
    TargetConnection,
    NetInfo,
    MetaData,
    bForceFullSerialization
  );
}

void FGMC_Move::SerializeLinearVelocity(FVector& LinearVelocity, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData)
{
  EGMC_FloatPrecision LinearVelocityCompression = ToNativeEnum(NetInfo.OwningComponent->ReplicationSettings.DefaultCompressionSettings.LinearVelocity);
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeVector(LinearVelocity, LinearVelocityCompression, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeVector(LinearVelocity, LinearVelocityCompression, Ar);
  }
}

void FGMC_Move::SerializeAngularVelocity(FVector& AngularVelocity, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData)
{
  EGMC_FloatPrecision AngularVelocityCompression = ToNativeEnum(NetInfo.OwningComponent->ReplicationSettings.DefaultCompressionSettings.AngularVelocity);
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeVector(AngularVelocity, AngularVelocityCompression, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeVector(AngularVelocity, AngularVelocityCompression, Ar);
  }
}

void FGMC_Move::SerializeActorLocation(FVector& ActorLocation, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData)
{
  EGMC_FloatPrecision ActorLocationCompression = ToNativeEnum(NetInfo.OwningComponent->ReplicationSettings.DefaultCompressionSettings.ActorLocation);
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeVector(ActorLocation, ActorLocationCompression, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeVector(ActorLocation, ActorLocationCompression, Ar);
  }
}

void FGMC_Move::SerializeActorRotation(FRotator& ActorRotation, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData)
{
  EGMC_FloatPrecision ActorRotationCompression = ToNativeEnum(NetInfo.OwningComponent->ReplicationSettings.DefaultCompressionSettings.ActorRotation);
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeRotator(ActorRotation, ActorRotationCompression, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeRotator(ActorRotation, ActorRotationCompression, Ar);
  }
}

void FGMC_Move::SerializeActorScale(FVector& ActorScale, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData)
{
  EGMC_FloatPrecision ActorScaleCompression = ToNativeEnum(NetInfo.OwningComponent->ReplicationSettings.DefaultCompressionSettings.ActorScale);
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeVector(ActorScale, ActorScaleCompression, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeVector(ActorScale, ActorScaleCompression, Ar);
  }
}

void FGMC_Move::SerializeControlRotation(FRotator& ControlRotation, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData)
{
  EGMC_FloatPrecision ControlRotationCompression = ToNativeEnum(NetInfo.OwningComponent->ReplicationSettings.DefaultCompressionSettings.ControlRotation);
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeRotator(ControlRotation, ControlRotationCompression, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeRotator(ControlRotation, ControlRotationCompression, Ar);
  }
}

void FGMC_ServerAuthPhysicsSettings::SaveSettings(UGMC_ReplicationCmp* ReplicationComponent)
{
  bUseClientPrediction = ReplicationComponent->bUseClientPrediction;
  bSmoothRemoteListenServerPawn = ReplicationComponent->bSmoothRemoteListenServerPawn;
  InterpolationMode = ReplicationComponent->InterpolationMode;
  ExtrapolationMode = ReplicationComponent->ExtrapolationMode;
  ClientInterpolationMode = ReplicationComponent->PhysicsInterpolationMode;
  ClientInterpolationMethod = ReplicationComponent->PhysicsInterpolationMethod;
}

void FGMC_ServerAuthPhysicsSettings::ApplySettings(UGMC_ReplicationCmp* ReplicationComponent)
{
  ReplicationComponent->bUseClientPrediction = bUseClientPrediction;
  ReplicationComponent->bSmoothRemoteListenServerPawn = bSmoothRemoteListenServerPawn;
  ReplicationComponent->InterpolationMode = InterpolationMode;
  ReplicationComponent->ExtrapolationMode = ExtrapolationMode;
  ReplicationComponent->PhysicsInterpolationMode = ClientInterpolationMode;
  ReplicationComponent->PhysicsInterpolationMethod = ClientInterpolationMethod;
}

bool FGMC_ServerAuthPhysicsSettings::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
  static_assert((uint8)EGMC_PhysicsInterpolationMode::MAX <= 4, "Number of serialized bits need to be increased");
  static_assert((uint8)EGMC_PhysicsInterpolationMethod::MAX <= 16, "Number of serialized bits need to be increased");

  uint8 PhysicsInterpolationModeByte{0};
  uint8 PhysicsInterpolationMethodByte{0};
  if (Ar.IsSaving())
  {
    PhysicsInterpolationModeByte = static_cast<uint8>(ClientInterpolationMode);
    Ar.SerializeBits(&PhysicsInterpolationModeByte, 2);
    PhysicsInterpolationMethodByte = static_cast<uint8>(ClientInterpolationMethod);
    Ar.SerializeBits(&PhysicsInterpolationMethodByte, 4);
  }
  else if (Ar.IsLoading())
  {
    Ar.SerializeBits(&PhysicsInterpolationModeByte, 2);
    ClientInterpolationMode = static_cast<EGMC_PhysicsInterpolationMode>(PhysicsInterpolationModeByte);
    Ar.SerializeBits(&PhysicsInterpolationMethodByte, 4);
    ClientInterpolationMethod = static_cast<EGMC_PhysicsInterpolationMethod>(PhysicsInterpolationMethodByte);
  }

  return (bOutSuccess = true);
}

void FGMC_ClientAuthPhysicsSettings::SaveSettings(UGMC_ReplicationCmp* ReplicationComponent)
{
  bUseClientPrediction = ReplicationComponent->bUseClientPrediction;
  bAssumeClientState = ReplicationComponent->bAssumeClientState;
  InterpolationMode = ReplicationComponent->InterpolationMode;
  ExtrapolationMode = ReplicationComponent->ExtrapolationMode;
  ServerInterpolationMode = ReplicationComponent->PhysicsInterpolationMode;
  ServerInterpolationMethod = ReplicationComponent->PhysicsInterpolationMethod;
  bRollbackServerPawns = ReplicationComponent->bRollBackServerPawns;
  bRollbackClientPawns = ReplicationComponent->bRollBackClientPawns;
  bRollbackGenericServerActors = ReplicationComponent->bRollBackGenericServerActors;
  bRollbackGenericClientActors = ReplicationComponent->bRollBackGenericClientActors;
}

void FGMC_ClientAuthPhysicsSettings::ApplySettings(UGMC_ReplicationCmp* ReplicationComponent)
{
  ReplicationComponent->bUseClientPrediction = bUseClientPrediction;
  ReplicationComponent->bAssumeClientState = bAssumeClientState;
  ReplicationComponent->InterpolationMode = InterpolationMode;
  ReplicationComponent->ExtrapolationMode = ExtrapolationMode;
  ReplicationComponent->PhysicsInterpolationMode = ServerInterpolationMode;
  ReplicationComponent->PhysicsInterpolationMethod = ServerInterpolationMethod;
  ReplicationComponent->bRollBackServerPawns = bRollbackServerPawns;
  ReplicationComponent->bRollBackClientPawns = bRollbackClientPawns;
  ReplicationComponent->bRollBackGenericServerActors = bRollbackGenericServerActors;
  ReplicationComponent->bRollBackGenericClientActors = bRollbackGenericClientActors;
}

bool FGMC_ClientAuthPhysicsSettings::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
  static_assert((uint8)EGMC_PhysicsInterpolationMode::MAX <= 4, "Number of serialized bits need to be increased");
  static_assert((uint8)EGMC_PhysicsInterpolationMethod::MAX <= 16, "Number of serialized bits need to be increased");

  uint8 PhysicsInterpolationModeByte{0};
  uint8 PhysicsInterpolationMethodByte{0};
  if (Ar.IsSaving())
  {
    PhysicsInterpolationModeByte = static_cast<uint8>(ServerInterpolationMode);
    Ar.SerializeBits(&PhysicsInterpolationModeByte, 2);
    PhysicsInterpolationMethodByte = static_cast<uint8>(ServerInterpolationMethod);
    Ar.SerializeBits(&PhysicsInterpolationMethodByte, 4);
  }
  else if (Ar.IsLoading())
  {
    Ar.SerializeBits(&PhysicsInterpolationModeByte, 2);
    ServerInterpolationMode = static_cast<EGMC_PhysicsInterpolationMode>(PhysicsInterpolationModeByte);
    Ar.SerializeBits(&PhysicsInterpolationMethodByte, 4);
    ServerInterpolationMethod = static_cast<EGMC_PhysicsInterpolationMethod>(PhysicsInterpolationMethodByte);
  }

  Ar.SerializeBits(&bRollbackServerPawns, 1);
  Ar.SerializeBits(&bRollbackClientPawns, 1);
  Ar.SerializeBits(&bRollbackGenericServerActors, 1);
  Ar.SerializeBits(&bRollbackGenericClientActors, 1);

  return (bOutSuccess = true);
}

void UGMC_ReplicationCmp::FComponentStatus::SaveFrameInfo(bool bDidUpdate, const UGMC_ReplicationCmp* const Outer)
{
  bDidGMCUpdateLastFrame = bDidUpdate;
  ControllerLastFrame = Outer->GetController();
  NetRoleLastFrame = Outer->PawnOwner ? Outer->PawnOwner->GetLocalRole() : ROLE_None;

  if (Outer->GetNetMode() < NM_Client)
  {
    bIsPlayerOwned = static_cast<bool>(Cast<APlayerController>((Outer->GetController())));
  }
  else if (Outer->IsAutonomousProxy())
  {
    bIsPlayerOwned = true;
  }
  else if (Outer->IsSimulatedProxy())
  {
    bIsPlayerOwned = Outer->SPMove().MetaData.bPlayerOwned;
  }
  else
  {
    // Client-spawned or "ROLE_None".
    bIsPlayerOwned = false;
  }
}

void UGMC_ReplicationCmp::FComponentStatus::PreTick(float DeltaTime)
{
  ElapsedRealTimeSinceSpawn += DeltaTime;
  MoveExecutionTimer += DeltaTime;
}

bool UGMC_ReplicationCmp::FComponentStatus::ShouldExecuteMove(float& InOutDeltaTime, const UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->PawnOwner)
  gmc_ck(Outer->GetNetMode() < NM_Client) // Only checked on the server for now.

  if (Outer->MoveExecutionFrequency > 0)
  {
    const auto& Owner = Outer->PawnOwner;
    const bool bIsLocalServerPawn = Outer->IsServerPawn() && (Owner->IsLocallyControlled() || !Owner->Controller);
    const bool bIsPlayerControlled = Outer->IsPlayerControlledPawn();

    if (bIsLocalServerPawn && !bIsPlayerControlled)
    {
      InOutDeltaTime = (AccumulatedMoveDeltaTime += InOutDeltaTime);

      if (MoveExecutionTimer < 1. / Outer->MoveExecutionFrequency)
      {
        return false;
      }
    }
  }

  InOutDeltaTime = FMath::Clamp(InOutDeltaTime, MIN_DELTA_TIME, Outer->MaxMoveDeltaTime);

  MoveExecutionTimer = 0.;
  AccumulatedMoveDeltaTime = 0.f;

  return true;
}

void UGMC_ReplicationCmp::FComponentStatus::CL_SetUseUnreliableClientMoves(int32 NumPendingReliablePackets, UGMC_ReplicationCmp* const Outer)
{
  if (Outer->GetElapsedTimeSinceSpawn() < UNRELIABLE_CLIENT_MOVES_SPAWN_TOLERANCE)
  {
    gmc_ck(!bUseUnreliableClientMoves)
    return;
  }

  const double Time = Outer->GetTime();
  const int32 MaxNumPendingReliablePackets = Outer->UseUnreliableClientMovesThreshold;

  if (MaxNumPendingReliablePackets < 0 || NumPendingReliablePackets > MaxNumPendingReliablePackets)
  {
    const bool bJustActivatedUnreliableClientMoves = !bUseUnreliableClientMoves;

    bUseUnreliableClientMoves = true;
    CL_UseUnreliableClientMovesActivationTime = Time;

    if (bJustActivatedUnreliableClientMoves)
    {
      gmc_ck(IsValid(Outer->GetGMCPawnOwner()))
      Outer->GetGMCPawnOwner()->SV_ReceiveClientSendStatus(bUseUnreliableClientMoves);

      GMC_LOG(LogGMCReplication, Outer->GetPawnOwner(), Verbose, TEXT("Unreliable client move transmission enabled."))
      CALL_NATIVE_EVENT_CONDITIONAL(Outer->bNoBlueprintEvents, Outer, OnUnreliableClientMovesActivated);
    }

    return;
  }

  if (!bUseUnreliableClientMoves)
  {
    gmc_ck(CL_UseUnreliableClientMovesActivationTime == 0.)
    return;
  }

  const double TimeDiff = Time - CL_UseUnreliableClientMovesActivationTime;

  if (TimeDiff <= 0.)
  {
    // There's probably some temporary sync inaccuracy with the client time, just update the activation time and wait until the next frame to check again.
    CL_UseUnreliableClientMovesActivationTime = Time;
    return;
  }

  if (TimeDiff > USE_UNRELIABLE_CLIENT_MOVES_TIMEOUT)
  {
    gmc_ck(bUseUnreliableClientMoves)
    bUseUnreliableClientMoves = false;
    CL_UseUnreliableClientMovesActivationTime = 0.;

    // Should have been reset during net serialization.
    gmc_ck(!CL_bJustDeactivatedUnreliableClientMoves)
    CL_bJustDeactivatedUnreliableClientMoves = true;

    if (CL_bJustDeactivatedUnreliableClientMoves)
    {
      gmc_ck(IsValid(Outer->GetGMCPawnOwner()))
      Outer->GetGMCPawnOwner()->SV_ReceiveClientSendStatus(bUseUnreliableClientMoves);

      GMC_LOG(LogGMCReplication, Outer->GetPawnOwner(), Verbose, TEXT("Unreliable client move transmission disabled."))
      CALL_NATIVE_EVENT_CONDITIONAL(Outer->bNoBlueprintEvents, Outer, OnUnreliableClientMovesDeactivated);
    }

    return;
  }
}

bool UGMC_ReplicationCmp::FComponentStatus::ShouldClearTransientData(const UGMC_ReplicationCmp* const Outer) const
{
  gmc_ck(IsValid(Outer->PawnOwner))
  return !bDidGMCUpdateLastFrame || ControllerLastFrame != Outer->GetController() || NetRoleLastFrame != Outer->PawnOwner->GetLocalRole();
}

void UGMC_ReplicationCmp::FSyncDataSwapBuffer::Initialize(bool bUseRelative, UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(!bInitialized)
  InitializeSyncData(
    Buffer,
    Outer->ReplicationSettings,
    Outer->GetSyncTagsData(),
    Outer->AliasData,
    GMCReplication::ESimState::None,
    GMCReplication::ESimType::None,
    Outer
  );
  ProcessSyncData(Buffer, {DataOp::Save}, Outer->AliasData, bUseRelative, Outer);
  bInitialized = true;
}

void UGMC_ReplicationCmp::FSyncDataSwapBuffer::Initialize(const FGMC_PawnState& InitState, UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(!bInitialized)
  InitializeSyncData(
    Buffer,
    Outer->ReplicationSettings,
    Outer->GetSyncTagsData(),
    Outer->AliasData,
    GMCReplication::ESimState::None,
    GMCReplication::ESimType::None,
    Outer
  );
  Buffer = InitState;
  bInitialized = true;
}

void UGMC_ReplicationCmp::FSyncDataSwapBuffer::Swap(bool bUseRelative, UGMC_ReplicationCmp* const Outer, EGMC_NetContext Context)
{
  // The swap state can be initialized from the buffer for the correct alias data.
  FGMC_PawnState SwapState = Buffer;
  ProcessSyncData(SwapState, {DataOp::Save}, Outer->AliasData, bUseRelative, Outer);
  ProcessSyncData(Buffer, {DataOp::Apply}, Outer->AliasData, bUseRelative, Outer);
  CALL_NATIVE_EVENT_CONDITIONAL(Outer->bNoBlueprintEvents, Outer, OnSyncDataApplied, SwapState, Context);
  Buffer = SwapState;
  ++NumSwaps;
}

bool UGMC_ReplicationCmp::FSyncDataSwapBuffer::HasEvenNumberOfSwaps() const
{
  return !(NumSwaps & 1);
}

void UGMC_ReplicationCmp::FNetReserializationAux::ReserializationTimer_Impl(const UGMC_ReplicationCmp* const Outer) const
{
  gmc_ck(Outer->IsServerPawn())

  if (Outer->NetReserializationInterval <= 0.f)
  {
    return;
  }

  if (const auto& Owner = Outer->GetGMCPawnOwner())
  {
    Owner->SV_MarkForNetReserialization();
  }
}

void UGMC_ReplicationCmp::FNetReserializationAux::SetTimer(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->IsServerPawn())

  if (const auto& World = Outer->GetWorld())
  {
    World->GetTimerManager().SetTimer(
      TimerHandle,
      Outer,
      &UGMC_ReplicationCmp::SV_NetReserializationTimer,
      Outer->NetReserializationInterval,
      true,
      -1.f
    );
  }
}

void UGMC_ReplicationCmp::FNetReserializationAux::ClearTimer(const UGMC_ReplicationCmp* const Outer)
{
  if (const auto& World = Outer->GetWorld())
  {
    World->GetTimerManager().ClearTimer(TimerHandle);
  }
}

void UGMC_ReplicationCmp::FNetReserializationAux::PreReplication(const UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->IsServerPawn())
  gmc_ck(Outer->GetGMCPawnOwner())

  const auto& Owner = Outer->GetGMCPawnOwner();
  if (!Owner || !Owner->SV_NeedsNetReserialization())
  {
    return;
  }

  bOwnerNeedsReserialization = true;
}

void UGMC_ReplicationCmp::FNetReserializationAux::PreTick(const UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->IsServerPawn())

  if (Outer->GetElapsedTimeSinceSpawn() <= NET_RESERIALIZATION_LEAD_TIME)
  {
    if (const auto& Owner = Outer->GetGMCPawnOwner())
    {
      // Always mark new pawns for reserialization for a short period of time after spawning. Clients may not receive the initial packets and could smooth the
      // pawn based on zero-initialized values otherwise.
      Owner->SV_MarkForNetReserialization();
      return;
    }
  }

  bOwnerNeedsReserialization = false;
}

bool UGMC_ReplicationCmp::FPredictedClientNetSerializationAux::ShouldUpdate(bool bValidClientMove) const
{
  if (!bValidClientMove || UpdateFrequency <= 0)
  {
    return true;
  }

  return TimeSinceLastUpdate >= 1. / UpdateFrequency;
}

void UGMC_ReplicationCmp::FPredictedClientNetSerializationAux::ResetTimer()
{
  TimeSinceLastUpdate = 0.;
}

void UGMC_ReplicationCmp::FPredictedClientNetSerializationAux::PreTick(const UGMC_ReplicationCmp* const Outer, float DeltaTime)
{
  gmc_ck(Outer->PawnOwner)
  TimeSinceLastUpdate += DeltaTime;
  UpdateFrequency = FMath::Min(Outer->PredictedClientNetUpdateFrequency, Outer->PawnOwner->NetUpdateFrequency);
}

bool UGMC_ReplicationCmp::FTimestampVerificationAux::ComputeAccumulatedClientTime(double& OutClientTime) const
{
  OutClientTime = 0.;
  const int32 Num = TimestampBuffer.GetNum();

  if (Num == 0)
  {
    return true;
  }

  const auto& Buffer = TimestampBuffer.GetBuffer();
  const int32 Offset = TimestampBuffer.GetOffset();
  const int32 Size = BUFFER_SIZE;

  const double OldestTimestamp = TimestampBuffer.GetOldestElement();

  double DeltaSum = 0.;
  double NextValue = 0.;
  double PrevValue = OldestTimestamp;

  if (Num < Size)
  {
    for (int32 Index = 0; Index < Num - 1; ++Index)
    {
      NextValue = Buffer[Index + 1];
      if (NextValue <= PrevValue)
      {
        return false;
      }
      DeltaSum += NextValue - PrevValue;
      PrevValue = NextValue;
    }
  }
  else
  {
    for (int32 Index = 0; Index < Num - 1; ++Index)
    {
      NextValue = Buffer[(Offset + Size + Index + 1) % Size];
      if (NextValue <= PrevValue)
      {
        return false;
      }
      DeltaSum += NextValue - PrevValue;
      PrevValue = NextValue;
    }
  }

  OutClientTime = OldestTimestamp + DeltaSum;
  return true;
}

void UGMC_ReplicationCmp::FTimestampVerificationAux::ResetClientCredibility_Impl(const UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->IsServerPawn())

  InfractionsThisPeriod = 0;
}

void UGMC_ReplicationCmp::FTimestampVerificationAux::SetTimer(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->IsServerPawn())

  if (const auto& World = Outer->GetWorld())
  {
    World->GetTimerManager().SetTimer(
      TimerHandle,
      Outer,
      &UGMC_ReplicationCmp::SV_ResetClientCredibility,
      Outer->VerificationPeriod,
      true,
      -1.f
    );
  }
}

void UGMC_ReplicationCmp::FTimestampVerificationAux::ClearTimer(const UGMC_ReplicationCmp* const Outer)
{
  if (const auto& World = Outer->GetWorld())
  {
    World->GetTimerManager().ClearTimer(TimerHandle);
  }
}

void UGMC_ReplicationCmp::FClientMoveExecutionAux::PreTick(const UGMC_ReplicationCmp* const Outer, float DeltaTime)
{
  gmc_ck(Outer->IsAutonomousProxy())

  TimeSinceLastMoveBatchWasSent += DeltaTime;
}

FGMC_Move& UGMC_ReplicationCmp::FClientMoveExecutionAux::GetCurrentMoveForExecution(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->IsAutonomousProxy())

  if (Outer->bUseClientPrediction)
  {
    auto& History = Outer->MoveHistory;
    const int32 HistoryNum = History.Num();
    return HistoryNum > 0 ? History[HistoryNum - 1] : GetDefaultMove(Outer);
  }
  else
  {
    const int32 NonPredictedMovesNum = NonPredictedMoves.Num();
    return NonPredictedMovesNum > 0 ? NonPredictedMoves[NonPredictedMovesNum - 1] : GetDefaultMove(Outer);
  }
}

FGMC_Move& UGMC_ReplicationCmp::FClientMoveExecutionAux::GetPreviousMoveForExecution(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->IsAutonomousProxy())

  if (Outer->bUseClientPrediction)
  {
    auto& History = Outer->MoveHistory;
    const int32 HistoryNum = History.Num();
    return HistoryNum > 1 ? History[HistoryNum - 2] : GetDefaultMove(Outer);
  }
  else
  {
    const int32 NonPredictedMovesNum = NonPredictedMoves.Num();
    return NonPredictedMovesNum > 1 ? NonPredictedMoves[NonPredictedMovesNum - 2] : GetDefaultMove(Outer);
  }
}

FGMC_Move& UGMC_ReplicationCmp::FClientMoveExecutionAux::GetDefaultMove(const UGMC_ReplicationCmp* const Outer)
{
  DefaultMove = FGMC_Move{};
  DefaultMove.NetInfo.OwningComponent = (UGMC_ReplicationCmp*)Outer;

  InitializeSyncData(
    DefaultMove.InputState,
    Outer->ReplicationSettings,
    Outer->GetSyncTagsData(),
    Outer->AliasData,
    GMCReplication::ESimState::None,
    GMCReplication::ESimType::None,
    const_cast<UGMC_ReplicationCmp*>(Outer)
  );

  InitializeSyncData(
    DefaultMove.OutputState,
    Outer->ReplicationSettings,
    Outer->GetSyncTagsData(),
    Outer->AliasData,
    GMCReplication::ESimState::None,
    GMCReplication::ESimType::None,
    const_cast<UGMC_ReplicationCmp*>(Outer)
  );

  return DefaultMove;
}

bool UGMC_ReplicationCmp::FSmoothClientCorrection::HasData() const
{
  // The accumulated time is only greater or equal to 0 when the buffer was initialized, otherwise it is -1.
  return Buffer.AccumulatedTime >= 0.f;
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::AddCorrectionTime(float TimeToAdd)
{
  // The time must necessarily be greater than 0 here so that we can determine that a correction has begun.
  Buffer.AccumulatedTime += FMath::Clamp(TimeToAdd, MIN_DELTA_TIME, UE_BIG_NUMBER);
}

bool UGMC_ReplicationCmp::FSmoothClientCorrection::HasBegunCorrection()
{
  // The correction has begun when time was added to the buffer at least once.
  return Buffer.AccumulatedTime > 0.f;
}

bool UGMC_ReplicationCmp::FSmoothClientCorrection::HasFinishedCorrection(float TotalDuration)
{
  return Buffer.AccumulatedTime >= TotalDuration;
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::SaveTargetTransform(const UGMC_ReplicationCmp* const Outer)
{
  // The target transform is the desired end state of the correction. It is not fixed but updated every frame with the real/current client pawn transform
  // while we are still interpolating visually.
  SaveTransform(Outer, TargetTransform);
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::SavePreReplayTransform(const UGMC_ReplicationCmp* const Outer)
{
  SaveTransform(Outer, PreReplayTransform);
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::SavePostReplayTransform(const UGMC_ReplicationCmp* const Outer)
{
  SaveTransform(Outer, PostReplayTransform);
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::SetTargetTransformReached(const UGMC_ReplicationCmp* const Outer)
{
  Status.bTargetLocationReached = PostReplayTransform.Location.Equals(PreReplayTransform.Location, Status.LOCATION_TOLERANCE);
  Status.bTargetRotationReached = PostReplayTransform.Rotation.Equals(PreReplayTransform.Rotation, Status.ROTATION_TOLERANCE);
  Status.bTargetScaleReached = PostReplayTransform.Scale.Equals(PreReplayTransform.Scale, Status.SCALE_TOLERANCE);
  Status.bTargetControlRotationReached = PostReplayTransform.ControlRotation.Equals(PreReplayTransform.ControlRotation, Status.CONTROL_ROTATION_TOLERANCE);
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::SwapTransform(UGMC_ReplicationCmp* const Outer)
{
  const FVector SwapActorLocation = Outer->GetActorLocation_GMC();
  const FRotator SwapActorRotation = Outer->GetActorRotation_GMC();
  const FVector SwapActorScale = Outer->GetActorScale_GMC();
  const FRotator SwapControlRotation = Outer->GetControllerRotation_GMC();

  Outer->SetActorLocation_GMC(Buffer.SwapTransform.Location, true);
  Outer->SetActorRotation_GMC(Buffer.SwapTransform.Rotation, true);
  Outer->SetActorScale_GMC(Buffer.SwapTransform.Scale);
  Outer->SetControllerRotation_GMC(Buffer.SwapTransform.ControlRotation);

  Buffer.SwapTransform.Location = SwapActorLocation;
  Buffer.SwapTransform.Rotation = SwapActorRotation;
  Buffer.SwapTransform.Scale = SwapActorScale;
  Buffer.SwapTransform.ControlRotation = SwapControlRotation;
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::Interpolate(const UGMC_ReplicationCmp* const Outer, float TotalDuration)
{
  gmc_ck(TotalDuration >= 0.01f)
  const float Ratio = FMath::Clamp(Buffer.AccumulatedTime / FMath::Clamp(TotalDuration, 0.01f, UE_BIG_NUMBER), 0.f, 1.f);

  const auto& Settings = Outer->SmoothCorrection;
  const bool bInterpLocation = Settings.bSmoothActorLocation && !Status.bTargetLocationReached;
  const bool bInterpRotation = Settings.bSmoothActorRotation && !Status.bTargetRotationReached;
  const bool bInterpScale = Settings.bSmoothActorScale && !Status.bTargetScaleReached;
  const bool bInterpControlRotation = Settings.bSmoothControlRotation && !Status.bTargetControlRotationReached;

  auto& SwapTransform = Buffer.SwapTransform;

  SwapTransform.Location = !bInterpLocation ? (StartTransform.Location = TargetTransform.Location) :
    FMath::LerpStable(
      StartTransform.Location,
      TargetTransform.Location,
      Ratio
    );

  SwapTransform.Rotation = !bInterpRotation ? (StartTransform.Rotation = TargetTransform.Rotation) :
    FQuat::Slerp(
      StartTransform.Rotation.Quaternion(),
      TargetTransform.Rotation.Quaternion(),
      Ratio
    ).Rotator();

  SwapTransform.Scale = !bInterpScale ? (StartTransform.Scale = TargetTransform.Scale) :
    FMath::LerpStable(
      StartTransform.Scale,
      TargetTransform.Scale,
      Ratio
    );

  SwapTransform.ControlRotation = !bInterpControlRotation ? (StartTransform.ControlRotation = TargetTransform.ControlRotation) :
    FQuat::Slerp(
      StartTransform.ControlRotation.Quaternion(),
      TargetTransform.ControlRotation.Quaternion(),
      Ratio
    ).Rotator();
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::InitCorrection()
{
  // Setting the accumulated time to 0 indicates that a new smooth correction has started.
  Buffer.AccumulatedTime = 0.f;
}

void UGMC_ReplicationCmp::FSmoothClientCorrection::SaveTransform(const UGMC_ReplicationCmp* const Outer, CorrectionTransform& Transform) const
{
  Transform.Location = Outer->GetActorLocation_GMC();
  Transform.Rotation = Outer->GetActorRotation_GMC();
  Transform.Scale = Outer->GetActorScale_GMC();
  Transform.ControlRotation = Outer->GetControllerRotation_GMC();
}

void UGMC_ReplicationCmp::FSimulationAux::PreTick(UGMC_ReplicationCmp* const Outer)
{
  bWasExtrapolatingLastUpdate = bIsExtrapolating;
  bIsExtrapolating = false;

  if (const auto& OwningPawn = Outer->PawnOwner)
  {
    if (const auto& Controller = OwningPawn->GetController())
    {
      // Keep the simulated control rotation in sync with the actual controller value.
      Outer->SimulatedControlRotation = Controller->GetControlRotation();
    }
  }
}

void UGMC_ReplicationCmp::FRollbackRestoreBuffer::Initialize(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(!bInitialized)
  InitializeSyncData(
    Buffer,
    Outer->ReplicationSettings,
    Outer->GetSyncTagsData(),
    Outer->AliasData,
    GMCReplication::ESimState::None,
    GMCReplication::ESimType::None,
    Outer
  );
  bInitialized = true;
}

void UGMC_ReplicationCmp::FRollbackRestoreBuffer::Save(bool bUseRelative, UGMC_ReplicationCmp* const Outer)
{
  ProcessSyncData(Buffer, {DataOp::Save}, Outer->AliasData, bUseRelative, Outer);
}

void UGMC_ReplicationCmp::FRollbackRestoreBuffer::Restore(bool bUseRelative, UGMC_ReplicationCmp* const Outer, EGMC_NetContext Context)
{
  ProcessSyncData(Buffer, {DataOp::Apply}, Outer->AliasData, bUseRelative, Outer);
  CALL_NATIVE_EVENT_CONDITIONAL(Outer->bNoBlueprintEvents, Outer, OnSyncDataApplied, Buffer, Context);
}

void UGMC_ReplicationCmp::FDynamicBufferTimeAux::Update(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay)

  const auto& World = Outer->GetWorld();
  if (!World)
  {
    return;
  }

  const float DeltaTime = World->DeltaRealTimeSeconds;

  RequestTimer += DeltaTime;

  // A value of 0 means no new value will be requested (but the previous one may be requested again if not applied yet).
  SignedDiff = 0.f;

  const int32 LastIdx = Outer->MoveHistory.Num() - 1;
  if (LastIdx < 0)
  {
    gmc_ck(SignedDiff == 0.f)
    return;
  }

  const double PreviousLatestTimestamp = Outer->SimulationAux.PrevLatestTime;
  if (PreviousLatestTimestamp < 0.)
  {
    gmc_ck(SignedDiff == 0.f)
    return;
  }

  const double LatestTimestamp = Outer->IsUsingServerAuthPhysicsReplication() ?
    Outer->MoveHistory[LastIdx].MetaData.ServerAuthPhysicsTimestamp : Outer->MoveHistory[LastIdx].MetaData.Timestamp;
  const double PreviousInterpTargetTimestamp = Outer->SimulationAux.PrevInterpTargetTime;
  const double PreviousExtrapTargetTimestamp = Outer->SimulationAux.PrevExtrapTargetTime;
  const double PreviousSmoothingTime = Outer->SimulationAux.PrevSmoothingTime;

  static_assert(MIN_LOW_DIST < 0.f);

  LowDist += DeltaTime * LOW_DIST_INCREASE_PER_SEC;
  gmc_ck(LowDist >= MIN_LOW_DIST)

  auto UpdateAllAndLowBuffer = [this](float Distance)
  {
    AllBuffer.Add(Distance);
    if (Distance < LowDist)
    {
      LowDist = FMath::Clamp(Distance, MIN_LOW_DIST, UE_BIG_NUMBER);
      LowBuffer.Add(LowDist);
    }
  };

  if (PreviousInterpTargetTimestamp >= 0.)
  {
    gmc_ck(PreviousSmoothingTime >= 0.)
    gmc_ck(PreviousExtrapTargetTimestamp == -1.)
    gmc_ck(LatestTimestamp >= PreviousInterpTargetTimestamp)
    gmc_ck(LatestTimestamp >= PreviousSmoothingTime)

    // When interpolating we want to maintain a certain time distance from the latest available state.
    UpdateAllAndLowBuffer(LatestTimestamp - PreviousSmoothingTime);
  }
  else if (PreviousExtrapTargetTimestamp >= 0.)
  {
    gmc_ck(PreviousSmoothingTime >= 0.)
    gmc_ck(PreviousSmoothingTime > PreviousExtrapTargetTimestamp)
    gmc_ck(PreviousInterpTargetTimestamp == -1.)

    // A negative distance means we are extrapolating.
    UpdateAllAndLowBuffer(PreviousExtrapTargetTimestamp - PreviousSmoothingTime);
  }
  else
  {
    gmc_ck(PreviousLatestTimestamp >= 0.)
    gmc_ck(PreviousSmoothingTime == -1.)

    // Extrapolation may be disabled, in this case our best bet is to take the distance from the current time.
    UpdateAllAndLowBuffer(PreviousLatestTimestamp - Outer->GetTime());
  }

  if (AllBuffer.GetNum() < MIN_BUFFER_NUM_FOR_AVG)
  {
    // Do not compute an average until the buffer is sufficiently filled.
    gmc_ck(SignedDiff == 0.f)
    return;
  }

  static_assert(TRIM_PERCENTAGE < 0.5f);

  const float CurrentAvgDistance = AllBuffer.GetTrimmedMean(TRIM_PERCENTAGE);

  if (LowBuffer.GetNum() >= MIN_LOW_BUFFER_NUM_FOR_AVG)
  {
    const float CurrentAvgLowDistance = LowBuffer.GetMean();

    if (CurrentAvgLowDistance <= 0.f)
    {
      // We are still experiencing some extrapolation, increase the target range.
      TargetDist += DeltaTime * TARGET_DIST_ADJUST_PER_SEC;
      TargetDistTolerance += DeltaTime * TARGET_DIST_TOLERANCE_ADJUST_PER_SEC;
      TargetDist = FMath::Clamp(TargetDist, MIN_TARGET_DIST, MAX_TARGET_DIST);
      TargetDistTolerance = FMath::Clamp(TargetDistTolerance, MIN_TARGET_DIST_TOLERANCE, MAX_TARGET_DIST_TOLERANCE);
    }
    else if (CurrentAvgDistance >= TARGET_DIST_DECREASE_THRESHOLD && CurrentAvgLowDistance >= TARGET_DIST_DECREASE_THRESHOLD)
    {
      // We are not in any danger of experiencing extrapolation and can decrease the target range.
      TargetDist -= DeltaTime * TARGET_DIST_ADJUST_PER_SEC;
      TargetDistTolerance -= DeltaTime * TARGET_DIST_TOLERANCE_ADJUST_PER_SEC;
      TargetDist = FMath::Clamp(TargetDist, MIN_TARGET_DIST, MAX_TARGET_DIST);
      TargetDistTolerance = FMath::Clamp(TargetDistTolerance, MIN_TARGET_DIST_TOLERANCE, MAX_TARGET_DIST_TOLERANCE);
    }
    else
    {
      gmc_ck(CurrentAvgLowDistance > 0.f)
      gmc_ck(CurrentAvgDistance < TARGET_DIST_DECREASE_THRESHOLD || CurrentAvgLowDistance < TARGET_DIST_DECREASE_THRESHOLD)
      // We want to keep the current target range in this case.
    }
  }

  gmc_ck(TargetDist >= MIN_TARGET_DIST)
  gmc_ck(TargetDist <= MAX_TARGET_DIST)
  gmc_ck(TargetDistTolerance >= MIN_TARGET_DIST_TOLERANCE)
  gmc_ck(TargetDistTolerance <= MAX_TARGET_DIST_TOLERANCE)

  if (HasPendingBufferTime())
  {
    // We want to keep adjusting the target distance while having a pending request but there's no need to update the diff buffer as well since all buffers will
    // be reset anyway when the pending buffer time is applied.
    gmc_ck(SignedDiff == 0.f)
    return;
  }

  gmc_ck(RequestedBufferTime == 0.f)

  const float Diff = FMath::Abs(TargetDist - CurrentAvgDistance);

  const auto UpdateDiffBuffer = [this](float Diff)
  {
    DiffBuffer.Add(Diff);

    if (DiffBuffer.GetNum() >= MIN_BUFFER_NUM_FOR_AVG)
    {
      SignedDiff = DiffBuffer.GetMean();

      if (SignedDiff > 0.f)
      {
        // Diffs below the target distance are always clamped by the min tolerance.
        if (SignedDiff <= MIN_TARGET_DIST_TOLERANCE)
        {
          SignedDiff = 0.f;
        }
        return;
      }

      if (SignedDiff < 0.f)
      {
        if (FMath::Abs(SignedDiff) <= TargetDistTolerance)
        {
          SignedDiff = 0.f;
        }
        return;
      }

      gmc_ck(SignedDiff == 0.f)
    }
  };

  if (CurrentAvgDistance <= TargetDist)
  {
    // The buffer time tends to be too low.
    UpdateDiffBuffer(+Diff);
  }
  else
  {
    // The buffer time tends to be too high.
    UpdateDiffBuffer(-Diff);
  }
}

bool UGMC_ReplicationCmp::FDynamicBufferTimeAux::Request(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(Outer->InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay)

  RequestTimer = 0.;

  if (!WantsNewBufferTime())
  {
    gmc_ck(SignedDiff == 0.f)

    // Since the client requests are unreliable we keep requesting the desired buffer time at the specified interval until it was applied.
    return HasPendingBufferTime();
  }

  gmc_ck(SignedDiff != 0.f)
  gmc_ck(RequestedBufferTime == 0.f)

  const auto& Owner = Outer->PawnOwner;
  if (!IsValid(Owner))
  {
    return false;
  }

  const float CurrentBufferTime = Outer->AdaptiveDelayParams.BufferTime;
  gmc_ck(CurrentBufferTime >= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
  gmc_ck(CurrentBufferTime <= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)

  RequestedBufferTime =
    FMath::Clamp(CurrentBufferTime + SignedDiff, FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME, FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME);

  if (
    (
      CurrentBufferTime == FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME &&
      RequestedBufferTime >= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME
    ) ||
    (
      CurrentBufferTime == FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME &&
      RequestedBufferTime <= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME
    )
  )
  {
    // No point in requesting a new buffer time if it will only be clamped to the current value.
    RequestedBufferTime = 0.f;
    return false;
  }

  // Rounding can introduce floating point error so we do it after the check above.
  GMCCompression::Round(RequestedBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION);

  gmc_ck(RequestedBufferTime >= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
  gmc_ck(RequestedBufferTime <= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)

  if (FMath::IsNearlyEqual(CurrentBufferTime, RequestedBufferTime, 1.f / (int32)FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION))
  {
    RequestedBufferTime = 0.f;
    return false;
  }

  gmc_ck(RequestedBufferTime != 0.f)

  return true;
}

void UGMC_ReplicationCmp::FDynamicBufferTimeAux::Apply(UGMC_ReplicationCmp* const Outer)
{
  gmc_ck(HasPendingBufferTime())

  // A requested buffer time of 0 means we have no pending request.
  RequestedBufferTime = 0.f;

  // Don't request a new buffer time too quickly again.
  RequestTimer = -GetRequestHoldTime(Outer);

  // Reset the values that are based on the outdated buffer time.
  AllBuffer.Reset();
  LowBuffer.Reset();
  DiffBuffer.Reset();
  LowDist = 0.f;
}

bool UGMC_ReplicationCmp::FDynamicBufferTimeAux::WantsNewBufferTime() const
{
  return SignedDiff != 0.f;
}

bool UGMC_ReplicationCmp::FDynamicBufferTimeAux::HasPendingBufferTime() const
{
  gmc_ck(RequestedBufferTime != 0.f ? RequestedBufferTime >= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER : true)
  gmc_ck(RequestedBufferTime != 0.f ? RequestedBufferTime <= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER : true)
  return RequestedBufferTime != 0.f;
}

bool UGMC_ReplicationCmp::FDynamicBufferTimeAux::CanRequest(UGMC_ReplicationCmp* const Outer, AGMC_PlayerController* const LocalController)
{
  if (LocalController->bCullNonPlayerServerPawnParams && !Outer->IsPlayerControlledPawn())
  {
    const auto& RefNonPlayerPawn = LocalController->GetRefNonPlayerPawn();

    if (!RefNonPlayerPawn)
    {
      // We have no reference pawn yet, wait for an update from the server.

      GMC_LOG(
        LogGMCReplication,
        Outer->GetPawnOwner(),
        Verbose,
        TEXT("No non-player reference pawn is set, cannot request buffer time.")
      )

      return false;
    }

    if (Outer->GetPawnOwner() != RefNonPlayerPawn)
    {
      // The client can only request buffer times for the reference pawn.
      RequestTimer = 0.;
      RequestedBufferTime = 0.f;
      return false;
    }
  }

  return RequestTimer >= REQUEST_RETRY_INTERVAL;
}

float UGMC_ReplicationCmp::FDynamicBufferTimeAux::GetRequestHoldTime(UGMC_ReplicationCmp* const Outer) const
{
  return Outer->GetElapsedTimeSinceSpawn() > START_UP_TIME ? REQUEST_HOLD_TIME : START_UP_REQUEST_HOLD_TIME;
}
