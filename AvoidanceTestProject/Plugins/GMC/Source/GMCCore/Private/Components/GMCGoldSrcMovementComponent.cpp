// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCGoldSrcMovementComponent.h"
#include "Compression.h"
#include "GMCLog.h"
#include "GMCGoldSrcMovementComponent_DBG.h"

namespace GMCCVars
{
#if ALLOW_CONSOLE

  int32 FlyMode = 0;
  FAutoConsoleVariableRef CVarFlyMode(
    TEXT("gmc.FlyMode"),
    FlyMode,
    TEXT("Cheat/Server only: Fly mode removes the effects of gravity on the pawn. 0: Disable, 1: Enable"),
    ECVF_Cheat
  );

  int32 NoClipMode = 0;
  FAutoConsoleVariableRef CVarNoClipMode(
    TEXT("gmc.NoClipMode"),
    NoClipMode,
    TEXT("Cheat/Server only: Noclip mode removes all collision from the pawn and allows for free movement in all directions. ")
    TEXT("0: Disable, 1: Enable"),
    ECVF_Cheat
  );

#endif
}

UGMC_GoldSrcMovementCmp::UGMC_GoldSrcMovementCmp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  // Set the actor rotation directly from the view direction.
  bOrientToInputDirection = false;
  bOrientToControlRotationDirection = true;
  RotationRate = 0.f;

  // Attuned to WaterControl.
  FluidMinExitSpeed = 320.f;

  // No buoyancy by default, we have SinkSpeed instead.
  Buoyancy = 0.f;

  // Set for consistent default values.
  MaxDesiredSpeed = MaxSpeed;

  // Also prevents some GoldSrc movement exploits if enabled.
  bPreventSlopeBoosting = false;

#if WITH_EDITORONLY_DATA && WITH_EDITOR

  bHideProperties = true;

  // Hide the GroundFriction property (we use FloorFriction instead).
  HideProperty(StaticClass(), "GroundFriction");

  // Hide the FallControl property (not applicable).
  HideProperty(StaticClass(), "FallControl");

#endif
}

void UGMC_GoldSrcMovementCmp::BeginPlay()
{
  // Enables us to use DefaultSpeed (set in parent function).
  MaxDesiredSpeed = MaxSpeed;

  Super::BeginPlay();

  if (!GetGMCPawnOwner())
  {
    return;
  }

  DefaultHalfHeight = GetRootCollisionHalfHeight(false);

  if (const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent()))
  {
    DefaultCollisionType = RootComponent->GetCollisionEnabled();
  }
}

void UGMC_GoldSrcMovementCmp::HaltMovement()
{
  Super::HaltMovement();
  bIsSprinting = false;
  bCanJump = false;
  bJustJumped = false;
  bJustLandedPrePhysics = false;
  bJustLandedPostPhysics = false;
  bJustLandedPostPhysicsLock = false;
  bJustLanded = false;
}

float UGMC_GoldSrcMovementCmp::CalculatePathFollowingBrakingDistance_Implementation(float InMaxSpeed, float DeltaSeconds) const
{
  return InMaxSpeed;
}

void UGMC_GoldSrcMovementCmp::ClampToValidValues()
{
  Super::ClampToValidValues();

  MinAnalogWalkSpeed = FMath::Min(MinAnalogWalkSpeed, StopSpeed);
  CrouchedHalfHeight = FMath::Clamp(CrouchedHalfHeight, UE_KINDA_SMALL_NUMBER, DefaultHalfHeight);
  EdgeFrictionMinRequiredHeight =
    FMath::Clamp(EdgeFrictionMinRequiredHeight, MAX_DISTANCE_TO_FLOOR + UE_KINDA_SMALL_NUMBER, FloorTraceLength - UE_KINDA_SMALL_NUMBER);
}

void UGMC_GoldSrcMovementCmp::BindReplicationData_Implementation()
{
  Super::BindReplicationData_Implementation();

  BI_WantsToSprint = BindBool(
    bWantsToSprint,
    EGMC_PredictionMode::ClientAuth_Input,
    EGMC_CombineMode::CombineIfUnchanged,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_WantsToCrouch = BindBool(
    bWantsToCrouch,
    EGMC_PredictionMode::ClientAuth_Input,
    EGMC_CombineMode::CombineIfUnchanged,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_WantsToJump = BindBool(
    bWantsToJump,
    EGMC_PredictionMode::ClientAuth_Input,
    EGMC_CombineMode::CombineIfUnchanged,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_CanJump = BindBool(
    bCanJump,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::None,
    {}
  );

  BI_JustLandedPostPhysics = BindBool(
    bJustLandedPostPhysics,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::None,
    {}
  );

  BI_JustJumped = BindBool(
    bJustJumped,
    EGMC_PredictionMode::Local,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::PeriodicAndOnChange_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_JustLanded = BindBool(
    bJustLanded,
    EGMC_PredictionMode::Local,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::PeriodicAndOnChange_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_IsSprinting = BindBool(
    bIsSprinting,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::Periodic_Output,
    EGMC_InterpolationFunction::NearestNeighbour
  );

  BI_MaxSpeed = BindCompressedSinglePrecisionFloat(
    MaxSpeed,
    EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
    EGMC_CombineMode::AlwaysCombine,
    EGMC_SimulationMode::None,
    {}
  );
}

void UGMC_GoldSrcMovementCmp::SetupPlayerInputComponent_Implementation(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent_Implementation(PlayerInputComponent);

  gmc_ck(PlayerInputComponent)

  if (const auto& EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
  {
    EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &UGMC_GoldSrcMovementCmp::StartSprintAction);
    EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &UGMC_GoldSrcMovementCmp::StopSprintAction);
    EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &UGMC_GoldSrcMovementCmp::StartCrouchAction);
    EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &UGMC_GoldSrcMovementCmp::StopCrouchAction);
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &UGMC_GoldSrcMovementCmp::StartJumpAction);
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &UGMC_GoldSrcMovementCmp::StopJumpAction);
  }
}

bool UGMC_GoldSrcMovementCmp::SV_OnProxyMoveInitialized_Implementation(FGMC_Move& Move, float DeltaTime, double Timestamp)
{
  bool ReturnValue = Super::SV_OnProxyMoveInitialized_Implementation(Move, DeltaTime, Timestamp);

  const int32 LastIdx = MoveHistory.Num() - 1;
  if (LastIdx >= 0)
  {
    const bool PreviousWantsToSprint = GetBoundBool(BI_WantsToSprint, MoveHistory[LastIdx].InputState);
    SetBoundBool(PreviousWantsToSprint, BI_WantsToSprint, Move.InputState);

    const bool PreviousWantsToCrouch = GetBoundBool(BI_WantsToCrouch, MoveHistory[LastIdx].InputState);
    SetBoundBool(PreviousWantsToCrouch, BI_WantsToCrouch, Move.InputState);

    const bool PreviousWantsToJump = GetBoundBool(BI_WantsToJump, MoveHistory[LastIdx].InputState);
    SetBoundBool(PreviousWantsToJump, BI_WantsToJump, Move.InputState);
  }

  return ReturnValue;
}

bool UGMC_GoldSrcMovementCmp::OnCumulativeMoveInitialized_Implementation(
  FGMC_PawnState& InputState,
  EGMC_InterpolationStates SimStates,
  float DeltaTime,
  double Timestamp
)
{
  bool ReturnValue = Super::OnCumulativeMoveInitialized_Implementation(InputState, SimStates, DeltaTime, Timestamp);

  const int32 LastIdx = MoveHistory.Num() - 1;
  gmc_ck(LastIdx >= 0)

  const auto& SourceState = SimStates == EGMC_InterpolationStates::Input ? MoveHistory[LastIdx].InputState : MoveHistory[LastIdx].OutputState;

  const bool LastWantsToSprint = GetBoundBool(BI_WantsToSprint, SourceState);
  SetBoundBool(LastWantsToSprint, BI_WantsToSprint, InputState);

  const bool LastWantsToCrouch = GetBoundBool(BI_WantsToCrouch, SourceState);
  SetBoundBool(LastWantsToCrouch, BI_WantsToCrouch, InputState);

  const bool LastWantsToJump = GetBoundBool(BI_WantsToJump, SourceState);
  SetBoundBool(LastWantsToJump, BI_WantsToJump, InputState);

  return ReturnValue;
}

bool UGMC_GoldSrcMovementCmp::HasJustJumped() const
{
  return bJustJumped;
}

bool UGMC_GoldSrcMovementCmp::HasJustLanded() const
{
  return bJustLanded;
}

bool UGMC_GoldSrcMovementCmp::HasJustLanded_Internal() const
{
  return bJustLandedPrePhysics || bJustLandedPostPhysics;
}

bool UGMC_GoldSrcMovementCmp::IsSprinting() const
{
  return bIsSprinting;
}

float UGMC_GoldSrcMovementCmp::GetDefaultHalfHeight() const
{
  return DefaultHalfHeight;
}

float UGMC_GoldSrcMovementCmp::GetCrouchedHalfHeight() const
{
  return CrouchedHalfHeight;
}

bool UGMC_GoldSrcMovementCmp::IsUsingFlyMode() const
{
  return MovementMode == static_cast<uint8>(MovementModeFlying());
}

bool UGMC_GoldSrcMovementCmp::IsUsingNoClipMode() const
{
  return MovementMode == static_cast<uint8>(MovementModeNoClip());
}

bool UGMC_GoldSrcMovementCmp::IsFullyCrouched() const
{
  return FMath::IsNearlyEqual(GetRootCollisionHalfHeight(false), CrouchedHalfHeight, 0.1f);
}

bool UGMC_GoldSrcMovementCmp::IsCrouching() const
{
  return IsFullyCrouched();
}

bool UGMC_GoldSrcMovementCmp::IsCrouchingDown() const
{
  const float CurrentHalfHeight = GetRootCollisionHalfHeight(false);
  return bWantsToCrouch && CurrentHalfHeight < DefaultHalfHeight && CurrentHalfHeight > CrouchedHalfHeight;
}

bool UGMC_GoldSrcMovementCmp::IsUncrouching() const
{
  const float CurrentHalfHeight = GetRootCollisionHalfHeight(false);
  return !bWantsToCrouch && CurrentHalfHeight > CrouchedHalfHeight && CurrentHalfHeight < DefaultHalfHeight;
}

void UGMC_GoldSrcMovementCmp::StartSprintAction(const FInputActionInstance& InputAction)
{
  bWantsToSprint = true;
}

void UGMC_GoldSrcMovementCmp::StopSprintAction(const FInputActionInstance& InputAction)
{
  bWantsToSprint = false;
}

void UGMC_GoldSrcMovementCmp::StartCrouchAction(const FInputActionInstance& InputAction)
{
  bWantsToCrouch = true;
}

void UGMC_GoldSrcMovementCmp::StopCrouchAction(const FInputActionInstance& InputAction)
{
  bWantsToCrouch = false;
}

void UGMC_GoldSrcMovementCmp::StartJumpAction(const FInputActionInstance& InputAction)
{
  bWantsToJump = true;
}

void UGMC_GoldSrcMovementCmp::StopJumpAction(const FInputActionInstance& InputAction)
{
  bWantsToJump = false;
}

void UGMC_GoldSrcMovementCmp::PreMovementUpdate_Implementation(float DeltaSeconds)
{
  Super::PreMovementUpdate_Implementation(DeltaSeconds);

  if (GetIterationNumber() == 1)
  {
    bJustJumped = false;
    bJustLanded = false;
  }

  // Reset this before just before updating the movement mode (which might call OnLanded).
  bJustLandedPrePhysics = false;

  // This may be set by ProcessLanded.
  bJustLandedPostPhysicsLock = false;

#if ALLOW_CONSOLE

  const bool bJustDisabledNoClipMode = IsUsingNoClipMode() && GMCCVars::NoClipMode == 0;
  const bool bJustDisabledFlyMode = IsUsingFlyMode() && GMCCVars::FlyMode == 0;
  if (bJustDisabledNoClipMode || bJustDisabledFlyMode)
  {
    // Set to airborne as default movement mode, it will be adjusted afterwards if another cheat movement mode is still active (e.g. noclip may have been
    // deactivated but flying could still be active).
    SetMovementMode(EGMC_MovementMode::Airborne);
  }

  // Cheat modes are only available on the server.
  if (IsLocallyControlledServerPawn())
  {
    if (GMCCVars::NoClipMode != 0)
    {
      // Noclip mode takes precedence over flying mode.
      SetMovementMode(MovementModeNoClip());
    }
    else if (GMCCVars::FlyMode != 0)
    {
      SetMovementMode(MovementModeFlying());
    }
  }

#endif
}

void UGMC_GoldSrcMovementCmp::OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode)
{
  if (IsUsingNoClipMode())
  {
    // Zero velocity when switching to noclip.
    HaltMovement();
  }

  Super::OnMovementModeChanged_Implementation(PreviousMovementMode);
}

void UGMC_GoldSrcMovementCmp::MovementUpdate_Implementation(float DeltaSeconds)
{
  Super::MovementUpdate_Implementation(DeltaSeconds);

  // The order in which these movement abilities are called matters.
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, Sprint, bWantsToSprint, DeltaSeconds);
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, Crouch, bWantsToCrouch, DeltaSeconds);
  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, Jump, bWantsToJump, DeltaSeconds);
}

void UGMC_GoldSrcMovementCmp::Sprint_Implementation(bool bRequestedSprint, float DeltaSeconds)
{
  if (bRequestedSprint && CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CanSprint))
  {
    MaxSpeed = SprintSpeed;
    bIsSprinting = true;
  }
  else
  {
    MaxSpeed = DefaultSpeed;
    bIsSprinting = false;
  }
}

bool UGMC_GoldSrcMovementCmp::CanSprint_Implementation() const
{
  return IsMovingOnGround() && !IsFullyCrouched();
}

void UGMC_GoldSrcMovementCmp::Crouch_Implementation(bool bRequestedCrouch, float DeltaSeconds)
{
  float CurrentHalfHeight = GetRootCollisionHalfHeight(false);

  constexpr float Tolerance = 0.1f;
  const bool bIsBelowDefaultHalfHeight = CurrentHalfHeight < DefaultHalfHeight - Tolerance;
  const bool bIsAboveCrouchedHalfHeight = CurrentHalfHeight > CrouchedHalfHeight + Tolerance;
  const bool bIsBelowCrouchedHalfHeight = CurrentHalfHeight < CrouchedHalfHeight - Tolerance;
  const bool bIsAtCrouchedHalfHeight = !bIsAboveCrouchedHalfHeight && !bIsBelowCrouchedHalfHeight;

  if (bIsBelowCrouchedHalfHeight || bIsAtCrouchedHalfHeight || (bImmediateCrouchWalkSpeed && bIsBelowDefaultHalfHeight))
  {
    if (IsMovingOnGround())
    {
      MaxSpeed = CrouchWalkSpeed;
      bIsSprinting = false;
    }
  }

  if (bRequestedCrouch && CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CanCrouch))
  {
    if (!bIsAtCrouchedHalfHeight)
    {
      const EGMC_AdjustDirection AdjustDirection = bIsAboveCrouchedHalfHeight ? EGMC_AdjustDirection::Down : EGMC_AdjustDirection::Up;
      LerpRootCollisionHalfHeight(CrouchedHalfHeight, CrouchInterpSpeed, 0.99f, DeltaSeconds, IsMovingOnGround(), AdjustDirection);
      CurrentHalfHeight = GetRootCollisionHalfHeight(false);
    }

  }
  else if (CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CanUncrouch))
  {
    if (LerpRootCollisionHalfHeight(DefaultHalfHeight, CrouchInterpSpeed, 0.99f, DeltaSeconds, IsMovingOnGround(), EGMC_AdjustDirection::Up) > 0.f)
    {
      CL_DoNotCombineNextMove();
    }
  }
}

bool UGMC_GoldSrcMovementCmp::CanCrouch_Implementation() const
{
  return IsMovingOnGround() || IsAirborne() || IsUsingFlyMode();
}

bool UGMC_GoldSrcMovementCmp::CanUncrouch_Implementation() const
{
  return GetRootCollisionHalfHeight(false) < DefaultHalfHeight;
}

void UGMC_GoldSrcMovementCmp::Jump_Implementation(bool bRequestedJump, float DeltaSeconds)
{
  if (IsSwimming())
  {
    bCanJump = false;
    if (bRequestedJump)
    {
      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, WaterJump, DeltaSeconds);
    }
    return;
  }

  if (bRequestedJump)
  {
    if (CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, CanJump))
    {
      AddImpulse({0., 0., JumpForce}, !bConsiderMassOnJump);

      if (bApplyJumpSpeedBoost)
      {
        CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, ApplyJumpSpeedBoost, DeltaSeconds);
      }

      MaxSpeed = DefaultSpeed;
      bJustJumped = true;

      if (JumpMode == EGMC_JumpMode::SemiAuto)
      {
        bCanJump = false;
      }

      CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnJumped);
    }

    if (JumpMode != EGMC_JumpMode::SemiAuto)
    {
      bCanJump = JumpMode == EGMC_JumpMode::FullAuto;
    }

    return;
  }

  bCanJump = true;
}

bool UGMC_GoldSrcMovementCmp::CanJump_Implementation() const
{
  if (!IsMovingOnGround())
  {
    return false;
  }

  return bCanJump;
}

void UGMC_GoldSrcMovementCmp::WaterJump_Implementation(float DeltaSeconds)
{
  if (GetCurrentImmersionDepth() < 1.f)
  {
    const float SpeedXY = GetSpeedXY();
    const float AddWaterJumpPush = WaterJumpPush - SpeedXY;
    const FVector ForwardDirection = GetControllerRotation_GMC().Vector().GetSafeNormal2D();
    const FVector AddSpeedXY = ForwardDirection * (AddWaterJumpPush > 0.f ? AddWaterJumpPush : 0.f);
    const FVector AddSpeedZ = {0., 0., WaterJumpForce - Velocity.Z};
    if (AddSpeedZ.Z > 0.)
    {
      AddImpulse(AddSpeedXY + AddSpeedZ, true);
    }
  }
}

void UGMC_GoldSrcMovementCmp::ApplyJumpSpeedBoost_Implementation(float DeltaSeconds)
{
  const bool bApplyLargeSpeedBoost = !IsSprinting() && !IsFullyCrouched();
  const float SpeedBoost = bApplyLargeSpeedBoost ? LargeJumpSpeedBoostFactor : SmallJumpSpeedBoostFactor;
  const float ForwardMove = GetRawInputVector().X;
  const float CurrentMaxSpeed = GetMaxSpeed();
  float SpeedAddition = FMath::Abs(ForwardMove) * CurrentMaxSpeed * SpeedBoost;
  const float MaxSpeedBoosted = CurrentMaxSpeed + SpeedAddition;
  const float NewSpeed = Velocity.Size2D() + SpeedAddition;
  gmc_ck(NewSpeed >= 0.f)

  if (NewSpeed > MaxSpeedBoosted)
  {
    SpeedAddition -= NewSpeed - MaxSpeedBoosted;
  }

  if (ForwardMove < 0.f)
  {
    SpeedAddition *= -1.f;
  }

  const FVector ForwardDirection = GetControllerRotation_GMC().Vector().GetSafeNormal2D();
  AddImpulse(ForwardDirection * SpeedAddition, !bConsiderMassOnJump);
}

void UGMC_GoldSrcMovementCmp::ProcessLanded(const FHitResult& Hit, FGMC_FloorParams& Floor, bool bUpdateFloor, float DeltaSeconds)
{
  Super::ProcessLanded(Hit, Floor, bUpdateFloor, DeltaSeconds);

  bJustLandedPostPhysicsLock = true;
}

void UGMC_GoldSrcMovementCmp::OnLanded_Implementation(const FVector& ImpactVelocity)
{
  Super::OnLanded_Implementation(ImpactVelocity);

  bJustLandedPrePhysics = bJustLandedPostPhysics = true;
}

void UGMC_GoldSrcMovementCmp::PostMovementUpdate_Implementation(float DeltaSeconds)
{
  Super::PostMovementUpdate_Implementation(DeltaSeconds);

  // We combine bJustLandedPrePhysics and bJustLandedPostPhysics into one variable which is used for animations and replicated to simulated proxies. Do not
  // overwrite a "true"-value on sub-stepped iterations.
  bJustLanded |= HasJustLanded_Internal();
}

bool UGMC_GoldSrcMovementCmp::UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds)
{
  const auto& RootComponent = Cast<UPrimitiveComponent>(PawnOwner->GetRootComponent());
  gmc_ck(RootComponent)

  if (IsUsingNoClipMode())
  {
    RootComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    return true;
  }

  RootComponent->SetCollisionEnabled(DefaultCollisionType);

  if (IsUsingFlyMode())
  {
    return true;
  }

  return Super::UpdateMovementModeDynamic_Implementation(Floor, DeltaSeconds);
}

void UGMC_GoldSrcMovementCmp::PostPhysicsUpdate_Implementation(float DeltaSeconds)
{
  Slide(MaxGroundedVelocityZ + UE_KINDA_SMALL_NUMBER, DeltaSeconds);

  Super::PostPhysicsUpdate_Implementation(DeltaSeconds);
}

void UGMC_GoldSrcMovementCmp::Slide(float MinRequiredZ, float DeltaSeconds)
{
  if (MinRequiredZ < 0.f || !IsMovingOnGround() || IsNavMeshWalking())
  {
    return;
  }

  if (CurrentFloor.IsDirty(FloorUpdateTolerance, this) || !CurrentFloor.HasValidShapeData())
  {
    UpdateFloor(CurrentFloor, FVector::DownVector, FloorTraceLength, FloorUpdateTolerance, FloorTraceShapeScale, ShouldDoFloorUpdateAdjust(), true);
  }

  if (!CurrentFloor.HasValidShapeData())
  {
    return;
  }

  const FHitResult FloorHit = CurrentFloor.ShapeHit();
  const FVector FloorNormal = FloorHit.ImpactNormal;
  const FVector ContactNormal = FloorHit.Normal;
  if (
    HitWalkableFloor(FloorHit) &&
    FloorNormal.Z > UE_DOUBLE_KINDA_SMALL_NUMBER &&
    FloorNormal.Z < (1. - UE_DOUBLE_KINDA_SMALL_NUMBER) &&
    ContactNormal.Z > UE_DOUBLE_KINDA_SMALL_NUMBER
  )
  {
    gmc_ck(Velocity.Z == 0.)

    // Project the velocity onto the floor but keep the speed the same.
    const FVector ProjectedVelocity =
      FVector(Velocity.X, Velocity.Y, -(FloorNormal | Velocity) / FloorNormal.Z).GetSafeNormal() * Velocity.Size();

    if (ProjectedVelocity.Z >= MinRequiredZ)
    {
      // The threshold is being exceeded, change to airborne movement to slide up the ramp.
      UpdateVelocity(ProjectedVelocity, DeltaSeconds);
      RaiseFromGround(CurrentFloor);
    }
  }
}

FVector UGMC_GoldSrcMovementCmp::PreProcessInputVector_Implementation(FVector InRawInputVector)
{
  FVector TransformedInputVector = InRawInputVector;
  const bool bInWater = IsSwimming();

  // Bots using acceleration already receive the move input in the direction of the target.
  if (IsPlayerControlledPawn())
  {
    if (bInWater && bUseAltSwimControls)
    {
      // Transform the absolute input vector into an "AllRelative" vector for swimming.
      TransformedInputVector = TransformInputVectorAllRelative(InRawInputVector);
    }
    else
    {
      // Transform the absolute input vector into an "AbsoluteZ" vector.
      TransformedInputVector = TransformInputVectorAbsoluteZ(InRawInputVector);
    }
  }

  if (bInWater)
  {
    // If we continue to receive upward input when close to the water line we set the Z input to a small value to prevent the pawn from repeatedly bouncing in
    // and out of the water.
    const bool bThrottleUpwardInput = CurrentImmersionDepth <= BuoyantStateMinImmersion + IMMERSION_DEPTH_TOLERANCE;
    if (bThrottleUpwardInput)
    {
      TransformedInputVector.Z = FMath::Min(TransformedInputVector.Z, UE_KINDA_SMALL_NUMBER);
    }
  }
  else if (IsMovingOnGround() || IsAirborne())
  {
    // No vertical input while grounded or airborne.
    TransformedInputVector.Z = 0.;
  }

  return bIgnoreInputModifier ? TransformedInputVector.GetSafeNormal() : TransformedInputVector;
}

void UGMC_GoldSrcMovementCmp::PhysicsCustom_Implementation(float DeltaSeconds)
{
  Super::PhysicsCustom_Implementation(DeltaSeconds);

  if (IsUsingNoClipMode())
  {
    CalculateVelocity(DeltaSeconds);
    MoveUpdatedComponent(Velocity * DeltaSeconds, UpdatedComponent->GetComponentRotation(), false);
    return;
  }

  if (IsUsingFlyMode())
  {
    PhysicsAirborne(DeltaSeconds);
    return;
  }
}

void UGMC_GoldSrcMovementCmp::CalculateVelocity(float DeltaSeconds)
{
  Super::CalculateVelocity(DeltaSeconds);

  // Enforce the speed cap if desired.
  UpdateVelocity(SpeedHardCap > 0.f ? Velocity.GetClampedToSize(0., SpeedHardCap) : Velocity);

  if (!bJustLandedPostPhysicsLock)
  {
    // Reset this before just before applying the actual location change (which might call OnLanded as well if currently airborne).
    bJustLandedPostPhysics = false;
  }
  else
  {
    // If we just landed the grounded physics are running with the remaining delta time and CalculateVelocity is called again, but we don't want to reset
    // bJustLandedPostPhysics in that case.
    gmc_ck(IsMovingOnGround())
  }
}

void UGMC_GoldSrcMovementCmp::ApplyDeceleration(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  if (Velocity.IsZero())
  {
    return;
  }

  if (HasRootMotion() && !RootMotionMetaData.bApplyDecelerationWithRootMotion)
  {
    return;
  }

  // This override is only for applying the slowdown when partially immersed in water.
  if (IsSwimming() || PartialImmersionSlowDown == 0.f)
  {
    return;
  }

  FVector Deceleration{0.};
  if (CurrentImmersionDepth >= PartialImmersionThreshold)
  {
    // We only apply the slowdown in the XY plane so the pawn can still jump to the full Z height.
    gmc_ck(PartialImmersionThreshold <= BuoyantStateMinImmersion - IMMERSION_DEPTH_TOLERANCE)
    Deceleration += PartialImmersionSlowDown * -FVector{Velocity.X, Velocity.Y, 0.};
  }

  const FVector StartVelocity = Velocity;
  if (!Deceleration.IsZero())
  {
    Deceleration = ClampToMinDeceleration(Deceleration);
    AddAcceleration(Deceleration, DeltaSeconds);
    const FVector VelocityXY = FVector(Velocity.X, Velocity.Y, 0.);
    const FVector VelocityZ = FVector(0., 0., Velocity.Z);
    if (DirectionsDifferXY(StartVelocity, Velocity) || VelocityXY.SizeSquared() <= FMath::Square(BRAKE_TO_STOP_VELOCITY))
    {
      UpdateVelocity(VelocityZ, DeltaSeconds);
    }
  }
}

void UGMC_GoldSrcMovementCmp::LimitSpeed(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  // Override to disable the super implementation, speed limiting is handled by PhysicsGoldSrc.
}

float UGMC_GoldSrcMovementCmp::GetInputAcceleration() const
{
  FLog(Warning, "<InputAcceleration> is not in use for this movement component.")
  return 0.f;
}

float UGMC_GoldSrcMovementCmp::GetBrakingDeceleration() const
{
  FLog(Warning, "<BrakingDeceleration> is not in use for this movement component.")
  return 0.f;
}

float UGMC_GoldSrcMovementCmp::GetOverMaxSpeedDeceleration() const
{
  FLog(Warning, "<OverMaxSpeedDeceleration> is not in use for this movement component.")
  return 0.f;
}

float UGMC_GoldSrcMovementCmp::GetMaxSpeed() const
{
  gmc_ck(MaxSpeed >= 0.f)

  if (IsServerBot())
  {
    // Get max speed for path following.
    if (bRequestedMoveWithMaxSpeed || UseAccelerationForPathFollowing())
    {
      return MaxSpeed;
    }
    return bUseRequestedVelocityMaxSpeed ? RequestedVelocity.Size() : MaxSpeed;
  }

  return MaxSpeed;
}

void UGMC_GoldSrcMovementCmp::ApplyInputVelocity(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
  if (HasRootMotion() && !RootMotionMetaData.bApplyInputWithRootMotion)
  {
    return;
  }

  const FVector WishMove = GetProcessedInputVector();
  FVector WishVelocity = WishMove * GetMaxSpeed();

  // Enforce min walk speed if moving.
  if (WishVelocity.SizeSquared() > 0.)
  {
    gmc_ck(MaxSpeed >= MinAnalogWalkSpeed)
    WishVelocity = WishVelocity.GetClampedToSize(MinAnalogWalkSpeed, UE_DOUBLE_BIG_NUMBER);
  }

  PhysicsGoldSrc(WishMove, WishVelocity, DeltaSeconds);
}

void UGMC_GoldSrcMovementCmp::PhysicsGoldSrc(const FVector& WishMove, FVector WishVelocity, float DeltaSeconds)
{
  const bool bInWater = IsSwimming();

  if (bInWater && WishMove.IsZero())
  {
    // When there's no input drift downwards.
    WishVelocity.Z -= SinkSpeed;
  }

  const bool bIsUsingCheatModes = IsUsingNoClipMode() || IsUsingFlyMode();

  if (!(bIsUsingCheatModes || bInWater))
  {
    WishVelocity.Z = 0.;
  }

  const FVector WishDirection = WishVelocity.GetSafeNormal();
  float WishSpeed = WishVelocity.Size();
  const float CurrentMaxSpeed = GetMaxSpeed();
  if (WishSpeed > CurrentMaxSpeed)
  {
    WishVelocity *= CurrentMaxSpeed / WishSpeed;
    WishSpeed = CurrentMaxSpeed;
  }

  if (bIsUsingCheatModes)
  {
    Velocity = WishVelocity;
    return;
  }

  if (bInWater)
  {
    WaterMove(WishDirection, WishSpeed, DeltaSeconds);
    return;
  }

  const bool bOnGround = IsMovingOnGround();
  if (IsAirborne() || (bOnGround && HasJustLanded_Internal()))
  {
    AirMove(WishDirection, WishSpeed, DeltaSeconds);
    return;
  }

  if (bOnGround)
  {
    WalkMove(WishDirection, WishSpeed, DeltaSeconds);
    return;
  }
}

void UGMC_GoldSrcMovementCmp::WalkMove(FVector WishDirection, float WishSpeed, float DeltaSeconds)
{
  ApplyFloorFriction(DeltaSeconds);
  Accelerate(WishDirection, WishSpeed, Velocity | WishDirection, DeltaSeconds);
}

void UGMC_GoldSrcMovementCmp::AirMove(FVector WishDirection, float WishSpeed, float DeltaSeconds)
{
  WishSpeed = FMath::Clamp(WishSpeed, 0.f, GetMaxSpeed() * AirControl);
  Accelerate(WishDirection, WishSpeed, Velocity | WishDirection, DeltaSeconds);
}

void UGMC_GoldSrcMovementCmp::WaterMove(FVector WishDirection, float WishSpeed, float DeltaSeconds)
{
  WishSpeed *= WaterControl;
  const float NewSpeed = ApplyWaterFriction(DeltaSeconds);
  Accelerate(WishDirection, WishSpeed, NewSpeed, DeltaSeconds);
}

void UGMC_GoldSrcMovementCmp::Accelerate(FVector WishDirection, float WishSpeed, float CurrentSpeed, float DeltaSeconds)
{
  float AddSpeed = WishSpeed - CurrentSpeed;
  if (AddSpeed <= 0)
  {
    return;
  }

  float AccelSpeed = (IsAirborne() ? AirAcceleration : Acceleration) * WishSpeed * DeltaSeconds;
  if (AccelSpeed > AddSpeed)
  {
    AccelSpeed = AddSpeed;
  }

  Velocity += AccelSpeed * WishDirection;
}

float UGMC_GoldSrcMovementCmp::ApplyFloorFriction(float DeltaSeconds)
{
  gmc_ck(Velocity.Z == 0.)

  float CurrentSpeed = GetSpeedXY();

  if (CurrentSpeed == 0.f)
  {
    return 0.f;
  }

  const bool bBelowMinWalkSpeed = CurrentSpeed < MinAnalogWalkSpeed;
  const bool bInStopRange = CurrentSpeed < StopSpeed && !bBelowMinWalkSpeed;
  const bool bNoInput = GMCCompression::IsNetNearlyZero(GetProcessedInputVector(), EGMC_FloatPrecision::TwoDecimals);
  const float Control = bInStopRange || (bBelowMinWalkSpeed && (bNoInput || bIgnoreInputModifier)) ? StopSpeed : CurrentSpeed;
  float NewSpeed = CurrentSpeed - DeltaSeconds * Control * GetFloorFriction();
  if (NewSpeed <= BRAKE_TO_STOP_VELOCITY)
  {
    NewSpeed = 0;
  }
  else if (NewSpeed < MinAnalogWalkSpeed && !bIgnoreInputModifier && !bNoInput)
  {
    const float VelocityDotInput = Velocity.GetSafeNormal() | GetProcessedInputVector().GetSafeNormal();
    if (FMath::IsNearlyEqual(VelocityDotInput, 1.f, UE_KINDA_SMALL_NUMBER))
    {
      NewSpeed = MinAnalogWalkSpeed;
    }
  }
  NewSpeed /= CurrentSpeed;

  Velocity *= NewSpeed;
  return NewSpeed;
}

float UGMC_GoldSrcMovementCmp::GetFloorFriction() const
{
  if (EdgeFrictionScale == 1.f || Velocity.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    return FloorFriction;
  }

  if (!bUseAltEdgeFriction)
  {
    const FVector VelocityXYDirection = Velocity.GetSafeNormal2D();
    const FVector LeadingEdge = VelocityXYDirection * ComputeDistanceToRootCollisionBoundaryXY(VelocityXYDirection);
    if (LeadingEdge.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
    {
      return FloorFriction;
    }

    FHitResult TraceHit{};
    const FVector TraceStart = GetLowerBound() + LeadingEdge;
    const FVector TraceEnd = TraceStart + FVector::DownVector * EdgeFrictionMinRequiredHeight;
    FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(GetFloorFriction), false, GetOwner());
    CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
    CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
    const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
    if (const auto& World = GetWorld())
    {
      World->LineTraceSingleByChannel(
        TraceHit,
        TraceStart,
        TraceEnd,
        UpdatedComponent->GetCollisionObjectType(),
        CollisionQueryParams,
        CollisionResponseParams
      );
    }

    if (!TraceHit.bBlockingHit)
    {
      // The leading edge is over a drop-off, increase the friction.
      return FloorFriction * EdgeFrictionScale;
    }
    return FloorFriction;
  }

  // Alternative behaviour: edge friction is only applied if the pawn is already halfway past the edge.
  const auto& TraceHit = CurrentFloor.LineHit();
  gmc_ck(FloorTraceLength > EdgeFrictionMinRequiredHeight)
  if (!TraceHit.bStartPenetrating && TraceHit.Distance > EdgeFrictionMinRequiredHeight)
  {
    return FloorFriction * EdgeFrictionScale;
  }
  return FloorFriction;
}

float UGMC_GoldSrcMovementCmp::ApplyWaterFriction(float DeltaSeconds)
{
  const float CurrentSpeed = Velocity.Size();

  if (CurrentSpeed == 0.f)
  {
    return 0.f;
  }

  float FluidFriction{0.f};
  if (const auto& PhysicsVolume = GetPhysicsVolume())
  {
    FluidFriction = PhysicsVolume->FluidFriction * FLUID_FRICTION_SCALE;
  }
  float NewSpeed = CurrentSpeed - FluidFriction * CurrentSpeed * DeltaSeconds;
  if (NewSpeed <= BRAKE_TO_STOP_VELOCITY)
  {
    NewSpeed = 0;
  }

  Velocity *= NewSpeed / CurrentSpeed;

  // Not actually the new speed of the pawn, this is intentional.
  return NewSpeed;
}
