// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCOrganicMovementComponent.h"
#include "GMCPawn.h"
#include "GMCFlatCapsuleComponent.h"
#include "GMCRollbackActor.h"
#include "GMCAggregator.h"
#include "Compression.h"
#include "GMCLog.h"
#include "GMCOrganicMovementComponent_DBG.h"

DECLARE_CYCLE_STAT(TEXT("GenPredictionTick"), STAT_GenPredictionTick, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("GenSimulationTick"), STAT_GenSimulationTick, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PreLocalMoveExecution"), STAT_PreLocalMoveExecution, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("SV_PreRemoteMoveExecution"), STAT_SV_PreRemoteMoveExecution, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PostLocalMoveExecution"), STAT_PostLocalMoveExecution, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("SV_PostRemoteMoveExecution"), STAT_SV_PostRemoteMoveExecution, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("CL_PreReplay"), STAT_CL_PreReplay, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("CL_PostReplay"), STAT_CL_PostReplay, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MontageUpdate"), STAT_MontageUpdate, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("AdvanceActiveMontage"), STAT_AdvanceActiveMontage, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PreProcessRootMotion"), STAT_PreProcessRootMotion, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("CallMontageEvents"), STAT_CallMontageEvents, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PlayMontageSimulated"), STAT_PlayMontageSimulated, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("GetRootMotionExtractionMetaData"), STAT_GetRootMotionExtractionMetaData, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("GetRootMotionVelocityMetaData"), STAT_GetRootMotionVelocityMetaData, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("GetSimulationMetaData"), STAT_GetSimulationMetaData, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("CalculateAnimRootMotionVelocity"), STAT_CalculateAnimRootMotionVelocity, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("ApplyAnimRootMotionRotation"), STAT_ApplyAnimRootMotionRotation, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("OnSyncDataApplied"), STAT_OnSyncDataApplied, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("OnMovementModeChanged"), STAT_OnMovementModeChanged, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PerformMovement"), STAT_PerformMovement, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateMovementMode"), STAT_UpdateMovementMode, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PreMovementUpdate"), STAT_PreMovementUpdate, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateBasedMovementVelocity"), STAT_UpdateBasedMovementVelocity, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateBasedMovementRelative"), STAT_UpdateBasedMovementRelative, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateBasedMovementNone"), STAT_UpdateBasedMovementNone, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("FindActorBase"), STAT_FindActorBase, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("TraceForActorBase"), STAT_TraceForActorBase, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("RunPhysics"), STAT_RunPhysics, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PhysicsGrounded"), STAT_PhysicsGrounded, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PhysicsAirborne"), STAT_PhysicsAirborne, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PhysicsBuoyant"), STAT_PhysicsBuoyant, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PhysicsCustom"), STAT_PhysicsCustom, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MoveAlongFloor"), STAT_MoveAlongFloor, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MoveAlongNavMesh"), STAT_MoveAlongNavMesh, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MoveThroughAir"), STAT_MoveThroughAir, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MoveThroughFluid"), STAT_MoveThroughFluid, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("CalculateVelocity"), STAT_CalculateVelocity, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("SlideAlongSurface"), STAT_SlideAlongSurface, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("TwoWallAdjust"), STAT_TwoWallAdjust, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("HandleImpact"), STAT_HandleImpact, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("StepUp"), STAT_StepUp, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MaintainDistanceToFloor"), STAT_MaintainDistanceToFloor, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MoveWithBaseVelocity"), STAT_MoveWithBaseVelocity, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MoveWithBaseRelative"), STAT_MoveWithBaseRelative, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("ProcessLanded"), STAT_ProcessLanded, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("Swim"), STAT_Swim, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("FindWaterLine"), STAT_FindWaterLine, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("CheckLeaveFluid"), STAT_CheckLeaveFluid, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MovementUpdate"), STAT_MovementUpdate, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("MovementUpdateSimulated"), STAT_MovementUpdateSimulated, STATGROUP_UGMC_OrganicMovementCmp)
DECLARE_CYCLE_STAT(TEXT("PhysicsInteraction"), STAT_PhysicsInteraction, STATGROUP_UGMC_OrganicMovementCmp)

namespace GMCCVars
{
#if ALLOW_CONSOLE && !NO_LOGGING

  int32 StatOrganicMovementValues = 0;
  FAutoConsoleVariableRef CVarStatOrganicMovementValues(
    TEXT("gmc.StatOrganicMovementValues"),
    StatOrganicMovementValues,
    TEXT("Display realtime motion values from the organic movement component. 0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogOrganicMovementValues = 0;
  FAutoConsoleVariableRef CVarLogOrganicMovementValues(
    TEXT("gmc.LogOrganicMovementValues"),
    LogOrganicMovementValues,
    TEXT("Log realtime motion values from the organic movement component. 0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 ShowFloorSweep = 0;
  FAutoConsoleVariableRef CVarShowFloorSweep(
    TEXT("gmc.ShowFloorSweep"),
    ShowFloorSweep,
    TEXT("Visualize the result of the floor sweep. 0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 VisualizeBaseEqualization = 0;
  FAutoConsoleVariableRef CVarVisualizeBaseEqualization(
    TEXT("gmc.VisualizeBaseEqualization"),
    VisualizeBaseEqualization,
    TEXT("Visualize the current actor base as equalized when applicable. 0: Disable, 1: Enable"),
    ECVF_Default
  );

#endif
}

UGMC_OrganicMovementCmp::UGMC_OrganicMovementCmp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  ReplicationSettings.DefaultPredictionSettings.AngularVelocity = EGMC_PredictionMode::Local;
  ReplicationSettings.DefaultSimulationSettings.AngularVelocity = EGMC_SimulationMode::None;
  ReplicationSettings.DefaultPredictionSettings.ActorScale = EGMC_PredictionMode::Local;
  ReplicationSettings.DefaultSimulationSettings.ActorScale = EGMC_SimulationMode::None;

  SetWalkableFloorAngle(WalkableFloorAngle);

  AvoidanceGroup.bGroup0 = true;
  GroupsToAvoid.Packed = 0xFFFF'FFFF;
  GroupsToIgnore.Packed = 0;

  bUseAccelerationForPaths = true;

  GetNavAgentPropertiesRef().bCanCrouch = false;
  GetNavAgentPropertiesRef().bCanJump = false;
  GetNavAgentPropertiesRef().bCanWalk = true;
  GetNavAgentPropertiesRef().bCanSwim = false;
  GetNavAgentPropertiesRef().bCanFly = false;
}

#if WITH_EDITOR

void UGMC_OrganicMovementCmp::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UGMC_OrganicMovementCmp, WalkableFloorAngle))
  {
    SetWalkableFloorAngle(WalkableFloorAngle);
  }
}

#endif

void UGMC_OrganicMovementCmp::BindReplicationData_Implementation()
{
  Super::BindReplicationData_Implementation();

  BI_RawInputVector = BindCompressedVector(
    RawInputVector,
    EGMC_PredictionMode::ClientAuth_Input,
    EGMC_CombineMode::CombineIfUnchanged,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::Linear
  );

  BI_CurrentRootCollisionShape = BindHalfByte(
    CurrentRootCollisionShape,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_CurrentRootCollisionExtent = BindCompressedVector(
    CurrentRootCollisionExtent,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::Linear
  );

  BI_MovementMode = BindHalfByte(
    MovementMode,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_ReceivedUpwardForce = BindBool(
    bReceivedUpwardForce,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::None,
    {}
  );

  BI_HasRootMotion = BindBool(
    bHasRootMotion,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::None,
    {}
  );

  BI_MontageTracker_Montage = BindAnimMontageReference(
    MontageTracker.Montage,
    MontageReplication.MontagePrediction.MontageReferencePredictionMode,
    MontageReplication.MontagePrediction.MontageReferenceCombineMode,
    MontageReplication.MontageSimulation.bReplicateMontageReference ? EGMC_SimulationMode::PeriodicAndOnChange_Output : EGMC_SimulationMode::None,
    MontageReplication.MontageSimulation.MontageReferenceInterpolation
  );

  BI_MontageTracker_MontagePosition = BindSinglePrecisionFloat(
    MontageTracker.MontagePosition,
    MontageReplication.MontagePrediction.MontagePositionPredictionMode,
    MontageReplication.MontagePrediction.MontagePositionCombineMode,
    MontageReplication.MontageSimulation.bReplicateMontagePosition ? EGMC_SimulationMode::Periodic_Output : EGMC_SimulationMode::None,
    MontageReplication.MontageSimulation.MontagePositionInterpolation
  );

  BI_MontageTracker_MontagePlayRate = BindSinglePrecisionFloat(
    MontageTracker.MontagePlayRate,
    MontageReplication.MontagePrediction.MontagePlayRatePredictionMode,
    MontageReplication.MontagePrediction.MontagePlayRateCombineMode,
    MontageReplication.MontageSimulation.bReplicateMontagePlayRate ? EGMC_SimulationMode::Periodic_Output : EGMC_SimulationMode::None,
    MontageReplication.MontageSimulation.MontagePlayRateInterpolation
  );

  BI_MontageTracker_MontagePaused = BindBool(
    MontageTracker.bMontagePaused,
    MontageReplication.MontagePrediction.MontagePausedPredictionMode,
    MontageReplication.MontagePrediction.MontagePausedCombineMode,
    MontageReplication.MontageSimulation.bReplicateMontagePaused ? EGMC_SimulationMode::PeriodicAndOnChange_Output : EGMC_SimulationMode::None,
    MontageReplication.MontageSimulation.MontagePausedInterpolation
  );

  BI_RelBasedMovementAux_ActorBaseRotation = BindCompressedRotator(
    RelBasedMovementAux.ActorBaseRotation,
    BasedMovement.Relative.bSendActorBaseRotation ? EGMC_PredictionMode::ClientAuth_Input : EGMC_PredictionMode::Local,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::None,
    EGMC_InterpolationFunction{}
  );
}

void UGMC_OrganicMovementCmp::WorldTickStart_Implementation(float DeltaTime)
{
  Super::WorldTickStart_Implementation(DeltaTime);

  if (DebugShouldVisualizeBaseEqualization() && RelBasedMovementAux.bDidEqualizeBase)
  {
    RelBasedMovementAux.EqualizeBase(false, true, this);
  }
}

void UGMC_OrganicMovementCmp::WorldTickEnd_Implementation(float DeltaTime)
{
  Super::WorldTickEnd_Implementation(DeltaTime);

  gmc_ck(BasedMovement.IsRelative() ? !RelBasedMovementAux.bDidEqualizeBase : true)
  if (DebugShouldVisualizeBaseEqualization() && !RelBasedMovementAux.bDidEqualizeBase)
  {
    RelBasedMovementAux.EqualizeBase(true, true, this);
  }
}

bool UGMC_OrganicMovementCmp::CL_ShouldUseSmoothCorrections() const
{
  // Smooth corrections must be disabled when using the debug visualization.
  return !DebugShouldVisualizeBaseEqualization() && Super::CL_ShouldUseSmoothCorrections();
}

bool UGMC_OrganicMovementCmp::DebugShouldVisualizeBaseEqualization() const
{
#if ALLOW_CONSOLE && !NO_LOGGING
  return GMCCVars::VisualizeBaseEqualization != 0 && BasedMovement.IsRelative() && BasedMovement.GetEqualizeBaseActor();
#else
  return false;
#endif
}

void UGMC_OrganicMovementCmp::ClearTransientData(bool bResetMoves)
{
  Super::ClearTransientData(bResetMoves);

  LandingVelocity = FVector::ZeroVector;
  VelocityBeforeMovementUpdate = FVector::ZeroVector;
  MovementMode = static_cast<uint8>(EGMC_MovementMode::Grounded);
  CurrentFloor.Reset();
  bReceivedUpwardForce = false;
  RelBasedMovementAux.Reset();
  RawInputVector = FVector::ZeroVector;
  ProcessedInputVector = FVector::ZeroVector;
  CurrentImmersionDepth = 0.f;
  LedgeFallOffDirection = FVector::ZeroVector;
  RootMotionParams.Clear();
  MontageTracker.ClearActiveMontage();
  bHasRootMotion = false;
  PreviousMontageSimulated = nullptr;
  BlockMontageTimerSimulated = 0.f;
  PreviousMovementModeSimulated = EGMC_MovementMode::None;
  PreviousControlRotationSimulated = FRotator::ZeroRotator;
  RequestedVelocity = FVector::ZeroVector;
  bRequestedMoveWithMaxSpeed = false;
  bIsUsingAvoidanceInternal = false;
  ResetLastValidTargetNavLocation();
  ResetAvoidanceData();
}

void UGMC_OrganicMovementCmp::SetupPlayerInputComponent_Implementation(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent_Implementation(PlayerInputComponent);

  gmc_ck(PlayerInputComponent)

  if (const auto& EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
  {
    EnhancedInputComponent->BindAction(DirectionalInput, ETriggerEvent::Triggered, this, &UGMC_OrganicMovementCmp::ApplyDirectionalInput);
    EnhancedInputComponent->BindAction(ViewInput, ETriggerEvent::Triggered, this, &UGMC_OrganicMovementCmp::ApplyViewInput);
  }
}

void UGMC_OrganicMovementCmp::ApplyDirectionalInput(const FInputActionInstance& InputAction)
{
  AddInputVector(InputAction.GetValue().Get<FVector>());
}

void UGMC_OrganicMovementCmp::ApplyViewInput(const FInputActionInstance& InputAction)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  const FVector& Value = InputAction.GetValue().Get<FVector>();
  PawnOwner->AddControllerRollInput(Value.Z);
  PawnOwner->AddControllerPitchInput(-Value.Y);
  PawnOwner->AddControllerYawInput(Value.X);
}

bool UGMC_OrganicMovementCmp::SV_OnProxyMoveInitialized_Implementation(FGMC_Move& Move, float DeltaTime, double Timestamp)
{
  bool ReturnValue = Super::SV_OnProxyMoveInitialized_Implementation(Move, DeltaTime, Timestamp);

  const int32 LastIdx = MoveHistory.Num() - 1;
  if (LastIdx >= 0)
  {
    const FVector PreviousRawInputVector = GetBoundCompressedVector(BI_RawInputVector, MoveHistory[LastIdx].InputState);
    SetBoundCompressedVector(PreviousRawInputVector, BI_RawInputVector, Move.InputState);
  }

  return ReturnValue;
}

bool UGMC_OrganicMovementCmp::OnCumulativeMoveInitialized_Implementation(
  FGMC_PawnState& InputState,
  EGMC_InterpolationStates SimStates,
  float DeltaTime,
  double Timestamp
)
{
  gmc_ck(IsSimulating())

  bool ReturnValue = Super::OnCumulativeMoveInitialized_Implementation(InputState, SimStates, DeltaTime, Timestamp);

  const int32 LastIdx = MoveHistory.Num() - 1;
  gmc_ck(LastIdx >= 0)

  const auto& SourceState = SimStates == EGMC_InterpolationStates::Input ? MoveHistory[LastIdx].InputState : MoveHistory[LastIdx].OutputState;

  const FVector LastRawInputVector = GetBoundCompressedVector(BI_RawInputVector, SourceState);
  SetBoundCompressedVector(LastRawInputVector, BI_RawInputVector, InputState);

  const bool bUseRelative = BasedMovement.IsRelative() || BasedMovement.IsForcedRelativeSimulation();
  const auto& Base = GetActorBase();

  const auto& SourceControlRotation = bUseRelative && Base ?
    GetWorldControlRotationFor(SourceState.ControlRotation.Read(), Base->GetComponentTransform()) : SourceState.ControlRotation.Read();
  FRotator SmoothControlRotation = SourceControlRotation;
  SmoothControlRotation.Roll = 0.;

  constexpr double InterpThreshold = 1.;
  if (!SourceControlRotation.Equals(PreviousControlRotationSimulated, InterpThreshold))
  {
    SmoothControlRotation = InterpolateValue(
      static_cast<uint8>(EGMC_InterpolationFunction::Linear),
      FGMC_NumericValue{EGMC_NumericType::Rotator, PreviousControlRotationSimulated},
      FGMC_NumericValue{EGMC_NumericType::Rotator, SourceControlRotation},
      FGMC_NumericValue{EGMC_NumericType::Rotator, PreviousControlRotationSimulated},
      FGMC_NumericValue{EGMC_NumericType::Rotator, SourceControlRotation},
      0.5
    ).GetRotator();

    SmoothControlRotation.Roll = 0.;

    if (FMath::Abs(FRotator::NormalizeAxis(SmoothControlRotation.Pitch - SourceControlRotation.Pitch)) < InterpThreshold)
    {
      SmoothControlRotation.Pitch = SourceControlRotation.Pitch;
    }

    if (FMath::Abs(FRotator::NormalizeAxis(SmoothControlRotation.Yaw - SourceControlRotation.Yaw)) < InterpThreshold)
    {
      SmoothControlRotation.Yaw = SourceControlRotation.Yaw;
    }
  }

  InputState.ControlRotation.Write(
    bUseRelative && Base ? GetBasedControlRotationFor(SmoothControlRotation, Base->GetComponentTransform()) : SmoothControlRotation
  );

  if (HasReachedMaxExtrapolationDistance())
  {
    // Zero the velocity when the max extrapolation distance was reached to stop movement and allow for a possible return towards the initial location.
    InputState.LinearVelocity.Write(FVector::ZeroVector);
    InputState.AngularVelocity.Write(FVector::ZeroVector);
  }

  if (bUseRelative)
  {
    RelBasedMovementAux.OnCumulativeMoveInitialized(InputState, this);
  }

  return ReturnValue;
}

bool UGMC_OrganicMovementCmp::ShouldRollBackGMCPawn_Implementation(const AGMC_Pawn* PawnToTest) const
{
  if (!Super::ShouldRollBackGMCPawn_Implementation(PawnToTest))
  {
    return false;
  }

  gmc_ck(IsValid(PawnToTest))

  if (BasedMovement.IsRelative())
  {
    const auto& Base = GetActorBase();
    if (Base)
    {
      const auto& BaseOwner = Base->GetOwner();
      if (BaseOwner == PawnToTest)
      {
        // Do not roll back the considered pawn if it is currently the base actor for this pawn (only for relative based movement).
        return false;
      }
    }

    const auto& ReplicationComponentOther = PawnToTest->GetReplicationComponent();
    gmc_ck(ReplicationComponentOther)

    const auto& BaseOther = ReplicationComponentOther->GetActorBase();
    if (BaseOther)
    {
      const auto& BaseOwnerOther = BaseOther->GetOwner();
      if (BaseOwnerOther == PawnOwner)
      {
        // Do not roll back the considered pawn if it is currently based on this pawn (only for relative based movement).
        return false;
      }
    }
  }

  return true;
}

bool UGMC_OrganicMovementCmp::ShouldRollBackGMCActor_Implementation(const class AGMC_RollbackActor* ActorToTest) const
{
  if (!Super::ShouldRollBackGMCActor_Implementation(ActorToTest))
  {
    return false;
  }

  if (BasedMovement.IsRelative())
  {
    return false;
  }

  return true;
}

bool UGMC_OrganicMovementCmp::ShouldTickGMCActor_Implementation(const AGMC_RollbackActor* RollbackActor) const
{
  if (!Super::ShouldTickGMCActor_Implementation(RollbackActor))
  {
    return false;
  }

  if (BasedMovement.IsRelative())
  {
    return false;
  }

  return true;
}

void UGMC_OrganicMovementCmp::OnServerAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings)
{
  Super::OnServerAuthPhysicsSimulationToggled_Implementation(bEnabled, Settings);

  // Movement simulation happens only the server while physics are active.
  if (!IsServerPawn())
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!RootComponent)
  {
    return;
  }

  if (bEnabled)
  {
    SetMovementMode(EGMC_MovementMode::Grounded);
    if (const auto& BodyInstance = RootComponent->GetBodyInstance())
    {
      // Set the physics velocity to the current kinematic velocity.
      BodyInstance->SetLinearVelocity(Velocity, false);
    }
  }
  else
  {
    // Reset any actor rotation around the roll and pitch axis since these are not supported by the movement implementation.
    SetRootCollisionRotation({0., GetRootCollisionRotation().Yaw, 0.});

    // Reset any potential roll rotation in the controller view.
    const FRotator& ControlRotation = GetControllerRotation_GMC();
    SetControllerRotation_GMC({ControlRotation.Pitch, ControlRotation.Yaw, 0.});

    SetMovementMode(EGMC_MovementMode::Airborne);
  }
}

void UGMC_OrganicMovementCmp::OnClientAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings)
{
  Super::OnClientAuthPhysicsSimulationToggled_Implementation(bEnabled, Settings);

  if (IsSimulatedProxy())
  {
    return;
  }

  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  if (!RootComponent)
  {
    return;
  }

  if (bEnabled)
  {
    SetMovementMode(EGMC_MovementMode::Grounded);
    if (const auto& BodyInstance = RootComponent->GetBodyInstance())
    {
      // Set the physics velocity to the current kinematic velocity.
      BodyInstance->SetLinearVelocity(Velocity, false);
    }
  }
  else
  {
    // Reset any actor rotation around the roll and pitch axis since these are not supported by the movement implementation.
    SetRootCollisionRotation({0., GetRootCollisionRotation().Yaw, 0.});

    // Reset any potential roll rotation in the controller view.
    const FRotator& ControlRotation = GetControllerRotation_GMC();
    SetControllerRotation_GMC({ControlRotation.Pitch, ControlRotation.Yaw, 0.});

    SetMovementMode(EGMC_MovementMode::Airborne);
  }
}

void UGMC_OrganicMovementCmp::OnClientPredictionEnabled_Implementation()
{
  Super::OnClientPredictionEnabled_Implementation();

  HaltMovement();
}

void UGMC_OrganicMovementCmp::OnClientPredictionDisabled_Implementation()
{
  Super::OnClientPredictionDisabled_Implementation();

  HaltMovement();
}

void UGMC_OrganicMovementCmp::OnGMCEnabled_Implementation()
{
  Super::OnGMCEnabled_Implementation();

  // Clears any pending input vector that was accumulated while the GMC was disabled.
  ConsumeInputVector();
}

void UGMC_OrganicMovementCmp::OnGMCDisabled_Implementation()
{
  Super::OnGMCDisabled_Implementation();

  HaltMovement();
  // Set the movement mode to grounded as default (we don't want to call OnMovementModeChanged here).
  MovementMode = static_cast<uint8>(EGMC_MovementMode::Grounded);
}

void UGMC_OrganicMovementCmp::BeginPlay()
{
  if (!GetGMCPawnOwner())
  {
    Super::BeginPlay();
    return;
  }

  // Preliminary assignment of the skeletal mesh reference before Super::BeginPlay, so it is available during initialization.
  SkeletalMesh = Cast<USkeletalMeshComponent>(PawnOwner->FindComponentByClass(USkeletalMeshComponent::StaticClass()));

  const EGMC_CollisionShape CollisionShape = Super::GetRootCollisionShape();
  CurrentRootCollisionShape = static_cast<uint8>(CollisionShape);
  Super::SetRootCollisionExtent(
    CurrentRootCollisionExtent = GetValidExtent(CollisionShape, Super::GetRootCollisionExtent(false)),
    false,
    true
  );
  gmc_ck(CurrentRootCollisionExtent == Super::GetRootCollisionExtent(false))

  DefaultSpeed = MaxDesiredSpeed;

  // Set the start movement mode directly (preventing a call to OnMovementModeChanged).
  MovementMode = static_cast<uint8>(EGMC_MovementMode::Grounded);

  SetBasedMovementSource(BasedMovement.Source);

  Super::BeginPlay();

  // Reinitialize the mesh reference after Super::BeginPlay, so GMCAggregator is assigned if present and can be used to register the mesh (and in case the
  // component hierarchy was changed).
  SkeletalMesh = nullptr;
  const auto& Mesh = Cast<USkeletalMeshComponent>(PawnOwner->FindComponentByClass(USkeletalMeshComponent::StaticClass()));
  if (IsValid(Mesh))
  {
    SetSkeletalMeshReference(Mesh);
    gmc_ck(SkeletalMesh == Mesh)

    if (bSmoothMesh)
    {
      SetComponentToSmooth(SkeletalMesh);
    }
  }
}

void UGMC_OrganicMovementCmp::PreLocalMoveExecution_Implementation(const FGMC_Move& LocalMove)
{
  SCOPE_CYCLE_COUNTER(STAT_PreLocalMoveExecution)

  Super::PreLocalMoveExecution_Implementation(LocalMove);

  RawInputVector = ConsumeInputVector();

  if (
    (BasedMovement.IsRelative() || (IsSimulating() && BasedMovement.IsForcedRelativeSimulation())) &&
    IsValid(UpdatedComponent) && 
    !UpdatedComponent->IsSimulatingPhysics()
  )
  {
    RelBasedMovementAux.PreMoveExecution(true, this, LocalMove.MetaData.DeltaTime);
  }
}

void UGMC_OrganicMovementCmp::SV_PreRemoteMoveExecution_Implementation(const FGMC_Move& RemoteMove)
{
  SCOPE_CYCLE_COUNTER(STAT_SV_PreRemoteMoveExecution)

  Super::SV_PreRemoteMoveExecution_Implementation(RemoteMove);

  if (
    (BasedMovement.IsRelative() || (IsSimulating() && BasedMovement.IsForcedRelativeSimulation())) &&
    IsValid(UpdatedComponent) && 
    !UpdatedComponent->IsSimulatingPhysics()
  )
  {
    RelBasedMovementAux.PreMoveExecution(false, this, RemoteMove.MetaData.DeltaTime);
  }
}

void UGMC_OrganicMovementCmp::PostLocalMoveExecution_Implementation(const FGMC_Move& ExecutedMove)
{
  SCOPE_CYCLE_COUNTER(STAT_PostLocalMoveExecution)

  if (
    (BasedMovement.IsRelative() || (IsSimulating() && BasedMovement.IsForcedRelativeSimulation())) &&
    IsValid(UpdatedComponent) && 
    !UpdatedComponent->IsSimulatingPhysics()
  )
  {
    RelBasedMovementAux.PostMoveExecution(true, this);
  }

  // Swapping of the no-prediction buffer must only happen after the relative based movement logic has finished.
  Super::PostLocalMoveExecution_Implementation(ExecutedMove);
}

void UGMC_OrganicMovementCmp::SV_PostRemoteMoveExecution_Implementation(const FGMC_Move& RemoteMove)
{
  SCOPE_CYCLE_COUNTER(STAT_SV_PostRemoteMoveExecution)

  Super::SV_PostRemoteMoveExecution_Implementation(RemoteMove);

  if (
    (BasedMovement.IsRelative() || (IsSimulating() && BasedMovement.IsForcedRelativeSimulation())) &&
    IsValid(UpdatedComponent) && 
    !UpdatedComponent->IsSimulatingPhysics()
  )
  {
    RelBasedMovementAux.PostMoveExecution(false, this);
  }
}

void UGMC_OrganicMovementCmp::CL_PreReplayMoveExecution_Implementation(const FGMC_Move& ReplayMove)
{
  Super::CL_PreReplayMoveExecution_Implementation(ReplayMove);

  if (BasedMovement.IsRelative())
  {
    RelBasedMovementAux.CL_PreReplayMoveExecution(ReplayMove, this);
  }
}

void UGMC_OrganicMovementCmp::CL_PreAdoptStateForReplay(const FGMC_Move& APMove, const FGMC_Move& SourceMove)
{
  Super::CL_PreAdoptStateForReplay(APMove, SourceMove);

  // Save the montage tracker values to pass to CL_CheckMontageStatusAfterReplay.
  CL_SavedPreReplayMontageTracker = MontageTracker;
}

void UGMC_OrganicMovementCmp::CL_PreReplay_Implementation()
{
  SCOPE_CYCLE_COUNTER(STAT_CL_PreReplay)

  Super::CL_PreReplay_Implementation();

  // Always update the floor when starting a replay.
  CurrentFloor.MarkDirty(true);

  if (BasedMovement.IsRelative())
  {
    RelBasedMovementAux.CL_PreReplay(this);
  }
}

void UGMC_OrganicMovementCmp::CL_PostReplay_Implementation()
{
  SCOPE_CYCLE_COUNTER(STAT_CL_PostReplay)

  CL_CheckMontageStatusAfterReplay(CL_SavedPreReplayMontageTracker);
  CL_SavedPreReplayMontageTracker.ClearActiveMontage();

  if (BasedMovement.IsRelative())
  {
    RelBasedMovementAux.CL_PostReplay(this);
  }

  // It is important that the parent function is only called after FGMC_RelBasedMovementAux::CL_PostReplay has run.
  Super::CL_PostReplay_Implementation();
}

void UGMC_OrganicMovementCmp::GenPredictionTick_Implementation(float DeltaTime)
{
  SCOPE_CYCLE_COUNTER(STAT_GenPredictionTick)

  Super::GenPredictionTick_Implementation(DeltaTime);

  if (ShouldSkipUpdate(DeltaTime))
  {
    FLog(VeryVerbose, "Movement update was skipped.")
    return;
  }

  if (UpdatedComponent->IsSimulatingPhysics())
  {
    FLog(VeryVerbose, "Pawn is simulating physics, no kinematic movement will be applied.")
    EnableSkeletalMeshPoseTick();
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PhysicsSimulationUpdate, DeltaTime);
    return;
  }

  if (IsUsingServerAuthPhysicsReplication() || IsUsingClientAuthPhysicsReplication())
  {
    // No update for simulated pawns that are just displaying the physics of the owner.
    EnableSkeletalMeshPoseTick();
    return;
  }

  if (IsServerBot())
  {
    CheckAvoidance();
  }

  if (ShouldComputeAvoidance())
  {
    AvoidanceLockTimer = FMath::Clamp(AvoidanceLockTimer - DeltaTime, 0.f, UE_BIG_NUMBER);
  }

  PerformMovement(DeltaTime);

  if (bEnablePhysicsInteraction)
  {
    SCOPE_CYCLE_COUNTER(STAT_PhysicsInteraction)
    ApplyDownwardForce(DeltaTime);
    ApplyRepulsionForce(DeltaTime);
  }

  if (ShouldComputeAvoidance())
  {
    UpdateAvoidance();
  }

  DEBUG_SHOW_FLOOR_SWEEP(CurrentFloor, FVector::DownVector, FloorTraceLength)
  DEBUG_STAT_AND_LOG_ORGANIC_MOVEMENT_VALUES
  DEBUG_LOG_NAN_DIAGNOSTIC
}

void UGMC_OrganicMovementCmp::GenSimulationTick_Implementation(float DeltaTime)
{
  SCOPE_CYCLE_COUNTER(STAT_GenSimulationTick)

  Super::GenSimulationTick_Implementation(DeltaTime);

  if (!CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CanMove))
  {
    BlockSkeletalMeshPoseTick();
    return;
  }

  if (MontageReplication.MontageSimulation.bAutoSimulateMontages)
  {
    FGMC_SimulationSettings SimulationSettings = GetSimulationMetaData(MontageTracker.Montage);
    PlayMontageSimulated(
      SkeletalMesh,
      MontageTracker.Montage,
      DeltaTime,
      MontageTracker.MontagePosition,
      MontageTracker.MontagePlayRate,
      NAME_None,
      MontageTracker.bMontagePaused,
      SimulationSettings.bCancelSelfOnBlendIn,
      SimulationSettings.bCancelSelfOnBlendOut,
      SimulationSettings.CancelMinPositionDiff,
      SimulationSettings.MinRemainingRatio,
      SimulationSettings.CompletionTimeout,
      SimulationSettings.CompletionTolerance,
      SimulationSettings.StopBlendTime,
      true
    );
  }

  {
    SCOPE_CYCLE_COUNTER(STAT_MovementUpdateSimulated)

    // Preferred entry point for subclasses to implement their own logic for simulated pawns.
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, MovementUpdateSimulated, DeltaTime);
  }

  if (PreviousMovementModeSimulated != GetMovementMode())
  {
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnMovementModeChangedSimulated, PreviousMovementModeSimulated);
    if (PreviousMovementModeSimulated == EGMC_MovementMode::Airborne && GetMovementMode() == EGMC_MovementMode::Grounded)
    {
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnLandedSimulated);
    }
  }

  PreviousMovementModeSimulated = GetMovementMode();
  PreviousControlRotationSimulated = GetControllerRotation_GMC();

  const auto& CurrentBase = GetActorBase();
  const auto& SavedBase = RelBasedMovementAux.SavedTransforms.GetSavedBase(true);
  if (CurrentBase != SavedBase)
  {
    HandleActorBaseChange(CurrentBase, SavedBase, true);
  }

  // Always save these for simplicity's sake (regardless of the current interpolation/extrapolation method or based movement source).
  RelBasedMovementAux.SavedTransforms.Save(true, this);
}

void UGMC_OrganicMovementCmp::PreSmoothing_Implementation(int32 TargetIndex, bool bExtrapolating, bool bRollback, EGMC_InterpolationStates InterpStates)
{
  Super::PreSmoothing_Implementation(TargetIndex, bExtrapolating, bRollback, InterpStates);

  if (bRollback || bExtrapolating)
  {
    return;
  }

  if (BasedMovement.IsRelative() || BasedMovement.IsForcedRelativeSimulation())
  {
    // This must be checked before the actual smoothing happens, otherwise the pawn would already be set to the relative location with the new base.
    SmoothActorBaseChangeSimulated(TargetIndex, bExtrapolating, InterpStates);
  }
}

void UGMC_OrganicMovementCmp::SaveMatchLatestSimulationStartState(FGMC_PawnState& InOutSimulationStartState)
{
  Super::SaveMatchLatestSimulationStartState(InOutSimulationStartState);

  if (!bUseRelativeValuesForSimulation)
  {
    return;
  }

  gmc_ck(BasedMovement.IsRelative() || BasedMovement.IsForcedRelativeSimulation())

  const auto& Base = InOutSimulationStartState.ActorBase.Read();
  const auto& PreviousBase = RelBasedMovementAux.SavedTransforms.GetSavedBase(true);
  gmc_ck(Base == GetActorBase())

  if (Base && Base == PreviousBase)
  {
    // The simulation start state must be relative to the transform of the base before it was moved this tick. However, the pawn itself is still required to
    // tick after the base, so we have to refer to the saved relative pawn state from the last tick.
    InOutSimulationStartState.LinearVelocity.Write(RelBasedMovementAux.SavedTransforms.GetSavedRelativeLinearVelocity(true));
    InOutSimulationStartState.ActorLocation.Write(RelBasedMovementAux.SavedTransforms.GetSavedRelativeActorLocation(true));
    InOutSimulationStartState.ActorRotation.Write(RelBasedMovementAux.SavedTransforms.GetSavedRelativeActorRotation(true));
    InOutSimulationStartState.ControlRotation.Write(RelBasedMovementAux.SavedTransforms.GetSavedRelativeControlRotation(true));
  }
}

void UGMC_OrganicMovementCmp::SaveSmoothComponentSimulationStartTransform(USceneComponent* SmoothComponent, FTransform& OutAbsoluteStartTransform)
{
  Super::SaveSmoothComponentSimulationStartTransform(SmoothComponent, OutAbsoluteStartTransform);

  if (!IsValid(SmoothComponent) || !bUseRelativeValuesForSimulation)
  {
    return;
  }

  gmc_ck(SmoothComponent == PostSmoothingParams.Component)
  gmc_ck(BasedMovement.IsRelative() || BasedMovement.IsForcedRelativeSimulation())

  const auto& Base = GetActorBase();
  const auto& PreviousBase = RelBasedMovementAux.SavedTransforms.GetSavedBase(true);

  if (Base && Base == PreviousBase)
  {
    const auto& RelativeComponentLocation = RelBasedMovementAux.SavedTransforms.GetSavedRelativeSmoothComponentLocation(true);
    const auto& RelativeComponentRotation = RelBasedMovementAux.SavedTransforms.GetSavedRelativeSmoothComponentRotation(true);
    const auto& BaseLocation = Base->GetComponentLocation();
    const auto& RotationDelta = Base->GetComponentRotation();

    const FVector& AbsoluteComponentLocation = BaseLocation + RotationDelta.RotateVector(RelativeComponentLocation);
    OutAbsoluteStartTransform.SetLocation(AbsoluteComponentLocation);

    const FRotator& AbsoluteComponentRotation = FRotator{0., RotationDelta.Yaw, 0.} + RelativeComponentRotation;
    OutAbsoluteStartTransform.SetRotation(AbsoluteComponentRotation.Quaternion());
  }
}

void UGMC_OrganicMovementCmp::OnSyncDataApplied_Implementation(const FGMC_PawnState& State, EGMC_NetContext Context)
{
  SCOPE_CYCLE_COUNTER(STAT_OnSyncDataApplied)

  Super::OnSyncDataApplied_Implementation(State, Context);

  MaintainRootCollisionCoherency();

  if (BasedMovement.IsRelative())
  {
    if (
      bUseClientPrediction &&
      SV_IsExecutingRemoteMoves() && (
        Context == EGMC_NetContext::RemoteServerPawn_PreMoveExecution ||
        Context == EGMC_NetContext::RemoteServerPawn_PostMoveExecution
      )
    )
    {
      RelBasedMovementAux.SV_ReconcileClientAuthValues(Context == EGMC_NetContext::RemoteServerPawn_PreMoveExecution, this);
    }
    else if (
      CL_IsReplaying() && (
        Context == EGMC_NetContext::LocalClientPawn_PreReplayMoveExecution ||
        Context == EGMC_NetContext::LocalClientPawn_PostReplayMoveExecution
      )
    )
    {
      gmc_ck(bUseClientPrediction)
      RelBasedMovementAux.CL_ReconcileClientAuthValues(Context == EGMC_NetContext::LocalClientPawn_PreReplayMoveExecution, this);
    }
  }
}

EGMC_CollisionShape UGMC_OrganicMovementCmp::GetRootCollisionShape() const
{
  gmc_ck(HasValidRootCollisionShape() && IsReadyForPlay() ? CurrentRootCollisionShape == static_cast<uint8>(Super::GetRootCollisionShape()) : true)
  return static_cast<EGMC_CollisionShape>(CurrentRootCollisionShape);
}

FVector UGMC_OrganicMovementCmp::GetRootCollisionExtent(bool bScaled) const
{
  gmc_ck(
    HasValidRootCollisionExtent() && IsReadyForPlay() ?
      GMCCompression::IsNetEqual(CurrentRootCollisionExtent, Super::GetRootCollisionExtent(false), EGMC_FloatPrecision::TwoDecimals) :
      true
  )
  return bScaled ? Super::GetRootCollisionExtent(true) : CurrentRootCollisionExtent;
}

USceneComponent* UGMC_OrganicMovementCmp::SetRootCollisionShape(
  EGMC_CollisionShape NewCollisionShape,
  const FVector& Extent,
  bool bScaled,
  FName Name
)
{
  if (!IsValid(PawnOwner))
  {
    gmc_ckne()
    return nullptr;
  }

  const auto& OriginalRootComponent = PawnOwner->GetRootComponent();
  gmc_ck(OriginalRootComponent)

  if (NewCollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return OriginalRootComponent;
  }

  if (!HasValidRootCollisionScale())
  {
    gmc_ckne()
    return OriginalRootComponent;
  }

  const FVector Scale = OriginalRootComponent->GetComponentScale();
  const FVector ValidExtent = GetValidExtent(NewCollisionShape, bScaled ? Extent / Scale : Extent);

  if (
    NewCollisionShape == static_cast<EGMC_CollisionShape>(Super::GetRootCollisionShape()) &&
    GMCCompression::IsNetEqual(ValidExtent, Super::GetRootCollisionExtent(false), EGMC_FloatPrecision::TwoDecimals)
  )
  {
    return OriginalRootComponent;
  }

  // Build a unique name with the appropriate prefix for the new object.
  FName DynamicName = [this](EGMC_CollisionShape CollisionShape) {
    switch (CollisionShape)
    {
      case EGMC_CollisionShape::VerticalCapsule:
        return MakeUniqueObjectName(PawnOwner, UCapsuleComponent::StaticClass(), ROOT_NAME_DYNAMIC_CAPSULE);
      case EGMC_CollisionShape::HorizontalCapsule:
        return MakeUniqueObjectName(PawnOwner, UGMC_FlatCapsuleCmp::StaticClass(), ROOT_NAME_DYNAMIC_FLAT_CAPSULE);
      case EGMC_CollisionShape::Box:
        return MakeUniqueObjectName(PawnOwner, UBoxComponent::StaticClass(), ROOT_NAME_DYNAMIC_BOX);
      case EGMC_CollisionShape::Sphere:
        return MakeUniqueObjectName(PawnOwner, USphereComponent::StaticClass(), ROOT_NAME_DYNAMIC_SPHERE);
      case EGMC_CollisionShape::Invalid:
      case EGMC_CollisionShape::MAX:
      default: gmc_ckne();
    }
    gmc_ckne()
    return FName{};
  }(NewCollisionShape);

  // The extent was already scaled previously if applicable.
  const auto& NewRootComponent = Super::SetRootCollisionShape(NewCollisionShape, ValidExtent, false, DynamicName);
  gmc_ck(NewCollisionShape == Super::GetRootCollisionShape())

  CurrentRootCollisionShape = static_cast<uint8>(NewCollisionShape);
  CurrentRootCollisionExtent = ValidExtent;
  gmc_ck(CurrentRootCollisionExtent.Equals(Super::GetRootCollisionExtent(false), 0.01))

  return NewRootComponent;
}

void UGMC_OrganicMovementCmp::SetRootCollisionExtent(const FVector& NewExtent, bool bScaled, bool bUpdateOverlaps)
{
  Super::SetRootCollisionExtent(NewExtent, bScaled, bUpdateOverlaps);
  // The extent could have been altered if it is not a valid extent vector or due to scaling so we query the actual root collision extent directly through the
  // parent function.
  CurrentRootCollisionExtent = Super::GetRootCollisionExtent(false);
}

void UGMC_OrganicMovementCmp::MaintainRootCollisionCoherency()
{
  // The parent versions of GetRootCollisionShape and GetRootCollisionExtent must be used here.
  const auto CurrentCollisionShape = static_cast<EGMC_CollisionShape>(CurrentRootCollisionShape);
  if (Super::GetRootCollisionShape() != CurrentCollisionShape)
  {
    SetRootCollisionShape(CurrentCollisionShape, CurrentRootCollisionExtent, false);
    return;
  }
  if (!GMCCompression::IsNetEqual(Super::GetRootCollisionExtent(false), CurrentRootCollisionExtent, EGMC_FloatPrecision::TwoDecimals))
  {
    SetRootCollisionExtent(CurrentRootCollisionExtent, false);
    return;
  }
}

void UGMC_OrganicMovementCmp::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
  if (!IsValid(NewUpdatedComponent))
  {
    Super::SetUpdatedComponent(NewUpdatedComponent);
    return;
  }

  const auto& NewGMCPawnOwner = Cast<AGMC_Pawn>(NewUpdatedComponent->GetOwner());
  if (!NewGMCPawnOwner)
  {
    Super::SetUpdatedComponent(NewUpdatedComponent);
    return;
  }

  if (
    !(
      Cast<UGMC_FlatCapsuleCmp>(NewUpdatedComponent) ||
      Cast<UCapsuleComponent>(NewUpdatedComponent) ||
      Cast<UBoxComponent>(NewUpdatedComponent) ||
      Cast<USphereComponent>(NewUpdatedComponent)
    )
  )
  {
    CFLog(GetWorld() && GetWorld()->IsGameWorld(), Error, "New updated component must have a supported collision shape.")
    Super::SetUpdatedComponent(NewUpdatedComponent);
    return;
  }

  if (IsValid(UpdatedPrimitive) && UpdatedPrimitive->OnComponentBeginOverlap.IsBound())
  {
    UpdatedPrimitive->OnComponentBeginOverlap.RemoveDynamic(this, &UGMC_OrganicMovementCmp::RootCollisionTouched);
  }

  Super::SetUpdatedComponent(NewUpdatedComponent);
  gmc_ck(IsValid(UpdatedComponent))
  gmc_ck(IsValid(UpdatedPrimitive))
  gmc_ck(IsValid(PawnOwner))

  if (!IsValid(UpdatedComponent) || !IsValid(UpdatedPrimitive))
  {
    CFLog(GetWorld() && GetWorld()->IsGameWorld(), Error, "New updated component is not valid.")
    DisableMovement();
    return;
  }

  if (PawnOwner->GetRootComponent() != UpdatedComponent)
  {
    CFLog(GetWorld() && GetWorld()->IsGameWorld(), Warning, "New updated component must be the root component.")
    PawnOwner->SetRootComponent(UpdatedComponent);
  }

  if (IsValid(UpdatedPrimitive))
  {
    UpdatedPrimitive->OnComponentBeginOverlap.AddUniqueDynamic(this, &UGMC_OrganicMovementCmp::RootCollisionTouched);
  }
}

void UGMC_OrganicMovementCmp::PerformMovement(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_PerformMovement)

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreMovementUpdate, DeltaSeconds);

  if (!CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CanMove))
  {
    FLog(VeryVerbose, "Pawn cannot move, returning.")
    BlockSkeletalMeshPoseTick();
    HaltMovement();
    return;
  }
  else
  {
    if (bDisablePoseTickOnDedicatedServer && IsNetMode(NM_DedicatedServer))
    {
      BlockSkeletalMeshPoseTick();
    }
    else
    {
      EnableSkeletalMeshPoseTick();
    }
  }

  {
    SCOPE_CYCLE_COUNTER(STAT_UpdateMovementMode)

    EGMC_MovementMode PreviousMovementMode = GetMovementMode();

    if (!CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, UpdateMovementModeDynamic, CurrentFloor, DeltaSeconds))
    {
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, UpdateMovementModeStatic, CurrentFloor, DeltaSeconds);
    }

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnMovementModeUpdated, PreviousMovementMode);
  }

  // Often the input vector is processed with regard to the current movement mode so we call this after the movement mode was updated.
  ProcessedInputVector = RoundInputVector(
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreProcessInputVector, GetRawInputVector()),
    EGMC_FloatPrecisionBlueprint::TwoDecimals
  );

  RunPhysics(DeltaSeconds);

  VelocityBeforeMovementUpdate = Velocity;

  {
    SCOPE_CYCLE_COUNTER(STAT_MovementUpdate)

    // Preferred entry point for subclasses to implement their own movement logic.
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, MovementUpdate, DeltaSeconds);
  }

  MontageUpdate(DeltaSeconds);

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostMovementUpdate, DeltaSeconds);
}

void UGMC_OrganicMovementCmp::PreMovementUpdate_Implementation(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_PreMovementUpdate)

  gmc_ck(UpdatedComponent)
  gmc_ck(!UpdatedComponent->IsSimulatingPhysics())

  ClampToValidValues();

  // This is a good place to (re)set all values that need to be frame-independent to guarantee accurate client replay.
  SetPhysDeltaTime(DeltaSeconds, true);
  ProcessedInputVector = FVector::ZeroVector;
  LedgeFallOffDirection = FVector::ZeroVector;
  gmc_ck(LandingVelocity == FVector::ZeroVector)
  LandingVelocity = FVector::ZeroVector;
  gmc_ck(VelocityBeforeMovementUpdate == FVector::ZeroVector)
  VelocityBeforeMovementUpdate = FVector::ZeroVector;
  RootMotionParams.Clear();
  gmc_ck(!MontageTracker.bStartedNewMontage)
  MontageTracker.bStartedNewMontage = false;
  gmc_ck(!MontageTracker.bMontageEnded)
  MontageTracker.bMontageEnded = false;
  gmc_ck(!MontageTracker.bInterruptedPreviousMontage)
  MontageTracker.bInterruptedPreviousMontage = false;

  // Make sure the floor is updated in every based movement branch.
  if (BasedMovement.IsRelative() || (IsSimulating() && BasedMovement.IsForcedRelativeSimulation()))
  {
    UpdateBasedMovementRelative(DeltaSeconds);
    FLog(
      VeryVerbose,
      "Based movement mode is \"Relative\", current movement base is \"%s\" (%s).",
      *GetObjectNameWithOuter(GetActorBase()),
      RelBasedMovementAux.SV_TransitionTimer >= 0. ?
        *FString::Printf(
          TEXT("has pending base \"%s\" with transition time %1.3f/%1.3f"),
          *GetObjectNameWithOuter(RelBasedMovementAux.SV_TransitionBase),
          RelBasedMovementAux.SV_TransitionTimer,
          BasedMovement.GetBaseChangeTransitionTime()
        ) :
        TEXT("no pending base")
    )
  }
  else if (BasedMovement.IsVelocity())
  {
    UpdateBasedMovementVelocity(DeltaSeconds);
    FLog(VeryVerbose, "Based movement mode is \"Velocity\", current movement base is \"%s\".", *GetObjectNameWithOuter(GetActorBase()))
  }
  else
  {
    gmc_ck(BasedMovement.IsNone())
    UpdateBasedMovementNone(DeltaSeconds);
    FLog(VeryVerbose, "Based movement mode is \"None\", current movement base is \"%s\".", *GetObjectNameWithOuter(GetActorBase()))
  }

  DEBUG_VERIFY_FLOOR_NOT_DIRTY(CurrentFloor)

  UpdateImmersionDepth();
}

void UGMC_OrganicMovementCmp::PostMovementUpdate_Implementation(float DeltaSeconds)
{
  CheckReceivedUpwardForce(VelocityBeforeMovementUpdate);
  VelocityBeforeMovementUpdate = FVector::ZeroVector;

  NavMeshWalkingAux.bWasNavMeshWalkingLastUpdate = IsNavMeshWalking();
}

void UGMC_OrganicMovementCmp::PhysicsSimulationUpdate_Implementation(float DeltaSeconds)
{
  const auto& PreviousBase = GetActorBase();
  USceneComponent* NewBase = nullptr;

  if (BasedMovement.IsRelative() || BasedMovement.IsVelocity())
  {
    // We still want to determine the actor base during physics simulation so we can potentially display interpolated pawns more accurately during simulation.
    SetActorBase(CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, FindActorBase));
    NewBase = GetActorBase();
  }
  else
  {
    gmc_ck(BasedMovement.IsNone())
    gmc_ck(!NewBase)
    SetActorBase(nullptr);
  }

  if (NewBase != PreviousBase)
  {
    HandleActorBaseChange(NewBase, PreviousBase, false);
  }
}

FVector UGMC_OrganicMovementCmp::PreProcessInputVector_Implementation(FVector InRawInputVector)
{
  if (!IsPlayerControlledPawn())
  {
    // Bots using acceleration already receive the move input in the direction of the target.
    return InRawInputVector;
  }

  return TransformInputVectorAbsoluteZ(InRawInputVector);
}

void UGMC_OrganicMovementCmp::ClampToValidValues()
{
  SetWalkableFloorAngle(WalkableFloorAngle);
  Mass = FMath::Clamp(Mass, UE_KINDA_SMALL_NUMBER, UE_BIG_NUMBER);
  MaxDesiredSpeed = FMath::Clamp(MaxDesiredSpeed, 0.f, UE_BIG_NUMBER);
  MinAnalogWalkSpeed = FMath::Clamp(MinAnalogWalkSpeed, BRAKE_TO_STOP_VELOCITY + UE_KINDA_SMALL_NUMBER, UE_BIG_NUMBER);
  MinAnalogWalkSpeed = FMath::Min(MinAnalogWalkSpeed, MaxDesiredSpeed);
  RotationRate = FMath::Clamp(RotationRate, 0.f, UE_BIG_NUMBER);
  MaxStepUpHeight = FMath::Clamp(MaxStepUpHeight, 0.f, UE_BIG_NUMBER);
  MaxStepDownHeight = FMath::Clamp(MaxStepDownHeight, 0.f, UE_BIG_NUMBER);
  LedgeFallOffThreshold = FMath::Clamp(LedgeFallOffThreshold, 0.f, 1.f);
  BuoyantStateMinImmersion = FMath::Clamp(BuoyantStateMinImmersion, UE_KINDA_SMALL_NUMBER, 1.f);
  PartialImmersionThreshold = FMath::Clamp(PartialImmersionThreshold, UE_KINDA_SMALL_NUMBER, 1.f);
  PartialImmersionThreshold = FMath::Min(PartialImmersionThreshold, BuoyantStateMinImmersion - IMMERSION_DEPTH_TOLERANCE);
  FluidMinExitSpeed = FMath::Clamp(FluidMinExitSpeed, 0.f, UE_BIG_NUMBER);
  GravityScale = FMath::Clamp(GravityScale, 0.f, UE_BIG_NUMBER);
  FloorTraceLength = FMath::Clamp(FloorTraceLength, GetMaxStepDownHeight() + MAX_DISTANCE_TO_FLOOR + UE_KINDA_SMALL_NUMBER, UE_BIG_NUMBER);
  MaxDepenetrationWithGeometry = FMath::Clamp(MaxDepenetrationWithGeometry, 0.f, UE_BIG_NUMBER);
  MaxDepenetrationWithPawn = FMath::Clamp(MaxDepenetrationWithPawn, 0.f, UE_BIG_NUMBER);
}

EGMC_MovementMode UGMC_OrganicMovementCmp::GetMovementMode() const
{
  return static_cast<EGMC_MovementMode>(MovementMode);
}

FVector UGMC_OrganicMovementCmp::GetRawInputVector() const
{
  return RawInputVector;
}

FVector UGMC_OrganicMovementCmp::GetProcessedInputVector() const
{
  return ProcessedInputVector;
}

float UGMC_OrganicMovementCmp::GetCurrentImmersionDepth() const
{
  return CurrentImmersionDepth;
}

void UGMC_OrganicMovementCmp::UpdateRelativeBasedMovementTransform(bool bSimulated)
{
  RelBasedMovementAux.UpdateSavedRelativeTransform(bSimulated, this);
}

bool UGMC_OrganicMovementCmp::IsAffectedByGravity() const
{
  return GetGravityZ() != 0.f;
}

bool UGMC_OrganicMovementCmp::IsMovingOnGround() const
{
  return MovementMode == static_cast<uint8>(EGMC_MovementMode::Grounded);
}

bool UGMC_OrganicMovementCmp::IsAirborne() const
{
  return MovementMode == static_cast<uint8>(EGMC_MovementMode::Airborne);
}

bool UGMC_OrganicMovementCmp::IsSwimming() const
{
  return MovementMode == static_cast<uint8>(EGMC_MovementMode::Buoyant);
}

bool UGMC_OrganicMovementCmp::IsFlying() const
{
  return IsAirborne() && !IsAffectedByGravity();
}

bool UGMC_OrganicMovementCmp::IsFalling() const
{
  return IsAirborne();
}

bool UGMC_OrganicMovementCmp::IsExceedingMaxGroundedVelocityZ() const
{
  return MaxGroundedVelocityZ >= 0.f && Velocity.Z > MaxGroundedVelocityZ;
}

bool UGMC_OrganicMovementCmp::HasRootMotion() const
{
  return bHasRootMotion;
}

float UGMC_OrganicMovementCmp::GetAnimRootMotionTranslationScale() const
{
  return AnimRootMotionTranslationScale;
}

void UGMC_OrganicMovementCmp::SetAnimRootMotionTranslationScale(float Scale)
{
  AnimRootMotionTranslationScale = Scale;
}

void UGMC_OrganicMovementCmp::SmoothActorBaseChangeSimulated(int32 TargetIndex, bool bExtrapolating, EGMC_InterpolationStates InterpStates)
{
  gmc_ck(BasedMovement.IsRelative() || BasedMovement.IsForcedRelativeSimulation())

  if (bExtrapolating || IsPerformingExtrapolationRecovery())
  {
    return;
  }

  bool bUseInputStates{false};

  switch (InterpolationMode)
  {
    case EGMC_InterpolationMode::FixedDelay:
    {
      bUseInputStates = InterpStates == EGMC_InterpolationStates::Input;
      break;
    }
    case EGMC_InterpolationMode::AdaptiveDelay:
    {
      bUseInputStates = InterpStates == EGMC_InterpolationStates::Input;
      break;
    }

    // Only supported for delay-based interpolation modes.
    case EGMC_InterpolationMode::None:
    case EGMC_InterpolationMode::MatchLatest:
    case EGMC_InterpolationMode::UniformSimulation:
    case EGMC_InterpolationMode::CumulativeSimulation:
    {
      return;
    }
  }

  const int32 StartIndex = TargetIndex - 1;

  if (!IsValidMoveHistoryIndex(StartIndex) || !IsValidMoveHistoryIndex(TargetIndex))
  {
    return;
  }

  const auto& StartState = bUseInputStates ? MoveHistory[StartIndex].InputState : MoveHistory[StartIndex].OutputState;
  const auto& TargetState = bUseInputStates ? MoveHistory[TargetIndex].InputState : MoveHistory[TargetIndex].OutputState;

  const auto& StartBase = StartState.ActorBase.Read();
  const auto& TargetBase = TargetState.ActorBase.Read();

  if (StartBase != TargetBase)
  {
    const auto& StartActorLocation = StartState.ActorLocation.Read();
    const auto& TargetActorLocation = TargetState.ActorLocation.Read();

    const auto& StartWorldActorLocation = StartBase ?
      GetWorldActorLocationFor(StartActorLocation, StartBase->GetComponentTransform()) : StartActorLocation;
    const auto& TargetWorldActorLocation = TargetBase ?
      GetWorldActorLocationFor(TargetActorLocation, TargetBase->GetComponentTransform()) : TargetActorLocation;

    const double WorldLocationDifference = FVector::Distance(StartWorldActorLocation, TargetWorldActorLocation);

    if (WorldLocationDifference >= SMOOTH_SIMULATED_BASE_CHANGE_MIN_LOC_DIFF)
    {
      TriggerExtrapolationRecovery(true);
    }
  }
}

EGMC_MovementMode UGMC_OrganicMovementCmp::GetPreviousMovementModeSimulated() const
{
  return PreviousMovementModeSimulated;
}

FRotator UGMC_OrganicMovementCmp::GetPreviousControlRotationSimulated() const
{
  return PreviousControlRotationSimulated;
}

void UGMC_OrganicMovementCmp::RunPhysics(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_RunPhysics)

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PrePhysicsUpdate, DeltaSeconds);

  switch (GetMovementMode())
  {
    case EGMC_MovementMode::None:
    {
      gmc_ckne()
      return;
    }
    case EGMC_MovementMode::Grounded:
    {
      PhysicsGrounded(DeltaSeconds);
      break;
    }
    case EGMC_MovementMode::Airborne:
    {
      PhysicsAirborne(DeltaSeconds);
      break;
    }
    case EGMC_MovementMode::Buoyant:
    {
      PhysicsBuoyant(DeltaSeconds);
      break;
    }
    default:
    {
      SCOPE_CYCLE_COUNTER(STAT_PhysicsCustom)
      FLog(VeryVerbose, "Running custom physics.")
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PhysicsCustom, DeltaSeconds);
    }
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostPhysicsUpdate, DeltaSeconds);
}

void UGMC_OrganicMovementCmp::PostPhysicsUpdate_Implementation(float DeltaSeconds)
{
  // Set the physics delta time again in case it was altered by custom physics.
  SetPhysDeltaTime(DeltaSeconds, false);
}

void UGMC_OrganicMovementCmp::MoveWithBaseRelative_Implementation(bool bSimulated)
{
  SCOPE_CYCLE_COUNTER(STAT_MoveWithBaseRelative)

  gmc_ck(BasedMovement.IsRelative() || BasedMovement.IsForcedRelativeSimulation())
  gmc_ck(!CL_IsReplaying())

  const auto& CurrentBase = GetActorBase();

  const auto& SavedBase = RelBasedMovementAux.SavedTransforms.GetSavedBase(bSimulated);

  if (!CurrentBase || !SavedBase || CurrentBase != SavedBase)
  {
    return;
  }

  const FVector SavedRelativeLinearVelocity = RelBasedMovementAux.SavedTransforms.GetSavedRelativeLinearVelocity(bSimulated);
  const FVector SavedRelativePawnLocation = RelBasedMovementAux.SavedTransforms.GetSavedRelativeActorLocation(bSimulated);
  const FRotator SavedRelativePawnRotation = RelBasedMovementAux.SavedTransforms.GetSavedRelativeActorRotation(bSimulated);
  const FRotator SavedRelativeControlRotation = RelBasedMovementAux.SavedTransforms.GetSavedRelativeControlRotation(bSimulated);

  const FVector SavedWorldLinearVelocity = RelBasedMovementAux.SavedTransforms.GetSavedWorldLinearVelocity(bSimulated);
  const FVector SavedWorldPawnLocation = RelBasedMovementAux.SavedTransforms.GetSavedWorldActorLocation(bSimulated);
  const FRotator SavedWorldPawnRotation = RelBasedMovementAux.SavedTransforms.GetSavedWorldActorRotation(bSimulated);
  const FRotator SavedWorldControlRotation = RelBasedMovementAux.SavedTransforms.GetSavedWorldControlRotation(bSimulated);

  const FVector NewWorldLinearVelocity = GetLinearVelocity_GMC();
  const FVector NewWorldPawnLocation = GetActorLocation_GMC();
  const FRotator NewWorldPawnRotation = GetActorRotation_GMC();
  const FRotator NewWorldControlRotation = GetControllerRotation_GMC();

  const FVector NewBaseLocation = CurrentBase->GetComponentLocation();
  const FRotator NewBaseRotation = CurrentBase->GetComponentRotation();

  const FVector WorldLinearVelocityDiff = NewWorldLinearVelocity - SavedWorldLinearVelocity;
  SetBasedLinearVelocity(SavedRelativeLinearVelocity + WorldLinearVelocityDiff);

  const FVector WorldPawnLocationDiff = NewWorldPawnLocation - SavedWorldPawnLocation;
  const FVector UpdatedWorldPawnLocation = NewBaseLocation + NewBaseRotation.RotateVector(SavedRelativePawnLocation + WorldPawnLocationDiff);

  const FRotator WorldPawnRotationYawDiff = NewWorldPawnRotation - SavedWorldPawnRotation;
  const FRotator UpdatedWorldPawnRotation = FRotator{0., NewBaseRotation.Yaw, 0.} + SavedRelativePawnRotation + WorldPawnRotationYawDiff;

  const FRotator WorldControlRotationDiff = NewWorldControlRotation - SavedWorldControlRotation;
  SetBasedControlRotation({SavedRelativeControlRotation + WorldControlRotationDiff});

  {
    FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

    DEBUG_MOVE_WITH_BASE_START_LOCATION
    DEBUG_MOVE_WITH_BASE_START_ROTATION

    FHitResult Hit{};

    UpdatedComponent->SetWorldLocationAndRotation(UpdatedWorldPawnLocation, UpdatedWorldPawnRotation, true, &Hit, ETeleportType::TeleportPhysics);

    gmc_ck(CurrentBase)

    if (Hit.bBlockingHit)
    {
      if (const auto& HitComponent = Hit.GetComponent())
      {
        const auto& HitComponentOwner = HitComponent->GetOwner();
        if (HitComponent == CurrentBase || Cast<AGMC_Pawn>(HitComponentOwner) || Cast<AGMC_RollbackActor>(HitComponentOwner))
        {
          // The pawn must not be blocked by potentially rolled back actors or the base itself.
          ScopedMovement.RevertMove();
          UpdatedComponent->SetWorldLocationAndRotation(UpdatedWorldPawnLocation, UpdatedWorldPawnRotation, false, &Hit, ETeleportType::TeleportPhysics);
        }
        else
        {
          USceneComponent* AttachParent = HitComponent;
          while (true)
          {
            AttachParent = AttachParent->GetAttachParent();
            if (AttachParent)
            {
              if (AttachParent == CurrentBase)
              {
                // The pawn must not be blocked by anything that is attached to the current base.
                ScopedMovement.RevertMove();
                UpdatedComponent->SetWorldLocationAndRotation(UpdatedWorldPawnLocation, UpdatedWorldPawnRotation, false, &Hit, ETeleportType::TeleportPhysics);
              }
            }
            else
            {
              break;
            }
          }
        }
      }
    }

    DEBUG_MOVE_WITH_BASE_MOVE_DELTA
    DEBUG_MOVE_WITH_BASE_ROTATION_DELTA

    FLog(
      VeryVerbose,
      "Pawn moved with base. MoveDelta: {X = %.3f, Y = %.3f, Z = %.3f}, RotationDelta: {Roll = %.3f, Pitch = %.3f, Yaw = %.3f}",
      Debug_MoveDelta.X, Debug_MoveDelta.Y, Debug_MoveDelta.Z,
      Debug_RotationDelta.Roll, Debug_RotationDelta.Pitch, Debug_RotationDelta.Yaw
    )
  }
}

bool UGMC_OrganicMovementCmp::ShouldMoveGMCPawnForBaseEqualization_Implementation(const AGMC_Pawn* PawnToTest) const
{
  gmc_ck(IsValid(PawnToTest))
  gmc_ck(PawnToTest != PawnOwner)

  const auto& ReplicationComponent = PawnToTest->GetReplicationComponent();
  gmc_ck(IsValid(ReplicationComponent))

  const auto& CurrentBase = GetActorBase();
  gmc_ck(CurrentBase)

  if (ReplicationComponent->GetActorBase() == CurrentBase)
  {
    // Only consider pawns that are on the same base.
    return true;
  }

  return false;
}

void UGMC_OrganicMovementCmp::UnEqualizeBase()
{
  if (IsNetMode(NM_Standalone))
  {
    return;
  }

  if (!RelBasedMovementAux.bDidEqualizeBase)
  {
    FLog(VeryVerbose, "Base is not equalized, returning.")
    return;
  }

  if (RelBasedMovementAux.SavedTransforms.GetSavedBase(false) != GetActorBase())
  {
    FLog(Error, "The saved actor base does not match the current actor base.")
    gmc_ckne()
    return;
  }

  RelBasedMovementAux.EqualizeBase(false, true, this);
}

void UGMC_OrganicMovementCmp::ReEqualizeBase()
{
  if (IsNetMode(NM_Standalone))
  {
    return;
  }

  if (RelBasedMovementAux.bDidEqualizeBase)
  {
    FLog(VeryVerbose, "Base is already equalized, returning.")
    return;
  }

  RelBasedMovementAux.EqualizeBase(true, true, this);
}

bool UGMC_OrganicMovementCmp::UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds)
{
  const bool bIsInWater = CurrentImmersionDepth >= BuoyantStateMinImmersion;

  if (bIsInWater)
  {
    return false;
  }

  if (IsAirborne())
  {
    if (bReceivedUpwardForce)
    {
      // Remain airborne as long as we are receiving upward velocity.
      FLog(VeryVerbose, "Pawn remaining airborne.")
      return true;
    }
  }

  if (IsMovingOnGround())
  {
    if (!IsAffectedByGravity() && !bGroundingWithoutGravity)
    {
      // Don't stick to or fall off any surfaces.
      return false;
    }

    // If our movement mode is currently grounded we would continue to stick to the ground even if we received upward velocity externally.
    if (bReceivedUpwardForce || IsExceedingMaxGroundedVelocityZ())
    {
      FLog(VeryVerbose, "Pawn being raised from ground.")
      RaiseFromGround(Floor);
      gmc_ck(IsAirborne())
      return true;
    }

    if (IsNavMeshWalking())
    {
      return true;
    }

    const bool bHasValidShapeData = Floor.HasValidShapeData();
    const bool bHasValidLineData = Floor.HasValidLineData();

    // We should continue to stick to the ground if we did not receive external upward force and other constrains are not violated (e.g. to not lose contact
    // with a dynamic base we are standing on that is moving downwards).
    const float StepDownHeight = GetMaxStepDownHeight();
    const bool bShapeHitWithinStepDownHeight =
      bHasValidShapeData && Floor.GetShapeDistanceToFloor() <= StepDownHeight + MAX_DISTANCE_TO_FLOOR;
    const bool bLineHitWithinStepDownHeight =
      bHasValidLineData && Floor.GetLineDistanceToFloor() <= StepDownHeight + MAX_DISTANCE_TO_FLOOR;
    const bool bShouldFallOffLedge =
      !bLineHitWithinStepDownHeight &&
      bHasValidShapeData &&
      IsExceedingFallOffThreshold(Floor.ShapeHit().ImpactPoint, GetLowerBound(), UpdatedComponent->GetComponentLocation());

    if (bShapeHitWithinStepDownHeight && !bShouldFallOffLedge && HitWalkableFloor(Floor.ShapeHit()))
    {
      gmc_ck(IsMovingOnGround())
      FLog(VeryVerbose, "Pawn remaining grounded.")
      return true;
    }
  }

  if (IsSwimming())
  {
    // Since we are not in water anymore the pawn must have just left the fluid volume.
    const bool bShouldRemainInFluid = CheckLeaveFluid(Floor, EXIT_FLUID_IMMERSION_TOLERANCE, FluidMinExitSpeed, 0.f);
    CFLog(bShouldRemainInFluid, VeryVerbose, "Pawn remaining in fluid (\"%s\" = %f).", TO_STR(CurrentImmersionDepth), CurrentImmersionDepth)
    return bShouldRemainInFluid;
  }

  return false;
}

bool UGMC_OrganicMovementCmp::UpdateMovementModeStatic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds)
{
  gmc_ck(BuoyantStateMinImmersion >= UE_KINDA_SMALL_NUMBER)

  // When swimming or flying (no gravity) we are never based on anything i.e. nothing is considered a walkable surface.

  if (CurrentImmersionDepth >= BuoyantStateMinImmersion)
  {
    FLog(VeryVerbose, "Pawn is in fluid (\"%s\" = %f).", TO_STR(CurrentImmersionDepth), CurrentImmersionDepth)
    SetMovementMode(EGMC_MovementMode::Buoyant);
    return true;
  }

  if (!IsAffectedByGravity() && !bGroundingWithoutGravity)
  {
    FLog(VeryVerbose, "Pawn is unaffected by gravity and can't be grounded.")
    SetMovementMode(EGMC_MovementMode::Airborne);
    return true;
  }

  gmc_ck(LedgeFallOffDirection.IsZero())

  bool bShouldFallOffLedge = false;
  if (Floor.HasValidShapeData())
  {
    const bool bShapeHitWithinFloorRange = Floor.GetShapeDistanceToFloor() <= MAX_DISTANCE_TO_FLOOR;

    const bool bLineHitWithinFloorRange = Floor.HasValidLineData() && Floor.GetLineDistanceToFloor() <= MAX_DISTANCE_TO_FLOOR;

    const bool bHitFloorIsWalkable =
      (bShapeHitWithinFloorRange && HitWalkableFloor(Floor.ShapeHit())) || (bLineHitWithinFloorRange && HitWalkableFloor(Floor.LineHit()));

    const bool bVelocityPointsAwayFromGround = (Floor.ShapeHit().ImpactNormal | Velocity.GetSafeNormal()) > DOT_PRODUCT_75;

    const bool bHasSignificantUpwardVelocity = Velocity.Z > 100.;

    const bool bShouldRemainAirborne = IsAirborne() && ((bHasSignificantUpwardVelocity && bVelocityPointsAwayFromGround) || IsExceedingMaxGroundedVelocityZ());

    const bool bLineHitWithinStepDownHeight =
      Floor.HasValidLineData() && Floor.GetLineDistanceToFloor() <= GetMaxStepDownHeight() + MAX_DISTANCE_TO_FLOOR;

    bShouldFallOffLedge =
      !bLineHitWithinStepDownHeight && IsExceedingFallOffThreshold(Floor.ShapeHit().ImpactPoint, GetLowerBound(), UpdatedComponent->GetComponentLocation());

    if (
      (bShapeHitWithinFloorRange || bLineHitWithinFloorRange) &&
      bHitFloorIsWalkable &&
      !bShouldRemainAirborne &&
      !bShouldFallOffLedge
    )
    {
      if (IsAirborne())
      {
        FLog(VeryVerbose, "Pawn landing on ground due to being in range of a walkable floor.")

        const FHitResult& Hit = Floor.ShapeHit().IsValidBlockingHit() ? Floor.ShapeHit() : Floor.LineHit();
        gmc_ck(Hit.IsValidBlockingHit())

        // Switching to grounded movement like this should trigger a hit notification as well.
        if (const auto& UpdatedComponentPrimitive = Cast<UPrimitiveComponent>(UpdatedComponent))
        {
          PawnOwner->DispatchBlockingHit(UpdatedComponentPrimitive, Hit.GetComponent(), true, Hit);
        }

        // Process landing and set the movement mode to grounded.
        ProcessLanded(Hit, Floor, false, DeltaSeconds);

        return true;
      }

      // The pawn was already on the ground.
      gmc_ck(IsMovingOnGround())
      FLog(VeryVerbose, "Pawn remaining grounded.")

      return true;
    }
  }

  if (!Floor.ShapeHit().bStartPenetrating)
  {
    CFLog(GetMovementMode() == EGMC_MovementMode::Airborne, VeryVerbose, "Pawn remaining airborne.")
    SetMovementMode(EGMC_MovementMode::Airborne);
  }
  else
  {
    FLog(VeryVerbose, "Movement mode not updated due to penetrating shape hit.")
  }

  if (bShouldFallOffLedge)
  {
    const FVector ImpactToCenter = (UpdatedComponent->GetComponentLocation() - Floor.ShapeHit().ImpactPoint);
    LedgeFallOffDirection = FVector(ImpactToCenter.X, ImpactToCenter.Y, 0.).GetSafeNormal();
  }

  return true;
}

void UGMC_OrganicMovementCmp::OnMovementModeUpdated_Implementation(EGMC_MovementMode PreviousMovementMode)
{
  DEBUG_VERIFY_FLOOR_NOT_DIRTY(CurrentFloor)

  // The flag should have been processed at this point, so this is a good place to reset it.
  bReceivedUpwardForce = false;

  if (!IsNavMeshWalking())
  {
    ResetLastValidTargetNavLocation();
  }
}

void UGMC_OrganicMovementCmp::RaiseFromGround(FGMC_FloorParams& Floor)
{
  gmc_ck(IsMovingOnGround())

  DEBUG_VERIFY_FLOOR_NOT_DIRTY(Floor)

  const bool bIsNavMeshWalking = IsNavMeshWalking();

  const bool bHasValidShapeData = Floor.HasValidShapeData();
  const bool bHasValidLineData = Floor.HasValidLineData();

  double DeltaZ = MAX_DISTANCE_TO_FLOOR;

  if (!bIsNavMeshWalking)
  {
    // Do not adjust the Z delta based on line data since the line distance could potentially be very large (e.g. when peeking over a ledge).
    if (bHasValidShapeData)
    {
      DeltaZ -= FMath::Clamp(Floor.GetShapeDistanceToFloor() - 0.01, 0., UE_DOUBLE_BIG_NUMBER);
      gmc_ck(DeltaZ <= MAX_DISTANCE_TO_FLOOR)
      DeltaZ = FMath::Clamp(DeltaZ, 0., MAX_DISTANCE_TO_FLOOR);
    }
  }

  const double StartLocationZ = UpdatedComponent->GetComponentLocation().Z;

  MoveUpdatedComponent({0., 0., DeltaZ}, UpdatedComponent->GetComponentQuat(), true, nullptr);

  gmc_ck(UpdatedComponent->GetComponentLocation().Z >= StartLocationZ)
  const float MovedDeltaZ = UpdatedComponent->GetComponentLocation().Z - StartLocationZ;

  if (!bIsNavMeshWalking)
  {
    if (bHasValidShapeData)
    {
      Floor.SetShapeDistanceToFloor(Floor.GetShapeDistanceToFloor() + MovedDeltaZ);
    }
    if (bHasValidLineData)
    {
      Floor.SetLineDistanceToFloor(Floor.GetLineDistanceToFloor() + MovedDeltaZ);
    }

    Floor.RefreshSourceData(this);
  }

  SetMovementMode(EGMC_MovementMode::Airborne);
}

bool UGMC_OrganicMovementCmp::MaintainDistanceToFloor(FGMC_FloorParams& Floor, bool bAnySurface)
{
  SCOPE_CYCLE_COUNTER(STAT_MaintainDistanceToFloor)

  if (!Floor.HasValidShapeData() || (!bAnySurface && !HitWalkableFloor(Floor.ShapeHit())))
  {
    FLog(
      VeryVerbose,
      "Returning: \"%s\" = %s, \"%s\" = %s.",
      TO_STR(Floor.HasValidShapeData()),
      BOOL_TO_STR(Floor.HasValidShapeData()),
      TO_STR(HitWalkableFloor(Floor.ShapeHit())),
      BOOL_TO_STR(HitWalkableFloor(Floor.ShapeHit()))
    )
    return false;
  }

  float CurrentDistanceToFloor = Floor.GetShapeDistanceToFloor();

  if (CurrentDistanceToFloor >= MIN_DISTANCE_TO_FLOOR && CurrentDistanceToFloor <= MAX_DISTANCE_TO_FLOOR)
  {
    FLog(
      VeryVerbose,
      "Returning: Pawn already within set limits (\"%s\" = %f, \"%s\" = %f).",
      TO_STR(MIN_DISTANCE_TO_FLOOR),
      MIN_DISTANCE_TO_FLOOR,
      TO_STR(MAX_DISTANCE_TO_FLOOR),
      MAX_DISTANCE_TO_FLOOR
    )
    return true;
  }

  constexpr float AvgDistanceToFloor = 0.5f * (MIN_DISTANCE_TO_FLOOR + MAX_DISTANCE_TO_FLOOR);
  const float DeltaZ = AvgDistanceToFloor - CurrentDistanceToFloor;

  // Here we check for undesirable behaviour that can happen with rounded collision shapes. The pawn could unintentionally get pushed up a barrier because the
  // underside of the collision "peeks" over a ledge (i.e. the impact point of the shape hit is higher than the impact point of the line hit).
  const bool bHasValidLineData = Floor.HasValidLineData();
  if (
    !bAnySurface &&
    DeltaZ > 0.f &&
    bHasValidLineData &&
    Floor.ShapeHit().ImpactPoint.Z > Floor.LineHit().ImpactPoint.Z &&
    Floor.GetLineDistanceToFloor() >= MIN_DISTANCE_TO_FLOOR &&
    Floor.GetLineDistanceToFloor() <= MAX_DISTANCE_TO_FLOOR
  )
  {
    return false;
  }

  // Try to maintain the average distance from the floor.
  {
    const FVector AdjustDelta = FVector(0., 0., DeltaZ);
    const double StartLocationZ = UpdatedComponent->GetComponentLocation().Z;

    MoveUpdatedComponent(AdjustDelta, UpdatedComponent->GetComponentQuat(), true, nullptr);

    const float MovedDeltaZ = FMath::Sign(DeltaZ) * FMath::Abs(UpdatedComponent->GetComponentLocation().Z - StartLocationZ);

    Floor.SetShapeDistanceToFloor(Floor.GetShapeDistanceToFloor() + MovedDeltaZ);

    if (bHasValidLineData)
    {
      Floor.SetLineDistanceToFloor(Floor.GetLineDistanceToFloor() + MovedDeltaZ);
    }

    Floor.RefreshSourceData(this);

    FLog(
      VeryVerbose,
      "Pawn moved %s by %f.",
      DeltaZ > 0.f ? TEXT("up") : TEXT("down"),
      MovedDeltaZ
    )

    if (MovedDeltaZ == 0.f)
    {
      FLog(
        VeryVerbose,
        "Could not maintain distance to floor. Current values are "
        "%f (shape trace) and %f (line trace) (set limits: \"%s\" = %f, \"%s\" = %f).",
        Floor.GetShapeDistanceToFloor(),
        bHasValidLineData ? Floor.GetLineDistanceToFloor() : -1.f,
        TO_STR(MIN_DISTANCE_TO_FLOOR),
        MIN_DISTANCE_TO_FLOOR,
        TO_STR(MAX_DISTANCE_TO_FLOOR),
        MAX_DISTANCE_TO_FLOOR
      )
      return false;
    }
  }

  CurrentDistanceToFloor = Floor.GetShapeDistanceToFloor();
  if (CurrentDistanceToFloor >= MIN_DISTANCE_TO_FLOOR && CurrentDistanceToFloor <= MAX_DISTANCE_TO_FLOOR)
  {
    FLog(
      VeryVerbose,
      "Adjustment successful, new values are %f (shape trace) and %f (line trace) "
      "(set limits: \"%s\" = %f, \"%s\" = %f).",
      Floor.GetShapeDistanceToFloor(),
      bHasValidLineData ? Floor.GetLineDistanceToFloor() : -1.f,
      TO_STR(MIN_DISTANCE_TO_FLOOR),
      MIN_DISTANCE_TO_FLOOR,
      TO_STR(MAX_DISTANCE_TO_FLOOR),
      MAX_DISTANCE_TO_FLOOR
    )
    return true;
  }

  FLog(
    VeryVerbose,
    "Adjustment not successful, new values are %f (shape trace) and %f (line trace) "
    "(set limits: \"%s\" = %f, \"%s\" = %f).",
    Floor.GetShapeDistanceToFloor(),
    bHasValidLineData ? Floor.GetLineDistanceToFloor() : -1.f,
    TO_STR(MIN_DISTANCE_TO_FLOOR),
    MIN_DISTANCE_TO_FLOOR,
    TO_STR(MAX_DISTANCE_TO_FLOOR),
    MAX_DISTANCE_TO_FLOOR
  )
  return false;
}

void UGMC_OrganicMovementCmp::PhysicsGrounded(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_PhysicsGrounded)

  CalculateVelocity(DeltaSeconds);

  const FVector LocationDelta = Velocity * DeltaSeconds;

  if (IsNavMeshWalking())
  {
    MoveAlongNavMesh(LocationDelta, CurrentFloor, DeltaSeconds);
  }
  else
  {
    MoveAlongFloor(LocationDelta, CurrentFloor, DeltaSeconds);
  }
}

void UGMC_OrganicMovementCmp::PhysicsAirborne(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_PhysicsAirborne)

  const FVector OldVelocity = Velocity;
  CalculateVelocity(DeltaSeconds);

  const FVector LocationDelta = 0.5 * (OldVelocity + Velocity) * DeltaSeconds;
  MoveThroughAir(LocationDelta, CurrentFloor, DeltaSeconds);
}

void UGMC_OrganicMovementCmp::PhysicsBuoyant(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_PhysicsBuoyant)

  CalculateVelocity(DeltaSeconds);

  const FVector LocationDelta = Velocity * DeltaSeconds;
  MoveThroughFluid(LocationDelta, CurrentFloor, DeltaSeconds);
}

float UGMC_OrganicMovementCmp::GetInputAcceleration() const
{
  switch (GetMovementMode())
  {
    case EGMC_MovementMode::None: return 0.f;
    case EGMC_MovementMode::Grounded: return InputAccelerationGrounded;
    case EGMC_MovementMode::Airborne: return InputAccelerationAirborne;
    case EGMC_MovementMode::Buoyant: return InputAccelerationBuoyant;
    default: return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, GetInputAccelerationCustom);
  }
}

float UGMC_OrganicMovementCmp::GetBrakingDeceleration() const
{
  switch (GetMovementMode())
  {
    case EGMC_MovementMode::None: return 0.f;
    case EGMC_MovementMode::Grounded: return BrakingDecelerationGrounded;
    case EGMC_MovementMode::Airborne: return BrakingDecelerationAirborne;
    case EGMC_MovementMode::Buoyant: return BrakingDecelerationBuoyant;
    default: return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, GetBrakingDecelerationCustom);
  }
}

float UGMC_OrganicMovementCmp::GetOverMaxSpeedDeceleration() const
{
  switch (GetMovementMode())
  {
    case EGMC_MovementMode::None: return 0.f;
    case EGMC_MovementMode::Grounded: return OverMaxSpeedDecelerationGrounded;
    case EGMC_MovementMode::Airborne: return OverMaxSpeedDecelerationAirborne;
    case EGMC_MovementMode::Buoyant: return OverMaxSpeedDecelerationBuoyant;
    default: return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, GetOverMaxSpeedDecelerationCustom);
  }
}

float UGMC_OrganicMovementCmp::GetGroundFriction() const
{
  return GroundFriction;
}

bool UGMC_OrganicMovementCmp::ShouldDoFloorUpdateAdjust() const
{
  return true;
}

void UGMC_OrganicMovementCmp::CalculateVelocity(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_CalculateVelocity)

  if (IsMovingOnGround())
  {
    UpdateVelocity({Velocity.X, Velocity.Y, 0.}, DeltaSeconds);
  }

  FGMC_RootMotionVelocitySettings RootMotionVelocitySettings = GetRootMotionVelocityMetaData(MontageTracker.Montage);

  const bool bCanPerformDirectMove = IsServerBot() && !UseAccelerationForPathFollowing();

  if (bCanPerformDirectMove && CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, ShouldPerformDirectMove))
  {
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, BotDirectMove, RequestedVelocity, RootMotionVelocitySettings, DeltaSeconds);
  }
  else
  {
    ApplyInputVelocity(RootMotionVelocitySettings, DeltaSeconds);

    ApplyRotation(false, RootMotionVelocitySettings, DeltaSeconds);

    ApplyExternalForces(RootMotionVelocitySettings, DeltaSeconds);

    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CalculateVelocityCustom, RootMotionVelocitySettings, DeltaSeconds);

    ApplyDeceleration(RootMotionVelocitySettings, DeltaSeconds);

    LimitSpeed(RootMotionVelocitySettings, DeltaSeconds);
  }

  if (ShouldComputeAvoidance())
  {
    CalculateAvoidanceVelocity(DeltaSeconds);
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostProcessPawnVelocity, RootMotionVelocitySettings);
}

bool UGMC_OrganicMovementCmp::ShouldPerformDirectMove_Implementation() const
{
  return !RequestedVelocity.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER) && !HasRootMotion();
}

void UGMC_OrganicMovementCmp::BotDirectMove_Implementation(
  const FVector& PathVelocity,
  const FGMC_RootMotionVelocitySettings& RootMotionMetaData,
  float DeltaSeconds
)
{
  FVector PathDirection = PathVelocity.GetSafeNormal();
  const double PathSpeed = FMath::Clamp(PathVelocity.Size(), 0., GetMaxSpeed());

  FVector NewVelocity{0.};
  if (IsMovingOnGround())
  {
    PathDirection.Z = Velocity.Z = 0.;
    NewVelocity = PathDirection * PathSpeed;
  }
  else if (IsAirborne() && IsAffectedByGravity() && PathDirection.Z == 0.)
  {
    // Apply gravity if no Z velocity was requested.
    const FVector NewVelocityXY = PathDirection * PathSpeed;
    const FVector NewVelocityZ = {0., 0., Velocity.Z + GetGravityZ() * DeltaSeconds};
    NewVelocity = NewVelocityXY + NewVelocityZ;
  }
  else
  {
    NewVelocity = PathDirection * PathSpeed;
  }

  UpdateVelocity(NewVelocity);

  ApplyRotation(true, RootMotionMetaData, DeltaSeconds);
}

void UGMC_OrganicMovementCmp::ApplyInputVelocity(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  if (!HasMoveInputEnabled())
  {
    return;
  }

  if (HasRootMotion() && !RootMotionMetaData.bApplyInputWithRootMotion)
  {
    return;
  }

  FVector InputAcceleration = CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, ComputeInputAcceleration);
  if (InputAcceleration.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    return;
  }

  if (IsMovingOnGround())
  {
    // Maintain horizontal velocity while grounded.
    InputAcceleration.Z = 0.;
    // The ground friction affects our ability to accelerate and change direction. For the input acceleration we clamp this to a multiplier of 1 but for
    // deceleration it can be larger to come to a halt faster.
    InputAcceleration *= FMath::Min(GetGroundFriction(), 1.f);
  }

  const bool bShouldApplyFallControl = FallControl > UE_KINDA_SMALL_NUMBER && IsAirborne() && IsAffectedByGravity();
  if (bShouldApplyFallControl)
  {
    // If we use fall control the input will be added in a different way.
    ApplyFallControl(FallControl, InputAcceleration, DeltaSeconds);
    return;
  }

  AddAcceleration(InputAcceleration, DeltaSeconds);
}

void UGMC_OrganicMovementCmp::ApplyRotation(bool bIsDirectBotMove, const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  if (HasRootMotion() && !RootMotionMetaData.bApplyRotationWithRootMotion)
  {
    return;
  }

  // If both options are set bOrientToControlRotationDirection takes precedence.
  if (bOrientToControlRotationDirection)
  {
    if (bUseSafeRotations)
    {
      RotateYawTowardsDirectionSafe(GetControllerRotation_GMC().Vector(), RotationRate, DeltaSeconds);
    }
    else
    {
      RotateYawTowardsDirection(GetControllerRotation_GMC().Vector(), RotationRate, DeltaSeconds);
    }
    return;
  }

  if (bOrientToInputDirection && HasMoveInputEnabled())
  {
    // For bots with direct movement the input direction is the requested velocity direction.
    if (bUseSafeRotations)
    {
      RotateYawTowardsDirectionSafe(bIsDirectBotMove ? Velocity : GetProcessedInputVector(), RotationRate, DeltaSeconds);
    }
    else
    {
      RotateYawTowardsDirection(bIsDirectBotMove ? Velocity : GetProcessedInputVector(), RotationRate, DeltaSeconds);
    }
    return;
  }
}

void UGMC_OrganicMovementCmp::ApplyExternalForces(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  gmc_ck(!LedgeFallOffDirection.IsZero() ? IsAirborne() : true)

  if (HasRootMotion() && !RootMotionMetaData.bApplyExternalForcesWithRootMotion)
  {
    return;
  }

  if (IsAirborne())
  {
    // Add gravity.
    AddAcceleration(GetGravity(), DeltaSeconds);

    // Make the pawn fall off a ledge if required.
    ApplyLedgeFallOffAcceleration(DeltaSeconds);

    return;
  }

  if (IsSwimming())
  {
    // Add buoyancy. Not applied if we are just at the water line, otherwise we would constantly switch in and out of the buoyant state.
    if (CurrentImmersionDepth > BuoyantStateMinImmersion + IMMERSION_DEPTH_TOLERANCE)
    {
      const float BuoyancyMagnitude = ComputeBuoyantForce(Mass, GetGravityZ(), Buoyancy);
      AddForce({0., 0., BuoyancyMagnitude}, DeltaSeconds);
    }
    return;
  }
}

void UGMC_OrganicMovementCmp::ApplyLedgeFallOffAcceleration(float DeltaSeconds)
{
  if (LedgeFallOffDirection.IsZero())
  {
    return;
  }

  const bool bNeedsPush = GetSpeedXY() <= MIN_LEDGE_FALL_OFF_VELOCITY || (LedgeFallOffDirection | Velocity) <= DOT_PRODUCT_15;
  if (bNeedsPush)
  {
    gmc_ck(LedgeFallOffDirection.IsNormalized())
    AddAcceleration(LedgeFallOffDirection * MIN_LEDGE_FALL_OFF_VELOCITY, DeltaSeconds);
  }
}

void UGMC_OrganicMovementCmp::ApplyDeceleration(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  if (Velocity.IsZero())
  {
    return;
  }

  if (HasRootMotion() && !RootMotionMetaData.bApplyDecelerationWithRootMotion)
  {
    return;
  }

  FVector Deceleration{0.};
  if (IsSwimming())
  {
    if (const auto& PhysicsVolume = GetPhysicsVolume())
    {
      // When in water the fluid friction slows us down in all directions.
      Deceleration += GetBrakingDeceleration() * FLUID_DECELERATION_SCALE * PhysicsVolume->FluidFriction * FLUID_FRICTION_SCALE * -Velocity;
    }
  }
  else
  {
    // When not in water we decelerate only in the direction we are currently not accelerating in, meaning there is no deceleration if the velocity direction
    // aligns with the acceleration direction.
    const float DecelerationScale = FMath::Clamp(1.f - (GetTransientAcceleration().GetSafeNormal() | Velocity.GetSafeNormal()), 0.f, 1.f);
    Deceleration = GetBrakingDeceleration() * DecelerationScale * -Velocity.GetSafeNormal();
    if (const bool bIsFlying = IsFlying(); IsMovingOnGround() || bIsFlying)
    {
      // Scale deceleration with the ground friction.
      Deceleration *= bIsFlying ? 1.f : GetGroundFriction();
      if (CurrentImmersionDepth >= PartialImmersionThreshold)
      {
        // We move slower while partially immersed in water.
        gmc_ck(PartialImmersionThreshold <= BuoyantStateMinImmersion - IMMERSION_DEPTH_TOLERANCE)
        Deceleration += PartialImmersionSlowDown * -Velocity;
      }
    }
    else if (IsAirborne())
    {
      // Do not decelerate against gravity.
      const bool bIsFallingDown = IsAffectedByGravity() && Velocity.Z < 0.;
      // Do not decelerate against input in positive Z-direction.
      const bool bIsReceivingUpwardInput = CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, ComputeInputAcceleration).Z > UE_DOUBLE_KINDA_SMALL_NUMBER;
      if (bIsFallingDown || bIsReceivingUpwardInput)
      {
        Deceleration.Z = 0.;
      }
    }
  }

  const FVector StartVelocity = Velocity;
  if (!Deceleration.IsZero())
  {
    Deceleration = ClampToMinDeceleration(Deceleration);
    AddAcceleration(Deceleration, DeltaSeconds);
    // Check whether we need to set the velocity to zero (the whole vector or specific components) because we either reversed direction or reached the
    // brake-to-stop velocity.
    const FVector VelocityXY = FVector(Velocity.X, Velocity.Y, 0.);
    const FVector VelocityZ = FVector(0., 0., Velocity.Z);
    if (DirectionsDiffer(StartVelocity, Velocity) || Velocity.SizeSquared() <= FMath::Square(BRAKE_TO_STOP_VELOCITY))
    {
      UpdateVelocity(FVector::ZeroVector, DeltaSeconds);
    }
    else
    {
      if (DirectionsDifferXY(StartVelocity, Velocity) || VelocityXY.SizeSquared() <= FMath::Square(BRAKE_TO_STOP_VELOCITY))
      {
        UpdateVelocity(VelocityZ, DeltaSeconds);
      }
      if (DirectionsDifferZ(StartVelocity, Velocity) || FMath::Abs(Velocity.Z) <= BRAKE_TO_STOP_VELOCITY)
      {
        UpdateVelocity(VelocityXY, DeltaSeconds);
      }
    }
  }
}

void UGMC_OrganicMovementCmp::LimitSpeed(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  if (Velocity.IsZero())
  {
    return;
  }

  if (IsAirborne() && IsAffectedByGravity())
  {
    // We always want to limit the speed of the pawn to terminal velocity (even when falling with root motion).

    const bool bIsFalling = Velocity.Z < 0.;
    if (bIsFalling)
    {
      // If falling, don't let the Z component in the direction of gravity exceed terminal velocity.
      LimitAirborneSpeedTerminalVelocity(DeltaSeconds);
    }

    if (HasRootMotion() && !RootMotionMetaData.bLimitSpeedWithRootMotion)
    {
      return;
    }

    // XY-velocity components will still be limited by the max desired speed. We don't enforce a limit on the upward Z-velocity when airborne, it is only slowed
    // down by gravity.
    LimitSpeedXY(DeltaSeconds);

    return;
  }

  if (HasRootMotion() && !RootMotionMetaData.bLimitSpeedWithRootMotion)
  {
    return;
  }

  LimitSpeedXYZ(DeltaSeconds);
}

void UGMC_OrganicMovementCmp::LimitSpeedXY(float DeltaSeconds)
{
  if (const float MaxSpeed = GetMaxSpeed(); FVector(Velocity.X, Velocity.Y, 0.).SizeSquared() > FMath::Square(MaxSpeed))
  {
    // Slow down XY components to max speed.
    const FVector StartVelocityXY = FVector(Velocity.X, Velocity.Y, 0.);
    AddAcceleration(-StartVelocityXY.GetSafeNormal() * GetOverMaxSpeedDeceleration(), DeltaSeconds);
    const FVector NewVelocityXY = FVector(Velocity.X, Velocity.Y, 0.);
    if (DirectionsDifferXY(StartVelocityXY, NewVelocityXY) || NewVelocityXY.SizeSquared() < FMath::Square(MaxSpeed))
    {
      // Don't decelerate the pawn below max speed.
      UpdateVelocity(FVector(0., 0., Velocity.Z) + StartVelocityXY.GetSafeNormal() * MaxSpeed, DeltaSeconds);
    }
  }
}

void UGMC_OrganicMovementCmp::LimitSpeedXYZ(float DeltaSeconds)
{
  if (const float MaxSpeed = GetMaxSpeed(); IsExceedingMaxSpeed(MaxSpeed))
  {
    // Slow down to max speed. The max speed may be affected by an analog input modifier.
    const FVector StartVelocity = Velocity;
    AddAcceleration(-Velocity.GetSafeNormal() * GetOverMaxSpeedDeceleration(), DeltaSeconds);
    if (DirectionsDiffer(StartVelocity, Velocity) || Velocity.SizeSquared() < FMath::Square(MaxSpeed))
    {
      // Don't decelerate the pawn below max speed.
      UpdateVelocity(StartVelocity.GetSafeNormal() * MaxSpeed, DeltaSeconds);
    }
  }
}

void UGMC_OrganicMovementCmp::LimitAirborneSpeedTerminalVelocity(float DeltaSeconds)
{
  const bool bIsFalling = IsAirborne() && IsAffectedByGravity() && Velocity.Z < 0.;
  if (!bIsFalling)
  {
    return;
  }

  if (const auto& PhysicsVolume = GetPhysicsVolume())
  {
    const float TerminalVelocity = PhysicsVolume->TerminalVelocity;
    if (FMath::Abs(Velocity.Z) > TerminalVelocity)
    {
      // Slow down Z component to terminal velocity.
      const FVector StartVelocityZ = FVector(0., 0., Velocity.Z);
      AddAcceleration(-StartVelocityZ.GetSafeNormal() * GetOverMaxSpeedDeceleration(), DeltaSeconds);
      if (DirectionsDifferZ(StartVelocityZ, Velocity) || FMath::Abs(Velocity.Z) < TerminalVelocity)
      {
        // Don't decelerate the pawn below max speed.
        UpdateVelocity(FVector(Velocity.X, Velocity.Y, 0.) + StartVelocityZ.GetSafeNormal() * TerminalVelocity, DeltaSeconds);
      }
    }
  }
}

FVector UGMC_OrganicMovementCmp::RoundInputVector(const FVector& WorldInputVector, EGMC_FloatPrecisionBlueprint Level) const
{
  FVector RoundedVector = RoundBasedVectorByRotation(WorldInputVector, GetRelativeActorBase(), Level);
  if (FMath::IsNearlyZero(WorldInputVector.X, UE_DOUBLE_KINDA_SMALL_NUMBER)) RoundedVector.X = 0.;
  if (FMath::IsNearlyZero(WorldInputVector.Y, UE_DOUBLE_KINDA_SMALL_NUMBER)) RoundedVector.Y = 0.;
  if (FMath::IsNearlyZero(WorldInputVector.Z, UE_DOUBLE_KINDA_SMALL_NUMBER)) RoundedVector.Z = 0.;
  return RoundedVector;
}

float UGMC_OrganicMovementCmp::ComputeInputModifier() const
{
  FVector Input = GetProcessedInputVector();
  // For grounded movement we disregard a negative Z component because we can't move into the ground.
  if (IsMovingOnGround()) Input.Z = FMath::Clamp(Input.Z, 0., 1.);
  const float InputModifier = FMath::Clamp(Input.Size(), 0., 1.);
  return InputModifier;
}

void UGMC_OrganicMovementCmp::UpdateImmersionDepth()
{
  CurrentImmersionDepth = ComputeImmersionDepth();
}

float UGMC_OrganicMovementCmp::ComputeBuoyantForce(float PawnMass, float GravityZ, float BuoyancyCoefficient) const
{
  return Mass * GravityZ * -BuoyancyCoefficient;
}

void UGMC_OrganicMovementCmp::CheckReceivedUpwardForce(const FVector& PreviousVelocity)
{
  if (bReceivedUpwardForce)
  {
    // The flag has already been set externally.
    return;
  }

  const bool bReceivedUpwardVelocity = Velocity.Z >= PreviousVelocity.Z + 0.01;
  if (bReceivedUpwardVelocity)
  {
    FLog(VeryVerbose, "Pawn received upward velocity: VelocityZ (%f) > PreviousVelocityZ (%f)", Velocity.Z, PreviousVelocity.Z)
    bReceivedUpwardForce = true;
    return;
  }

  if (HasRootMotion())
  {
    // If we have root motion the input vector usually doesn't matter.
    return;
  }

  const bool bReceivedUpwardInput = GetProcessedInputVector().Z >= 0.01;
  if (bReceivedUpwardInput)
  {
    FLog(VeryVerbose, "Pawn received upward input: ProcessedInputVectorZ = %f", GetProcessedInputVector().Z)
    bReceivedUpwardForce = true;
    return;
  }
}

bool UGMC_OrganicMovementCmp::CheckLeaveFluid(
  const FGMC_FloorParams& Floor,
  float ImmersionTolerance,
  float SpeedZTolerance,
  float DeltaSeconds
)
{
  SCOPE_CYCLE_COUNTER(STAT_CheckLeaveFluid)

  gmc_ck(IsSwimming())
  gmc_ck(ImmersionTolerance >= 0.f)
  gmc_ck(BuoyantStateMinImmersion >= UE_KINDA_SMALL_NUMBER)
  gmc_ck(BuoyantStateMinImmersion <= 1.f)

  if (SpeedZTolerance <= UE_KINDA_SMALL_NUMBER)
  {
    return false;
  }

  if (CurrentImmersionDepth == 0.f)
  {
    // The pawn may have left the water volume from the side in which case this check does not apply.
    return false;
  }

  const bool bShapeHitWithinFloorRange = Floor.HasValidShapeData() && Floor.GetShapeDistanceToFloor() <= MAX_DISTANCE_TO_FLOOR;
  const bool bHitWalkableFloor = bShapeHitWithinFloorRange && HitWalkableFloor(Floor.ShapeHit());
  if (
    CurrentImmersionDepth + ImmersionTolerance > BuoyantStateMinImmersion &&
    GetSpeedZ() < SpeedZTolerance &&
    !bHitWalkableFloor
  )
  {
    UpdateVelocity({Velocity.X, Velocity.Y, FMath::Min(GetSpeedZ(), 0.f)}, DeltaSeconds);
    const float DeltaZ = 2.f * GetRootCollisionHalfHeight(true) * (BuoyantStateMinImmersion - CurrentImmersionDepth);
    if (DeltaZ > 0.f)
    {
      // Move the pawn back into the fluid volume.
      MoveUpdatedComponent({0., 0., -(DeltaZ + 2.f * UU_MILLIMETER)}, UpdatedComponent->GetComponentQuat(), true);
      UpdateImmersionDepth();
    }
    return true;
  }

  return false;
}

FVector UGMC_OrganicMovementCmp::ComputeInputAcceleration_Implementation() const
{
  return GetProcessedInputVector().GetClampedToMaxSize(1.) * GetInputAcceleration();
}

FVector UGMC_OrganicMovementCmp::ClampToMinDeceleration(const FVector& Deceleration) const
{
  const FVector DecelerationXY = FVector(Deceleration.X, Deceleration.Y, 0.);
  const FVector DecelerationZ = FVector(0., 0., Deceleration.Z);
  const float DecelerationXYSizeSquared = DecelerationXY.SizeSquared();
  const float DecelerationZSize = FMath::Abs(Deceleration.Z);
  FVector ClampedDeceleration = Deceleration;
  if (DecelerationXYSizeSquared > 0.f && DecelerationXYSizeSquared < FMath::Square(MIN_DECELERATION))
  {
    ClampedDeceleration = DecelerationXY.GetClampedToSize(MIN_DECELERATION, UE_DOUBLE_BIG_NUMBER) + DecelerationZ;
  }
  if (DecelerationZSize > 0.f && DecelerationZSize < MIN_DECELERATION)
  {
    ClampedDeceleration.Z = DecelerationZ.Z < 0. ? -MIN_DECELERATION : MIN_DECELERATION;
  }
  return ClampedDeceleration;
}

USceneComponent* UGMC_OrganicMovementCmp::GetOutermostAttachParent(USceneComponent* Component) const
{
  if (!IsValid(Component))
  {
    return nullptr;
  }

  USceneComponent* PreviousAttachParent{nullptr};
  USceneComponent* AttachParent = Component;
  while (true)
  {
    PreviousAttachParent = AttachParent;
    AttachParent = AttachParent->GetAttachParent();
    if (!AttachParent)
    {
      gmc_ck(PreviousAttachParent)
      return PreviousAttachParent;
    }
  }
}

void UGMC_OrganicMovementCmp::SetBasedMovementSource(EGMC_BasedMovementSource NewSource)
{
  if (IsExecutingMove())
  {
    GMC_LOG(LogGMCMovement, PawnOwner, Error, TEXT("\"%s\" must not be called during move execution."), TO_STR(SetBasedMovementSource))
    return;
  }

  if (BasedMovement.Source != NewSource)
  {
    // Reset aux data upon change but still continue execution in either case because the sync settings for prediction and simulation may not be set yet.
    RelBasedMovementAux.Reset();
  }

  BasedMovement.Source = NewSource;

  if (BasedMovement.IsVelocity())
  {
    bUseRelativeValuesForPrediction = false;
    bUseRelativeValuesForSimulation = BasedMovement.IsForcedRelativeSimulation();
  }
  else if (BasedMovement.IsRelative())
  {
    bUseRelativeValuesForPrediction = true;
    bUseRelativeValuesForSimulation = true;
  }
  else
  {
    gmc_ck(BasedMovement.IsNone())
    bUseRelativeValuesForPrediction = false;
    bUseRelativeValuesForSimulation = false;
  }
}

void UGMC_OrganicMovementCmp::UpdateBasedMovementVelocity(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_UpdateBasedMovementVelocity)

  // Follow the base first if we have one as it may have moved inside the pawn. If we don't have a base we want to update the floor first to resolve potential
  // penetrations with a new base that may be determined afterwards.

  const auto& PreviousBase = GetActorBase();

  if (!PreviousBase)
  {
    UpdateFloor(CurrentFloor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), false);
  }

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, MoveWithBaseVelocity, DeltaSeconds);

  SetActorBase(CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, FindActorBase));

  const auto& NewBase = GetActorBase();

  const bool bActorBaseChanged = NewBase != PreviousBase;

  if (bActorBaseChanged)
  {
    HandleActorBaseChange(NewBase, PreviousBase, false);
  }

  if (BasedMovement.GetIgnoreBasedPawns())
  {
    RelBasedMovementAux.HandleMoveIgnoreActors(IsSimulating(), this);
  }

  if (PreviousBase || bActorBaseChanged)
  {
    UpdateFloor(CurrentFloor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), false);
  }
}

void UGMC_OrganicMovementCmp::UpdateBasedMovementRelative(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_UpdateBasedMovementRelative)

  // Finding/following the base is handled before move execution.

  const auto& CurrentBase = GetActorBase();
  const auto& PreviousBase = RelBasedMovementAux.SavedChangeBase;
  if (CurrentBase != PreviousBase)
  {
    HandleActorBaseChange(CurrentBase, PreviousBase, false);
  }

  if (BasedMovement.GetIgnoreBasedPawns())
  {
    RelBasedMovementAux.HandleMoveIgnoreActors(IsSimulating(), this);
  }

  UpdateFloor(CurrentFloor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), false);
}

void UGMC_OrganicMovementCmp::UpdateBasedMovementNone(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_UpdateBasedMovementNone)

  const auto& PreviousBase = GetActorBase();

  SetActorBase(nullptr);

  if (PreviousBase)
  {
    HandleActorBaseChange(nullptr, PreviousBase, false);
  }

  UpdateFloor(CurrentFloor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), false);
}

USceneComponent* UGMC_OrganicMovementCmp::GetRelativeActorBase() const
{
  const bool bUseRelative = IsSimulating() ? bUseRelativeValuesForSimulation : bUseRelativeValuesForPrediction;
  return bUseRelative ? GetActorBase() : nullptr;
}

void UGMC_OrganicMovementCmp::MoveWithBaseVelocity_Implementation(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_MoveWithBaseVelocity)

  gmc_ck(BasedMovement.IsVelocity())

  const auto& CurrentBase = Cast<UPrimitiveComponent>(GetActorBase());
  if (!CurrentBase)
  {
    return;
  }

  const FVector LinearBaseVelocity = GetLinearComponentVelocity(CurrentBase);
  const FVector LinearMoveDelta = LinearBaseVelocity * DeltaSeconds;

  const FVector AngularBaseVelocity = GetAngularComponentVelocityInDegrees(CurrentBase);
  const FVector AngularBaseVelocitySigned = {-AngularBaseVelocity.X, -AngularBaseVelocity.Y, AngularBaseVelocity.Z};
  const FRotator RotationDeltaRotator = ToAngularVelocityRotator(AngularBaseVelocitySigned * DeltaSeconds);
  const FQuat RotationDeltaQuat = RotationDeltaRotator.Quaternion();
  const FVector RelativePawnLocation = GetActorLocation_GMC() - CurrentBase->GetComponentLocation();
  const FVector AngularMoveDelta = RotationDeltaQuat.RotateVector(RelativePawnLocation) - RelativePawnLocation;

  const FVector TotalMoveDelta = LinearMoveDelta + AngularMoveDelta;

  {
    FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

    DEBUG_MOVE_WITH_BASE_START_LOCATION
    DEBUG_MOVE_WITH_BASE_START_ROTATION

    FHitResult Hit{};

    FRotator NewRotation = UpdatedComponent->GetComponentRotation();
    if (BasedMovement.GetFollowBaseRotation())
    {
      NewRotation += FRotator{0., RotationDeltaRotator.Yaw, 0.};
    }

    MoveUpdatedComponent(TotalMoveDelta, NewRotation, true, &Hit, ETeleportType::TeleportPhysics);

    gmc_ck(CurrentBase)
    if (Hit.bStartPenetrating && Hit.GetComponent() == CurrentBase)
    {
      // The base likely moved inside of the pawn, reapply the delta without sweeping.
      ScopedMovement.RevertMove();
      MoveUpdatedComponent(TotalMoveDelta, NewRotation, false, &Hit, ETeleportType::TeleportPhysics);
    }

    DEBUG_MOVE_WITH_BASE_MOVE_DELTA
    DEBUG_MOVE_WITH_BASE_ROTATION_DELTA

    FLog(
      VeryVerbose,
      "Pawn moved with base. MoveDelta: {X = %.3f, Y = %.3f, Z = %.3f}, RotationDelta: {Roll = %.3f, Pitch = %.3f, Yaw = %.3f}",
      Debug_MoveDelta.X, Debug_MoveDelta.Y, Debug_MoveDelta.Z,
      Debug_RotationDelta.Roll, Debug_RotationDelta.Pitch, Debug_RotationDelta.Yaw
    )
  }
}

FVector UGMC_OrganicMovementCmp::MoveAlongNavMesh(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_MoveAlongNavMesh)

  gmc_ck(IsNavMeshWalking())

  const INavigationDataInterface* NavData = GetNavData();
  if (!NavData)
  {
    FLog(VeryVerbose, "Nav mesh walking is enabled but pawn has no nav data, returning.")
    return FVector::ZeroVector;
  }

  FLog(
    VeryVerbose,
    "Starting nav mesh walking (LocationDelta = %s, Velocity = %s).",
    *LocationDelta.ToString(),
    *Velocity.ToString()
  )

  const FVector& StartLocation = UpdatedComponent->GetComponentLocation();
  const FVector& StartLowerBound = GetLowerBound();
  const FVector& OriginalMoveDelta = {LocationDelta.X, LocationDelta.Y, 0.};
  const FVector& OriginalTargetLocation = StartLowerBound + OriginalMoveDelta;

  if (bUseFloorUpdateToleranceForNavMeshWalking && !IsFloorDirty(Floor, FloorUpdateTolerance))
  {
    // Apply the location delta directly without projection.
    const FVector NewLocation = {OriginalTargetLocation.X, OriginalTargetLocation.Y, StartLocation.Z};
    UpdatedComponent->SetWorldLocation(NewLocation, false, nullptr);
  }
  else
  {
    const auto& NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    const float SearchExentXY = GetAgentRadius(NavSystem) * 2.f;
    const float SearchHeightScale = GetNavWalkingSearchHeightScale(NavSystem);
    const float SearchExtentZ = GetAgentHeight(NavSystem) * SearchHeightScale;
    const FVector& SearchExtent = {SearchExentXY, SearchExentXY, SearchExtentZ};

    FVector AdjustedMoveDelta = OriginalMoveDelta;
    FNavLocation TargetNavLocation{};
    if (ProjectPointOntoNavMesh(AdjustedMoveDelta, TargetNavLocation, SearchExtent, NAV_MESH_PROJECTION_MAX_RETRIES, NAV_MESH_PROJECTION_TOLERANCE_XY, NavData))
    {
      gmc_ck(TargetNavLocation.HasNodeRef())

      const double HeightDiff = StartLocation.Z - StartLowerBound.Z;
      constexpr float AvgDistanceToFloor = 0.5f * (MIN_DISTANCE_TO_FLOOR + MAX_DISTANCE_TO_FLOOR);
      const FVector NewLocation = {TargetNavLocation.Location.X, TargetNavLocation.Location.Y, TargetNavLocation.Location.Z + HeightDiff + AvgDistanceToFloor};

      UpdatedComponent->SetWorldLocation(NewLocation, false, nullptr);

      NavMeshWalkingAux.LastValidTargetNavLocation = TargetNavLocation;

      // Set some speculative floor parameters manually based on the projection result.
      FHitResult NavHit{};
      NavHit.TraceStart = GetLowerBound();
      NavHit.TraceEnd = TargetNavLocation.Location;
      NavHit.bBlockingHit = true;
      NavHit.bStartPenetrating = false;
      NavHit.ImpactPoint = TargetNavLocation.Location;
      NavHit.Location = {TargetNavLocation.Location.X, TargetNavLocation.Location.Y, TargetNavLocation.Location.Z + HeightDiff};
      NavHit.Distance = (NavHit.ImpactPoint - NavHit.TraceStart).Size();
      NavHit.Time = 1.f;
      Floor = FGMC_FloorParams(NavHit, NavHit, this);
    }
    else
    {
      CALL_NATIVE_EVENT_CONDITIONAL(
        bNoBlueprintEvents,
        this,
        OnNavMeshProjectionFailed,
        OriginalTargetLocation,
        OriginalMoveDelta,
        NavMeshWalkingAux.LastValidTargetNavLocation,
        SearchExtent
      );
    }
  }

  const FVector AppliedDelta = UpdatedComponent->GetComponentLocation() - StartLocation;

  FLog(
    VeryVerbose,
    "Finished nav mesh walking. Velocity is %s, applied location delta is %s.",
    *Velocity.ToString(),
    *AppliedDelta.ToString()
  )

  return AppliedDelta;
}

bool UGMC_OrganicMovementCmp::ProjectPointOntoNavMesh(
  FVector& InOutMoveDelta,
  FNavLocation& OutTargetNavLocation,
  const FVector& SearchExtent,
  int32 MaxRetries,
  double ToleranceXY,
  const INavigationDataInterface* NavData
)
{
  int32 NumRetries = 0;
  bool bProjectionSuccessful = false;
  bool bWithinTolerance = false;

  const FVector& LowerBound = GetLowerBound();
  FVector TargetLocation = LowerBound + InOutMoveDelta;
  double ToleranceXYSquared = FMath::Square(ToleranceXY);

  while (
    (bProjectionSuccessful = NavData->ProjectPoint(TargetLocation, OutTargetNavLocation, SearchExtent)) == false ||
    (bWithinTolerance = (OutTargetNavLocation.Location - TargetLocation).SizeSquared2D() <= ToleranceXYSquared) == false
  )
  {
    if (++NumRetries > MaxRetries)
    {
      --NumRetries;
      break;
    }

    InOutMoveDelta *= 0.5;
    TargetLocation = LowerBound + InOutMoveDelta;
  }

  FLog(VeryVerbose, "Projection onto nav mesh%s successful: %d retries (max = %d), within XY tolerance (%f) = %s.",
    bProjectionSuccessful ? TEXT("") : TEXT(" not"),
    NumRetries,
    MaxRetries,
    ToleranceXY,
    BOOL_TO_STR(bWithinTolerance)
  )

  return bProjectionSuccessful;
}

void UGMC_OrganicMovementCmp::OnNavMeshProjectionFailed_Implementation(
  const FVector& TargetLocation,
  const FVector& MoveDelta,
  const FVector& LastValidTargetLocation,
  const FVector& SearchExtent
)
{
  SetMovementMode(EGMC_MovementMode::Airborne);

  // Force an update and an adjustment in case we are stuck.
  UpdateFloor(CurrentFloor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, true, true);
}

FVector UGMC_OrganicMovementCmp::MoveAlongFloor(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_MoveAlongFloor)

  gmc_ck(IsMovingOnGround())

  FLog(
    VeryVerbose,
    "Starting to move along floor (LocationDelta = %s, Velocity = %s).",
    *LocationDelta.ToString(),
    *Velocity.ToString()
  )

  // When moving along the floor we ignore the Z-component of the location delta.
  const FVector MoveDelta = FVector(LocationDelta.X, LocationDelta.Y, 0.);
  if (MoveDelta.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    FLog(VeryVerbose, "MoveDelta is zero, returning.")
    MaintainDistanceToFloor(Floor, false);
    return FVector::ZeroVector;
  }

  FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

  FGMC_FloorParams InitialFloor = Floor;
  FHitResult MoveHitResult{};
  const float MaxSpeedXY = FVector(Velocity.X, Velocity.Y, 0.).Size();
  const FVector StartLocation = UpdatedComponent->GetComponentLocation();
  const float StepDownHeight = GetMaxStepDownHeight();
  const bool bStartedWithinLineMaxStepDownHeight = Floor.HasValidLineData() && Floor.GetLineDistanceToFloor() <= StepDownHeight + MAX_DISTANCE_TO_FLOOR;
  bool bDidStepUp = false;

  FVector RampVector = ComputeRampVector(MoveDelta, Floor.ShapeHit());
  SafeMoveUpdatedComponent(RampVector, UpdatedComponent->GetComponentQuat(), true, MoveHitResult);
  const float MoveHitTime = MoveHitResult.Time;
  const float MoveHitTimeRemaining = 1.f - MoveHitTime;
  const FVector MoveDeltaRemaining = MoveDelta * MoveHitTimeRemaining;
  float RemainingDeltaSeconds = DeltaSeconds * MoveHitTimeRemaining;
  if (MoveHitResult.bStartPenetrating)
  {
    FLog(VeryVerbose, "MoveUpdatedComponent produced invalid blocking hit.")

    HandleImpact(MoveHitResult, DeltaSeconds, RampVector);
    // Try again by sliding along the surface with the full location delta.
    SlideAlongSurface(MoveDelta, 1.f, MoveHitResult.Normal, MoveHitResult, true);
  }
  else if (MoveHitResult.IsValidBlockingHit())
  {
    FLog(VeryVerbose, "MoveUpdatedComponent produced valid blocking hit.")

    if (HitWalkableFloor(MoveHitResult) && MoveHitTime > 0.f && MoveHitResult.Normal.Z > UE_DOUBLE_KINDA_SMALL_NUMBER)
    {
      FLog(VeryVerbose, "Moving up walkable surface.")

      // We hit another walkable surface.
      RampVector = ComputeRampVector(MoveDeltaRemaining, MoveHitResult);
      SafeMoveUpdatedComponent(RampVector, UpdatedComponent->GetComponentQuat(), true, MoveHitResult);
    }
    else
    {
      FLog(VeryVerbose, "Pawn hit a barrier.")

      const FVector HitStartLocation = UpdatedComponent->GetComponentLocation();

      // The floor sweep shape may have hit the same unwalkable surface as the move hit result but the floor line trace should still hit a walkable floor for us
      // to try a step-up.
      if (CanStepUp(MoveHitResult) && HitWalkableFloor(Floor.LineHit()))
      {
        FLog(VeryVerbose, "Trying to step up barrier.")

        // Try to step up onto the barrier.
        if (!StepUp(MoveDeltaRemaining, MoveHitResult, Floor))
        {
          FLog(VeryVerbose, "Step up was not successful, sliding along surface.")

          // The step-up was not successful, handle the impact and slide along the surface instead.
          HandleImpact(MoveHitResult, DeltaSeconds, RampVector);
          SlideAlongSurface(MoveDelta, MoveHitTimeRemaining, MoveHitResult.Normal, MoveHitResult, true);
          RemainingDeltaSeconds *= 1.f - MoveHitResult.Time;
        }
        else
        {
          FLog(VeryVerbose, "Step up was successful.")

          // The step-up was successful. We can also arrive at this point when hitting a wall (i.e. we didn't actually step up onto anything). This is correct
          // behaviour since it would be difficult to determine beforehand how high the barrier actually is. Sliding along the surface is handled within the
          // step-up function in this case (while processing the forward hit). Recalculation of the velocity from the moved distance is therefore also required
          // in this case.
          bDidStepUp = true;
        }
      }
      else
      {
        FLog(VeryVerbose, "Cannot step up this barrier, sliding along surface.")

        // We cannot step up onto this barrier so just slide along the hit surface.
        HandleImpact(MoveHitResult, DeltaSeconds, RampVector);
        SlideAlongSurface(MoveDelta, MoveHitTimeRemaining, MoveHitResult.Normal, MoveHitResult, true);
        RemainingDeltaSeconds *= 1.f - MoveHitResult.Time;
      }

      // If we collided with something we need to adjust the velocity.
      const float AppliedDeltaSeconds = DeltaSeconds - RemainingDeltaSeconds;
      const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
      if (!bDidStepUp && AppliedDeltaSeconds >= MIN_DELTA_TIME)
      {
        FLog(VeryVerbose, "Recalculating velocity from moved distance after barrier hit.")

        UpdateVelocity((CurrentLocation - StartLocation) / AppliedDeltaSeconds, AppliedDeltaSeconds);
      }
      else if (AppliedDeltaSeconds < MIN_DELTA_TIME && CurrentLocation.Equals(HitStartLocation, UE_DOUBLE_KINDA_SMALL_NUMBER))
      {
        FLog(VeryVerbose, "Zeroing velocity after barrier hit.")

        // We are most likely stuck in a corner.
        UpdateVelocity(FVector::ZeroVector, DeltaSeconds);
      }
      else if (bDidStepUp && CurrentLocation.Z <= HitStartLocation.Z + UU_MILLIMETER)
      {
        FLog(VeryVerbose, "Adjusting velocity from impact normal after barrier hit.")

        // We did not actually step up onto anything.
        AdjustVelocityFromHit(MoveHitResult, IsSimulating() ? bUseRelativeValuesForSimulation : bUseRelativeValuesForPrediction, DeltaSeconds);
      }

      // The new velocity should remain horizontal and never exceed the velocity that was used as the basis for the calculation of the location delta.
      UpdateVelocity(FVector(Velocity.X, Velocity.Y, 0.).GetClampedToMaxSize(MaxSpeedXY), AppliedDeltaSeconds);
    }
  }
  else if (!MoveHitResult.bBlockingHit)
  {
    FLog(VeryVerbose, "MoveUpdatedComponent produced no blocking hit.")
  }

  FVector AppliedDelta = UpdatedComponent->GetComponentLocation() - StartLocation;
  if (!AppliedDelta.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    FLog(VeryVerbose, "Updating floor after move.")

    UpdateFloor(Floor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), false);

    bool bHitWalkableFloor = HitWalkableFloor(Floor.ShapeHit());
    bool bStillWithinLineMaxStepDownHeight = Floor.HasValidLineData() && Floor.GetLineDistanceToFloor() <= StepDownHeight + MAX_DISTANCE_TO_FLOOR;

    if (bHitWalkableFloor && bStillWithinLineMaxStepDownHeight)
    {
      FLog(VeryVerbose, "Pawn can continue to walk on floor, maintaining distance to floor.")

      // This will also make the pawn stick to the floor again if we exceed the max step down height after moving.
      MaintainDistanceToFloor(Floor, false);
    }
    else
    {
      FLog(
        VeryVerbose,
        "Pawn cannot continue to walk on floor (ShapeHitWalkableFloor = %s, WithinLineMaxStepDownHeight = %s).",
        BOOL_TO_STR(bHitWalkableFloor),
        BOOL_TO_STR(bStillWithinLineMaxStepDownHeight)
      )

      if (bStillWithinLineMaxStepDownHeight)
      {
        if (
          bPreventSlopeBoosting &&
          !bHitWalkableFloor &&
          Floor.HasValidShapeData() &&
          InitialFloor.HasValidShapeData() &&
          !bDidStepUp &&
          !MoveHitResult.bBlockingHit
        )
        {
          const auto& IsWithinConstraints = [this](const FVector& Delta, const FVector& ImpactPoint)
          {
            const FVector& PawnLocation = UpdatedComponent->GetComponentLocation();
            bool bImpactWithinZConstraints = ImpactPoint.Z > GetLowerBound().Z && ImpactPoint.Z < PawnLocation.Z;
            bool bImpactInFront = (Delta.GetSafeNormal2D() | (ImpactPoint - PawnLocation).GetSafeNormal2D()) > DOT_PRODUCT_90;
            return bImpactWithinZConstraints && bImpactInFront;
          };

          if (IsWithinConstraints(AppliedDelta, Floor.ShapeHit().ImpactPoint))
          {
            FLog(VeryVerbose, "Pawn hit an unwalkable slope.")

            const FHitResult& SlopeHit = Floor.ShapeHit();

            ScopedMovement.RevertMove();
            RemainingDeltaSeconds = DeltaSeconds;

            const FVector SlopeNormalBefore = (HitWalkableFloor(InitialFloor.ShapeHit()) ? InitialFloor.ShapeHit() : InitialFloor.LineHit()).ImpactNormal;
            const FVector SlopeNormalBeforeXY = SlopeNormalBefore.GetSafeNormal2D();
            const FVector SlopeNormalAfter = SlopeHit.ImpactNormal;
            const FVector SlopeNormalAfterXY = SlopeNormalAfter.GetSafeNormal2D();
            const FVector AdjustDirection = SlopeNormalBefore.GetSafeNormal().Cross(SlopeNormalAfter);
            const FVector AdjustDirectionXY = AdjustDirection.GetSafeNormal2D();

            const FVector AppliedDeltaXY = FVector{AppliedDelta.X, AppliedDelta.Y, 0.};
            const FVector AdjustNormalXY = AdjustDirectionXY.RotateAngleAxis(-90., FVector::UpVector);
            FHitResult Temp = InitialFloor.ShapeHit();
            SlideAlongSurface(AppliedDeltaXY, 1.f, AdjustNormalXY, Temp, false);
            RemainingDeltaSeconds *= 1.f - Temp.Time;

            UpdateFloor(Floor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), true);
            bHitWalkableFloor = HitWalkableFloor(Floor.ShapeHit());
            bStillWithinLineMaxStepDownHeight = Floor.HasValidLineData() && Floor.GetLineDistanceToFloor() <= StepDownHeight + MAX_DISTANCE_TO_FLOOR;

            if (bStillWithinLineMaxStepDownHeight)
            {
              if (bHitWalkableFloor)
              {
                MaintainDistanceToFloor(Floor, false);
              }
              else if (IsWithinConstraints(UpdatedComponent->GetComponentLocation() - StartLocation, Floor.ShapeHit().ImpactPoint))
              {
                constexpr double AngleThreshold = 0.98;
                constexpr double DistThreshold = 5.;
                const bool bCorner =
                  (Floor.ShapeHit().ImpactNormal.GetSafeNormal2D() | SlopeNormalAfterXY) < AngleThreshold &&
                  (Floor.ShapeHit().ImpactPoint - SlopeHit.ImpactPoint).Size() > DistThreshold;

                if (bCorner)
                {
                  ScopedMovement.RevertMove();
                  RemainingDeltaSeconds = DeltaSeconds;
                  Floor = InitialFloor;
                }
              }
            }

            AppliedDelta = UpdatedComponent->GetComponentLocation() - StartLocation;
            const float AppliedDeltaSeconds = DeltaSeconds - RemainingDeltaSeconds;

            if (AppliedDeltaSeconds < MIN_DELTA_TIME)
            {
              UpdateVelocity(FVector::ZeroVector, DeltaSeconds);
            }
            else
            {
              UpdateVelocity((UpdatedComponent->GetComponentLocation() - StartLocation) / DeltaSeconds, DeltaSeconds);
              UpdateVelocity(FVector(Velocity.X, Velocity.Y, 0.).GetClampedToMaxSize(MaxSpeedXY), DeltaSeconds);
            }
          }
        }
      }
      else if (!bCanWalkOffLedges && bStartedWithinLineMaxStepDownHeight)
      {
        FLog(VeryVerbose, "\"%s\" is false: reverting move and zeroing velocity.", TO_STR(bCanWalkOffLedges))

        ScopedMovement.RevertMove();
        UpdateVelocity(FVector::ZeroVector, DeltaSeconds);
        Floor = InitialFloor;

        // Make sure that the pawn is relatively close to the ledge already to prevent stutter in subsequent updates.
        FGMC_FloorParams PreviousFloor{};
        for (int32 Divisor = 2; Divisor <= 4; Divisor += 2)
        {
          PreviousFloor = Floor;

          MoveUpdatedComponent(AppliedDelta / Divisor, UpdatedComponent->GetComponentQuat(), false, nullptr, ETeleportType::TeleportPhysics);

          // We need to force the update here because even a very small deviation could cause the pawn to go over the ledge.
          UpdateFloor(Floor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, false/*never adjust*/, true);

          bStillWithinLineMaxStepDownHeight = Floor.HasValidLineData() && Floor.GetLineDistanceToFloor() <= StepDownHeight + MAX_DISTANCE_TO_FLOOR;
          if (HitWalkableFloor(Floor.ShapeHit()) && bStillWithinLineMaxStepDownHeight)
          {
            MaintainDistanceToFloor(Floor, false);
          }
          else
          {
            SetActorTransform_GMC(PreviousFloor.GetSourceWorldPawnTransform(), true);

            Floor = PreviousFloor;

            DEBUG_VERIFY_FLOOR_NOT_DIRTY(Floor)
          }
        }
      }
      else
      {
        // The movement mode must not be changed here.
      }
    }
  }
  else
  {
    FLog(VeryVerbose, "No movement occured.")
  }

  FLog(
    VeryVerbose,
    "Finished moving along floor. New velocity is %s, applied location delta is %s.",
    *Velocity.ToString(),
    *AppliedDelta.ToString()
  )

  return AppliedDelta;
}

FVector UGMC_OrganicMovementCmp::ComputeRampVector(const FVector& LocationDelta, const FHitResult& RampHit) const
{
  gmc_ck(LocationDelta.Z == 0.)

  const FVector FloorNormal = RampHit.ImpactNormal;
  const FVector ContactNormal = RampHit.Normal;
  if (
    HitWalkableFloor(RampHit) &&
    FloorNormal.Z > UE_DOUBLE_KINDA_SMALL_NUMBER &&
    FloorNormal.Z < (1.f - UE_DOUBLE_KINDA_SMALL_NUMBER) &&
    ContactNormal.Z > UE_DOUBLE_KINDA_SMALL_NUMBER
  )
  {
    // Compute a vector that moves parallel to the surface by projecting the horizontal movement onto the ramp.
    const FVector RampVector = FVector(LocationDelta.X, LocationDelta.Y, -(FloorNormal | LocationDelta) / FloorNormal.Z);
    return bRescaleSlopeDelta ? RampVector.GetSafeNormal() * LocationDelta.Size() : RampVector;
  }
  return LocationDelta;
}

bool UGMC_OrganicMovementCmp::StepUp(const FVector& LocationDelta, const FHitResult& BarrierHit, const FGMC_FloorParams& Floor)
{
  SCOPE_CYCLE_COUNTER(STAT_StepUp)

  gmc_ck(IsMovingOnGround())
  gmc_ck(CanStepUp(BarrierHit))
  gmc_ck(Floor.HasValidLineData())
  gmc_ck(HitWalkableFloor(Floor.LineHit()))

  const float StepUpHeight = GetMaxStepUpHeight();

  const FVector StepLocationDelta = FVector(LocationDelta.X, LocationDelta.Y, 0.);
  if (StepUpHeight <= 0.f || StepLocationDelta.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    return false;
  }

  FLog(VeryVerbose, "Starting step-up...")
  const FVector StartLocation = UpdatedComponent->GetComponentLocation();
  float PawnHalfHeight{};
  float UpperBoundZ{};
  FVector Extent = GetRootCollisionExtent(true);
  switch (GetRootCollisionShape())
  {
    case EGMC_CollisionShape::VerticalCapsule:
      PawnHalfHeight = Extent.Z;
      UpperBoundZ = StartLocation.Z + (PawnHalfHeight - Extent.X);
      break;
    case EGMC_CollisionShape::Box:
      PawnHalfHeight = Extent.Z;
      UpperBoundZ = StartLocation.Z + PawnHalfHeight;
      break;
    case EGMC_CollisionShape::HorizontalCapsule:
    case EGMC_CollisionShape::Sphere:
      PawnHalfHeight = Extent.X;
      UpperBoundZ = StartLocation.Z;
      break;
    default: gmc_ckne();
  }
  const float BarrierImpactPointZ = BarrierHit.ImpactPoint.Z;
  if (BarrierImpactPointZ > UpperBoundZ)
  {
    // The top of the collision is hitting something so we cannot travel upward at all.
    FLog(VeryVerbose, "Aborting step-up, pawn is blocked from traveling upward.")
    return false;
  }

  float TravelUpHeight = StepUpHeight;
  float TravelDownHeight = StepUpHeight;
  float StartFloorPointZ = StartLocation.Z - PawnHalfHeight;
  float StartDistanceToFloor = 0.f;
  if (Floor.HasValidShapeData() && HitWalkableFloor(Floor.ShapeHit()))
  {
    StartDistanceToFloor = Floor.GetShapeDistanceToFloor();
    TravelDownHeight = StepUpHeight + 2.f * MAX_DISTANCE_TO_FLOOR;
  }
  else if (HitWalkableFloor(Floor.LineHit()) && StartDistanceToFloor <= MAX_DISTANCE_TO_FLOOR)
  {
    StartDistanceToFloor = Floor.GetLineDistanceToFloor();
    TravelDownHeight = StepUpHeight + 2.f * MAX_DISTANCE_TO_FLOOR;
  }
  // We float a variable distance above the floor so we want to calculate the max step-up height based on that distance if possible.
  TravelUpHeight = FMath::Max(StepUpHeight - StartDistanceToFloor, 0.f);
  StartFloorPointZ -= StartDistanceToFloor;

  if (BarrierImpactPointZ <= StartFloorPointZ)
  {
    // The impact point is under the lower bound of the collision.
    FLog(VeryVerbose, "Aborting step-up, barrier impact point is located below the pawn's base.")
    return false;
  }

  FVector ClampedForwardLocationDelta = StepLocationDelta;
  if (StepLocationDelta.SizeSquared() < FMath::Square(MIN_STEP_UP_DELTA))
  {
    // We need to enforce a minimal location delta for the forward sweep, otherwise we could fail a step-up that should actually be feasible when hitting a
    // non-walkable surface (because the forward step would have been very short).
    ClampedForwardLocationDelta = StepLocationDelta.GetClampedToSize(MIN_STEP_UP_DELTA, UE_DOUBLE_BIG_NUMBER);
  }
  gmc_ck(ClampedForwardLocationDelta.Z == 0.)

  const FQuat& PawnRotation = UpdatedComponent->GetComponentQuat();

  FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

  if (bSimplifyStepUp)
  {
    MoveUpdatedComponent(ClampedForwardLocationDelta + FVector{0., 0., TravelUpHeight}, PawnRotation, false, nullptr);
    FLog(VeryVerbose, "Performed simplified forward/up movement without sweeping.")
  }
  else
  {
    // Step up.
    FHitResult UpHit{};
    MoveUpdatedComponent(FVector(0., 0., TravelUpHeight), PawnRotation, true, &UpHit);
    if (UpHit.bStartPenetrating)
    {
      // Invalid move, abort the step-up.
      FLog(VeryVerbose, "Aborting step-up, upward sweep started in penetration.")
      ScopedMovement.RevertMove();
      return false;
    }
    FLog(VeryVerbose, "Upward sweep has %s blocking hit.", UpHit.bBlockingHit ? TEXT("valid") : TEXT("no"))

    // Step forward.
    FHitResult ForwardHit{};
    MoveUpdatedComponent(ClampedForwardLocationDelta, PawnRotation, true, &ForwardHit);
    if (ForwardHit.bBlockingHit)
    {
      if (ForwardHit.bStartPenetrating)
      {
        // Invalid move, abort the step-up.
        FLog(VeryVerbose, "Aborting step-up, forward sweep started in penetration.")
        ScopedMovement.RevertMove();
        return false;
      }
      FLog(VeryVerbose, "Forward sweep has valid blocking hit.")

      // If we hit something above us and also something ahead of us we should notify about the upward hit as well. If we hit something above us but not in front
      // of us we don't notify about any hit since we are not blocked from moving (in which case we don't enter this branch in the first place).
      if (UpHit.bBlockingHit)
      {
        // Notify upward hit.
        HandleImpact(UpHit);
      }
      // Notify forward hit.
      HandleImpact(ForwardHit);

      // Slide along the hit surface, but do so based on the original step location delta.
      const float ForwardHitTime = ForwardHit.Time;
      const float SlideTime = SlideAlongSurface(StepLocationDelta, 1.f - ForwardHit.Time, ForwardHit.Normal, ForwardHit, true);
      if (ForwardHitTime == 0.f && SlideTime == 0.f)
      {
        // No movement occurred, abort the step-up.
        FLog(VeryVerbose, "Aborting step-up, pawn was stuck after forward sweep.")
        ScopedMovement.RevertMove();
        return false;
      }
    }
    else
    {
      FLog(VeryVerbose, "Forward sweep has no blocking hit.")
    }
  }

  // Step down.
  FHitResult DownHit{};
  const EMoveComponentFlags SavedMoveComponentFlags = MoveComponentFlags;
  MoveComponentFlags |= MOVECOMP_NeverIgnoreBlockingOverlaps;
  MoveUpdatedComponent(FVector(0., 0., -TravelDownHeight), PawnRotation, true, &DownHit);
  MoveComponentFlags = SavedMoveComponentFlags;

  if (DownHit.bStartPenetrating)
  {
    // Invalid move, abort the step-up.
    FLog(VeryVerbose, "Aborting step-up, downward sweep started in penetration.")
    ScopedMovement.RevertMove();
    return false;
  }
  if (DownHit.bBlockingHit)
  {
    // We do not notify about hits from the downward sweep since its only purpose is to find a valid end location for the step up, it does not go in the
    // direction of actual movement.
    FLog(VeryVerbose, "Downward sweep has valid blocking hit.")
    const float DeltaZ = DownHit.ImpactPoint.Z - StartFloorPointZ;
    if (DeltaZ > StepUpHeight)
    {
      // This step sequence would have allowed us to travel higher than the max step-up height.
      FLog(
        VeryVerbose,
        "Aborting step-up, \"%s\" (%f) would have been exceeded by actual step-up height (%f).",
        TO_STR(StepUpHeight),
        StepUpHeight,
        DeltaZ
      )
      ScopedMovement.RevertMove();
      return false;
    }
    // Reject unwalkable surface normals.
    if (!HitWalkableFloor(DownHit))
    {
      FLog(VeryVerbose, "Downward sweep hit unwalkable surface.")
      if ((StepLocationDelta | DownHit.ImpactNormal) < 0.)
      {
        // The normal points towards the pawn i.e. opposes the current movement direction.
        FLog(
          VeryVerbose,
          "Aborting step-up, unwalkable impact normal (%s) opposes movement direction (%s).",
          *DownHit.ImpactNormal.ToString(),
          *StepLocationDelta.GetSafeNormal().ToString()
        )
        ScopedMovement.RevertMove();
        return false;
      }
      if (DownHit.Location.Z > StartLocation.Z)
      {
        // Do not step up to an unwalkable normal above us. It is fine to step down onto an unwalkable normal below us though (we will slide off) since
        // rejecting those moves would prevent us from being able to walk off edges.
        FLog(
          VeryVerbose,
          "Aborting step-up, new unwalkable location (%s) would have been higher than the start location (%s).",
          *DownHit.Location.ToString(),
          *StartLocation.ToString()
        )
        ScopedMovement.RevertMove();
        return false;
      }
    }
    CFLog(HitWalkableFloor(DownHit), VeryVerbose, "Downward sweep hit walkable surface.")

    // Reject moves where the downward sweep hit something very close to the edge of the collision. Does not apply to box shapes.
    if (
      GetRootCollisionShape() != EGMC_CollisionShape::Box &&
      !IsWithinEdgeTolerance(DownHit.Location, DownHit.ImpactPoint, EDGE_TOLERANCE)
    )
    {
      FLog(VeryVerbose, "Aborting step-up, impact point is not within edge-tolerance.")
      ScopedMovement.RevertMove();
      return false;
    }
    // Don't step up onto surfaces that are not a valid base.
    if (DeltaZ > 0.f && !CanStepUp(DownHit))
    {
      FLog(VeryVerbose, "Aborting step-up, pawn would have stepped up onto invalid surface.")
      ScopedMovement.RevertMove();
      return false;
    }
  }
  else
  {
    FLog(VeryVerbose, "Downward sweep has no blocking hit.")
  }
  FLog(VeryVerbose, "Finished step-up successfully.")
  return true;
}

bool UGMC_OrganicMovementCmp::MoveThroughAir(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_MoveThroughAir)

  gmc_ck(IsAirborne())
  FLog(VeryVerbose, "Starting to move through air (LocationDelta = %s).", *LocationDelta.ToString())

  if (LocationDelta.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    FLog(VeryVerbose, "MoveDelta is zero, returning.")
    return false;
  }

  FHitResult Hit{};
  const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
  SafeMoveUpdatedComponent(LocationDelta, PawnRotation, true, Hit);

  float LastMoveTimeSlice = DeltaSeconds;
  float DeltaSecondsRemaining = DeltaSeconds * (1.f - Hit.Time);
  float DeltaSecondsApplied = DeltaSeconds - DeltaSecondsRemaining;

  if (Hit.bBlockingHit)
  {
    if (IsValidLandingSpot(Hit))
    {
      // The pawn has landed on a walkable surface.
      FLog(VeryVerbose, "Pawn landing on ground after first blocking hit.")
      ProcessLanded(Hit, Floor, true, DeltaSecondsRemaining);
      PhysicsGrounded(DeltaSecondsRemaining);
      return true;
    }
    else
    {
      // We hit a wall.
      HandleImpact(Hit, LastMoveTimeSlice, LocationDelta);
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, AdjustVelocityFromHitAirborne, Hit, DeltaSecondsApplied);

      const FVector OldHitNormal = Hit.Normal;
      const FVector OldHitImpactNormal = Hit.ImpactNormal;
      const FVector SavedDelta = Velocity * DeltaSecondsRemaining;
      FVector Delta = ComputeSlideVector(LocationDelta, 1.f - Hit.Time, OldHitNormal, Hit);

      if (DeltaSecondsRemaining >= MIN_DELTA_TIME && !DirectionsDiffer(Delta, SavedDelta))
      {
        // It is okay to move with the deflection vector since that will not change our direction.
        SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

        if (Hit.bBlockingHit)
        {
          // We hit another wall.
          LastMoveTimeSlice = DeltaSecondsRemaining;
          DeltaSecondsRemaining *= 1.f - Hit.Time;
          DeltaSecondsApplied = DeltaSeconds - DeltaSecondsRemaining;
          if (IsValidLandingSpot(Hit))
          {
            FLog(VeryVerbose, "Pawn landing on ground after second blocking hit.")
            ProcessLanded(Hit, Floor, true, DeltaSecondsRemaining);
            PhysicsGrounded(DeltaSecondsRemaining);
            return true;
          }
          HandleImpact(Hit, LastMoveTimeSlice, Delta);
          CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, AdjustVelocityFromHitAirborne, Hit, DeltaSecondsApplied);

          FVector PreTwoWallDelta = Delta;
          TwoWallAdjust(Delta, Hit, OldHitNormal);
          SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

          // When bDitch is true the pawn is straddling two slopes, neither of which are walkable.
          bool bDitch =
            OldHitImpactNormal.Z > 0. &&
            Hit.ImpactNormal.Z > 0. &&
            FMath::Abs(Delta.Z) <= UE_DOUBLE_KINDA_SMALL_NUMBER &&
            (Hit.ImpactNormal | OldHitImpactNormal) < 0.;

          if (Hit.Time == 0.f)
          {
            // If we are stuck try to side step.
            FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
            if (SideDelta.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
            {
              SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0.).GetSafeNormal();
            }
            SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
          }

          if ((IsAffectedByGravity() || bGroundingWithoutGravity) && (bDitch || IsValidLandingSpot(Hit) || Hit.Time == 0.f))
          {
            FLog(VeryVerbose, "Pawn landing on ground after two-wall-adjustment.")
            ProcessLanded(Hit, Floor, true, DeltaSecondsRemaining);
            PhysicsGrounded(DeltaSecondsRemaining);
            return true;
          }
        }
      }
    }
  }

  FLog(VeryVerbose, "Moved through air without any blocking hit.")

  return false;
}

void UGMC_OrganicMovementCmp::AdjustVelocityFromHitAirborne_Implementation(const FHitResult& Hit, float DeltaSeconds)
{
  const bool bUseRelative = IsSimulating() ? bUseRelativeValuesForSimulation : bUseRelativeValuesForPrediction;

  if (bPreventSlopeBoosting && IsAffectedByGravity())
  {
    const float SavedVelocityZ = Velocity.Z;
    AdjustVelocityFromHit(Hit, bUseRelative, DeltaSeconds);
    UpdateVelocity({Velocity.X, Velocity.Y, FMath::Clamp(Velocity.Z, -UE_DOUBLE_BIG_NUMBER, SavedVelocityZ)}, DeltaSeconds);
    return;
  }

  AdjustVelocityFromHit(Hit, bUseRelative, DeltaSeconds);
}

bool UGMC_OrganicMovementCmp::IsValidLandingSpot(const FHitResult& Hit)
{
  gmc_ck(IsAirborne())

  if (!UpdatedComponent->GetCollisionEnabled())
  {
    return false;
  }

  if (!IsAffectedByGravity() && !bGroundingWithoutGravity)
  {
    return false;
  }

  if (!Hit.bBlockingHit)
  {
    return false;
  }

  if (Hit.bStartPenetrating)
  {
    ResolvePenetration(GetPenetrationAdjustment(Hit), Hit, UpdatedComponent->GetComponentQuat());
    return false;
  }

  if (IsExceedingMaxGroundedVelocityZ())
  {
    return false;
  }

  if (!HitWalkableFloor(Hit))
  {
    return false;
  }

  FHitResult StepDownHeightTest{};
  const FVector LineTraceStart = Hit.Location;
  const FVector LineTraceEnd = LineTraceStart + FVector::DownVector * (GetMaxStepDownHeight() + MAX_DISTANCE_TO_FLOOR);
  FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(IsValidLandingSpot), false, GetOwner());
  CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
  CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
  const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
  if (const auto& World = GetWorld())
  {
    World->LineTraceSingleByChannel(
      StepDownHeightTest,
      LineTraceStart,
      LineTraceEnd,
      UpdatedComponent->GetCollisionObjectType(),
      CollisionQueryParams,
      CollisionResponseParams
    );
  }

  const bool bCanLandOnLedge =
    StepDownHeightTest.IsValidBlockingHit() ||
    !IsExceedingFallOffThreshold(Hit.ImpactPoint, Hit.Location - FVector(0., 0., GetRootCollisionHalfHeight(true)), Hit.Location);
  if (!bCanLandOnLedge)
  {
    return false;
  }

  float HitLocationLowerBoundZ{};
  const float HitLocationZ = Hit.Location.Z;
  const FVector Extent = GetRootCollisionExtent(true);
  switch (GetRootCollisionShape())
  {
    case EGMC_CollisionShape::VerticalCapsule:
      HitLocationLowerBoundZ = HitLocationZ - (Extent.Z/*HalfHeight*/ - Extent.X/*Radius*/);
      break;
    case EGMC_CollisionShape::Box:
      HitLocationLowerBoundZ = HitLocationZ - Extent.Z;
      break;
    case EGMC_CollisionShape::HorizontalCapsule:
    case EGMC_CollisionShape::Sphere:
      HitLocationLowerBoundZ = HitLocationZ;
      break;
    default: gmc_ckne();
  }
  if (Hit.ImpactPoint.Z >= HitLocationLowerBoundZ)
  {
    // The impact point is on the vertical side of the collision shape or above it.
    return false;
  }

  if (!IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, EDGE_TOLERANCE))
  {
    // We reject hits that are barely on the cusp of the outer side of the collision.
    return false;
  }

  // The pawn hit a valid landing spot.
  return true;
}

void UGMC_OrganicMovementCmp::ProcessLanded(const FHitResult& Hit, FGMC_FloorParams& Floor, bool bUpdateFloor, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_ProcessLanded)

  gmc_ck(IsAirborne())

  LedgeFallOffDirection = FVector::ZeroVector;

  if (bEnablePhysicsInteraction)
  {
    ApplyImpactPhysicsForces(Hit, GetTransientAcceleration(), Velocity);
  }

  if (bUpdateFloor)
  {
    UpdateFloor(Floor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), false);
  }

  gmc_ck(LandingVelocity == FVector::ZeroVector)
  LandingVelocity = Velocity;

  AdjustVelocityFromHit(Hit, IsSimulating() ? bUseRelativeValuesForSimulation : bUseRelativeValuesForPrediction, DeltaSeconds);

  SetMovementMode(EGMC_MovementMode::Grounded);

  LandingVelocity = FVector::ZeroVector;
}

bool UGMC_OrganicMovementCmp::MoveThroughFluid(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_MoveThroughFluid)

  gmc_ck(IsSwimming())
  FLog(VeryVerbose, "Starting to move through fluid (LocationDelta = %s).", *LocationDelta.ToString())

  if (LocationDelta.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    FLog(VeryVerbose, "MoveDelta is zero, returning.")
    return false;
  }

  FHitResult Hit{};
  const float MaxSpeed = Velocity.Size();
  const float TimeOutOfWater = Swim(LocationDelta, Hit, Floor, DeltaSeconds);
  float DeltaSecondsRemaining = DeltaSeconds * TimeOutOfWater;
  const float TimeInWater = DeltaSeconds - DeltaSecondsRemaining;

  if (TimeOutOfWater > 0.f)
  {
    if (CheckLeaveFluid(Floor, EXIT_FLUID_IMMERSION_TOLERANCE, FluidMinExitSpeed, DeltaSecondsRemaining))
    {
      UpdateVelocityFromMovedDistance(TimeInWater, MaxSpeed);
      FLog(VeryVerbose, "Pawn was prevented from leaving the fluid.")
      return false;
    }

    // Pawn has left the water volume.
    if (TimeInWater >= UE_KINDA_SMALL_NUMBER)
    {
      // Don't slow down the Z-velocity so we can still jump out of the water.
      const float PreviousVelocityZ = Velocity.Z;
      UpdateVelocityFromMovedDistance(TimeInWater, MaxSpeed);
      UpdateVelocity({Velocity.X, Velocity.Y, PreviousVelocityZ}, TimeInWater);
    }

    FLog(VeryVerbose, "Pawn is leaving the fluid.")
    ProcessLeftFluid(DeltaSecondsRemaining);

    PhysicsAirborne(DeltaSecondsRemaining);

    return true;
  }

  if (Hit.IsValidBlockingHit())
  {
    // Pawn did not leave the water volume, but hit a wall underwater.
    HandleImpact(Hit, DeltaSeconds, LocationDelta);
    const bool bUseRelative = IsSimulating() ? bUseRelativeValuesForSimulation : bUseRelativeValuesForPrediction;
    AdjustVelocityFromHit(Hit, bUseRelative, DeltaSeconds - DeltaSecondsRemaining);
    SlideAlongSurface(LocationDelta, 1.f - Hit.Time, Hit.Normal, Hit, true);
    DeltaSecondsRemaining *= 1.f - Hit.Time;
    AdjustVelocityFromHit(Hit, bUseRelative, DeltaSeconds - DeltaSecondsRemaining);
  }

  FLog(VeryVerbose, "Pawn remaining in fluid.")

  return false;
}

void UGMC_OrganicMovementCmp::ProcessLeftFluid(float DeltaSeconds)
{
  gmc_ck(IsSwimming())

  SetMovementMode(EGMC_MovementMode::Airborne);
}

float UGMC_OrganicMovementCmp::Swim(const FVector& LocationDelta, FHitResult& OutHit, FGMC_FloorParams& Floor, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_Swim)

  gmc_ck(BuoyantStateMinImmersion >= UE_KINDA_SMALL_NUMBER)

  const FVector StartLocation = UpdatedComponent->GetComponentLocation();
  float TimeOutOfWater = 0.f;

  if (SafeMoveUpdatedComponent(LocationDelta, UpdatedComponent->GetComponentQuat(), true, OutHit))
  {
    UpdateImmersionDepth();
  }

  if (CurrentImmersionDepth < BuoyantStateMinImmersion)
  {
    // Pawn has left the water volume.
    const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
    FVector WaterLine = FindWaterLine(StartLocation, CurrentLocation, CurrentImmersionDepth);

    // Adjust the water line according to the configured min immersion depth.
    if (!FMath::IsNearlyEqual(BuoyantStateMinImmersion, 0.5f, UE_KINDA_SMALL_NUMBER))
    {
      const FVector TraveledInWater = WaterLine - StartLocation;
      WaterLine = StartLocation + (1.5f - BuoyantStateMinImmersion) * TraveledInWater;
    }

    const float DesiredDistance = LocationDelta.Size();
    if (!WaterLine.Equals(CurrentLocation) && DesiredDistance > UE_KINDA_SMALL_NUMBER)
    {
      const float DistanceOutOfWater = (CurrentLocation - WaterLine).Size();
      // For the rare case that the pawn leaves the water volume from the side the water line may not have been calculated correctly.
      if (DesiredDistance > DistanceOutOfWater)
      {
        TimeOutOfWater = DistanceOutOfWater / DesiredDistance;
        // Move us back to the water line. We shouldn't hit anything on the way back because the was no collision before either.
        MoveUpdatedComponent(WaterLine - CurrentLocation, UpdatedComponent->GetComponentQuat(), false);
      }
    }
  }

  return TimeOutOfWater;
}

FVector UGMC_OrganicMovementCmp::FindWaterLine(
  const FVector& LocationInWater,
  const FVector& LocationOutOfWater,
  float ImmersionDepth
) const
{
  SCOPE_CYCLE_COUNTER(STAT_FindWaterLine)

  const auto& World = GetWorld();
  if (!World) return FVector{0.};

  FVector Result = LocationOutOfWater;

  // Do a line trace that goes into the body of water.
  TArray<FHitResult> Hits{};
  FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(FindWaterLine), true, GetOwner());
  const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
  World->LineTraceMultiByChannel(
    Hits,
    LocationOutOfWater,
    LocationInWater,
    UpdatedComponent->GetCollisionObjectType(),
    CollisionQueryParams,
    CollisionResponseParams
  );

  for (int32 Index = 0; Index < Hits.Num(); ++Index)
  {
    const FHitResult& Hit = Hits[Index];
    if (!PawnOwner->IsOwnedBy(Hit.GetActor()) && !Hit.GetComponent()->IsWorldGeometry())
    {
      APhysicsVolume* Volume = Cast<APhysicsVolume>(Hit.GetActor());
      if (Volume && Volume->bWaterVolume)
      {
        // This is the water volume the pawn came out of so the water line is at the impact location of the trace.
        Result = Hit.Location;

        // We want to remain in the same state that we are currently in so we adjust the result slightly in the appropriate direction.
        const FVector IntoWaterDirection = (LocationInWater - LocationOutOfWater).GetSafeNormal();
        if (ImmersionDepth >= BuoyantStateMinImmersion)
        {
          // We are currently in the buoyant movement state i.e. considered to be inside the water.
          Result += 0.1 * IntoWaterDirection;
        }
        else
        {
          // We are currently not considered to be inside the water.
          Result -= 0.1 * IntoWaterDirection;
        }
        break;
      }
    }
  }

  return Result;
}

void UGMC_OrganicMovementCmp::ApplyFallControl(float Control, const FVector& InputAcceleration, float DeltaSeconds)
{
  gmc_ck(Control > UE_KINDA_SMALL_NUMBER && IsAirborne() && IsAffectedByGravity())

  FVector Velocity2D = {Velocity.X, Velocity.Y, 0.};
  const FVector InputAcceleration2D = {InputAcceleration.X, InputAcceleration.Y, 0.};
  if (InputAcceleration2D.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    return;
  }

  FVector VelocityToAdd = InputAcceleration2D * Control * DeltaSeconds;

  const FVector ProposedVelocity = Velocity2D + VelocityToAdd;
  const float MaxSpeed = GetMaxSpeed();
  if (ProposedVelocity.Size() <= MaxSpeed)
  {
    // We won't exceed the max speed, add the full input velocity.
    AddVelocity(VelocityToAdd, DeltaSeconds);
    return;
  }

  // We are either already above max speed or will be after adding the full input velocity. In this case we only want to apply the direction change from the
  // user input but don't want to gain any additional speed after being at or above max speed.
  const float Speed2D = Velocity2D.Size();
  const FVector NewVelocity2D = ProposedVelocity.GetClampedToMaxSize(FMath::Max(Speed2D, MaxSpeed));

  // Set the new 2D velocity but leave the Z velocity unchanged.
  UpdateVelocity({NewVelocity2D.X, NewVelocity2D.Y, Velocity.Z}, DeltaSeconds);
}

bool UGMC_OrganicMovementCmp::IsExceedingFallOffThreshold(
  const FVector& ImpactPoint,
  const FVector& PawnLowerBound,
  const FVector& PawnCenter
) const
{
  gmc_ck(LedgeFallOffThreshold >= 0.f)
  gmc_ck(LedgeFallOffThreshold <= 1.f)

  // If the pawn cannot walk off ledges we always use the center of the collision as fall-off threshold, otherwise the pawn could partially land on a ledge
  // (from an airborne state) and still walk off afterwards.
  const float TestThreshold = bCanWalkOffLedges ? LedgeFallOffThreshold : 0.f;

  if (TestThreshold == 1.f)
  {
    // A threshold of 1 means we do not want to fall off at all.
    return false;
  }

  const FVector CenterToImpactXY = FVector(ImpactPoint.X, ImpactPoint.Y, 0.) - FVector(PawnCenter.X, PawnCenter.Y, 0.);
  const FVector ImpactDirectionXY = CenterToImpactXY.GetSafeNormal();
  if (ImpactDirectionXY.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    // The impact point is at the center.
    return false;
  }

  const float DistanceToImpactXY = CenterToImpactXY.Size();
  const float DistanceToBoundaryXY = ComputeDistanceToRootCollisionBoundaryXY(ImpactDirectionXY);
  gmc_ck(DistanceToBoundaryXY > 0.f)
  // The relatively large tolerance of one millimeter is imperative here.
  const bool bImpactWithinZConstraints =
    ImpactPoint.Z >= (PawnLowerBound.Z - MAX_DISTANCE_TO_FLOOR - UU_MILLIMETER) && ImpactPoint.Z < (PawnCenter.Z + UU_MILLIMETER);
  const bool bImpactWithinXYConstraints = (DistanceToBoundaryXY + UU_MILLIMETER) > DistanceToImpactXY;
  if (!bImpactWithinZConstraints || !bImpactWithinXYConstraints)
  {
    // The impact point is not on the lower side of the collision shape.
    return false;
  }

  if (GetRootCollisionShape() == EGMC_CollisionShape::Box)
  {
    // Due to the way Unreal handles box collisions we assume the threshold to be either just 1 (if it is actually >= 0.5) or 0 (if it is actually < 0.5). Any
    // values in between are not reliable because the impact point is not guaranteed to be exactly on the edge of the ledge. Do not check this any earlier than
    // here, the constraints for the impact point must be met.
    return TestThreshold < 0.5f;
  }

  return (DistanceToImpactXY / DistanceToBoundaryXY) > TestThreshold;
}

UPrimitiveComponent* UGMC_OrganicMovementCmp::FindActorBase_Implementation()
{
  SCOPE_CYCLE_COUNTER(STAT_FindActorBase)

  gmc_ck(!BasedMovement.IsNone())

  FHitResult Hit = CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, TraceForActorBase);

  if (Hit.bStartPenetrating)
  {
    // It is better to keep the current actor base in this case (instead of setting it to null) to prevent major corrections for the client due to temporary
    // collision issues. Do not try to resolve the collision here, the pawn must not be moved outside of the prediction logic apart from following the base
    // (relevant for relative based movement).
    const auto& PreviousBase = RelBasedMovementAux.SV_TransitionTimer >= 0.f ? RelBasedMovementAux.SV_TransitionBase.Get() : GetActorBase();
    gmc_ck(RelBasedMovementAux.SV_TransitionTimer >= 0.f ? BasedMovement.IsRelative() : true)
    gmc_ck(RelBasedMovementAux.SV_TransitionTimer >= 0.f ? IsServerPawn() : true)
    FLog(
      VeryVerbose,
      "No new actor base could be determined (hit started in penetration), returning previous actor base \"%s\".",
      *GetObjectNameWithOuter(PreviousBase)
    )
    return Cast<UPrimitiveComponent>(PreviousBase);
  }

  if (!Hit.bBlockingHit)
  {
    FLog(VeryVerbose, "Found actor base \"Null\" (no blocking hit).")
    return nullptr;
  }

  const auto& HitComponent = Hit.GetComponent();

  if (!IsValid(HitComponent))
  {
    FLog(VeryVerbose, "Found actor base \"Null\" (hit component not valid).")
    return nullptr;
  }

  if (HitComponent->Mobility != EComponentMobility::Movable)
  {
    FLog(VeryVerbose, "Found actor base \"Null\" (hit component not movable).")
    return nullptr;
  }

  if (!IsNetMode(NM_Standalone))
  {
    if (!HitComponent->IsSupportedForNetworking())
    {
      FLog(VeryVerbose, "Found actor base \"Null\" (hit component not supported for networking).")
      return nullptr;
    }

    const auto& HitComponentOwner = Hit.GetActor();
    if (IsValid(HitComponentOwner) && !HitComponentOwner->IsSupportedForNetworking())
    {
      FLog(VeryVerbose, "Found actor base \"Null\" (hit component owner not supported for networking).")
      return nullptr;
    }
  }

  const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();

  if (BasedMovement.GetSurfacesBelowPawnOnly() && Hit.ImpactPoint.Z >= CurrentLocation.Z)
  {
    FLog(VeryVerbose, "Found actor base \"Null\" (impact point not under the lower part of the root collision and \"SurfacesBelowPawnOnly\" is true).")
    return nullptr;
  }

  if (BasedMovement.GetWalkableSurfacesOnly() && !HitWalkableFloor(Hit))
  {
    FLog(VeryVerbose, "Found actor base \"Null\" (hit component not walkable and \"bWalkableSurfacesOnly\" is true).")
    return nullptr;
  }

  const auto GetBaseComponent = [&](bool bUseAttachParentAsBase)
  {
    gmc_ck(IsValid(HitComponent))

    if (bUseAttachParentAsBase)
    {
      const auto& OutermostAttachParent = GetOutermostAttachParent(HitComponent);
      gmc_ck(OutermostAttachParent)
      return Cast<UPrimitiveComponent>(OutermostAttachParent);
    }

    return HitComponent;
  };

  if (BasedMovement.GetMaxHeight() <= 0.f)
  {
    if (IsMovingOnGround())
    {
      const auto& BaseComponent = GetBaseComponent(BasedMovement.GetUseAttachParentAsBase());
      FLog(VeryVerbose, "Found actor base \"%s\".", *GetObjectNameWithOuter(BaseComponent))
      return BaseComponent;
    }

    FLog(VeryVerbose, "Found actor base \"Null\" (pawn is not grounded).")
    return nullptr;
  }

  if (!BasedMovement.GetSurfacesBelowPawnOnly() || FMath::Abs(CurrentLocation.Z - Hit.Location.Z) <= BasedMovement.GetMaxHeight())
  {
    const auto& BaseComponent = GetBaseComponent(BasedMovement.GetUseAttachParentAsBase());
    FLog(VeryVerbose, "Found actor base \"%s\".", *GetObjectNameWithOuter(BaseComponent))
    return BaseComponent;
  }

  FLog(VeryVerbose, "Found actor base \"Null\" (pawn exceeding max based movement height).")
  return nullptr;
}

FHitResult UGMC_OrganicMovementCmp::TraceForActorBase_Implementation()
{
  SCOPE_CYCLE_COUNTER(STAT_TraceForActorBase)

  if (!UpdatedComponent->GetCollisionEnabled())
  {
    return FHitResult();
  }

  return SweepRootCollisionSingleByChannel(
    FVector::DownVector,
    FMath::Clamp(BasedMovement.GetMaxHeight(), MIN_ACTOR_BASE_TRACE_LENGTH, UE_BIG_NUMBER),
    FVector::ZeroVector,
    FQuat::Identity,
    UpdatedComponent->GetCollisionObjectType()
  );
}

void UGMC_OrganicMovementCmp::HandleActorBaseChange(USceneComponent* NewBase, USceneComponent* PreviousBase, bool bSimulated)
{
  gmc_ck(NewBase != PreviousBase)

  if (IsSimulating() != bSimulated)
  {
    return;
  }

  // Since smoothed listen server pawns maintain two states at the same we want to restrict managing the prerequisite ticks to the authoritative version.
  const bool bIsSmoothedListenServerPawnSimulation = IsSimulating() && IsSmoothedListenServerPawn();

  if (!bIsSmoothedListenServerPawnSimulation)
  {
    if (CL_IsReplaying())
    {
      // The actor base is not predicted for clients when relative based movement is used.
      gmc_ck(!IsSimulating())
      return;
    }

    if (GetIterationNumber() > 1)
    {
      // Only call once per move and ignore sub-stepping (the saved base is only updated after move execution for relative based movement).
      return;
    }

    // Rollback actors are ticked internally, they must not be added as a prerequisite.

    if (PreviousBase)
    {
      if (!Cast<AGMC_RollbackActor>(PreviousBase->GetOwner()))
      {
        SetBasedMovementPrerequisiteTicks(false, PreviousBase);
      }
    }

    if (NewBase)
    {
      if (!Cast<AGMC_RollbackActor>(NewBase->GetOwner()))
      {
        SetBasedMovementPrerequisiteTicks(true, NewBase);
      }
    }

    SetBasedMovementTickGroup(NewBase);
  }

  if (bSimulated)
  {
    CALL_NATIVE_EVENT_CONDITIONAL(
      bNoBlueprintEvents, this, OnActorBaseChangedSimulated, Cast<UPrimitiveComponent>(NewBase), Cast<UPrimitiveComponent>(PreviousBase)
    );
  }
  else
  {
    CALL_NATIVE_EVENT_CONDITIONAL(
      bNoBlueprintEvents, this, OnActorBaseChanged, Cast<UPrimitiveComponent>(NewBase), Cast<UPrimitiveComponent>(PreviousBase)
    );
  }
}

void UGMC_OrganicMovementCmp::SetBasedMovementPrerequisiteTicks(bool bAdd, USceneComponent* Base)
{
  gmc_ck(Base)

  const auto& BaseOwner = Base->GetOwner();

  const TSet<UActorComponent*>& BaseOwnerComponents = [&]()
  {
    if (BaseOwner)
    {
      return BaseOwner->GetComponents();
    }

    return TSet<UActorComponent*>{};
  }();

  bAdd ? AddTickPrerequisiteComponent(Base) : RemoveTickPrerequisiteComponent(Base);

  if (BaseOwner)
  {
    bAdd ? AddTickPrerequisiteActor(BaseOwner) : RemoveTickPrerequisiteActor(BaseOwner);
  }

  for (UActorComponent* Component : BaseOwnerComponents)
  {
    if (Component)
    {
      bAdd ? AddTickPrerequisiteComponent(Component) : RemoveTickPrerequisiteComponent(Component);
    }
  }

  if (IsValid(GMCAggregator))
  {
    bAdd ? GMCAggregator->AddTickPrerequisiteComponent(Base) : GMCAggregator->RemoveTickPrerequisiteComponent(Base);

    if (BaseOwner)
    {
      bAdd ? GMCAggregator->AddTickPrerequisiteActor(BaseOwner) : GMCAggregator->RemoveTickPrerequisiteActor(BaseOwner);
    }

    for (UActorComponent* Component : BaseOwnerComponents)
    {
      if (Component)
      {
        bAdd ? GMCAggregator->AddTickPrerequisiteComponent(Component) : GMCAggregator->RemoveTickPrerequisiteComponent(Component);
      }
    }
  }
}

void UGMC_OrganicMovementCmp::SetBasedMovementTickGroup(USceneComponent* NewBase)
{
  if (NewBase)
  {
    if (NewBase->IsSimulatingPhysics())
    {
      // If the base is simulating physics, it is preferable that the movement only runs after the physics were updated. Otherwise the pawn and the base will
      // always be offset by one frame (which may or may not be noticeable).

      SetTickGroup(TG_PostPhysics);

      if (IsValid(GMCAggregator))
      {
        GMCAggregator->SetTickGroup(TG_PostPhysics);
      }
    }
    else
    {
      SetTickGroup(TG_PrePhysics);

      if (IsValid(GMCAggregator))
      {
        GMCAggregator->SetTickGroup(TG_PrePhysics);
      }
    }
  }
  else
  {
    SetTickGroup(TG_PrePhysics);

    if (IsValid(GMCAggregator))
    {
      GMCAggregator->SetTickGroup(TG_PrePhysics);
    }
  }
}

float UGMC_OrganicMovementCmp::SlideAlongSurface(
  const FVector& Delta,
  float Time,
  const FVector& Normal,
  FHitResult& Hit,
  bool bHandleImpact
)
{
  SCOPE_CYCLE_COUNTER(STAT_SlideAlongSurface)

  if (!Hit.bBlockingHit)
  {
    return 0.f;
  }

  FVector NewNormal = Normal;

  if (IsMovingOnGround())
  {
    if (Normal.Z > 0.)
    {
      if (bPreventSlopeBoosting)
      {
        // Do not push the pawn up an unwalkable surface.
        if (!HitWalkableFloor(Hit))
        {
          NewNormal = NewNormal.GetSafeNormal2D();
        }
      }
    }
    else if (Normal.Z < -UE_DOUBLE_KINDA_SMALL_NUMBER)
    {
      if (CurrentFloor.HasValidShapeData() && CurrentFloor.GetShapeDistanceToFloor() < MIN_DISTANCE_TO_FLOOR)
      {
        // Do not push the pawn down further into the floor when the impact point is on the top part of the collision shape.
        const FVector FloorNormal = CurrentFloor.ShapeHit().Normal;
        const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0. && (FloorNormal.Z < 1. - UE_DOUBLE_DELTA);
        if (bFloorOpposedToMovement)
        {
          NewNormal = FloorNormal;
        }
        NewNormal = NewNormal.GetSafeNormal2D();
      }
    }
  }

  return Super::SlideAlongSurface(Delta, Time, NewNormal, Hit, bHandleImpact);
}

void UGMC_OrganicMovementCmp::TwoWallAdjust(FVector& Delta, const FHitResult& Hit, const FVector& OldHitNormal) const
{
  SCOPE_CYCLE_COUNTER(STAT_TwoWallAdjust)

  const FVector InDelta = Delta;

  Super::TwoWallAdjust(Delta, Hit, OldHitNormal);

  if (IsMovingOnGround())
  {
    if (Delta.Z > 0.)
    {
      if (bPreventSlopeBoosting)
      {
        // Only allow the pawn to slide up walkable surfaces. Unwalkable surfaces are treated as vertical walls.
        if ((Hit.Normal.Z >= WalkableFloorZ || HitWalkableFloor(Hit)) && Hit.Normal.Z > UE_DOUBLE_KINDA_SMALL_NUMBER)
        {
          // Maintain horizontal movement.
          const float Time = (1.f - Hit.Time);
          const FVector ScaledDelta = Delta.GetSafeNormal() * InDelta.Size();
          Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
          // The delta Z should never exceed the max allowed step up height so we rescale if necessary. This should be rare (the hit normal Z divisor would have
          // to be very small) but we'd rather lose horizontal movement than go too high.
          const float StepUpHeight = GetMaxStepUpHeight();
          if (Delta.Z > StepUpHeight)
          {
            const float Rescale = StepUpHeight / Delta.Z;
            Delta *= Rescale;
          }
        }
        else
        {
          Delta.Z = 0.;
        }
      }
    }
    else if (Delta.Z < 0.)
    {
      if (CurrentFloor.HasValidShapeData() && CurrentFloor.GetShapeDistanceToFloor() < MIN_DISTANCE_TO_FLOOR)
      {
        // Do not push the pawn further down into the floor.
        Delta.Z = 0.;
      }
    }
  }
}

FVector UGMC_OrganicMovementCmp::ComputeSlideVector(
  const FVector& Delta,
  float Time,
  const FVector& Normal,
  const FHitResult& Hit
) const
{
  FVector Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

  if (bPreventSlopeBoosting && IsAirborne() && IsAffectedByGravity())
  {
    Result = PreventAirborneSlopeBoosting(Result, Delta, Time, Normal, Hit);
  }

  return Result;
}

FVector UGMC_OrganicMovementCmp::PreventAirborneSlopeBoosting(
  const FVector& SlideResult,
  const FVector& Delta,
  float Time,
  const FVector& Normal,
  const FHitResult& Hit
) const
{
  FVector Result = SlideResult;
  if (Result.Z > 0.f)
  {
    // Don't move any higher than we originally intended.
    const float ZLimit = Delta.Z * Time;
    if (Result.Z - ZLimit > UE_DOUBLE_KINDA_SMALL_NUMBER)
    {
      if (ZLimit > 0.f)
      {
        // Rescale the entire vector (not just the Z-component), otherwise we would change the direction and most likely head right back into the impact.
        const float UpPercent = ZLimit / Result.Z;
        Result *= UpPercent;
      }
      else
      {
        // We were heading downwards but would have deflected upwards, make the deflection horizontal instead.
        Result = FVector::ZeroVector;
      }
      // Adjust the remaining portion of the original slide result to be horizontal and parallel to impact normal.
      const FVector RemainderXY = (SlideResult - Result) * FVector(1., 1., 0.);
      const FVector NormalXY = Normal.GetSafeNormal2D();
      const FVector Adjust = Super::ComputeSlideVector(RemainderXY, 1.f, NormalXY, Hit);
      Result += Adjust;
    }
  }
  return Result;
}

bool UGMC_OrganicMovementCmp::UpdateFloor(
  FGMC_FloorParams& Floor,
  const FVector& Direction,
  float TraceLength,
  float Tolerance,
  float ShapeExtentScale,
  bool bAutoAdjust,
  bool bForceUpdate
)
{
  const bool bIsNavMeshWalking = IsNavMeshWalking();

  if (bIsNavMeshWalking)
  {
    // Updated manually in the nav mesh walking function.
    return true;
  }
  else if (WasNavMeshWalkingLastUpdate())
  {
    // Force a floor update if nav mesh walking was just disabled to get accurate values again.
    bForceUpdate = true;
  }

  return Super::UpdateFloor(Floor, Direction, TraceLength, Tolerance, ShapeExtentScale, bAutoAdjust, bForceUpdate);
}

void UGMC_OrganicMovementCmp::SetMovementMode(EMovementMode NewMovementMode)
{
  switch (NewMovementMode)
  {
    case MOVE_None:
      SetMovementMode(EGMC_MovementMode::None);
      return;
    case MOVE_Walking:
    case MOVE_NavWalking:
      SetMovementMode(EGMC_MovementMode::Grounded);
      return;
    case MOVE_Swimming:
      SetMovementMode(EGMC_MovementMode::Buoyant);
      return;
    case MOVE_Flying:
    case MOVE_Falling:
      SetMovementMode(EGMC_MovementMode::Airborne);
      return;
    case MOVE_Custom:
    case MOVE_MAX:
    default: return;
  }
}

void UGMC_OrganicMovementCmp::SetMovementMode(uint8 NewMovementMode)
{
  if (NewMovementMode != MovementMode)
  {
    uint8 PreviousMovementMode = MovementMode;
    MovementMode = NewMovementMode;
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnMovementModeChanged, static_cast<EGMC_MovementMode>(PreviousMovementMode));
  }
}

void UGMC_OrganicMovementCmp::SetMovementMode(EGMC_MovementMode NewMovementMode)
{
  SetMovementMode(static_cast<uint8>(NewMovementMode));
}

float UGMC_OrganicMovementCmp::GetMaxSpeed() const
{
  gmc_ck(MaxDesiredSpeed >= 0.f)

  if (IsServerBot())
  {
    // Get max speed for path following.
    if (bRequestedMoveWithMaxSpeed || UseAccelerationForPathFollowing())
    {
      return MaxDesiredSpeed;
    }
    return bUseRequestedVelocityMaxSpeed ? RequestedVelocity.Size() : MaxDesiredSpeed;
  }

  if (bIgnoreInputModifier)
  {
    // Modifier is being ignored.
    return MaxDesiredSpeed;
  }

  if (
    GMCCompression::IsNetNearlyZero(
      RoundInputVector(GetProcessedInputVector(), EGMC_FloatPrecisionBlueprint::TwoDecimals),
      EGMC_FloatPrecision::TwoDecimals
    )
  )
  {
    // The input vector is zero, do not apply modifier.
    return MaxDesiredSpeed;
  }

  // Calculate the modified max speed.
  gmc_ck(MaxDesiredSpeed >= MinAnalogWalkSpeed)
  return FMath::Clamp(ComputeInputModifier() * MaxDesiredSpeed, MinAnalogWalkSpeed, BIG_NUMBER);
}

bool UGMC_OrganicMovementCmp::HitWalkableFloor(const FHitResult& Hit) const
{
  if (!Hit.IsValidBlockingHit())
  {
    return false;
  }

  if (!CanStepUp(Hit))
  {
    return false;
  }

  // Check if the impact normal Z exceeds the walkable Z value.
  float WalkableZ = WalkableFloorZ;
  const UPrimitiveComponent* HitComponent = Hit.GetComponent();
  if (HitComponent)
  {
    // Hit component may override the walkable floor Z.
    const FWalkableSlopeOverride& SlopeOverride = HitComponent->GetWalkableSlopeOverride();
    WalkableZ = SlopeOverride.ModifyWalkableFloorZ(WalkableZ);
  }
  if (Hit.ImpactNormal.Z + UE_DOUBLE_KINDA_SMALL_NUMBER < WalkableZ)
  {
    if (bLandOnEdges && IsAirborne())
    {
      // For rounded collision shapes more impacts that are on hard edges will be recognized as walkable with this check.
      if (Hit.Normal.Z + UE_DOUBLE_KINDA_SMALL_NUMBER < WalkableZ)
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }

  return true;
}

void UGMC_OrganicMovementCmp::HandleImpact(const FHitResult& Impact, float TimeSlice, const FVector& MoveDelta)
{
  SCOPE_CYCLE_COUNTER(STAT_HandleImpact)

  if (!Impact.bBlockingHit)
  {
    gmc_ckne()
    return;
  }

  if (const auto& PFAgent = GetPathFollowingAgent())
  {
    PFAgent->OnMoveBlockedBy(Impact);
  }

  if (const auto& OtherPawn = Cast<APawn>(Impact.GetActor()))
  {
    NotifyBumpedPawn(OtherPawn);
  }

  if (bEnablePhysicsInteraction)
  {
    ApplyImpactPhysicsForces(Impact, GetTransientAcceleration(), Velocity);
  }
}

void UGMC_OrganicMovementCmp::SetWalkableFloorAngle(float NewWalkableFloorAngle)
{
  WalkableFloorAngle = FMath::Clamp(NewWalkableFloorAngle, 0.f, 90.0f);
  WalkableFloorZ = FMath::Cos(FMath::DegreesToRadians(WalkableFloorAngle));
}

void UGMC_OrganicMovementCmp::SetWalkableFloorZ(float NewWalkableFloorZ)
{
  WalkableFloorZ = FMath::Clamp(NewWalkableFloorZ, 0.f, 1.f);
  WalkableFloorAngle = FMath::RadiansToDegrees(FMath::Acos(WalkableFloorZ));
}

bool UGMC_OrganicMovementCmp::IsExceedingMaxSpeed(float MaxSpeed) const
{
  return Velocity.SizeSquared() > FMath::Square(FMath::Max(MaxSpeed, 0.f));
}

bool UGMC_OrganicMovementCmp::CanMove_Implementation() const
{
  return Super::CanMove_Implementation() && MovementMode != static_cast<uint8>(EGMC_MovementMode::None);
}

bool UGMC_OrganicMovementCmp::HasMoveInputEnabled() const
{
  return Super::HasMoveInputEnabled() && MovementMode != static_cast<uint8>(EGMC_MovementMode::None);
}

float UGMC_OrganicMovementCmp::GetMaxStepUpHeight() const
{
  if (IsNavMeshWalking())
  {
    const auto& NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    const float AgentStepHeight = GetAgentStepHeight(NavSystem);
    if (AgentStepHeight >= 0.f)
    {
      return AgentStepHeight;
    }
  }

  return MaxStepUpHeight;
}

float UGMC_OrganicMovementCmp::GetMaxStepDownHeight() const
{
  if (IsNavMeshWalking())
  {
    const auto& NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    const float AgentStepHeight = GetAgentStepHeight(NavSystem);
    if (AgentStepHeight >= 0.f)
    {
      return AgentStepHeight;
    }
  }

  return MaxStepDownHeight;
}

void UGMC_OrganicMovementCmp::OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode)
{
  SCOPE_CYCLE_COUNTER(STAT_OnMovementModeChanged)

  FLog(
    VeryVerbose,
    "Movement mode changed from %s to %s.",
    *GetMovementModeAsString(static_cast<uint8>(PreviousMovementMode)),
    *GetMovementModeAsString(MovementMode)
  )

  if (IsMovingOnGround() && PreviousMovementMode == EGMC_MovementMode::Airborne)
  {
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnLanded, LandingVelocity);
    if (const auto& PFAgent = GetPathFollowingAgent()) PFAgent->OnLanded();
  }

  if (IsAirborne() && PreviousMovementMode == EGMC_MovementMode::Grounded)
  {
    if (CurrentFloor.HasValidShapeData())
    {
      const auto& Base = Cast<UPrimitiveComponent>(GetActorBase());
      if (CanImpartVelocityFromBase(Base))
      {
        const FVector VelocityToImpart = GetImpartVelocity(Base, BasedMovement.GetImpartLinearVelocity(), BasedMovement.GetImpartTangentialVelocity());
        AddImpulse(
          CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostProcessVelocityToImpart, Base, VelocityToImpart),
          !BasedMovement.GetConsiderMassOnImpartVelocity()
        );
      }
    }
  }
}

bool UGMC_OrganicMovementCmp::CanImpartVelocityFromBase(UPrimitiveComponent* CurrentBase) const
{
  if (!IsValid(CurrentBase))
  {
    return false;
  }

  if (!IsMovable(CurrentBase))
  {
    return false;
  }

  if (CurrentBase->IsSimulatingPhysics())
  {
    // Do not impart the velocity of the movement base if it is simulating physics (can cause the pawn to receive a very large/unrealistic impulse, especially
    // from small objects).
    return false;
  }

  return true;
}

FVector UGMC_OrganicMovementCmp::GetPenetrationAdjustment(const FHitResult& Hit) const
{
  if (!Hit.bStartPenetrating)
  {
    return FVector::ZeroVector;
  }

  FHitResult AdjustedHit = Hit;

  AdjustedHit.Normal = RoundBasedVectorByRotation(AdjustedHit.Normal, GetRelativeActorBase(), EGMC_FloatPrecisionBlueprint::TwoDecimals).GetSafeNormal();

  // Do not call UGMC_MovementUtilityCmp::GetPenetrationAdjustment here (does not use based rounding).
  FVector Result = UMovementComponent::GetPenetrationAdjustment(AdjustedHit);
  Result = Result.GetSafeNormal() * (Result.Size() + PENETRATION_ADJUSTMENT_TOLERANCE);

  float MaxDistance = MaxDepenetrationWithGeometry;
  if (Cast<APawn>(AdjustedHit.GetActor())) MaxDistance = MaxDepenetrationWithPawn;
  Result = Result.GetClampedToMaxSize(MaxDistance);
  
  return Result;
}

float UGMC_OrganicMovementCmp::GetGravityZ() const
{
  gmc_ck(GravityScale >= 0.f)
  const float GravityZ = FMath::Clamp(UMovementComponent::GetGravityZ(), -UE_BIG_NUMBER, 0.f) * GravityScale;
  return GravityZ;
}

FVector UGMC_OrganicMovementCmp::GetGravity() const
{
  return {0., 0., GetGravityZ()};
}

void UGMC_OrganicMovementCmp::HaltMovement()
{
  Super::HaltMovement();

  bReceivedUpwardForce = false;

  if (MontageTracker.HasActiveRootMotionMontage())
  {
    MontageTracker.ClearActiveMontage();
  }
  RootMotionParams.Clear();
  bHasRootMotion = false;

  RequestedVelocity = FVector::ZeroVector;
  bRequestedMoveWithMaxSpeed = false;
  bIsUsingAvoidanceInternal = false;
  ResetAvoidanceData();
}

void UGMC_OrganicMovementCmp::DisableMovement()
{
  HaltMovement();
  SetMovementMode(EGMC_MovementMode::None);
}

void UGMC_OrganicMovementCmp::RootCollisionTouched(
  UPrimitiveComponent* OverlappedComponent,
  AActor* OtherActor,
  UPrimitiveComponent* OtherComponent,
  int32 OtherBodyIndex,
  bool bFromSweep,
  const FHitResult& SweepResult
)
{
  if (!bEnablePhysicsInteraction || !ShouldApplyPhysicsInteraction(OtherComponent) || !OtherComponent->IsAnySimulatingPhysics())
  {
    return;
  }

  gmc_ck(OtherComponent)

  FName BoneName = NAME_None;
  if (OtherBodyIndex != INDEX_NONE)
  {
    const auto& OtherSkinnedMeshComponent = Cast<USkinnedMeshComponent>(OtherComponent);
    gmc_ck(OtherSkinnedMeshComponent)
    BoneName = OtherSkinnedMeshComponent->GetBoneName(OtherBodyIndex);
  }

  float TouchForceFactorModified = TouchForceScale;
  if (bScaleTouchForceToMass)
  {
    const auto& BodyInstance = OtherComponent->GetBodyInstance(BoneName);
    TouchForceFactorModified *= BodyInstance ? BodyInstance->GetBodyMass() : 1.f;
  }

  float ImpulseStrength = FMath::Clamp(
    Velocity.Size2D() * TouchForceFactorModified,
    MinTouchForce > 0.f ? MinTouchForce : -FLT_MAX,
    MaxTouchForce > 0.f ? MaxTouchForce : FLT_MAX
  );

  const FVector OtherComponentLocation = OtherComponent->GetComponentLocation();
  const FVector ShapeLocation = UpdatedComponent->GetComponentLocation();
  FVector ImpulseDirection =
    FVector(OtherComponentLocation.X - ShapeLocation.X, OtherComponentLocation.Y - ShapeLocation.Y, 0.25).GetSafeNormal();
  ImpulseDirection = (ImpulseDirection + Velocity.GetSafeNormal2D()) * 0.5;
  ImpulseDirection.Normalize();

  OtherComponent->AddImpulse(ImpulseDirection * ImpulseStrength, BoneName);
}

void UGMC_OrganicMovementCmp::ApplyImpactPhysicsForces(const FHitResult& Impact, const FVector& ImpactAcceleration, const FVector& ImpactVelocity)
{
  gmc_ck(bEnablePhysicsInteraction)

  if (!Impact.IsValidBlockingHit())
  {
    return;
  }

  const auto& ImpactComponent = Impact.GetComponent();
  if (!ShouldApplyPhysicsInteraction(ImpactComponent))
  {
    return;
  }

  gmc_ck(ImpactComponent)

  const auto& BodyInstance = ImpactComponent->GetBodyInstance(Impact.BoneName);
  if (!BodyInstance || !BodyInstance->IsInstanceSimulatingPhysics())
  {
    return;
  }

  FVector ForceLocation = Impact.ImpactPoint;
  if (bUsePushForceZOffset)
  {
    FVector Center, Extents;
    BodyInstance->GetBodyBounds().GetCenterAndExtents(Center, Extents);
    if (!Extents.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
    {
      ForceLocation.Z = Center.Z + Extents.Z * PushForceZOffsetFactor;
    }
  }

  float PushForceModifier = 1.f;
  const FVector ComponentVelocity = ImpactComponent->GetPhysicsLinearVelocity();
  const FVector VirtualVelocity = ImpactAcceleration.IsZero() ? ImpactVelocity : ImpactAcceleration.GetSafeNormal() * GetMaxSpeed();

  if (bScalePushForceToVelocity && !ComponentVelocity.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    const float DotProduct = ComponentVelocity | VirtualVelocity;
    if (DotProduct > 0.f && DotProduct < 1.f)
    {
      PushForceModifier *= DotProduct;
    }
  }

  const float BodyMass = FMath::Max(BodyInstance->GetBodyMass(), 1.f);
  if (bScalePushForceToMass)
  {
    PushForceModifier *= BodyMass;
  }

  FVector ForceToApply = Impact.ImpactNormal * -1.;
  ForceToApply *= PushForceModifier;
  if (ComponentVelocity.IsNearlyZero(1.))
  {
    ForceToApply *= InitialPushForceScale;
    ImpactComponent->AddImpulseAtLocation(ForceToApply, ForceLocation, Impact.BoneName);
  }
  else
  {
    ForceToApply *= PushForceScale;
    ImpactComponent->AddForceAtLocation(ForceToApply, ForceLocation, Impact.BoneName);
  }
}

void UGMC_OrganicMovementCmp::ApplyDownwardForce(float DeltaSeconds)
{
  gmc_ck(bEnablePhysicsInteraction)

  const auto& CurrentBase = Cast<UPrimitiveComponent>(GetActorBase());

  if (!ShouldApplyPhysicsInteraction(CurrentBase) || !CurrentFloor.HasValidShapeData() || DownwardForceScale == 0.f)
  {
    return;
  }

  gmc_ck(CurrentBase)

  const FVector Gravity = GetGravity();
  if (!Gravity.IsZero() && CurrentBase->GetOwner() != PawnOwner && CurrentBase->IsAnySimulatingPhysics())
  {
    CurrentBase->AddForceAtLocation(
      Gravity * Mass * DownwardForceScale,
      CurrentFloor.ShapeHit().ImpactPoint,
      CurrentFloor.ShapeHit().BoneName
    );
  }
}

void UGMC_OrganicMovementCmp::ApplyRepulsionForce(float DeltaSeconds)
{
  gmc_ck(bEnablePhysicsInteraction)

  if (!PawnOwner || !UpdatedPrimitive || RepulsionForce <= 0.f)
  {
    return;
  }

  const TArray<FOverlapInfo>& Overlaps = UpdatedPrimitive->GetOverlapInfos();
  if (Overlaps.Num() == 0)
  {
    return;
  }

  for (int32 Index = 0; Index < Overlaps.Num(); ++Index)
  {
    const FOverlapInfo& Overlap = Overlaps[Index];

    const auto& OverlapComponent = Overlap.OverlapInfo.GetComponent();

    if (!ShouldApplyPhysicsInteraction(OverlapComponent))
    {
      continue;
    }

    gmc_ck(OverlapComponent)

    // Use the body instead of the component for cases where multi-body overlaps are enabled.
    FBodyInstance* OverlapBody = nullptr;
    const int32 OverlapBodyIndex = Overlap.GetBodyIndex();
    const auto& SkeletalMeshForBody = OverlapBodyIndex != INDEX_NONE ? Cast<USkeletalMeshComponent>(OverlapComponent) : nullptr;
    if (SkeletalMeshForBody)
    {
      OverlapBody = SkeletalMeshForBody->Bodies.IsValidIndex(OverlapBodyIndex) ? SkeletalMeshForBody->Bodies[OverlapBodyIndex] : nullptr;
    }
    else
    {
      OverlapBody = OverlapComponent->GetBodyInstance();
    }

    if (!OverlapBody)
    {
      FLog(Verbose, "%s could not find overlap-body for body index %d.", *GetName(), OverlapBodyIndex)
      continue;
    }

    if (!OverlapBody->IsInstanceSimulatingPhysics())
    {
      continue;
    }

    // Trace to get the hit location on the collision.
    FHitResult Hit{};
    FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ApplyRepulsionForce), false);
    CollisionQueryParams.bReturnFaceIndex = false;
    CollisionQueryParams.bReturnPhysicalMaterial = false;
    const FVector ShapeLocation = UpdatedPrimitive->GetComponentLocation();
    const FVector BodyLocation = OverlapBody->GetUnrealWorldTransform().GetLocation();
    bool bHasHit = UpdatedPrimitive->LineTraceComponent(
      Hit,
      BodyLocation,
      {ShapeLocation.X, ShapeLocation.Y, BodyLocation.Z},
      CollisionQueryParams
    );

    constexpr float StopBodyDistance = 2.5f;
    bool bIsPenetrating = Hit.bStartPenetrating || Hit.PenetrationDepth > StopBodyDistance;
    FVector HitLocation = Hit.ImpactPoint;

    // If we did not hit the collision, we are inside the shape.
    if (!bHasHit)
    {
      HitLocation = BodyLocation;
      bIsPenetrating = true;
    }

    const FVector BodyVelocity = OverlapBody->GetUnrealWorldVelocity();
    const float DistanceNow = (HitLocation - BodyLocation).SizeSquared2D();
    const float DistanceLater = (HitLocation - (BodyLocation + BodyVelocity * DeltaSeconds)).SizeSquared2D();

    if (bHasHit && DistanceNow < StopBodyDistance && !bIsPenetrating)
    {
      OverlapBody->SetLinearVelocity(FVector::ZeroVector, false);
    }
    else if (DistanceLater <= DistanceNow || bIsPenetrating)
    {
      FVector ForceCenter = ShapeLocation;

      if (bHasHit)
      {
        ForceCenter.Z = HitLocation.Z;
      }
      else
      {
        float ShapeHalfHeight = GetRootCollisionHalfHeight(true);
        ForceCenter.Z = FMath::Clamp(BodyLocation.Z, ShapeLocation.Z - ShapeHalfHeight, ShapeLocation.Z + ShapeHalfHeight);
      }

      constexpr float ToleranceFactor = 1.2f;
      const float RepulsionForceRadius = ComputeDistanceToRootCollisionBoundaryXY(BodyLocation - ShapeLocation) * ToleranceFactor;
      OverlapBody->AddRadialForceToBody(ForceCenter, RepulsionForceRadius, RepulsionForce * Mass, ERadialImpulseFalloff::RIF_Constant);
    }
  }
}

bool UGMC_OrganicMovementCmp::ShouldApplyPhysicsInteraction(UPrimitiveComponent* Component)
{
  if (!IsValid(Component))
  {
    return false;
  }

  if (!IsMovable(Component))
  {
    return false;
  }

  if (!BasedMovement.IsNone() && BasedMovement.GetNoPhysicsInteractionWithBase())
  {
    // Do not apply any physics interaction if the component is or is attached to our current movement base.

    const auto& CurrentBase = GetActorBase();

    if (CurrentBase)
    {
      if (CurrentBase == Component)
      {
        return false;
      }

      USceneComponent* AttachParent = Component;
      while (true)
      {
        AttachParent = AttachParent->GetAttachParent();
        if (AttachParent)
        {
          if (AttachParent == CurrentBase)
          {
            return false;
          }
        }
        else
        {
          break;
        }
      }
    }
  }

  return true;
}

const FGMC_FloorParams& UGMC_OrganicMovementCmp::GetCurrentFloor() const
{
  return CurrentFloor;
}

void UGMC_OrganicMovementCmp::SetSkeletalMeshReference(USkeletalMeshComponent* Mesh)
{
  if (SkeletalMesh && SkeletalMesh != Mesh)
  {
    SkeletalMesh->RemoveTickPrerequisiteComponent(this);

    if (IsValid(GMCAggregator))
    {
      GMCAggregator->UnregisterMeshComponent(SkeletalMesh);
    }
  }

  SkeletalMesh = Mesh;

  if (SkeletalMesh)
  {
    SkeletalMesh->AddTickPrerequisiteComponent(this);

    if (IsValid(GMCAggregator))
    {
      GMCAggregator->RegisterMeshComponent(SkeletalMesh);
    }
  }
}

USkeletalMeshComponent* UGMC_OrganicMovementCmp::GetSkeletalMeshReference() const
{
  return SkeletalMesh;
}

void UGMC_OrganicMovementCmp::BlockSkeletalMeshPoseTick() const
{
  if (!SkeletalMesh) return;
  SkeletalMesh->bIsAutonomousTickPose = false;
  SkeletalMesh->bOnlyAllowAutonomousTickPose = true;
}

void UGMC_OrganicMovementCmp::EnableSkeletalMeshPoseTick() const
{
  if (!SkeletalMesh) return;
  SkeletalMesh->bIsAutonomousTickPose = false;
  SkeletalMesh->bOnlyAllowAutonomousTickPose = false;
}

void UGMC_OrganicMovementCmp::SetMontageStartDelegate(FGMC_OnMontageStart& InOnMontageStart, const FGMC_MontageTracker& InMontageTracker)
{
  if (InMontageTracker.HasActiveMontage())
  {
    MontageStartDelegate = InOnMontageStart;
    return;
  }

  gmc_ckne()
}

void UGMC_OrganicMovementCmp::SetMontageBlendInDelegate(FGMC_OnMontageBlendInComplete& InOnMontageBlendIn, const FGMC_MontageTracker& InMontageTracker)
{
  if (InMontageTracker.HasActiveMontage())
  {
    MontageBlendInDelegate = InOnMontageBlendIn;
    return;
  }

  gmc_ckne()
}

void UGMC_OrganicMovementCmp::SetMontageBlendOutDelegate(FGMC_OnMontageBlendOutBegin& InOnMontageBlendOut, const FGMC_MontageTracker& InMontageTracker)
{
  if (InMontageTracker.HasActiveMontage())
  {
    MontageBlendOutDelegate = InOnMontageBlendOut;
    return;
  }

  gmc_ckne()
}

void UGMC_OrganicMovementCmp::SetMontageNotifyBeginDelegate(FGMC_OnMontageNotifyBegin& InOnMontageNotifyBegin, const FGMC_MontageTracker& InMontageTracker)
{
  if (InMontageTracker.HasActiveMontage())
  {
    MontageNotifyBeginDelegate = InOnMontageNotifyBegin;
    return;
  }

  gmc_ckne()
}

void UGMC_OrganicMovementCmp::SetMontageNotifyEndDelegate(FGMC_OnMontageNotifyEnd& InOnMontageNotifyEnd, const FGMC_MontageTracker& InMontageTracker)
{
  if (InMontageTracker.HasActiveMontage())
  {
    MontageNotifyEndDelegate = InOnMontageNotifyEnd;
    return;
  }

  gmc_ckne()
}

void UGMC_OrganicMovementCmp::SetMontageCompleteDelegate(FGMC_OnMontageComplete& InOnMontageComplete, const FGMC_MontageTracker& InMontageTracker)
{
  if (InMontageTracker.HasActiveMontage())
  {
    MontageCompleteDelegate = InOnMontageComplete;
    return;
  }

  gmc_ckne()
}

void UGMC_OrganicMovementCmp::MontageUpdate(float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_MontageUpdate)

  // Always reset the flag at this point.
  bHasRootMotion = false;

  if (!SkeletalMesh || !MontageTracker.HasActiveMontage())
  {
    MontageTracker.ClearActiveMontage();
    RootMotionParams.Clear();
    return;
  }

  if (MontageTracker.bMontagePaused)
  {
    FLog(
      VeryVerbose,
      "Active montage is paused, current parameters are: "
      "Montage = %s | MontagePosition = %f / %f | MontagePlayRate = %f",
      *MontageTracker.Montage->GetName(),
      MontageTracker.MontagePosition,
      MontageTracker.Montage->GetPlayLength(),
      MontageTracker.MontagePlayRate
    )
    return;
  }

  FLog(
    VeryVerbose,
    "Advancing active montage, current parameters are: "
    "Montage = %s | MontagePosition = %f / %f | MontagePlayRate = %f",
    *MontageTracker.Montage->GetName(),
    MontageTracker.MontagePosition,
    MontageTracker.Montage->GetPlayLength(),
    MontageTracker.MontagePlayRate
  )

  FGMC_AnimMontageInstance MontageInstance{MontageTracker.Montage, MontageTracker.MontagePosition, MontageTracker.MontagePlayRate, true};

  bool bFinishedBlendIn = false;
  bool bStartedBlendOut = false;

  TArray<const FAnimNotifyEvent*> MontageNotifyBeginEvents{};
  TArray<const FAnimNotifyEvent*> MontageNotifyEndEvents{};

  {
    SCOPE_CYCLE_COUNTER(STAT_AdvanceActiveMontage)

    gmc_ck(MontageTracker.HasActiveMontage())
    gmc_ck(!MontageTracker.bMontageEnded)
    gmc_ck(!MontageTracker.bMontagePaused)

    MontageTracker.bMontageEnded = MontageInstance.Advance(
      DeltaSeconds,
      RootMotionParams,
      bFinishedBlendIn,
      bStartedBlendOut,
      MontageNotifyBeginEvents,
      MontageNotifyEndEvents
    );

    MontageTracker.MontagePosition = MontageInstance.GetPosition();
  }

  gmc_ck(MontageTracker.MontagePosition >= 0.)
  gmc_ck(MontageTracker.MontagePosition <= MontageTracker.Montage->GetPlayLength())

  FLog(
    VeryVerbose,
    "Active montage was advanced, the updated parameters are: "
    "Montage = %s | MontagePosition = %f / %f | MontagePlayRate = %f",
    MontageTracker.Montage ? *MontageTracker.Montage->GetName() : TEXT("None"),
    MontageTracker.MontagePosition,
    MontageTracker.Montage ? MontageTracker.Montage->GetPlayLength() : 0.f,
    MontageTracker.MontagePlayRate
  )

  FGMC_RootMotionExtractionSettings ExtractionSettings = GetRootMotionExtractionMetaData(MontageTracker.Montage);

  ProcessRootMotionMetaData(MontageInstance, RootMotionParams, ExtractionSettings, DeltaSeconds);

  if (RootMotionParams.bHasRootMotion)
  {
    bHasRootMotion = true;

    // Processing of the root motion parameters before the final velocity is calculated.
    PreProcessRootMotion(MontageInstance, RootMotionParams, DeltaSeconds);

    // Calculate the root motion velocity from the world space root motion translation.
    CalculateAnimRootMotionVelocity(ExtractionSettings, DeltaSeconds);

    // Apply the root motion rotation now. Translation is applied with the next update from the calculated velocity. Splitting root motion translation and
    // rotation up like this may not be optimal but the alternative is to replicate the rotation for replay which is far more undesirable.
    ApplyAnimRootMotionRotation(ExtractionSettings, DeltaSeconds);
  }

  RootMotionParams.Clear();

  CallMontageEvents(MontageTracker, bFinishedBlendIn, bStartedBlendOut, MontageNotifyBeginEvents, MontageNotifyEndEvents, DeltaSeconds);

  gmc_ck(!MontageTracker.bStartedNewMontage)
  gmc_ck(!MontageTracker.bMontageEnded)
  gmc_ck(!MontageTracker.bInterruptedPreviousMontage)
}

FGMC_RootMotionExtractionSettings UGMC_OrganicMovementCmp::GetRootMotionExtractionMetaData(UAnimMontage* Montage) const
{
  SCOPE_CYCLE_COUNTER(STAT_GetRootMotionExtractionMetaData)

  FGMC_RootMotionExtractionSettings RootMotionExtractionSettings{};

  if (!IsValid(Montage))
  {
    return RootMotionExtractionSettings;
  }

  if (const auto& GMCMetaData = GetGMCMontageMetaData(Montage))
  {
    RootMotionExtractionSettings = GMCMetaData->RootMotionExtraction;
  }

  return RootMotionExtractionSettings;
}

FGMC_RootMotionVelocitySettings UGMC_OrganicMovementCmp::GetRootMotionVelocityMetaData(UAnimMontage* Montage) const
{
  SCOPE_CYCLE_COUNTER(STAT_GetRootMotionVelocityMetaData)

  FGMC_RootMotionVelocitySettings RootMotionVelocitySettings{};

  if (!IsValid(Montage))
  {
    return RootMotionVelocitySettings;
  }

  if (const auto& GMCMetaData = GetGMCMontageMetaData(Montage))
  {
    RootMotionVelocitySettings = GMCMetaData->RootMotionVelocity;
  }

  return RootMotionVelocitySettings;
}

FGMC_SimulationSettings UGMC_OrganicMovementCmp::GetSimulationMetaData(UAnimMontage* Montage) const
{
  SCOPE_CYCLE_COUNTER(STAT_GetSimulationMetaData)

  FGMC_SimulationSettings SimulationSettings{};

  if (!IsValid(Montage))
  {
    return SimulationSettings;
  }

  if (const auto& GMCMetaData = GetGMCMontageMetaData(Montage))
  {
    SimulationSettings = GMCMetaData->Simulation;
  }

  return SimulationSettings;
}

void UGMC_OrganicMovementCmp::ProcessRootMotionMetaData(
  const FGMC_AnimMontageInstance& MontageInstance,
  FRootMotionMovementParams& InOutRootMotionParams,
  const FGMC_RootMotionExtractionSettings& ExtractionSettings,
  float DeltaSeconds
)
{
  SCOPE_CYCLE_COUNTER(STAT_PreProcessRootMotion)

  if (!InOutRootMotionParams.bHasRootMotion)
  {
    return;
  }

  if (ExtractionSettings.bDisableRootMotionDuringBlendIn || ExtractionSettings.bDisableRootMotionDuringBlendOut)
  {
    bool bIsBlendingIn{false};
    bool bIsBlendingOut{false};
    IsBlendingAtPosition(MontageInstance, bIsBlendingIn, bIsBlendingOut);

    if (ExtractionSettings.bDisableRootMotionDuringBlendIn)
    {
      if (bIsBlendingIn)
      {
        InOutRootMotionParams.Clear();
        FLog(VeryVerbose, "Extracted root motion was discarded for blend in (actuating meta data: %s).", TO_STR(bDisableRootMotionDuringBlendIn))
      }
    }

    if (ExtractionSettings.bDisableRootMotionDuringBlendOut)
    {
      if (bIsBlendingOut)
      {
        InOutRootMotionParams.Clear();
        FLog(VeryVerbose, "Extracted root motion was discarded for blend out (actuating meta data: %s).", TO_STR(bDisableRootMotionDuringBlendOut))
      }
    }
  }
}

void UGMC_OrganicMovementCmp::PreProcessRootMotion(
  const FGMC_AnimMontageInstance& MontageInstance,
  FRootMotionMovementParams& InOutRootMotionParams,
  float DeltaSeconds
)
{
  // The root motion translation can be scaled by the user.
  InOutRootMotionParams.ScaleRootMotionTranslation(GetAnimRootMotionTranslationScale());

  // The root motion transform must be saved in world space before continuing.
  InOutRootMotionParams.Set(SkeletalMesh->ConvertLocalRootMotionToWorld(InOutRootMotionParams.GetRootMotionTransform()));
}

UGMC_MontageMetaData* UGMC_OrganicMovementCmp::GetGMCMontageMetaData(UAnimMontage* Montage) const
{
  if (!IsValid(Montage))
  {
    return nullptr;
  }

  const auto& AllMetaData = Montage->GetMetaData();
  for (const auto& MetaData : AllMetaData)
  {
    if (const auto& GMCMetaData = Cast<UGMC_MontageMetaData>(MetaData))
    {
      return GMCMetaData;
    }
  }

  return nullptr;
}

void UGMC_OrganicMovementCmp::CallMontageEvents(
  FGMC_MontageTracker& InOutMontageTracker,
  bool bFinishedBlendIn,
  bool bStartedBlendOut,
  const TArray<const FAnimNotifyEvent*>& MontageNotifyBeginEvents,
  const TArray<const FAnimNotifyEvent*>& MontageNotifyEndEvents,
  float DeltaSeconds
)
{
  SCOPE_CYCLE_COUNTER(STAT_CallMontageEvents)

  const auto& ActiveMontage = InOutMontageTracker.Montage;

  if (!IsValid(ActiveMontage))
  {
    gmc_ckne()
    return;
  }

  const float CurrentPosition = InOutMontageTracker.MontagePosition;
  const float CurrentPlayRate = InOutMontageTracker.MontagePlayRate;
  const float MontageLength = ActiveMontage->GetPlayLength();

  gmc_ck(CurrentPosition >= 0.f)
  gmc_ck(CurrentPosition <= MontageLength)
  gmc_ck(CurrentPosition == MontageLength ? InOutMontageTracker.bMontageEnded : true)

  if (InOutMontageTracker.bStartedNewMontage)
  {
    OnMontageStarted(ActiveMontage, CurrentPosition, CurrentPlayRate, InOutMontageTracker.bInterruptedPreviousMontage, DeltaSeconds);
    MontageStartDelegate.ExecuteIfBound();
    InOutMontageTracker.bStartedNewMontage = false;
    InOutMontageTracker.bMontageEnded = false;
    InOutMontageTracker.bInterruptedPreviousMontage = false;
    CL_DoNotCombineNextMove();
  }

  if (bFinishedBlendIn)
  {
    OnMontageBlendIn(ActiveMontage, CurrentPosition, CurrentPlayRate, DeltaSeconds);
    MontageBlendInDelegate.ExecuteIfBound();
    CL_DoNotCombineNextMove();
  }

  if (bStartedBlendOut)
  {
    OnMontageBlendOut(ActiveMontage, CurrentPosition, CurrentPlayRate, DeltaSeconds);
    MontageBlendOutDelegate.ExecuteIfBound();
    CL_DoNotCombineNextMove();
  }

  const auto GetNotifyName = [](const FAnimNotifyEvent* NotifyEvent)
  {
    if (const auto& Notify = NotifyEvent->Notify)
    {
      gmc_ck(Cast<UAnimNotify_PlayMontageNotify>(Notify))
      return FName(Notify->GetNotifyName());
    }

    if (const auto& NotifyState = NotifyEvent->NotifyStateClass)
    {
      gmc_ck(Cast<UAnimNotify_PlayMontageNotifyWindow>(NotifyState))
      return FName(NotifyState->GetNotifyName());
    }

    gmc_ckne()
    return FName{};
  };

  for (const auto& Event : MontageNotifyBeginEvents)
  {
    gmc_ck(static_cast<bool>(Event->Notify) ^ static_cast<bool>(Event->NotifyStateClass))
    const auto& NotifyName = GetNotifyName(Event);
    OnMontageNotifyBegin(NotifyName, ActiveMontage, CurrentPosition, CurrentPlayRate, DeltaSeconds);
    MontageNotifyBeginDelegate.ExecuteIfBound(NotifyName);
    CL_DoNotCombineNextMove();
  }

  for (const auto& Event : MontageNotifyEndEvents)
  {
    gmc_ck(!Event->Notify)
    gmc_ck(Event->NotifyStateClass)
    const auto& NotifyName = GetNotifyName(Event);
    OnMontageNotifyEnd(NotifyName, ActiveMontage, CurrentPosition, CurrentPlayRate, DeltaSeconds);
    MontageNotifyEndDelegate.ExecuteIfBound(NotifyName);
    CL_DoNotCombineNextMove();
  }

  if (InOutMontageTracker.bMontageEnded)
  {
    OnMontageCompleted(ActiveMontage, CurrentPosition, CurrentPlayRate, DeltaSeconds);
    MontageCompleteDelegate.ExecuteIfBound();
    InOutMontageTracker.ClearActiveMontage();
    CL_DoNotCombineNextMove();
  }
}

void UGMC_OrganicMovementCmp::CL_CheckMontageStatusAfterReplay(const FGMC_MontageTracker& PreReplayMontageTracker)
{
  if (!SkeletalMesh)
  {
    return;
  }

  if (!MontageTracker.Montage && !PreReplayMontageTracker.Montage)
  {
    // No montage was active before the replay and none is now.
    return;
  }

  const auto& AnimInstance = SkeletalMesh->GetAnimInstance();
  if (!IsValid(AnimInstance))
  {
    return;
  }

  if (MontageTracker.Montage != PreReplayMontageTracker.Montage)
  {
    if (MontageTracker.Montage)
    {
      // A new montage is active now after the replay.
      const float MontageLength = Montage_PlayWithBlendIn(
        AnimInstance,
        MontageTracker.Montage,
        FAlphaBlendArgs(MontageReplication.MontagePrediction.CorrectionBlendTime),
        MontageTracker.MontagePlayRate,
        EMontagePlayReturnType::MontageLength,
        MontageTracker.MontagePosition,
        true
      );
      gmc_ck(MontageLength > 0.f)
      return;
    }

    if (PreReplayMontageTracker.Montage)
    {
      // There was a montage active before the replay, but none is now after the replay.
      gmc_ck(!MontageTracker.Montage)
      Montage_Stop(AnimInstance, MontageReplication.MontagePrediction.CorrectionBlendTime, nullptr/*stop all active montages*/);
      return;
    }

    gmc_ckne()
    return;
  }

  // The same montage from before the replay is still playing now.

  if (MontageTracker.MontagePosition != PreReplayMontageTracker.MontagePosition)
  {
    // The position of the montage has changed.
    Montage_SetPosition(AnimInstance, MontageTracker.Montage, MontageTracker.MontagePosition);
  }

  if (MontageTracker.MontagePlayRate != PreReplayMontageTracker.MontagePlayRate)
  {
    // The play rate of the montage has changed.
    Montage_SetPlayRate(AnimInstance, MontageTracker.Montage, MontageTracker.MontagePlayRate);
  }

  if (MontageTracker.bMontagePaused != PreReplayMontageTracker.bMontagePaused)
  {
    if (MontageTracker.bMontagePaused)
    {
      // The montage wasn't paused before the replay but is now.
      Montage_Pause(AnimInstance, MontageTracker.Montage);
      return;
    }

    if (PreReplayMontageTracker.bMontagePaused)
    {
      // The montage was paused before the replay but isn't now.
      gmc_ck(!MontageTracker.bMontagePaused)
      Montage_Resume(AnimInstance, MontageTracker.Montage);
      return;
    }

    gmc_ckne()
    return;
  }
}

void UGMC_OrganicMovementCmp::PlayMontageSimulated(
  USkeletalMeshComponent* Mesh,
  UAnimMontage* MontageToPlay,
  float DeltaSeconds,
  float StartPosition,
  float PlayRate,
  FName StartSection,
  bool bPaused,
  bool bCancelSelfOnBlendIn,
  bool bCancelSelfOnBlendOut,
  float CancelMinPositionDiff,
  float MinRemainingRatio,
  float CompletionTimeout,
  float CompletionTolerance,
  float StopBlendTime,
  bool bStopAllMontages
)
{
  SCOPE_CYCLE_COUNTER(STAT_PlayMontageSimulated)

  if (
    !PrePlayMontageSimulated(
      Mesh,
      MontageToPlay,
      DeltaSeconds,
      StartPosition,
      bPaused,
      bCancelSelfOnBlendIn,
      bCancelSelfOnBlendOut,
      CancelMinPositionDiff,
      MinRemainingRatio,
      CompletionTimeout,
      CompletionTolerance,
      StopBlendTime
    )
  )
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  gmc_ck(IsValid(AnimInstance))

  if (Montage_Play(AnimInstance, MontageToPlay, PlayRate, EMontagePlayReturnType::MontageLength, StartPosition, bStopAllMontages) > 0.f)
  {
    if (StartSection != NAME_None)
    {
      Montage_JumpToSection(AnimInstance, StartSection, MontageToPlay);
    }

    PreviousMontageSimulated = MontageToPlay;
  }
  else
  {
    gmc_ck(!PreviousMontageSimulated)
    gmc_ck(BlockMontageTimerSimulated == 0.f)
  }
}

bool UGMC_OrganicMovementCmp::PrePlayMontageSimulated(
  USkeletalMeshComponent* Mesh,
  UAnimMontage* MontageToPlay,
  float DeltaSeconds,
  float StartPosition,
  bool bPaused,
  bool bCancelSelfOnBlendIn,
  bool bCancelSelfOnBlendOut,
  float CancelMinPositionDiff,
  float MinRemainingRatio,
  float CompletionTimeout,
  float CompletionTolerance,
  float StopBlendTime
)
{
  if (DeltaSeconds < MIN_DELTA_TIME || !IsValid(Mesh) || !IsSimulatedPawn())
  {
    PreviousMontageSimulated = nullptr;
    BlockMontageTimerSimulated = 0.f;
    EndMontageTimerSimulated = 0.f;
    return false;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!IsValid(AnimInstance))
  {
    PreviousMontageSimulated = nullptr;
    BlockMontageTimerSimulated = 0.f;
    EndMontageTimerSimulated = 0.f;
    return false;
  }

  if (!IsValid(MontageToPlay))
  {
    gmc_ck(EndMontageTimerSimulated >= 0.f)

    BlockMontageTimerSimulated = 0.f;

    if (PreviousMontageSimulated)
    {
      EndMontageTimerSimulated += DeltaSeconds;

      if (EndMontageTimerSimulated > CompletionTolerance)
      {
        if (AnimInstance->GetActiveInstanceForMontage(PreviousMontageSimulated))
        {
          const float BlendOutTime = StopBlendTime < 0.f ? PreviousMontageSimulated->GetDefaultBlendOutTime() : StopBlendTime;
          Montage_Stop(AnimInstance, BlendOutTime, PreviousMontageSimulated);
        }

        PreviousMontageSimulated = nullptr;
        EndMontageTimerSimulated = 0.f;
      }
    }
    else
    {
      EndMontageTimerSimulated = 0.f;
    }

    return false;
  }

  EndMontageTimerSimulated = 0.f;

  if (IsExtrapolating())
  {
    // Do not start a new montage when extrapolating (but keep any data as it is at this point).
    return false;
  }

  const float MontageLength = MontageToPlay->GetPlayLength();

  if (MontageLength <= 0.f || StartPosition < 0.f || StartPosition > MontageLength)
  {
    // Do not reset any data when an invalid start position is passed (should not happen).
    gmc_ckne()
    return false;
  }

  const auto HasEnoughTimeRemaining = [](float MontageToPlayLength, float StartPosition, float MinRemainingRatio)
  {
    gmc_ck(MinRemainingRatio >= 0.f)
    gmc_ck(MinRemainingRatio <= 1.f)
    if (1.f - StartPosition / MontageToPlayLength < MinRemainingRatio)
    {
      return false;
    }
    return true;
  };

  const auto& ActiveMontageInstance = GetMontageInstance(Mesh, MontageToPlay);
  if (ActiveMontageInstance && ActiveMontageInstance->IsValid())
  {
    UAnimMontage* ActiveMontage = ActiveMontageInstance->Montage;
    gmc_ck(ActiveMontage == MontageToPlay)

    const bool bIsPlaying = ActiveMontageInstance->IsPlaying();
    if (bPaused)
    {
      if (bIsPlaying)
      {
        Montage_Pause(AnimInstance, ActiveMontage);
      }
    }
    else
    {
      if (!bIsPlaying)
      {
        Montage_Resume(AnimInstance, ActiveMontage);
      }
    }

    const auto ShouldCancelActiveMontage = [this](
      float StartPosition,
      float CancelMinPositionDiff,
      bool bCancelSelfOnBlendIn,
      bool bCancelSelfOnBlendOut,
      FAnimMontageInstance* ActiveMontageInstance
    )
    {
      gmc_ck(ActiveMontageInstance)
      gmc_ck(ActiveMontageInstance->IsValid())

      const auto& ActiveMontage = ActiveMontageInstance->Montage;
      const float ActiveMontagePosition = ActiveMontageInstance->GetPosition();
      const float ActiveMontagePlayRate = ActiveMontageInstance->GetPlayRate();

      if (CancelMinPositionDiff >= 0.f && FMath::Abs(ActiveMontagePosition - StartPosition) >= CancelMinPositionDiff)
      {
        return true;
      }

      if (!bCancelSelfOnBlendIn && !bCancelSelfOnBlendOut)
      {
        return false;
      }

      FGMC_AnimMontageInstance MontageInstance{ActiveMontage, ActiveMontagePosition, ActiveMontagePlayRate, true};

      bool bIsBlendingIn{false};
      bool bIsBlendingOut{false};
      IsBlendingAtPosition(MontageInstance, bIsBlendingIn, bIsBlendingOut);

      if ((bCancelSelfOnBlendIn && bIsBlendingIn) ||(bCancelSelfOnBlendOut && bIsBlendingOut))
      {
        PreviousMontageSimulated = nullptr;
        BlockMontageTimerSimulated = 0.f;
        return true;
      }

      return false;
    };

    return ShouldCancelActiveMontage(StartPosition, CancelMinPositionDiff, bCancelSelfOnBlendIn, bCancelSelfOnBlendOut, ActiveMontageInstance) ?
      HasEnoughTimeRemaining(MontageLength, StartPosition, MinRemainingRatio) : false;
  }
  else if (PreviousMontageSimulated)
  {
    BlockMontageTimerSimulated += DeltaSeconds;

    gmc_ck(MontageToPlay)
    gmc_ck(BlockMontageTimerSimulated >= 0.f)
    if (MontageToPlay == PreviousMontageSimulated && BlockMontageTimerSimulated < FMath::Max(CompletionTimeout, 0.f))
    {
      return false;
    }
  }

  PreviousMontageSimulated = nullptr;
  BlockMontageTimerSimulated = 0.f;

  return HasEnoughTimeRemaining(MontageLength, StartPosition, MinRemainingRatio);
}

void UGMC_OrganicMovementCmp::CalculateAnimRootMotionVelocity(const FGMC_RootMotionExtractionSettings& ExtractionSettings, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_CalculateAnimRootMotionVelocity)

  gmc_ck(HasRootMotion())
  gmc_ck(SkeletalMesh)

  FVector RootMotionDelta = RootMotionParams.GetRootMotionTransform().GetTranslation();

  // Ignore components with very small delta values.
  RootMotionDelta.X = FMath::Abs(RootMotionDelta.X) < 0.01 ? 0. : RootMotionDelta.X;
  RootMotionDelta.Y = FMath::Abs(RootMotionDelta.Y) < 0.01 ? 0. : RootMotionDelta.Y;
  RootMotionDelta.Z = FMath::Abs(RootMotionDelta.Z) < 0.01 ? 0. : RootMotionDelta.Z;

  FVector RootMotionVelocity = RootMotionDelta / DeltaSeconds;

  if (ExtractionSettings.bIgnoreRootMotionXY)
  {
    RootMotionVelocity.X = Velocity.X;
    RootMotionVelocity.Y = Velocity.Y;
    FLog(VeryVerbose, "Extracted XY root motion ignored (actuating meta data: %s).", TO_STR(bIgnoreRootMotionXY))
  }

  if (ExtractionSettings.bIgnoreRootMotionZ)
  {
    RootMotionVelocity.Z = Velocity.Z;
    FLog(VeryVerbose, "Extracted Z root motion ignored (actuating meta data: %s).", TO_STR(bIgnoreRootMotionZ))
  }

  UpdateVelocity(CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PostProcessAnimRootMotionVelocity, RootMotionVelocity, DeltaSeconds));

  FLog(VeryVerbose, "Applied final root motion velocity: %s", *Velocity.ToString())
}

void UGMC_OrganicMovementCmp::ApplyAnimRootMotionRotation(const FGMC_RootMotionExtractionSettings& ExtractionSettings, float DeltaSeconds)
{
  SCOPE_CYCLE_COUNTER(STAT_ApplyAnimRootMotionRotation)

  const FQuat RootMotionQuat = RootMotionParams.GetRootMotionTransform().GetRotation();
  if (RootMotionQuat.IsIdentity(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    return;
  }

  FRotator RootMotionRotator = RootMotionQuat.Rotator();
  FRotator RootMotionRotatorXY = FRotator(RootMotionRotator.Pitch, 0., RootMotionRotator.Roll);
  FRotator RootMotionRotatorZ = FRotator(0., RootMotionRotator.Yaw, 0.);

  if (FMath::Abs(RootMotionRotatorZ.Yaw) >= 0.01)
  {
    const FRotator NewRotationZ = UpdatedComponent->GetComponentRotation() + RootMotionRotatorZ;
    // Passing a rate of 0 will set the target rotation directly while still adjusting for collisions.
    if (bUseSafeRotations)
    {
      RotateYawTowardsDirectionSafe(NewRotationZ.Vector(), 0.f, DeltaSeconds);
    }
    else
    {
      RotateYawTowardsDirection(NewRotationZ.Vector(), 0.f, DeltaSeconds);
    }
  }

  const bool bPitchAboveThreshold = FMath::Abs(FRotator::NormalizeAxis(RootMotionRotatorXY.Pitch)) >= 0.01;
  const bool bRollAboveThreshold = FMath::Abs(FRotator::NormalizeAxis(RootMotionRotatorXY.Roll)) >= 0.01;
  if (bPitchAboveThreshold || bRollAboveThreshold)
  {
    if (!bPitchAboveThreshold) RootMotionRotatorXY.Pitch = 0.;
    if (!bRollAboveThreshold) RootMotionRotatorXY.Roll = 0.;
    const FRotator NewRotationXY = UpdatedComponent->GetComponentRotation() + RootMotionRotatorXY;
    // We set the roll- and pitch-rotations in a safe way but we won't get any adjustment in case of collisions.
    SetRootCollisionRotationSafe(NewRotationXY);
  }

  FLog(VeryVerbose, "Applied final root motion rotation: %s", *UpdatedComponent->GetComponentRotation().ToString())
}

void UGMC_OrganicMovementCmp::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
  RequestedVelocity = CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, NavAgentPropsAdjustment, MoveVelocity);
  bRequestedMoveWithMaxSpeed = bForceMaxSpeed;
}

void UGMC_OrganicMovementCmp::RequestPathMove(const FVector& MoveInput)
{
  FVector AdjustedMoveInput = CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, NavAgentPropsAdjustment, MoveInput);

  Super::RequestPathMove(AdjustedMoveInput);
}

FNavLocation UGMC_OrganicMovementCmp::GetLastValidTargetNavLocation() const
{
  return NavMeshWalkingAux.LastValidTargetNavLocation;
}

FVector UGMC_OrganicMovementCmp::K2_GetLastValidTargetNavLocation() const
{
  return NavMeshWalkingAux.LastValidTargetNavLocation.Location;
}

bool UGMC_OrganicMovementCmp::WasNavMeshWalkingLastUpdate() const
{
  return NavMeshWalkingAux.bWasNavMeshWalkingLastUpdate;
}

FVector UGMC_OrganicMovementCmp::GetDistanceToNavMesh(const FVector& Location) const
{
  return NavMeshWalkingAux.GetDistanceToNavMesh(Location);
}

void UGMC_OrganicMovementCmp::ResetLastValidTargetNavLocation()
{
  NavMeshWalkingAux.ResetLastValidTargetNavLocation();
}

INavigationDataInterface* UGMC_OrganicMovementCmp::GetNavData() const
{
  if (!IsValid(PawnOwner))
  {
    return nullptr;
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return nullptr;
  }

  return FNavigationSystem::GetNavDataForActor(*PawnOwner);
}

FVector UGMC_OrganicMovementCmp::NavAgentPropsAdjustment_Implementation(const FVector& NavOutput)
{
  FVector AdjustedOutput = NavOutput;

  if (!CanEverFly())
  {
    if (IsMovingOnGround() || IsAirborne()) AdjustedOutput.Z = 0.;
  }

  if (!CanEverSwim())
  {
    if (IsSwimming()) AdjustedOutput = FVector::ZeroVector;
  }

  if (!CanEverMoveOnGround())
  {
    if (IsMovingOnGround()) AdjustedOutput = FVector::ZeroVector;
  }
  else
  {
    if (IsMovingOnGround()) AdjustedOutput = NavOutput.GetSafeNormal2D() * NavOutput.Size();
  }

  return AdjustedOutput;
}

bool UGMC_OrganicMovementCmp::CanStartPathFollowing() const
{
  if (!CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CanMove) || HasRootMotion() || !UpdatedComponent || UpdatedComponent->IsSimulatingPhysics())
  {
    gmc_ck(UpdatedComponent == PawnOwner->GetRootComponent())
    return false;
  }

  return Super::CanStartPathFollowing();
}

bool UGMC_OrganicMovementCmp::CanStopPathFollowing() const
{
  return Super::CanStopPathFollowing() && !IsAirborne();
}

float UGMC_OrganicMovementCmp::GetPathFollowingBrakingDistance(float MaxSpeed) const
{
  if (bUseFixedBrakingDistanceForPaths)
  {
    return FixedPathBrakingDistance;
  }

  return CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CalculatePathFollowingBrakingDistance, MaxSpeed, GetMoveDeltaTime());
}

float UGMC_OrganicMovementCmp::CalculatePathFollowingBrakingDistance_Implementation(float MaxSpeed, float DeltaSeconds) const
{
  const float BrakingDeceleration = FMath::Max(GetBrakingDeceleration(), UE_KINDA_SMALL_NUMBER);
  return FMath::Square(MaxSpeed) / (2.f * BrakingDeceleration);
}

bool UGMC_OrganicMovementCmp::IsNavMeshWalking_Implementation() const
{
  return bUseNavMeshWalking && IsMovingOnGround() && GetNavData();
}

void UGMC_OrganicMovementCmp::NotifyBumpedPawn(APawn* BumpedPawn)
{
  Super::NotifyBumpedPawn(BumpedPawn);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
  if (const auto& World = GetWorld())
  {
    const auto& Avoidance = World->GetAvoidanceManager();
    if (Avoidance && Avoidance->IsDebugEnabled(AvoidanceUID))
    {
      DrawDebugLine(
        World,
        GetActorFeetLocation(),
        GetActorFeetLocation() + FVector(0., 0., 500.),
        AvoidanceLockTimer > 0.f ? FColor(255, 64, 64) : FColor(64, 64, 255),
        false,
        2.f,
        SDPG_MAX,
        20.f
      );
    }
  }
#endif

  // Unlock avoidance move. This mostly happens when two pawns that are locked into avoidance moves collide with each other.
  AvoidanceLockTimer = 0.f;
}

void UGMC_OrganicMovementCmp::CalculateAvoidanceVelocity(float DeltaSeconds)
{
  gmc_ck(ShouldComputeAvoidance())
  const auto& World = GetWorld();
  if (!World) return;
  const auto& AvoidanceManager = World->GetAvoidanceManager();
  if (!AvoidanceManager || AvoidanceWeight >= 1.0f) return;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
  const bool bShowDebug = AvoidanceManager->IsDebugEnabled(AvoidanceUID);
#else
  const bool bShowDebug = false;
#endif

  // Currently we adjust velocity only if we are grounded.
  if (!Velocity.IsZero() && IsMovingOnGround())
  {
    // If we are doing a locked avoidance move already skip testing.
    if (AvoidanceLockTimer > 0.f)
    {
      UpdateVelocity(AvoidanceLockVelocity, DeltaSeconds);
      if (bShowDebug)
      {
        DrawDebugLine(
          World,
          GetActorFeetLocation(),
          GetActorFeetLocation() + Velocity,
          FColor::Blue, false,
          0.5f,
          SDPG_MAX
        );
      }
    }
    else
    {
      FVector NewVelocity = AvoidanceManager->GetAvoidanceVelocityForComponent(this);

      // User-defined post-processing.
      PostProcessAvoidanceVelocity(NewVelocity);

      if (!NewVelocity.Equals(Velocity))
      {
        // We had to divert course so we lock this avoidance move in for a short time. This will make us a VO so other unlocked pawns will know that we should
        // be avoided.
        UpdateVelocity(NewVelocity, DeltaSeconds);
        SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterAvoid);
        if (bShowDebug)
        {
          DrawDebugLine(
            World,
            GetActorFeetLocation(),
            GetActorFeetLocation() + Velocity,
            FColor::Red,
            false,
            0.05f,
            SDPG_MAX,
            10.f
          );
        }
      }
      else
      {
        // Although we did not have to divert course, our velocity for this frame is decided. We will not reciprocate anything further so we should be treated
        // as a VO for the remainder of this frame.
        SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterClean);
        if (bShowDebug)
        {
          DrawDebugLine(
            World,
            GetActorFeetLocation(),
            GetActorFeetLocation() + Velocity,
            FColor::Green,
            false,
            0.05f,
            SDPG_MAX,
            10.f
          );
        }
      }
    }
    AvoidanceManager->UpdateRVO(this);
    bWasAvoidanceUpdated = true;
  }
  else if (bShowDebug)
  {
    DrawDebugLine(
      World,
      GetActorFeetLocation(),
      GetActorFeetLocation() + Velocity,
      FColor::Yellow,
      false,
      0.05f,
      SDPG_MAX
    );
  }

  if (bShowDebug)
  {
    const FVector UpLine = {0., 0., 500.};
    DrawDebugLine(
      World,
      GetActorFeetLocation(),
      GetActorFeetLocation() + UpLine,
      AvoidanceLockTimer > 0.01f ? FColor::Red : FColor::Blue,
      false,
      0.05f,
      SDPG_MAX,
      5.f
    );
  }
}

void UGMC_OrganicMovementCmp::UpdateAvoidance()
{
  gmc_ck(ShouldComputeAvoidance())

  if (const auto& World = GetWorld())
  {
    const auto& AvoidanceManager = World->GetAvoidanceManager();
    if (AvoidanceManager && !bWasAvoidanceUpdated)
    {
      AvoidanceManager->UpdateRVO(this);

      // We consider this a clean move because we didn't even try to avoid.
      SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterClean);
    }
  }
  // Reset for next iteration.
  bWasAvoidanceUpdated = false;
}

void UGMC_OrganicMovementCmp::CheckAvoidance()
{
  gmc_ck(IsServerBot())

  if (bIsUsingAvoidanceInternal != bUseAvoidance)
  {
    bIsUsingAvoidanceInternal = bUseAvoidance;
    if (bIsUsingAvoidanceInternal)
    {
      EnableRVOAvoidance();
    }
    else
    {
      ResetAvoidanceData();
    }
  }
}

void UGMC_OrganicMovementCmp::EnableRVOAvoidance()
{
  gmc_ck(bIsUsingAvoidanceInternal)

  ResetAvoidanceData();

  if (const auto& World = GetWorld())
  {
    const auto& AvoidanceManager = World->GetAvoidanceManager();
    if (AvoidanceManager)
    {
      // Initializes update timers in the avoidance manager.
      verify(AvoidanceManager->RegisterMovementComponent(this, AvoidanceWeight))
    }
  }
}

void UGMC_OrganicMovementCmp::ResetAvoidanceData()
{
  // Do not reset bIsUsingAvoidanceInternal in here.
  AvoidanceUID = 0;
  bWasAvoidanceUpdated = false;
  AvoidanceLockTimer = 0.f;
  AvoidanceLockVelocity = FVector::ZeroVector;
}

bool UGMC_OrganicMovementCmp::ShouldComputeAvoidance()
{
  // Avoidance calculations only need to run on the server.
  return bIsUsingAvoidanceInternal && IsServerBot();
}

void UGMC_OrganicMovementCmp::SetAvoidanceVelocityLock(UAvoidanceManager* Avoidance, float Duration)
{
  gmc_ck(bIsUsingAvoidanceInternal)

  Avoidance->OverrideToMaxWeight(AvoidanceUID, Duration);
  AvoidanceLockVelocity = Velocity;
  AvoidanceLockTimer = Duration;
}

void UGMC_OrganicMovementCmp::SetRVOAvoidanceUID(int32 UID)
{
  AvoidanceUID = UID;
}

int32 UGMC_OrganicMovementCmp::GetRVOAvoidanceUID()
{
  return AvoidanceUID;
}

void UGMC_OrganicMovementCmp::SetRVOAvoidanceWeight(float Weight)
{
  AvoidanceWeight = Weight;
}

float UGMC_OrganicMovementCmp::GetRVOAvoidanceWeight()
{
  return AvoidanceWeight;
}

FVector UGMC_OrganicMovementCmp::GetRVOAvoidanceOrigin()
{
  return GetActorFeetLocation();
}

float UGMC_OrganicMovementCmp::GetRVOAvoidanceRadius()
{
  // Use the largest extent that would be required to enclose the root component in a 2D circle.
  return GetMaxDistanceToRootCollisionBoundaryXY();
}

float UGMC_OrganicMovementCmp::GetRVOAvoidanceConsiderationRadius()
{
  return AvoidanceConsiderationRadius;
}

float UGMC_OrganicMovementCmp::GetRVOAvoidanceHeight()
{
  return GetRootCollisionHalfHeight(true);
}

FVector UGMC_OrganicMovementCmp::GetVelocityForRVOConsideration()
{
  return Velocity;
}

int32 UGMC_OrganicMovementCmp::GetAvoidanceGroupMask()
{
  return AvoidanceGroup.Packed;
}

int32 UGMC_OrganicMovementCmp::GetGroupsToAvoidMask()
{
  return GroupsToAvoid.Packed;
}

int32 UGMC_OrganicMovementCmp::GetGroupsToIgnoreMask()
{
  return GroupsToIgnore.Packed;
}

void UGMC_OrganicMovementCmp::SetAvoidanceGroupMask(int32 GroupFlags)
{
  AvoidanceGroup.SetFlagsDirectly(GroupFlags);
}

void UGMC_OrganicMovementCmp::SetGroupsToAvoidMask(int32 GroupFlags)
{
  GroupsToAvoid.SetFlagsDirectly(GroupFlags);
}

void UGMC_OrganicMovementCmp::SetGroupsToIgnoreMask(int32 GroupFlags)
{
  GroupsToIgnore.SetFlagsDirectly(GroupFlags);
}

void UGMC_OrganicMovementCmp::SetAvoidanceGroupMask(const FNavAvoidanceMask& GroupMask)
{
  SetAvoidanceGroupMask(GroupMask.Packed);
}

void UGMC_OrganicMovementCmp::SetGroupsToAvoidMask(const FNavAvoidanceMask& GroupMask)
{
  SetGroupsToAvoidMask(GroupMask.Packed);
}

void UGMC_OrganicMovementCmp::SetGroupsToIgnoreMask(const FNavAvoidanceMask& GroupMask)
{
  SetGroupsToIgnoreMask(GroupMask.Packed);
}

bool FGMC_BasedMovement::IsVelocity() const
{
  return Source == EGMC_BasedMovementSource::Velocity;
}

bool FGMC_BasedMovement::IsForcedRelativeSimulation() const
{
  return Source == EGMC_BasedMovementSource::Velocity && Velocity.bForceRelativeBasedMovementForSimulation;
}

bool FGMC_BasedMovement::IsRelative() const
{
  return Source == EGMC_BasedMovementSource::Relative;
}

bool FGMC_BasedMovement::IsNone() const
{
  return Source == EGMC_BasedMovementSource::None;
}

float FGMC_BasedMovement::GetMaxHeight() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.MaxHeight : IsRelative() ? Relative.MaxHeight : 0.f;
}

bool FGMC_BasedMovement::GetSurfacesBelowPawnOnly() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bSurfacesBelowPawnOnly : IsRelative() ? Relative.bSurfacesBelowPawnOnly : false;
}

bool FGMC_BasedMovement::GetWalkableSurfacesOnly() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bWalkableSurfacesOnly : IsRelative() ? Relative.bWalkableSurfacesOnly : false;
}

bool FGMC_BasedMovement::GetImpartLinearVelocity() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bImpartLinearVelocity : IsRelative() ? Relative.bImpartLinearVelocity : false;
}

bool FGMC_BasedMovement::GetImpartTangentialVelocity() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bImpartTangentialVelocity : IsRelative() ? Relative.bImpartTangentialVelocity : false;
}

bool FGMC_BasedMovement::GetConsiderMassOnImpartVelocity() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bConsiderMassOnImpartVelocity : IsRelative() ? Relative.bConsiderMassOnImpartVelocity : false;
}

bool FGMC_BasedMovement::GetUseAttachParentAsBase() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bUseAttachParentAsBase : IsRelative() ? Relative.bUseAttachParentAsBase : false;
}

bool FGMC_BasedMovement::GetIgnoreBasedPawns() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bIgnoreBasedPawns : IsRelative() ? Relative.bIgnoreBasedPawns : false;
}

bool FGMC_BasedMovement::GetNoPhysicsInteractionWithBase() const
{
  gmc_ck(!IsNone())
  return IsVelocity() ? Velocity.bNoPhysicsInteractionWithBase : IsRelative() ? Relative.bNoPhysicsInteractionWithBase : false;
}

bool FGMC_BasedMovement::GetFollowBaseRotation() const
{
  gmc_ck(IsVelocity())
  return IsVelocity() ? Velocity.bFollowBaseRotation : false;
}

bool FGMC_BasedMovement::GetForceRelativeBasedMovementForSimulation() const
{
  gmc_ck(IsVelocity())
  return IsVelocity() ? Velocity.bForceRelativeBasedMovementForSimulation : false;
}

float FGMC_BasedMovement::GetBaseChangeTransitionTime() const
{
  gmc_ck(IsRelative())
  return IsRelative() ? Relative.BaseChangeTransitionTime : 0.f;
}

bool FGMC_BasedMovement::GetSendActorBaseRotation() const
{
  gmc_ck(IsRelative())
  return IsRelative() ? Relative.bSendActorBaseRotation : false;
}

bool FGMC_BasedMovement::GetEqualizeBaseActor() const
{
  gmc_ck(IsRelative())
  return IsRelative() ? Relative.bEqualizeBaseActor : false;
}

bool FGMC_BasedMovement::GetEqualizeInPlace() const
{
  gmc_ck(IsRelative())
  return IsRelative() ? Relative.bEqualizeInPlace : false;
}

FVector FGMC_BasedMovement::GetEqualizationLocation() const
{
  gmc_ck(IsRelative())
  return IsRelative() ? Relative.EqualizationLocation : FVector::ZeroVector;
}

float FGMC_BasedMovement::GetMaxEqualizationAngle() const
{
  gmc_ck(IsRelative())
  return IsRelative() ? Relative.MaxEqualizationAngle : 0.f;
}

void FGMC_RelBasedMovementAux::PreMoveExecution(bool bLocalMove, UGMC_OrganicMovementCmp* const Outer, float DeltaSeconds)
{
  gmc_ck(Outer->BasedMovement.IsRelative())
  gmc_ck(!Outer->IsSimulating())
  gmc_ck(!Outer->CL_IsReplaying())
  gmc_ck(Outer->UpdatedComponent)
  gmc_ck(!Outer->UpdatedComponent->IsSimulatingPhysics())

  // Resolve the delta first since the base may have moved inside the pawn.
  CALL_NATIVE_EVENT_CONDITIONAL(Outer->bNoBlueprintEvents, Outer, MoveWithBaseRelative, false);

  GMC_CLOG(
    Outer->IsNetMode(NM_Client) && !IsServerAuth(Outer->ReplicationSettings.DefaultPredictionSettings.ActorBase),
    LogGMCReplication,
    Outer->PawnOwner,
    Error,
    TEXT("\"%s\" only supports server-authoritative handling of the actor base for relative based movement."),
    TO_STR(UGMC_OrganicMovementCmp)
  )

  if (Outer->IsServerPawn())
  {
    // The actor base is managed by the server exclusively.
    SV_HandleBaseTransition(Outer, DeltaSeconds);
  }

  // Resave to be able to refer to the saved base during base equalization and post move execution. Additionally, these wouldn't reflect the correct values when
  // executing a combined move so overwriting here is also safer.
  SavedTransforms.Save(false, Outer);

  // Must be saved before the base is equalized.
  if (const auto& Base = Outer->GetActorBase())
  {
    ActorBaseRotation = Base->GetComponentRotation();
  }
  else
  {
    ActorBaseRotation = FRotator::ZeroRotator;
  }

  if (Outer->BasedMovement.GetEqualizeBaseActor())
  {
    EqualizeBase(true, bLocalMove, Outer);
  }
}

void FGMC_RelBasedMovementAux::PostMoveExecution(bool bLocalMove, UGMC_OrganicMovementCmp* const Outer)
{
  gmc_ck(Outer->BasedMovement.IsRelative())
  gmc_ck(!Outer->IsSimulating())
  gmc_ck(SavedTransforms.GetSavedBase(false) == Outer->GetActorBase())
  gmc_ck(Outer->UpdatedComponent)
  gmc_ck(!Outer->UpdatedComponent->IsSimulatingPhysics())

  const auto& CurrentBase = Outer->GetActorBase();

  if (SavedTransforms.GetSavedBase(false) != CurrentBase)
  {
    GMC_LOG(
      LogGMCReplication,
      Outer->PawnOwner,
      Error,
      TEXT("The actor base must not be changed or destroyed during move execution when relative based movement is used.")
    )
    gmc_ckne()
    SavedTransforms.Reset(false);
    return;
  }

  if (Outer->BasedMovement.GetEqualizeBaseActor())
  {
    EqualizeBase(false, bLocalMove, Outer);
  }

  SavedTransforms.Save(false, Outer);

  if (!Outer->CL_IsReplaying())
  {
    SavedChangeBase = CurrentBase;
  }
}

void FGMC_RelBasedMovementAux::CL_PreReplay(UGMC_OrganicMovementCmp* const Outer)
{
  gmc_ck(Outer->BasedMovement.IsRelative())
  gmc_ck(Outer->CL_IsReplaying())
  gmc_ck(!Outer->IsSimulating())
  gmc_ck(!Outer->IsNonPredictedAutonomousProxy())
  gmc_ck(Outer->UpdatedComponent)
  gmc_ck(!Outer->UpdatedComponent->IsSimulatingPhysics())

  SavedTransforms.Save(false, Outer);

  if (Outer->BasedMovement.GetEqualizeBaseActor())
  {
    EqualizeBase(true, false, Outer);
  }
}

void FGMC_RelBasedMovementAux::CL_PostReplay(UGMC_OrganicMovementCmp* const Outer)
{
  gmc_ck(Outer->BasedMovement.IsRelative())
  gmc_ck(Outer->CL_IsReplaying())
  gmc_ck(!Outer->IsSimulating())
  gmc_ck(!Outer->IsNonPredictedAutonomousProxy())
  gmc_ck(SavedTransforms.GetSavedBase(false) == Outer->GetActorBase())
  gmc_ck(Outer->UpdatedComponent)
  gmc_ck(!Outer->UpdatedComponent->IsSimulatingPhysics())

  // Only called once after the whole replay was executed, not after every replay move.
  PostMoveExecution(false, Outer);
}

void FGMC_RelBasedMovementAux::OnCumulativeMoveInitialized(FGMC_PawnState& InputState, UGMC_OrganicMovementCmp* const Outer)
{
  gmc_ck(Outer->BasedMovement.IsRelative() || Outer->BasedMovement.IsForcedRelativeSimulation())
  gmc_ck(Outer->IsSimulating())
  gmc_ck(!Outer->IsPredictedAutonomousProxy())
  gmc_ck(Outer->InterpolationMode == EGMC_InterpolationMode::CumulativeSimulation || Outer->ExtrapolationMode == EGMC_ExtrapolationMode::CumulativeSimulation)

  const auto& CurrentBase = Outer->GetActorBase();

  const auto& SavedBase = SavedTransforms.GetSavedBase(true);

  if (!CurrentBase || !SavedBase || CurrentBase != SavedBase)
  {
    return;
  }

  const FVector SavedRelativeLinearVelocity = SavedTransforms.GetSavedRelativeLinearVelocity(true);
  const FVector SavedRelativePawnLocation = SavedTransforms.GetSavedRelativeActorLocation(true);
  const FRotator SavedRelativePawnRotation = SavedTransforms.GetSavedRelativeActorRotation(true);
  const FRotator SavedRelativeControlRotation = SavedTransforms.GetSavedRelativeControlRotation(true);

  const FVector SavedWorldLinearVelocity = SavedTransforms.GetSavedWorldLinearVelocity(true);
  const FVector SavedWorldPawnLocation = SavedTransforms.GetSavedWorldActorLocation(true);
  const FRotator SavedWorldPawnRotation = SavedTransforms.GetSavedWorldActorRotation(true);
  const FRotator SavedWorldControlRotation = SavedTransforms.GetSavedWorldControlRotation(true);

  const FVector NewBaseLocation = CurrentBase->GetComponentLocation();
  const FRotator NewBaseRotation = CurrentBase->GetComponentRotation();
  const FRotator NewBaseRotationYaw = FRotator{0., NewBaseRotation.Yaw, 0.};

  const FVector NewWorldLinearVelocity = NewBaseRotationYaw.RotateVector(InputState.LinearVelocity.Read());
  const FVector NewWorldPawnLocation = NewBaseLocation + NewBaseRotation.RotateVector(InputState.ActorLocation.Read());
  const FRotator NewWorldPawnRotation = NewBaseRotationYaw + InputState.ActorRotation.Read();
  const FRotator NewWorldControlRotation = NewBaseRotationYaw + InputState.ControlRotation.Read();

  const FVector WorldLinearVelocityDiff = NewWorldLinearVelocity - SavedWorldLinearVelocity;
  InputState.LinearVelocity.Write(SavedRelativeLinearVelocity + WorldLinearVelocityDiff);

  const FVector WorldPawnLocationDiff = NewWorldPawnLocation - SavedWorldPawnLocation;
  InputState.ActorLocation.Write(SavedRelativePawnLocation + WorldPawnLocationDiff);

  const FRotator WorldPawnRotationYawDiff = NewWorldPawnRotation - SavedWorldPawnRotation;
  InputState.ActorRotation.Write({0., SavedRelativePawnRotation.Yaw + WorldPawnRotationYawDiff.Yaw, 0.});

  const FRotator WorldControlRotationDiff = NewWorldControlRotation - SavedWorldControlRotation;
  InputState.ControlRotation.Write({SavedRelativeControlRotation + WorldControlRotationDiff});
}

void FGMC_RelBasedMovementAux::SV_HandleBaseTransition(UGMC_OrganicMovementCmp* const Outer, float DeltaSeconds)
{
  gmc_ck(Outer->BasedMovement.IsRelative())
  gmc_ck(Outer->IsServerPawn())
  gmc_ck(!Outer->IsSimulating())
  gmc_ck(Outer->BasedMovement.GetBaseChangeTransitionTime() >= 0.f)

  USceneComponent* CurrentBase = Outer->GetActorBase();
  USceneComponent* NewBase = CALL_NATIVE_EVENT_CONDITIONAL(Outer->bNoBlueprintEvents, Outer, FindActorBase);

  const auto& InitTransition = [this](USceneComponent* NewBase, float DeltaSeconds)
  {
    SV_TransitionBase = NewBase;
    SV_TransitionTimer = DeltaSeconds;
  };

  const auto& ClearTransition = [this]()
  {
    SV_TransitionBase = nullptr;
    SV_TransitionTimer = -1.f;
  };

  if (SV_TransitionTimer == -1.f)
  {
    if (NewBase != CurrentBase)
    {
      InitTransition(NewBase, DeltaSeconds);
    }
  }
  else
  {
    gmc_ck(SV_TransitionTimer >= 0.f)
    if (NewBase == CurrentBase)
    {
      ClearTransition();
    }
    else if (NewBase == SV_TransitionBase)
    {
      SV_TransitionTimer += DeltaSeconds;
    }
    else if (NewBase != SV_TransitionBase)
    {
      InitTransition(NewBase, DeltaSeconds);
    }
    else
    {
      gmc_ckne()
      ClearTransition();
    }
  }

  if (SV_TransitionTimer >= Outer->BasedMovement.GetBaseChangeTransitionTime())
  {
    gmc_ck(NewBase == SV_TransitionBase)
    Outer->SetActorBase(NewBase);
    ClearTransition();
  }
}

void FGMC_RelBasedMovementAux::EqualizeBase(bool bEqualize, bool bMoveAllPawns, UGMC_OrganicMovementCmp* const Outer)
{
  if (Outer->IsNetMode(NM_Standalone))
  {
    return;
  }

  // Still allow resetting of an already equalized base (normally not supposed to happen).
  if (bEqualize || !bDidEqualizeBase)
  {
    if (!Outer->BasedMovement.IsRelative() || !Outer->BasedMovement.GetEqualizeBaseActor() || Outer->IsSimulating())
    {
      gmc_ckne()
      return;
    }

    if (!Outer->IsAutonomousProxy() && !Outer->IsRemotelyControlledServerPawn())
    {
      return;
    }
  }

  const auto& Base = Outer->GetActorBase();

  if (!Base)
  {
    return;
  }

  if (bEqualize)
  {
    gmc_ck(!bDidEqualizeBase)

    const float MaxEqualizationAngle = Outer->BasedMovement.GetMaxEqualizationAngle();
    gmc_ck(MaxEqualizationAngle >= 0.f)
    gmc_ck(MaxEqualizationAngle <= 180.f)

    if (MaxEqualizationAngle < 180.f - UE_KINDA_SMALL_NUMBER)
    {
      const FRotator& BaseRotation = Base->GetComponentRotation().GetNormalized();
      if (FMath::Abs(BaseRotation.Roll) > MaxEqualizationAngle || FMath::Abs(BaseRotation.Pitch) > MaxEqualizationAngle)
      {
        return;
      }
    }
  }

  const auto& BaseOwner = Base->GetOwner();

  struct PawnData
  {
    UGMC_ReplicationCmp* ReplicationComponent{nullptr};
    FVector SavedLinearVelocity{0.};
    FVector SavedAngularVelocity{0.};
    FVector SavedActorLocation{0.};
    FRotator SavedActorRotation{0.};
    FVector SavedActorScale{1.};
    FRotator SavedControlRotation{0.};

    PawnData(UGMC_ReplicationCmp* InReplicationComponent)
    {
      gmc_ck(InReplicationComponent)
      ReplicationComponent = InReplicationComponent;
      SavedLinearVelocity = ReplicationComponent->GetBasedLinearVelocity();
      SavedAngularVelocity = ReplicationComponent->GetBasedAngularVelocity();
      SavedActorLocation = ReplicationComponent->GetBasedActorLocation();
      SavedActorRotation = ReplicationComponent->GetBasedActorRotation();
      SavedActorScale = ReplicationComponent->GetBasedActorScale();
      SavedControlRotation = ReplicationComponent->GetBasedControlRotation();
    }

    void Load()
    {
      ReplicationComponent->SetBasedLinearVelocity(SavedLinearVelocity);
      ReplicationComponent->SetBasedAngularVelocity(SavedAngularVelocity);
      ReplicationComponent->SetBasedActorLocation(SavedActorLocation);
      ReplicationComponent->SetBasedActorRotation(SavedActorRotation);
      ReplicationComponent->SetBasedActorScale(SavedActorScale);
      ReplicationComponent->SetBasedControlRotation(SavedControlRotation);
    }
  };

  // The first entry always refers to the pawn that is currently performing the movement.
  TArray<PawnData> SavedPawnData{PawnData{Outer}};

  const bool bMoveOtherPawns = bMoveAllPawns && (bEqualize || bDidEqualizeBase);

  if (bMoveOtherPawns)
  {
    // For local moves we also want to move the other pawns to the equalized base position.

    TArray<AActor*> Actors{};
    if (IsValid(Outer->GMCAggregator))
    {
      Actors = static_cast<const TArray<AActor*>>(Outer->GMCAggregator->GetPawns());
    }
    else
    {
      UGameplayStatics::GetAllActorsOfClass(Outer->GetWorld(), AGMC_Pawn::StaticClass(), Actors);
    }

    for (const auto& Actor : Actors)
    {
      const auto& GMCPawn = Cast<AGMC_Pawn>(Actor);
      if (!IsValid(GMCPawn))
      {
        continue;
      }

      // Never save the current pawn (which is already in the array) or the base itself (which could potentially be a GMC pawn).
      if (GMCPawn == Outer->PawnOwner || GMCPawn == BaseOwner)
      {
        continue;
      }

      const auto& ReplicationComponent = GMCPawn->GetReplicationComponent();
      if (!IsValid(ReplicationComponent))
      {
        continue;
      }

      gmc_ck(ReplicationComponent != Outer)

      if (!CALL_NATIVE_EVENT_CONDITIONAL(Outer->bNoBlueprintEvents, Outer, ShouldMoveGMCPawnForBaseEqualization, GMCPawn))
      {
        continue;
      }

      SavedPawnData.Emplace(PawnData{ReplicationComponent});
    }
  }

  if (bEqualize)
  {
    gmc_ck(SavedBaseEqualizationTransform.Equals(FTransform::Identity))
    SavedBaseEqualizationTransform = BaseOwner ? BaseOwner->GetActorTransform() : Base->GetComponentTransform();

    gmc_ck(!SavedEqualizationBase)
    SavedEqualizationBase = Base;

    gmc_ck(!bDidEqualizeBase)
    bDidEqualizeBase = true;

    if (BaseOwner)
    {
      BaseOwner->SetActorTransform(
        FTransform{
          FQuat::Identity,
          Outer->BasedMovement.GetEqualizeInPlace() ? BaseOwner->GetActorLocation() : Outer->BasedMovement.GetEqualizationLocation(),
          BaseOwner->GetActorScale3D()
        },
        false,
        nullptr,
        ETeleportType::TeleportPhysics
      );
    }
    else
    {
      Base->SetWorldTransform(
        FTransform{
          FQuat::Identity,
          Outer->BasedMovement.GetEqualizeInPlace() ? Base->GetComponentLocation() : Outer->BasedMovement.GetEqualizationLocation(),
          Base->GetComponentScale()
        },
        false,
        nullptr,
        ETeleportType::TeleportPhysics
      );
    }
  }
  else if (bDidEqualizeBase)
  {
    if (BaseOwner)
    {
      BaseOwner->SetActorTransform(
        SavedBaseEqualizationTransform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics
      );
    }
    else
    {
      Base->SetWorldTransform(
        SavedBaseEqualizationTransform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics
      );
    }

    SavedBaseEqualizationTransform = FTransform::Identity;
    SavedEqualizationBase = nullptr;
    bDidEqualizeBase = false;
  }

  gmc_ck(SavedPawnData.Num() > 0)
  SavedPawnData[0].Load();

  if (bMoveOtherPawns)
  {
    const int32 SavedPawnDataNum = SavedPawnData.Num();
    for (int32 Index = 1; Index < SavedPawnDataNum; ++Index)
    {
      SavedPawnData[Index].Load();
    }
  }
}

void FGMC_RelBasedMovementAux::UpdateSavedRelativeTransform(bool bSimulated, UGMC_OrganicMovementCmp* const Outer)
{
  // Only update if we still have the same base.
  if (Outer->GetActorBase() == SavedTransforms.GetSavedBase(bSimulated))
  {
    // Only update the pawn transform even if the base has already moved. Essentially we are pretending that the updated transform was already set at the end of
    // the last prediction tick.
    SavedTransforms.SavePawnRelative(bSimulated, Outer);
    SavedTransforms.SavePawnWorld(bSimulated, Outer);
    SavedTransforms.SaveSmoothComponentRelative(bSimulated, Outer);
  }
}

void FGMC_RelBasedMovementAux::HandleMoveIgnoreActors(bool bSimulated, UGMC_OrganicMovementCmp* const Outer)
{
  if (bSimulated)
  {
    return;
  }

  const auto& GMCAggregator = Outer->GMCAggregator;

  TArray<AActor*> Actors{};

  if (IsValid(GMCAggregator))
  {
    Actors = static_cast<const TArray<AActor*>>(GMCAggregator->GetPawns());
  }
  else
  {
    UGameplayStatics::GetAllActorsOfClass(Outer->GetWorld(), AGMC_Pawn::StaticClass(), Actors);
  }

  const auto& PawnOwner = Outer->PawnOwner;
  const auto& UpdatedPrimitive = Outer->UpdatedPrimitive;
  gmc_ck(PawnOwner)
  gmc_ck(UpdatedPrimitive)

  for (const auto& Actor : Actors)
  {
    const auto& GMCPawn = Cast<AGMC_Pawn>(Actor);
    gmc_ck(GMCPawn)

    if (!IsValid(GMCPawn) || PawnOwner == GMCPawn)
    {
      continue;
    }

    const auto& ReplicationComponentOther = GMCPawn->GetReplicationComponent();
    if (!IsValid(ReplicationComponentOther))
    {
      continue;
    }

    const auto& ActorBaseOther = ReplicationComponentOther->GetActorBase();
    if (ActorBaseOther && ActorBaseOther == PawnOwner->GetRootComponent())
    {
      // This pawn should ignore any other pawns that are based on it during movement.
      UpdatedPrimitive->IgnoreActorWhenMoving(GMCPawn, true);
    }
    else
    {
      UpdatedPrimitive->IgnoreActorWhenMoving(GMCPawn, false);
    }
  }
}

void FGMC_RelBasedMovementAux::SV_ReconcileClientAuthValues(bool bPreMoveExecution, UGMC_OrganicMovementCmp* const Outer)
{
  gmc_ck(Outer->IsRemotelyControlledServerPawn())
  gmc_ck(Outer->bUseClientPrediction)
  gmc_ck(Outer->SV_IsExecutingRemoteMoves())
  gmc_ck(Outer->BasedMovement.IsRelative())

  // Due to the fact that the actor base is not predicted for relative based movement, client-auth variables can cause issues during a base change. The values
  // will still refer to the previous base for any packet that was sent out while the client had not received the correction with the updated base actor yet,
  // but the server will still apply the "wrong" value regardless because it is client-auth. We try to mitigate this here to make the base transition smoother
  // for the client.

  const auto& LastClientData = Outer->SV_GetLastClientData();
  const auto& ReplicationSettings = Outer->ReplicationSettings.DefaultPredictionSettings;
  bool bReconcile{false};
  USceneComponent* ClientBase{nullptr};
  bool bHasActorBaseRotation{false};

  const auto& ClientState =
    [&LastClientData, &ReplicationSettings, bPreMoveExecution, Outer]
    (bool& bOutReconcile, USceneComponent*& OutClientBase, bool& bOutHasActorBaseRotation)
  {
    gmc_ck(!bOutReconcile)
    gmc_ck(!OutClientBase)
    gmc_ck(!bOutHasActorBaseRotation)

    // Only works if the actor base is server validated, otherwise we cannot know the client's base for the move.

    OutClientBase = [&]() -> USceneComponent*
    {
      if (IsServerAuthInputServerValidated(ReplicationSettings.ActorBase))
      {
        bOutReconcile = true;
        return LastClientData.InputState.ActorBase.Read();
      }

      if (IsServerAuthOutputServerValidated(ReplicationSettings.ActorBase))
      {
        bOutReconcile = true;
        return LastClientData.OutputState.ActorBase.Read();
      }

      gmc_ckne()
      gmc_ck(!bOutReconcile)
      return nullptr;
    }();

    if (bOutReconcile && Outer->BasedMovement.GetSendActorBaseRotation())
    {
      bOutHasActorBaseRotation = static_cast<bool>(OutClientBase);
      // The prediction mode for the actor base rotation should be "ClientAuth_Input".
    }

    return bPreMoveExecution ? LastClientData.InputState : LastClientData.OutputState;
  }(bReconcile, ClientBase, bHasActorBaseRotation);

  if (!bReconcile)
  {
    return;
  }

  const auto& ServerBase = Outer->GetActorBase();
  if (ClientBase == ServerBase)
  {
    // The client is currently not transitioning to a new base.
    return;
  }

  // The current client move still refers to the previous actor base.

  const bool bSetBasedValue = Outer->BasedMovement.GetEqualizeBaseActor() && bDidEqualizeBase;
  const FTransform ActorBaseRotationTransform = FTransform{ActorBaseRotation};

  if (bPreMoveExecution ? IsClientAuthInput(ReplicationSettings.LinearVelocity) : IsClientAuthOutput(ReplicationSettings.LinearVelocity))
  {
    if (bSetBasedValue)
    {
      const auto& ClientLinearVelocity = ClientState.LinearVelocity.Read();
      Outer->SetLinearVelocity_GMC(Outer->GetBasedLinearVelocityFor(ClientLinearVelocity, SavedBaseEqualizationTransform));
    }
    else if (!bHasActorBaseRotation && ClientBase)
    {
      Outer->SetLinearVelocity_GMC(SavedTransforms.GetSavedWorldLinearVelocity(false));
    }
    else
    {
      const auto& ClientLinearVelocity = ClientState.LinearVelocity.Read();
      Outer->SetLinearVelocity_GMC(Outer->GetWorldLinearVelocityFor(ClientLinearVelocity, ActorBaseRotationTransform));
    }
  }

  if (bPreMoveExecution ? IsClientAuthInput(ReplicationSettings.ActorLocation) : IsClientAuthOutput(ReplicationSettings.ActorLocation))
  {
    if (bSetBasedValue)
    {
      const auto& ClientActorLocation = ClientState.ActorLocation.Read();
      Outer->SetActorLocation_GMC(Outer->GetBasedActorLocationFor(ClientActorLocation, SavedBaseEqualizationTransform), true);
    }
    else if (!bHasActorBaseRotation && ClientBase)
    {
      Outer->SetActorLocation_GMC(SavedTransforms.GetSavedWorldActorLocation(false), true);
    }
    else
    {
      const auto& ClientActorLocation = ClientState.ActorLocation.Read();
      Outer->SetActorLocation_GMC(Outer->GetWorldActorLocationFor(ClientActorLocation, ActorBaseRotationTransform), true);
    }
  }

  if (bPreMoveExecution ? IsClientAuthInput(ReplicationSettings.ActorRotation) : IsClientAuthOutput(ReplicationSettings.ActorRotation))
  {
    if (bSetBasedValue)
    {
      const auto& ClientActorRotation = ClientState.ActorRotation.Read();
      Outer->SetActorRotation_GMC(Outer->GetBasedActorRotationFor(ClientActorRotation, SavedBaseEqualizationTransform), true);
    }
    else if (!bHasActorBaseRotation && ClientBase)
    {
      Outer->SetActorRotation_GMC(SavedTransforms.GetSavedWorldActorRotation(false), true);
    }
    else
    {
      const auto& ClientActorRotation = ClientState.ActorRotation.Read();
      Outer->SetActorRotation_GMC(Outer->GetWorldActorRotationFor(ClientActorRotation, ActorBaseRotationTransform), true);
    }
  }

  if (bPreMoveExecution ? IsClientAuthInput(ReplicationSettings.ControlRotation) : IsClientAuthOutput(ReplicationSettings.ControlRotation))
  {
    if (bSetBasedValue)
    {
      const auto& ClientControlRotation = ClientState.ControlRotation.Read();
      Outer->SetControllerRotation_GMC(Outer->GetBasedControlRotationFor(ClientControlRotation, SavedBaseEqualizationTransform));
    }
    else if (!bHasActorBaseRotation && ClientBase)
    {
      Outer->SetControllerRotation_GMC(SavedTransforms.GetSavedWorldControlRotation(false));
    }
    else
    {
      const auto& ClientControlRotation = ClientState.ControlRotation.Read();
      Outer->SetControllerRotation_GMC(Outer->GetWorldControlRotationFor(ClientControlRotation, ActorBaseRotationTransform));
    }
  }
}

void FGMC_RelBasedMovementAux::CL_PreReplayMoveExecution(const FGMC_Move& ReplayMove, UGMC_OrganicMovementCmp* const Outer)
{
  // Save these values for use in CL_ReconcileClientAuthValues before they are overwritten during the replay. It is only necessary to do this once before the
  // input state is resaved because the base component is updated by the server exclusively for relative based movement and will not change during the move
  // execution.
  CL_OriginalReplayMoveBase = ReplayMove.InputState.ActorBase.Read();
  CL_OriginalReplayMoveBaseRotation = Outer->GetBoundCompressedRotator(Outer->BI_RelBasedMovementAux_ActorBaseRotation, ReplayMove.InputState);
}

void FGMC_RelBasedMovementAux::CL_ReconcileClientAuthValues(bool bPreMoveExecution, UGMC_OrganicMovementCmp* const Outer)
{
  gmc_ck(Outer->IsPredictedAutonomousProxy())
  gmc_ck(Outer->bUseClientPrediction)
  gmc_ck(Outer->CL_IsReplaying())
  gmc_ck(Outer->BasedMovement.IsRelative())

  const auto& CurrentBase = Outer->GetActorBase();
  const auto& OriginalBase = CL_OriginalReplayMoveBase;

  if (CurrentBase == OriginalBase)
  {
    return;
  }

  const auto& ReplicationSettings = Outer->ReplicationSettings.DefaultPredictionSettings;
  bool bReconcile{false};

  const auto& OriginalClientState = [&](bool& bOutReconcile)
  {
    bOutReconcile = false;

    // Only relevant when the actor base is replicated server-authoritatively.

    if (IsServerAuth(ReplicationSettings.ActorBase))
    {
      bOutReconcile = true;
    }

    return bPreMoveExecution ? Outer->CL_GetCurrentReplayMove().InputState : Outer->CL_GetCurrentReplayMove().OutputState;
  }(bReconcile);

  if (!bReconcile)
  {
    return;
  }

  // Client-auth variables are not resaved during a replay, so we can still refer to the values from the original move.

  const bool bSetBasedValue = Outer->BasedMovement.GetEqualizeBaseActor() && bDidEqualizeBase;
  const FTransform OriginalReplayMoveBaseRotationTransform = FTransform{ActorBaseRotation};

  if (bPreMoveExecution ? IsClientAuthInput(ReplicationSettings.LinearVelocity) : IsClientAuthOutput(ReplicationSettings.LinearVelocity))
  {
    const auto& OriginalValue = OriginalClientState.LinearVelocity.Read();
    if (bSetBasedValue)
    {
      const auto& BasedValue = Outer->GetBasedLinearVelocityFor(OriginalValue, SavedBaseEqualizationTransform);
      Outer->SetLinearVelocity_GMC(BasedValue);
    }
    else
    {
      const auto& WorldValue = OriginalBase ? Outer->GetWorldLinearVelocityFor(OriginalValue, OriginalReplayMoveBaseRotationTransform) : OriginalValue;
      Outer->SetLinearVelocity_GMC(WorldValue);
    }
  }

  if (bPreMoveExecution ? IsClientAuth(ReplicationSettings.ActorLocation) : IsClientAuthOutput(ReplicationSettings.ActorLocation))
  {
    const auto& OriginalValue = OriginalClientState.ActorLocation.Read();
    if (bSetBasedValue)
    {
      const auto& BasedValue = Outer->GetBasedActorLocationFor(OriginalValue, SavedBaseEqualizationTransform);
      Outer->SetActorLocation_GMC(BasedValue, true);
    }
    else
    {
      const auto& WorldValue = OriginalBase ? Outer->GetWorldActorLocationFor(OriginalValue, OriginalReplayMoveBaseRotationTransform) : OriginalValue;
      Outer->SetActorLocation_GMC(WorldValue, true);
    }
  }

  if (bPreMoveExecution ? IsClientAuth(ReplicationSettings.ActorRotation) : IsClientAuthOutput(ReplicationSettings.ActorRotation))
  {
    const auto& OriginalValue = OriginalClientState.ActorRotation.Read();
    if (bSetBasedValue)
    {
      const auto& BasedValue = Outer->GetBasedActorRotationFor(OriginalValue, SavedBaseEqualizationTransform);
      Outer->SetActorRotation_GMC(BasedValue, true);
    }
    else
    {
      const auto& WorldValue = OriginalBase ? Outer->GetWorldActorRotationFor(OriginalValue, OriginalReplayMoveBaseRotationTransform) : OriginalValue;
      Outer->SetActorRotation_GMC(WorldValue, true);
    }
  }

  if (bPreMoveExecution ? IsClientAuth(ReplicationSettings.ControlRotation) : IsClientAuthOutput(ReplicationSettings.ControlRotation))
  {
    const auto& OriginalValue = OriginalClientState.ControlRotation.Read();
    if (bSetBasedValue)
    {
      const auto& BasedValue = Outer->GetBasedControlRotationFor(OriginalValue, SavedBaseEqualizationTransform);
      Outer->SetControllerRotation_GMC(BasedValue);
    }
    else
    {
      const auto& WorldValue = OriginalBase ? Outer->GetWorldControlRotationFor(OriginalValue, OriginalReplayMoveBaseRotationTransform) : OriginalValue;
      Outer->SetControllerRotation_GMC(WorldValue);
    }
  }
}

void FGMC_RelBasedMovementAux::Reset()
{
  SavedTransforms.Reset(true);
  SavedTransforms.Reset(false);
  SV_TransitionBase = nullptr;
  SV_TransitionTimer = -1.f;
  SavedBaseEqualizationTransform = FTransform::Identity;
  SavedEqualizationBase = nullptr;
  bDidEqualizeBase = false;
  SavedChangeBase = nullptr;
  CL_OriginalReplayMoveBase = nullptr;
  CL_OriginalReplayMoveBaseRotation = FRotator::ZeroRotator;
  ActorBaseRotation = FRotator::ZeroRotator;
}

void FGMC_SavedRelBasedMovement::Save(bool bSimulated, UGMC_ReplicationCmp* const Outer)
{
  // All saved values must refer to the same base.
  (bSimulated ? BaseSimulated : Base) = Outer->GetActorBase();
  SavePawnRelative(bSimulated, Outer);
  SavePawnWorld(bSimulated, Outer);
  SaveBaseWorld(bSimulated, Outer);
  SaveSmoothComponentRelative(bSimulated, Outer);
}

void FGMC_SavedRelBasedMovement::SavePawnRelative(bool bSimulated, UGMC_ReplicationCmp* const Outer)
{
  if (bSimulated ? BaseSimulated.IsValid() : Base.IsValid())
  {
    (bSimulated ? PawnRelativeSimulated.LinearVelocity : PawnRelative.LinearVelocity) = Outer->GetBasedLinearVelocity();
    (bSimulated ? PawnRelativeSimulated.Location : PawnRelative.Location) = Outer->GetBasedActorLocation();
    (bSimulated ? PawnRelativeSimulated.Rotation : PawnRelative.Rotation) = Outer->GetBasedActorRotation();
    (bSimulated ? PawnRelativeSimulated.ControlRotation : PawnRelative.ControlRotation) = Outer->GetBasedControlRotation();
    return;
  }

  (bSimulated ? PawnRelativeSimulated.LinearVelocity : PawnRelative.LinearVelocity) = FVector::ZeroVector;
  (bSimulated ? PawnRelativeSimulated.Location : PawnRelative.Location) = FVector::ZeroVector;
  (bSimulated ? PawnRelativeSimulated.Rotation : PawnRelative.Rotation) = FRotator::ZeroRotator;
  (bSimulated ? PawnRelativeSimulated.ControlRotation : PawnRelative.ControlRotation) = FRotator::ZeroRotator;
}

void FGMC_SavedRelBasedMovement::SavePawnWorld(bool bSimulated, UGMC_ReplicationCmp* const Outer)
{
  // Always save the pawn world transform since it does not rely on a valid actor base and the saved values may be used by SV_ReconcileClientAuthValues.
  (bSimulated ? PawnWorldSimulated.LinearVelocity : PawnWorld.LinearVelocity) = Outer->GetLinearVelocity_GMC();
  (bSimulated ? PawnWorldSimulated.Location : PawnWorld.Location) = Outer->GetActorLocation_GMC();
  (bSimulated ? PawnWorldSimulated.Rotation : PawnWorld.Rotation) = Outer->GetActorRotation_GMC();
  (bSimulated ? PawnWorldSimulated.ControlRotation : PawnWorld.ControlRotation) = Outer->GetControllerRotation_GMC();
}

void FGMC_SavedRelBasedMovement::SaveBaseWorld(bool bSimulated, UGMC_ReplicationCmp* const Outer)
{
  if (bSimulated ? BaseSimulated.IsValid() : Base.IsValid())
  {
    (bSimulated ? BaseWorldSimulated.Location : BaseWorld.Location) = bSimulated ? BaseSimulated->GetComponentLocation() : Base->GetComponentLocation();
    (bSimulated ? BaseWorldSimulated.Rotation : BaseWorld.Rotation) = bSimulated ? BaseSimulated->GetComponentQuat() : Base->GetComponentQuat();
    return;
  }

  (bSimulated ? BaseWorldSimulated.Location : BaseWorld.Location) = FVector::ZeroVector;
  (bSimulated ? BaseWorldSimulated.Rotation : BaseWorld.Rotation) = FQuat::Identity;
}

void FGMC_SavedRelBasedMovement::SaveSmoothComponentRelative(bool bSimulated, UGMC_ReplicationCmp* const Outer)
{
  const auto& SmoothComponent = Outer->PostSmoothingParams.Component;

  if (IsValid(SmoothComponent) && (bSimulated ? BaseSimulated.IsValid() : Base.IsValid()))
  {
    const auto& SmoothComponentLocation = SmoothComponent->GetComponentLocation();
    const auto& SmoothComponentRotation = SmoothComponent->GetComponentRotation();
    const auto& BaseTransform = (bSimulated ? BaseSimulated : Base)->GetComponentTransform();

    (bSimulated ? SmoothComponentRelativeSimulated.Location : SmoothComponentRelative.Location) =
      Outer->GetBasedActorLocationFor(SmoothComponentLocation, BaseTransform);

    (bSimulated ? SmoothComponentRelativeSimulated.Rotation : SmoothComponentRelative.Rotation) =
      Outer->GetBasedActorRotationFor(SmoothComponentRotation, BaseTransform);

    return;
  }

  (bSimulated ? SmoothComponentRelativeSimulated.Location : SmoothComponentRelative.Location) = FVector::ZeroVector;
  (bSimulated ? SmoothComponentRelativeSimulated.Rotation : SmoothComponentRelative.Rotation) = FRotator::ZeroRotator;
}

void FGMC_SavedRelBasedMovement::Reset(bool bSimulated)
{
  if (bSimulated)
  {
    BaseSimulated.Reset();
    PawnRelativeSimulated = PawnData{};
    PawnWorldSimulated = PawnData{};
    BaseWorldSimulated = BaseData{};
    SmoothComponentRelativeSimulated = SmoothComponentData{};
  }
  else
  {
    Base.Reset();
    PawnRelative = PawnData{};
    PawnWorld = PawnData{};
    BaseWorld = BaseData{};
    SmoothComponentRelative = SmoothComponentData{};
  }
}

FVector UGMC_OrganicMovementCmp::GetWorldLinearVelocityFor(const FVector& InRelativeLinearVelocity, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = FRotator{0., InBaseTransform.GetRotation().Rotator().Yaw, 0.};
  const auto& WorldLinearVelocity = RotationDelta.RotateVector(InRelativeLinearVelocity);
  gmc_ck(WorldLinearVelocity.Z == InRelativeLinearVelocity.Z)
  return WorldLinearVelocity;
}

FVector UGMC_OrganicMovementCmp::GetBasedLinearVelocityFor(const FVector& InLinearVelocity, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = FRotator{0., InBaseTransform.GetRotation().Rotator().Yaw, 0.};
  const auto& RelativeLinearVelocity = RotationDelta.UnrotateVector(InLinearVelocity);
  gmc_ck(RelativeLinearVelocity.Z == InLinearVelocity.Z)
  return RelativeLinearVelocity;
}

FRotator UGMC_OrganicMovementCmp::GetWorldActorRotationFor(const FRotator& InRelativeActorRotation, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = FRotator{0., InBaseTransform.GetRotation().Rotator().Yaw, 0.};
  const auto& WorldActorRotation = (InRelativeActorRotation + RotationDelta).GetNormalized();
  return WorldActorRotation;
}

FRotator UGMC_OrganicMovementCmp::GetBasedActorRotationFor(const FRotator& InActorRotation, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = FRotator{0., InBaseTransform.GetRotation().Rotator().Yaw, 0.};
  const auto& RelativeActorRotation = (InActorRotation - RotationDelta).GetNormalized();
  return RelativeActorRotation;
}

FRotator UGMC_OrganicMovementCmp::GetWorldControlRotationFor(const FRotator& InRelativeControlRotation, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = FRotator{0., InBaseTransform.GetRotation().Rotator().Yaw, 0.};
  const auto& WorldControlRotation = (InRelativeControlRotation + RotationDelta).GetNormalized();
  return WorldControlRotation;
}

FRotator UGMC_OrganicMovementCmp::GetBasedControlRotationFor(const FRotator& InControlRotation, const FTransform& InBaseTransform) const
{
  const auto& RotationDelta = FRotator{0., InBaseTransform.GetRotation().Rotator().Yaw, 0.};
  const auto& RelativeControlRotation = (InControlRotation - RotationDelta).GetNormalized();
  return RelativeControlRotation;
}

FVector FGMC_NavMeshWalkingAux::GetDistanceToNavMesh(const FVector& Location) const
{
  return LastValidTargetNavLocation.HasNodeRef() ? LastValidTargetNavLocation.Location - Location : FVector::ZeroVector;
}

void FGMC_NavMeshWalkingAux::ResetLastValidTargetNavLocation()
{
  LastValidTargetNavLocation.NodeRef = INVALID_NAVNODEREF;
  LastValidTargetNavLocation.Location = FVector::ZeroVector;
}
