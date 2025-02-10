// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCMovementUtilityComponent.h"
#include "GMCPawn.h"
#include "GMCFlatCapsuleComponent.h"
#include "GMCRollbackActor.h"
#include "Compression.h"
#include "GMCLog.h"
#include "GMCMovementUtilityComponent_DBG.h"

DECLARE_CYCLE_STAT(TEXT("PredictionTick"), STAT_PredictionTick, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("SimulationTick"), STAT_SimulationTick, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("AncillaryTick"), STAT_AncillaryTick, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("UpdateFloor"), STAT_UpdateFloor, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("ComputeImmersionDepth"), STAT_ComputeImmersionDepth, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("SetRootCollisionShape"), STAT_SetRootCollisionShape, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("SetRootCollisionExtent"), STAT_SetRootCollisionExtent, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("IsValidPosition"), STAT_IsValidPosition, STATGROUP_UGMC_MovementUtilityCmp)
DECLARE_CYCLE_STAT(TEXT("AutoResolvePenetration"), STAT_AutoResolvePenetration, STATGROUP_UGMC_MovementUtilityCmp)

extern ENGINE_API float GAverageFPS;
extern ENGINE_API float GAverageMS;

namespace GMCCVars
{
#if ALLOW_CONSOLE && !NO_LOGGING

  int32 ShowMovementVectors = 0;
  FAutoConsoleVariableRef CVarShowMovementVectors(
    TEXT("gmc.ShowMovementVectors"),
    ShowMovementVectors,
    TEXT("Visualize velocity and acceleration vectors. 0: Disable, 1: Enable"),
    ECVF_Default
  );

#endif
}

UGMC_MovementUtilityCmp::UGMC_MovementUtilityCmp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{}

void UGMC_MovementUtilityCmp::PredictionTick(
  const FGMC_PawnState& InputState,
  float DeltaTime,
  double Timestamp,
  int32 Iteration,
  bool bSubSteppedIteration,
  bool bLocal,
  bool bSimulated,
  bool bCombined
)
{
  SCOPE_CYCLE_COUNTER(STAT_PredictionTick)

  MoveTimestamp = Timestamp;
  MoveDeltaTime = DeltaTime;
  MoveInputState = InputState;
  MoveIteration = Iteration;
  bIsSubSteppedMoveIteration = bSubSteppedIteration;
  bIsLocalMove = bLocal;
  bIsSimulatedMove = bSimulated;
  bIsCombinedMove = bCombined;

  MoveDeltaTime = CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, PreProcessMoveDeltaTime, MoveDeltaTime);

  // Reset physics values.
  SetPhysDeltaTime(MoveDeltaTime, true);

  FLog(
    VeryVerbose,
    "Starting execution of move with timestamp %f (move delta time = %f, sub-stepped iteration = %s).",
    MoveTimestamp,
    MoveDeltaTime,
    BOOL_TO_STR(bIsSubSteppedMoveIteration)
  )

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, GenPredictionTick, MoveDeltaTime);

  FLog(
    VeryVerbose,
    "Finished execution of move with timestamp %f (move delta time = %f, sub-stepped iteration = %s).",
    MoveTimestamp,
    MoveDeltaTime,
    BOOL_TO_STR(bIsSubSteppedMoveIteration)
  )

  DEBUG_SHOW_MOVEMENT_VECTORS
}

void UGMC_MovementUtilityCmp::SimulationTick(float DeltaTime, double SimTime, int32 TargetIdx, const TArray<int32>& SkippedIdx)
{
  SCOPE_CYCLE_COUNTER(STAT_SimulationTick)

  CurrentSmoothingTime = SimTime;
  CurrentSmoothingTargetIdx = TargetIdx;
  CurrentSmoothingSkippedIdx = SkippedIdx;

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, GenSimulationTick, DeltaTime);
}

void UGMC_MovementUtilityCmp::AncillaryTick(
  const FGMC_Move& Move,
  float DeltaTime,
  double Timestamp,
  bool bLocalMove,
  bool bCombinedClientMove,
  bool& bOutDoNotCombineNextClientMove
)
{
  SCOPE_CYCLE_COUNTER(STAT_AncillaryTick)

  CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, GenAncillaryTick, DeltaTime, bLocalMove, bCombinedClientMove);
}

void UGMC_MovementUtilityCmp::ClearTransientData(bool bResetMoves)
{
  Super::ClearTransientData(bResetMoves);

  MoveTimestamp = 0.;
  MoveDeltaTime = 0.f;

  MoveInputState = FGMC_PawnState{};

  CurrentSmoothingTime = 0.;
  CurrentSmoothingTargetIdx = 0;
  CurrentSmoothingSkippedIdx.Reset();

  PhysDeltaTime = 0.f;
  Acceleration = FVector::ZeroVector;
  Force = FVector::ZeroVector;
  Velocity0 = FVector::ZeroVector;
  Location0 = FVector::ZeroVector;

  MoveIteration = 0;
  bIsSubSteppedMoveIteration = false;
  bIsLocalMove = false;
  bIsSimulatedMove = false;
  bIsCombinedMove = false;
}

float UGMC_MovementUtilityCmp::PreProcessMoveDeltaTime_Implementation(float RealDeltaTime) const
{
  const auto& World = GetWorld();
  if (!World)
  {
    return RealDeltaTime;
  }

  const auto& WorldSettings = World->GetWorldSettings();
  if (!WorldSettings)
  {
    return RealDeltaTime;
  }

  const float CustomTimeDilation = PawnOwner ? PawnOwner->CustomTimeDilation : 1.f;
  return FMath::Clamp(RealDeltaTime * WorldSettings->GetEffectiveTimeDilation() * CustomTimeDilation, MIN_DELTA_TIME, UE_BIG_NUMBER);
}

double UGMC_MovementUtilityCmp::GetMoveTimestamp() const
{
  return MoveTimestamp;
}

float UGMC_MovementUtilityCmp::GetMoveDeltaTime() const
{
  return MoveDeltaTime;
}

FVector UGMC_MovementUtilityCmp::GetTransientAcceleration() const
{
  return Acceleration;
}

FVector UGMC_MovementUtilityCmp::GetTransientForce() const
{
  return Force;
}

FVector UGMC_MovementUtilityCmp::GetStartVelocity() const
{
  return Velocity0;
}

FVector UGMC_MovementUtilityCmp::GetStartLocation() const
{
  return Location0;
}

float UGMC_MovementUtilityCmp::GetSpeed() const
{
  return Velocity.Size();
}

float UGMC_MovementUtilityCmp::GetSpeedXY() const
{
  return Velocity.Size2D();
}

float UGMC_MovementUtilityCmp::GetSpeedZ() const
{
  return Velocity.Z;
}

FVector UGMC_MovementUtilityCmp::ResizeVector(const FVector& Vector, double Size)
{
  const double OriginalSize = Vector.Size();

  if (Size < UE_SMALL_NUMBER || OriginalSize < UE_SMALL_NUMBER)
  {
    return FVector::ZeroVector;
  }

  return Vector * (Size / OriginalSize);
}

bool UGMC_MovementUtilityCmp::DirectionsDiffer(const FVector& D1, const FVector& D2)
{
  return (D1 | D2) <= 0.;
}

bool UGMC_MovementUtilityCmp::DirectionsDifferXY(const FVector& D1, const FVector& D2)
{
  return (FVector(D1.X, D1.Y, 0.) | FVector(D2.X, D2.Y, 0.)) <= 0.;
}

bool UGMC_MovementUtilityCmp::DirectionsDifferZ(const FVector& D1, const FVector& D2)
{
  return D1.Z * D2.Z <= 0.;
}

const FGMC_PawnState& UGMC_MovementUtilityCmp::GetMoveInputState() const
{
  return MoveInputState;
}

int32 UGMC_MovementUtilityCmp::GetIterationNumber() const
{
  return MoveIteration;
}

bool UGMC_MovementUtilityCmp::IsSubSteppedIteration() const
{
  return bIsSubSteppedMoveIteration;
}

bool UGMC_MovementUtilityCmp::IsLocalMove() const
{
  return bIsLocalMove;
}

bool UGMC_MovementUtilityCmp::IsSimulatedMove() const
{
  return bIsSimulatedMove;
}

bool UGMC_MovementUtilityCmp::IsCombinedMove() const
{
  return bIsCombinedMove;
}

double UGMC_MovementUtilityCmp::GetSmoothingTime() const
{
  return CurrentSmoothingTime;
}

int32 UGMC_MovementUtilityCmp::GetSmoothingTargetIdx() const
{
  return CurrentSmoothingTargetIdx;
}

TArray<int32> UGMC_MovementUtilityCmp::GetSkippedSmoothingIndices() const
{
  return CurrentSmoothingSkippedIdx;
}

float UGMC_MovementUtilityCmp::GetPhysDeltaTime() const
{
  return PhysDeltaTime;
}

void UGMC_MovementUtilityCmp::UpdateVelocity(const FVector& NewVelocity, float DeltaTime)
{
  if (NewVelocity != Velocity)
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    if (DeltaTime >= MIN_DELTA_TIME)
    {
      Velocity = NewVelocity;
      Acceleration = (Velocity - Velocity0) / DeltaTime;
      Force = Mass * Acceleration;
    }
  }
}

void UGMC_MovementUtilityCmp::UpdateAcceleration(const FVector& NewAcceleration, float DeltaTime)
{
  if (NewAcceleration != Acceleration)
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    if (DeltaTime >= MIN_DELTA_TIME)
    {
      Acceleration = NewAcceleration;
      Velocity = Velocity0 + Acceleration * DeltaTime;
      Force = Mass * Acceleration;
    }
  }
}

void UGMC_MovementUtilityCmp::UpdateForce(const FVector& NewForce, float DeltaTime)
{
  if (NewForce != Force)
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    if (DeltaTime >= MIN_DELTA_TIME)
    {
      gmc_ck(Mass >= UE_KINDA_SMALL_NUMBER)
      Force = NewForce;
      Velocity = Velocity0 + Force * DeltaTime / FMath::Max(Mass, UE_KINDA_SMALL_NUMBER);
      Acceleration = (Velocity - Velocity0) / DeltaTime;
    }
  }
}

void UGMC_MovementUtilityCmp::AddVelocity(const FVector& AddVelocity, float DeltaTime)
{
  if (!AddVelocity.IsZero())
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    if (DeltaTime >= MIN_DELTA_TIME)
    {
      Velocity += AddVelocity;
      const FVector AddAcceleration = AddVelocity / DeltaTime;
      Acceleration += AddAcceleration;
      Force += AddAcceleration * Mass;
    }
  }
}

void UGMC_MovementUtilityCmp::AddAcceleration(const FVector& AddAcceleration, float DeltaTime)
{
  if (!AddAcceleration.IsZero())
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    if (DeltaTime >= MIN_DELTA_TIME)
    {
      Acceleration += AddAcceleration;
      Velocity += AddAcceleration * DeltaTime;
      Force += AddAcceleration * Mass;
    }
  }
}

void UGMC_MovementUtilityCmp::AddForce(const FVector& AddForce, float DeltaTime)
{
  if (!AddForce.IsZero())
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    if (DeltaTime >= MIN_DELTA_TIME)
    {
      gmc_ck(Mass >= UE_KINDA_SMALL_NUMBER)
      Force += AddForce;
      const FVector AddVelocity = AddForce * DeltaTime / FMath::Max(Mass, UE_KINDA_SMALL_NUMBER);
      Velocity += AddVelocity;
      Acceleration = AddVelocity / DeltaTime;
    }
  }
}

void UGMC_MovementUtilityCmp::AddImpulse(const FVector& Impulse, bool bVelChange)
{
  if (!Impulse.IsZero())
  {
    if (!bVelChange)
    {
      gmc_ck(Mass >= UE_KINDA_SMALL_NUMBER)
      AddVelocity(Impulse / FMath::Max(Mass, UE_KINDA_SMALL_NUMBER), 1.f);
    }
    else
    {
      // No scaling by mass i.e. mass is assumed to be 1 kg.
      AddVelocity(Impulse, 1.f);
    }
  }
}

void UGMC_MovementUtilityCmp::AddRadialForce(
  const FVector& Origin,
  float Radius,
  float Strength,
  EGMC_RadialImpulseFalloff Falloff,
  float DeltaTime
)
{
  if (Radius > 0.f && Strength > 0.f)
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    if (DeltaTime >= MIN_DELTA_TIME)
    {
      const FVector Delta = UpdatedComponent->GetComponentLocation() - Origin;
      const float DistanceFromSource = Delta.Size();
      if (DistanceFromSource > Radius)
      {
        // We are not affected if we are outside of the force radius.
        return;
      }

      float EffectiveStrength = Strength;
      if (Falloff == EGMC_RadialImpulseFalloff::Linear && Radius > UE_KINDA_SMALL_NUMBER)
      {
        // Falloff is not constant meaning <Strength> is the max strength of the force at the point of origin and drops linearly with the distance from the
        // origin.
        EffectiveStrength = Strength * (1.f - (DistanceFromSource / Radius));
      }
      const FVector ForceDirection = Delta.GetSafeNormal();
      AddForce(ForceDirection * EffectiveStrength, DeltaTime);
    }
  }
}

void UGMC_MovementUtilityCmp::AddRadialForce(const FVector& Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff)
{
  AddRadialForce(Origin, Radius, Strength, static_cast<EGMC_RadialImpulseFalloff>(Falloff), GetPhysDeltaTime());
}

void UGMC_MovementUtilityCmp::AddRadialImpulse(
  const FVector& Origin,
  float Radius,
  float Strength,
  EGMC_RadialImpulseFalloff Falloff,
  bool bVelChange
)
{
  if (Radius > 0.f && Strength > 0.f)
  {
    const FVector Delta = UpdatedComponent->GetComponentLocation() - Origin;
    const float DistanceFromSource = Delta.Size();
    if (DistanceFromSource > Radius)
    {
      // We are not affected if we are outside of the impulse radius.
      return;
    }

    float EffectiveStrength = Strength;
    if (Falloff == EGMC_RadialImpulseFalloff::Linear && Radius > UE_KINDA_SMALL_NUMBER)
    {
      // Falloff is not constant meaning <Strength> is the max strength of the impulse at the point of origin and drops linearly with the distance from the
      // origin.
      EffectiveStrength = Strength * (1.f - (DistanceFromSource / Radius));
    }
    const FVector ImpulseDirection = Delta.GetSafeNormal();
    AddImpulse(ImpulseDirection * EffectiveStrength, bVelChange);
  }
}

void UGMC_MovementUtilityCmp::AddRadialImpulse(
  const FVector& Origin,
  float Radius,
  float Strength,
  ERadialImpulseFalloff Falloff,
  bool bVelChange
)
{
  AddRadialImpulse(Origin, Radius, Strength, static_cast<EGMC_RadialImpulseFalloff>(Falloff), bVelChange);
}

void UGMC_MovementUtilityCmp::SetPhysDeltaTime(float NewDeltaTime, bool bReset)
{
  PhysDeltaTime = NewDeltaTime;
  if (bReset && IsValid(UpdatedComponent))
  {
    Location0 = UpdatedComponent->GetComponentLocation();
    Velocity0 = Velocity;
    Acceleration = FVector::ZeroVector;
    Force = FVector::ZeroVector;
  }
}

FVector UGMC_MovementUtilityCmp::TransformInputVectorAbsoluteZ(const FVector& AbsoluteInputVector) const
{
  if (!IsValid(PawnOwner) || AbsoluteInputVector.IsZero())
  {
    return FVector{0.};
  }

  FRotator ControlRotation = GetControllerRotation_GMC();
  ControlRotation.Pitch = 0.;
  const FVector InputVectorAbsoluteZ = ControlRotation.RotateVector(AbsoluteInputVector);
  return {InputVectorAbsoluteZ.X, InputVectorAbsoluteZ.Y, AbsoluteInputVector.Z};
}

FVector UGMC_MovementUtilityCmp::TransformInputVectorAllRelative(const FVector& AbsoluteInputVector) const
{
  if (!IsValid(PawnOwner) || AbsoluteInputVector.IsZero())
  {
    return FVector{0.};
  }

  return GetControllerRotation_GMC().RotateVector(AbsoluteInputVector);
}

void UGMC_MovementUtilityCmp::UpdateVelocityFromMovedDistance(float DeltaTime, float MaxSize)
{
  DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
  if (DeltaTime >= MIN_DELTA_TIME && IsValid(UpdatedComponent))
  {
    UpdateVelocity((UpdatedComponent->GetComponentLocation() - Location0) / DeltaTime, DeltaTime);
    if (MaxSize > 0.f)
    {
      UpdateVelocity(Velocity.GetClampedToMaxSize(MaxSize), DeltaTime);
    }
  }
}

bool UGMC_MovementUtilityCmp::HasMoved() const
{
  if (!IsValid(UpdatedComponent))
  {
    return false;
  }

  return !UpdatedComponent->GetComponentLocation().Equals(Location0);
}

void UGMC_MovementUtilityCmp::AdjustVelocityFromHit(const FHitResult& Hit, bool bUseRelative, float DeltaTime)
{
  if (!Hit.IsValidBlockingHit())
  {
    return;
  }

  // The hit normal can differ slightly between client and server which can lead to more significant errors when projecting the velocity. To mitigate this
  // problem we round the normal to 2 decimal places to get consistent results across client and server.
  const FVector Normal = RoundBasedVectorByRotation(Hit.Normal, bUseRelative ? GetActorBase() : nullptr, EGMC_FloatPrecisionBlueprint::TwoDecimals);

  if (Normal.IsZero())
  {
    return;
  }

  if ((Velocity | Normal) <= 0.)
  {
    // When colliding with something, we need to adjust the velocity into a direction that does not go into the hit object. We also adjust the saved start
    // velocity here to avoid artifacts in the acceleration calculation.
    Velocity0 = FVector::VectorPlaneProject(Velocity0, Normal);
    UpdateVelocity(FVector::VectorPlaneProject(Velocity, Normal), DeltaTime);
  }
}

void UGMC_MovementUtilityCmp::AdjustVelocityFromNormal(const FVector& Normal, bool bUseRelative, float DeltaTime)
{
  FVector SafeNormal = Normal.GetSafeNormal();
  SafeNormal = RoundBasedVectorByRotation(SafeNormal, bUseRelative ? GetActorBase() : nullptr, EGMC_FloatPrecisionBlueprint::TwoDecimals);

  if (SafeNormal.IsZero())
  {
    return;
  }

  if ((Velocity | SafeNormal) <= 0.)
  {
    Velocity0 = FVector::VectorPlaneProject(Velocity0, SafeNormal);
    UpdateVelocity(FVector::VectorPlaneProject(Velocity, SafeNormal), DeltaTime);
  }
}

double UGMC_MovementUtilityCmp::GenerateNetRandomNumber(int32 Max) const
{
  if (Max > 0)
  {
    constexpr double Factor = 99999989./*prime*/;
    return FMath::Fmod(GetMoveTimestamp() * Factor, (double)Max);
  }

  return 0.;
}

double UGMC_MovementUtilityCmp::GenerateNetRandomAngle() const
{
  constexpr double Factor = 99991./*prime*/;
  return FMath::Fmod(GetMoveTimestamp() * Factor, 360.);
}

float UGMC_MovementUtilityCmp::RoundFloat(float FloatToRound, EGMC_FloatPrecisionBlueprint Level)
{
  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return FloatToRound;
  }

  float RoundedFloat = FloatToRound;
  GMCCompression::Round(RoundedFloat, Scale);
  return RoundedFloat;
}

double UGMC_MovementUtilityCmp::RoundDouble(double DoubleToRound, EGMC_FloatPrecisionBlueprint Level)
{
  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return DoubleToRound;
  }

  double RoundedDouble = DoubleToRound;
  GMCCompression::Round(RoundedDouble, Scale);
  return RoundedDouble;
}


FVector UGMC_MovementUtilityCmp::RoundVector(const FVector& VectorToRound, EGMC_FloatPrecisionBlueprint Level)
{
  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return VectorToRound;
  }

  FVector RoundedVector = VectorToRound;
  GMCCompression::Round3D(RoundedVector, Scale);
  return RoundedVector;
}

FRotator UGMC_MovementUtilityCmp::RoundRotator(const FRotator& RotatorToRound, EGMC_FloatPrecisionBlueprint Level)
{
  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return RotatorToRound;
  }

  FRotator RoundedRotator = RotatorToRound;
  GMCCompression::Round3D(RoundedRotator, Scale);
  return RoundedRotator;
}

FVector UGMC_MovementUtilityCmp::RoundBasedVector(const FVector& WorldVectorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level)
{
  if (!IsValid(Base))
  {
    return RoundVector(WorldVectorToRound, Level);
  }

  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return WorldVectorToRound;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  const auto& BaseLocation = Base->GetComponentLocation();
  FVector RoundedVector = RotationDelta.UnrotateVector(WorldVectorToRound - BaseLocation);
  GMCCompression::Round3D(RoundedVector, Scale);
  RoundedVector = BaseLocation + RotationDelta.RotateVector(RoundedVector);
  return RoundedVector;
}

FVector UGMC_MovementUtilityCmp::RoundBasedVectorByRotation(const FVector& WorldVectorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level)
{
  if (!IsValid(Base))
  {
    return RoundVector(WorldVectorToRound, Level);
  }

  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return WorldVectorToRound;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  FVector RoundedVector = RotationDelta.UnrotateVector(WorldVectorToRound);
  GMCCompression::Round3D(RoundedVector, Scale);
  RoundedVector = RotationDelta.RotateVector(RoundedVector);
  return RoundedVector;
}

FRotator UGMC_MovementUtilityCmp::RoundBasedRotator(const FRotator& WorldRotatorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level)
{
  if (!IsValid(Base))
  {
    return RoundRotator(WorldRotatorToRound, Level);
  }

  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return WorldRotatorToRound;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  FRotator RoundedRotator = (RotationDelta.Inverse() * WorldRotatorToRound.Quaternion()).Rotator();
  GMCCompression::Round3D(RoundedRotator, Scale);
  RoundedRotator = (RotationDelta * RoundedRotator.Quaternion()).Rotator();
  return RoundedRotator;
}

FRotator UGMC_MovementUtilityCmp::RoundBasedRotatorYaw(const FRotator& WorldRotatorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level)
{
  if (!IsValid(Base))
  {
    return {WorldRotatorToRound.Pitch, RoundDouble(WorldRotatorToRound.Yaw, Level), WorldRotatorToRound.Roll};
  }

  EGMC_FloatPrecision Scale = ToNativeEnum(Level);
  if (Scale == FullPrecision)
  {
    return WorldRotatorToRound;
  }

  const FRotator RotationDelta = {0., Base->GetComponentRotation().Yaw, 0.};
  FRotator RoundedRotator = WorldRotatorToRound - RotationDelta;
  GMCCompression::Round(RoundedRotator.Yaw, Scale);
  RoundedRotator = WorldRotatorToRound + RotationDelta;
  return RoundedRotator.GetNormalized();
}

FVector UGMC_MovementUtilityCmp::GetBasedVector(const FVector& WorldVector, USceneComponent* Base)
{
  if (!IsValid(Base))
  {
    return WorldVector;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  return RotationDelta.UnrotateVector(WorldVector - Base->GetComponentLocation());
}

FVector UGMC_MovementUtilityCmp::GetBasedVectorByRotation(const FVector& WorldVector, USceneComponent* Base)
{
  if (!IsValid(Base))
  {
    return WorldVector;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  return RotationDelta.UnrotateVector(WorldVector);
}

FRotator UGMC_MovementUtilityCmp::GetBasedRotator(const FRotator& WorldRotator, USceneComponent* Base)
{
  if (!IsValid(Base))
  {
    return WorldRotator;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  return (RotationDelta.Inverse() * WorldRotator.Quaternion()).Rotator();
}

FRotator UGMC_MovementUtilityCmp::GetBasedRotatorYaw(const FRotator& WorldRotator, USceneComponent* Base)
{
  if (!IsValid(Base))
  {
    return WorldRotator;
  }

  const FRotator RotationDelta = {0., Base->GetComponentRotation().Yaw, 0.};
  return (WorldRotator - RotationDelta).GetNormalized();
}

FVector UGMC_MovementUtilityCmp::GetWorldVector(const FVector& RelativeVector, USceneComponent* Base)
{
  if (!IsValid(Base))
  {
    return RelativeVector;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  return Base->GetComponentLocation() + RotationDelta.RotateVector(RelativeVector);
}

FVector UGMC_MovementUtilityCmp::GetWorldVectorByRotation(const FVector& RelativeVector, USceneComponent* Base)
{
  if (!IsValid(Base))
  {
    return RelativeVector;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  return RotationDelta.RotateVector(RelativeVector);
}

FRotator UGMC_MovementUtilityCmp::GetWorldRotator(const FRotator& RelativeRotator, USceneComponent* Base)
{
  if (!IsValid(Base))
  {
    return RelativeRotator;
  }

  const auto& RotationDelta = Base->GetComponentQuat();
  return (RotationDelta * RelativeRotator.Quaternion()).Rotator();
}

FRotator UGMC_MovementUtilityCmp::ToAngularVelocityRotator(const FVector& InAngularVelocity)
{
  return FRotator{InAngularVelocity.Y, InAngularVelocity.Z, InAngularVelocity.X}.GetNormalized();
}

FQuat UGMC_MovementUtilityCmp::ToAngularVelocityQuat(const FVector& InAngularVelocity)
{
  return ToAngularVelocityRotator(InAngularVelocity).Quaternion();
}

bool UGMC_MovementUtilityCmp::IsMovable(UPrimitiveComponent* Component)
{
  return IsValid(Component) && Component->Mobility == EComponentMobility::Movable;
}

FVector UGMC_MovementUtilityCmp::GetLinearComponentVelocity(UPrimitiveComponent* Component)
{
  if (!IsValid(Component))
  {
    return FVector::ZeroVector;
  }

  if (!IsMovable(Component))
  {
    return FVector::ZeroVector;
  }

  const auto& Owner = Component->GetOwner();

  if (const auto& RollbackActor = Cast<AGMC_RollbackActor>(Owner))
  {
    return RollbackActor->GetVelocity();
  }

  if (const auto& GMCPawn = Cast<AGMC_Pawn>(Owner))
  {
    if (const auto& GMCReplicationCmpBase = GMCPawn->GetReplicationComponent())
    {
      return GMCReplicationCmpBase->GetLinearVelocity_GMC();
    }
  }

  if (const auto& BodyInstance = Component->GetBodyInstance())
  {
    return BodyInstance->GetUnrealWorldVelocity();
  }

  return FVector::ZeroVector;
}

FVector UGMC_MovementUtilityCmp::GetAngularComponentVelocityInRadians(UPrimitiveComponent* Component)
{
  if (!IsValid(Component))
  {
    return FVector::ZeroVector;
  }

  if (!IsMovable(Component))
  {
    return FVector::ZeroVector;
  }

  const auto& Owner = Component->GetOwner();

  if (const auto& RollbackActor = Cast<AGMC_RollbackActor>(Owner))
  {
    return RollbackActor->GetAngularVelocityInRadians();
  }

  if (const auto& GMCPawn = Cast<AGMC_Pawn>(Owner))
  {
    if (const auto& GMCReplicationCmpBase = GMCPawn->GetReplicationComponent())
    {
      return GMCReplicationCmpBase->GetAngularVelocity_GMC();
    }
  }

  if (const auto& BodyInstance = Component->GetBodyInstance())
  {
    return BodyInstance->GetUnrealWorldAngularVelocityInRadians();
  }

  return FVector::ZeroVector;
}

FVector UGMC_MovementUtilityCmp::GetAngularComponentVelocityInDegrees(UPrimitiveComponent* Component)
{
  return FMath::RadiansToDegrees(GetAngularComponentVelocityInRadians(Component));
}

FVector UGMC_MovementUtilityCmp::ComputeTangentialVelocity(const FVector& WorldLocation, UPrimitiveComponent* Component)
{
  if (!IsValid(Component))
  {
    return FVector::ZeroVector;
  }

  if (!IsMovable(Component))
  {
    return FVector::ZeroVector;
  }

  const auto& Owner = Component->GetOwner();

  if (const auto& RollbackActor = Cast<AGMC_RollbackActor>(Owner))
  {
    return RollbackActor->ComputeTangentialVelocity(WorldLocation);
  }

  if (const auto& GMCPawn = Cast<AGMC_Pawn>(Owner))
  {
    if (const auto& GMCReplicationCmpBase = GMCPawn->GetReplicationComponent())
    {
      return GMCReplicationCmpBase->GetTangentialVelocity_GMC(WorldLocation);
    }
  }

  if (const auto& BodyInstance = Component->GetBodyInstance())
  {
    const FVector AngularVelocity = BodyInstance->GetUnrealWorldAngularVelocityInRadians();
    if (!AngularVelocity.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
    {
      const FVector Location = Component->GetComponentLocation();
      const FVector DistanceToComponent = WorldLocation - Location;
      const FVector TangentialVelocity = AngularVelocity ^ DistanceToComponent;
      return TangentialVelocity;
    }
  }

  return FVector::ZeroVector;
}

FVector UGMC_MovementUtilityCmp::GetImpartVelocity(UPrimitiveComponent* Component, bool bGetLinearVelocity, bool bGetTangentialVelocity)
{
  if (!IsValid(Component))
  {
    return FVector::ZeroVector;
  }

  if (!IsMovable(Component))
  {
    return FVector::ZeroVector;
  }

  FVector ImpartVelocity{0.};

  if (bGetLinearVelocity)
  {
    ImpartVelocity += GetLinearComponentVelocity(Component);
  }

  if (bGetTangentialVelocity)
  {
    ImpartVelocity += ComputeTangentialVelocity(GetLowerBound(), Component);
  }

  return ImpartVelocity;
}

FVector UGMC_MovementUtilityCmp::GetLowerBound() const
{
  if (!IsValid(UpdatedComponent))
  {
    return FVector{0.};
  }

  const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
  const FVector Bounds = UpdatedComponent->Bounds.BoxExtent;
  return FVector(CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z - Bounds.Z);
}

FVector UGMC_MovementUtilityCmp::CalculateAirResistance(const FVector& CurrentVelocity, float DragCoefficient)
{
  if (DragCoefficient <= 0.f)
  {
    return FVector{0.};
  }

  return -CurrentVelocity.GetSafeNormal() * CurrentVelocity.SizeSquared() * DragCoefficient;
}

FVector UGMC_MovementUtilityCmp::CalculateRollingResistance(
  const FVector& CurrentVelocity,
  float PawnMass,
  float GravityZ,
  float RollingCoefficient
)
{
  if (RollingCoefficient <= 0.f || PawnMass < UE_KINDA_SMALL_NUMBER)
  {
    return FVector{0.};
  }

  const float NormalForce = PawnMass * FMath::Abs(GravityZ);
  return -CurrentVelocity.GetSafeNormal() * NormalForce * RollingCoefficient;
}

FVector UGMC_MovementUtilityCmp::GetPlaneNormalWithWorldZ(const FVector& Direction)
{
  return (Direction ^ FVector::UpVector).GetSafeNormal();
}

bool UGMC_MovementUtilityCmp::UpdateFloor(
  FGMC_FloorParams& Floor,
  const FVector& Direction,
  float TraceLength,
  float Tolerance,
  float ShapeExtentScale,
  bool bAutoAdjust,
  bool bForceUpdate
)
{
  SCOPE_CYCLE_COUNTER(STAT_UpdateFloor)

  if (!IsUpdatedComponentRootCollision())
  {
    return false;
  }

  if (!bForceUpdate && !Floor.IsDirty(Tolerance, this))
  {
    FLog(VeryVerbose, "Floor is up to date (tolerance = %f), returning.", Tolerance)
    return false;
  }

  FHitResult ShapeHit{};
  FHitResult LineHit{};

  if (!UpdatedComponent->GetCollisionEnabled())
  {
    Floor = FGMC_FloorParams(ShapeHit, LineHit, this);
    return true;
  }

  const FVector NormalizedDirection = Direction.GetSafeNormal();
  if (NormalizedDirection.IsZero())
  {
    return false;
  }

  // Execute the shape trace.
  bool bValidShapeHitData{false};
  ShapeHit = SweepRootCollisionSingleByChannel(
    NormalizedDirection,
    TraceLength,
    GetRootCollisionExtent(true) * ShapeExtentScale,
    FQuat::Identity,
    UpdatedComponent->GetCollisionObjectType()
  );

  FVector CurrentLocation = UpdatedComponent->GetComponentLocation();

  if (bAutoAdjust && ShapeHit.bStartPenetrating)
  {
    if (ResolveRootCollisionPenetration(ShapeHit))
    {
      CurrentLocation = UpdatedComponent->GetComponentLocation();

      // Execute the shape trace again after the adjustment.
      ShapeHit = SweepRootCollisionSingleByChannel(
        NormalizedDirection,
        TraceLength,
        FVector::ZeroVector,
        FQuat::Identity,
        UpdatedComponent->GetCollisionObjectType()
      );
    }
  }

  if (ShapeHit.IsValidBlockingHit())
  {
    // Only consider hits that are below the vertical portion of the collision shape.
    const float MaxSweepZ = CurrentLocation.Z - GetRootCollisionOuterHalfHeight(true);
    if (ShapeHit.ImpactPoint.Z < MaxSweepZ)
    {
      bValidShapeHitData = true;
      FLog(VeryVerbose, "Has valid shape hit (\"%s\" = %f).", TO_STR(ShapeHit.Distance), ShapeHit.Distance)
    }
  }

  // Execute the line trace.
  bool bValidLineHitData{false};
  const FVector LineTraceStart = CurrentLocation + NormalizedDirection * GetRootCollisionHalfHeight(true);
  const FVector LineTraceEnd = LineTraceStart + NormalizedDirection * TraceLength;
  FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(UpdateFloor), false, GetOwner());
  CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
  CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
  const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
  if (const auto& World = GetWorld())
  {
    World->LineTraceSingleByChannel(
      LineHit,
      LineTraceStart,
      LineTraceEnd,
      UpdatedComponent->GetCollisionObjectType(),
      CollisionQueryParams,
      CollisionResponseParams
    );
  }
  if (LineHit.IsValidBlockingHit())
  {
    bValidLineHitData = true;
    FLog(VeryVerbose, "Has valid line hit (\"%s\" = %f).", TO_STR(LineHit.Distance), LineHit.Distance)
  }

  Floor = FGMC_FloorParams(ShapeHit, LineHit, this);
  return true;
}

bool UGMC_MovementUtilityCmp::ResolveRootCollisionPenetration(const FHitResult& Hit, float MaxAdjustment)
{
  if (!Hit.bStartPenetrating)
  {
    return true;
  }

  // Move a little further away from the surface and try again. This should not be a major adjustment.
  FVector PenetrationAdjustment = GetPenetrationAdjustment(Hit);
  PenetrationAdjustment = PenetrationAdjustment.GetClampedToMaxSize(MaxAdjustment);

  const FVector StartLocation = UpdatedComponent->GetComponentLocation();

  if (ResolvePenetration(PenetrationAdjustment, Hit, UpdatedComponent->GetComponentQuat()))
  {
    FVector ActualAdjustment = UpdatedComponent->GetComponentLocation() - StartLocation;

    if (ActualAdjustment.Z > UE_DOUBLE_KINDA_SMALL_NUMBER)
    {
      // The pawn's position was adjusted upward. Move back as far as possible so we don't potentially lose contact with the ground.
      MoveUpdatedComponent({0., 0., -ActualAdjustment.Z}, UpdatedComponent->GetComponentRotation(), true, nullptr, ETeleportType::TeleportPhysics);
      ActualAdjustment = UpdatedComponent->GetComponentLocation() - StartLocation;
    }

    FLog(
      Verbose,
      "Updated component location was adjusted by {%s}.",
      *ActualAdjustment.ToString()
    )

    return true;
  }

  return false;
}

bool UGMC_MovementUtilityCmp::CanMove_Implementation() const
{
  if (!IsValid(UpdatedComponent))
  {
    return false;
  }

  return UpdatedComponent->Mobility == EComponentMobility::Movable && Mass >= UE_KINDA_SMALL_NUMBER;
}

bool UGMC_MovementUtilityCmp::HasMoveInputEnabled() const
{
  if (IsValid(PawnOwner))
  {
    bool bInputEnabled = PawnOwner->InputEnabled();
    if (const auto& Controller = PawnOwner->GetController())
    {
      bInputEnabled &= !Controller->IsMoveInputIgnored();
      if (const auto& PlayerController = Cast<APlayerController>(Controller))
      {
        bInputEnabled &= PlayerController->InputEnabled();
      }
    }
    return bInputEnabled;
  }

  return false;
}

void UGMC_MovementUtilityCmp::HaltMovement()
{
  Velocity = Velocity0/*zero start velocity as well*/ = Acceleration = Force = FVector::ZeroVector;
}

EGMC_CollisionShape UGMC_MovementUtilityCmp::GetRootCollisionShape() const
{
  if (IsValid(PawnOwner))
  {
    const auto& RootComponent = PawnOwner->GetRootComponent();

    // The horizontal capsule must be checked first because it inherits from UCapsuleComponent.
    if (Cast<UGMC_FlatCapsuleCmp>(RootComponent)) return EGMC_CollisionShape::HorizontalCapsule;
    if (Cast<UCapsuleComponent>(RootComponent)) return EGMC_CollisionShape::VerticalCapsule;
    if (Cast<UBoxComponent>(RootComponent)) return EGMC_CollisionShape::Box;
    if (Cast<USphereComponent>(RootComponent)) return EGMC_CollisionShape::Sphere;

    FLog(Warning, "Collision shape not supported.")
  }

  return EGMC_CollisionShape::Invalid;
}

bool UGMC_MovementUtilityCmp::HasVerticalCapsuleCollision() const
{
  return UGMC_MovementUtilityCmp::GetRootCollisionShape() == EGMC_CollisionShape::VerticalCapsule;
}

bool UGMC_MovementUtilityCmp::HasHorizontalCapsuleCollision() const
{
  return UGMC_MovementUtilityCmp::GetRootCollisionShape() == EGMC_CollisionShape::HorizontalCapsule;
}

bool UGMC_MovementUtilityCmp::HasBoxCollision() const
{
  return UGMC_MovementUtilityCmp::GetRootCollisionShape() == EGMC_CollisionShape::Box;
}

bool UGMC_MovementUtilityCmp::HasSphereCollision() const
{
  return UGMC_MovementUtilityCmp::GetRootCollisionShape() == EGMC_CollisionShape::Sphere;
}

FVector UGMC_MovementUtilityCmp::GetRootCollisionExtent(bool bScaled) const
{
  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return FVector{0.};
  }

  if (!IsValid(PawnOwner))
  {
    gmc_ckne()
    return FVector{0.};
  }

  const auto& RootComponent = PawnOwner->GetRootComponent();
  if (!IsValid(RootComponent))
  {
    gmc_ckne()
    return FVector{0.};
  }

  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    {
      const auto& RootVCapsule = Cast<UCapsuleComponent>(RootComponent);
      const float Radius = FMath::Max(0.f, bScaled ? RootVCapsule->GetScaledCapsuleRadius() : RootVCapsule->GetUnscaledCapsuleRadius());
      const float HalfHeight = FMath::Max(0.f, bScaled ? RootVCapsule->GetScaledCapsuleHalfHeight() : RootVCapsule->GetUnscaledCapsuleHalfHeight());
      return FCollisionShape::MakeCapsule(Radius, HalfHeight).GetExtent();
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      const auto& RootHCapsule = Cast<UGMC_FlatCapsuleCmp>(RootComponent);
      const float Radius = FMath::Max(0.f, bScaled ? RootHCapsule->GetScaledCapsuleRadius() : RootHCapsule->GetUnscaledCapsuleRadius());
      const float HalfHeight = FMath::Max(0.f, bScaled ? RootHCapsule->GetScaledCapsuleHalfHeight() : RootHCapsule->GetUnscaledCapsuleHalfHeight());
      return FCollisionShape::MakeCapsule(Radius, HalfHeight).GetExtent();
    }
    case EGMC_CollisionShape::Box:
    {
      const auto& RootBox = Cast<UBoxComponent>(RootComponent);
      const FVector Extent = bScaled ? RootBox->GetScaledBoxExtent() : RootBox->GetUnscaledBoxExtent();
      return FCollisionShape::MakeBox(Extent).GetExtent();
    }
    case EGMC_CollisionShape::Sphere:
    {
      const auto& RootSphere = Cast<USphereComponent>(RootComponent);
      const float Radius = FMath::Max(0.f, bScaled ? RootSphere->GetScaledSphereRadius() : RootSphere->GetUnscaledSphereRadius());
      return FCollisionShape::MakeSphere(Radius).GetExtent();
    }
    default: gmc_ckne();
  }
  gmc_ckne()
  return FVector{0.};
}

float UGMC_MovementUtilityCmp::GetRootCollisionHalfHeight(bool bScaled) const
{
  if (!HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  const EGMC_CollisionShape RootCollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  const FVector Extent = GetRootCollisionExtent(bScaled);
  if (RootCollisionShape == EGMC_CollisionShape::HorizontalCapsule)
  {
    // For a horizontal capsule the height is the radius.
    return Extent.X;
  }

  return Extent.Z;
}

float UGMC_MovementUtilityCmp::GetRootCollisionOuterHalfHeight(bool bScaled) const
{
  if (!HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  const FVector Extent = GetRootCollisionExtent(bScaled);
  switch (UGMC_MovementUtilityCmp::GetRootCollisionShape())
  {
    case EGMC_CollisionShape::VerticalCapsule: return Extent.Z/*HalfHeight*/ - Extent.X/*Radius*/;
    case EGMC_CollisionShape::Box: return Extent.Z;
    case EGMC_CollisionShape::HorizontalCapsule:
    case EGMC_CollisionShape::Sphere: return 0.f;
    default: gmc_ckne();
  }
  gmc_ckne()
  return 0.f;
}

float UGMC_MovementUtilityCmp::GetRootCollisionWidth(const FVector& Direction) const
{
  if (!HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  const FVector Extent = GetRootCollisionExtent(true);
  float Width{0.f};
  switch (UGMC_MovementUtilityCmp::GetRootCollisionShape())
  {
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::Sphere:
      Width = Extent.X;
      break;
    case EGMC_CollisionShape::HorizontalCapsule:
    case EGMC_CollisionShape::Box:
    {
      const FVector NormalizedDirectionXY = FVector(Direction.X, Direction.Y, 0.).GetSafeNormal();
      if (NormalizedDirectionXY.IsZero()) return 0.f;
      Width = ComputeDistanceToRootCollisionBoundaryXY(Direction);
      break;
    }
    default: gmc_ckne();
  }
  return Width;
}

float UGMC_MovementUtilityCmp::GetMaxDistanceToRootCollisionBoundaryXY() const
{
  if (!HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  const FVector Extent = GetRootCollisionExtent(true);
  float MaxWidth{0.f};
  switch (UGMC_MovementUtilityCmp::GetRootCollisionShape())
  {
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::Sphere:
      MaxWidth = Extent.X;
      break;
    case EGMC_CollisionShape::HorizontalCapsule:
      MaxWidth = Extent.Z;
      break;
    case EGMC_CollisionShape::Box:
      MaxWidth = FVector(Extent.X, Extent.Y, 0.).Size();
      break;
    default: gmc_ckne();
  }

  return MaxWidth;
}

float UGMC_MovementUtilityCmp::ComputeDistanceRootCollisionToImpactPercentXY(const FVector& ImpactPoint) const
{
  if (!HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  const FVector CollisionCenter = UpdatedComponent->GetComponentLocation();
  const FVector CenterToImpactXY = FVector(ImpactPoint.X, ImpactPoint.Y, 0.) - FVector(CollisionCenter.X, CollisionCenter.Y, 0.);
  const FVector ImpactDirectionXY = CenterToImpactXY.GetSafeNormal();
  if (ImpactDirectionXY.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
  {
    // The impact point is at the center of the collision.
    return 0.f;
  }

  const float DistanceToImpactXY = CenterToImpactXY.Size();
  const float DistanceToBoundaryXY = ComputeDistanceToRootCollisionBoundaryXY(ImpactDirectionXY);
  gmc_ck(DistanceToBoundaryXY > 0.f)
  return DistanceToImpactXY / DistanceToBoundaryXY;
}

bool UGMC_MovementUtilityCmp::IsFloorDirty(const FGMC_FloorParams& Floor, double Tolerance) const
{
  return Floor.IsDirty(Tolerance, this);
}

void UGMC_MovementUtilityCmp::SetRootCollisionExtent(const FVector& NewExtent, bool bScaled, bool bUpdateOverlaps)
{
  SCOPE_CYCLE_COUNTER(STAT_SetRootCollisionExtent)

  if (!HasValidRootCollisionScale())
  {
    gmc_ckne()
    return;
  }

  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return;
  }

  gmc_ck(IsValid(PawnOwner))
  const auto& RootComponent = PawnOwner->GetRootComponent();
  gmc_ck(IsValid(RootComponent))

  const FVector ValidExtent = GetValidExtent(CollisionShape, bScaled ? NewExtent / RootComponent->GetComponentScale() : NewExtent);
  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    {
      UCapsuleComponent* RootVCapsule = Cast<UCapsuleComponent>(RootComponent);
      RootVCapsule->SetCapsuleSize(ValidExtent.X, ValidExtent.Z, bUpdateOverlaps);
      return;
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      UGMC_FlatCapsuleCmp* RootHCapsule = Cast<UGMC_FlatCapsuleCmp>(RootComponent);
      RootHCapsule->SetCapsuleSize(ValidExtent.X, ValidExtent.Z, bUpdateOverlaps);
      return;
    }
    case EGMC_CollisionShape::Box:
    {
      UBoxComponent* RootBox = Cast<UBoxComponent>(RootComponent);
      RootBox->SetBoxExtent(ValidExtent, bUpdateOverlaps);
      return;
    }
    case EGMC_CollisionShape::Sphere:
    {
      USphereComponent* RootSphere = Cast<USphereComponent>(RootComponent);
      RootSphere->SetSphereRadius(ValidExtent.X, bUpdateOverlaps);
      return;
    }
    default:
    {
      gmc_ckne();
    }
  }
  gmc_ckne()
}

void UGMC_MovementUtilityCmp::SetRootCollisionExtentSafe(const FVector& NewExtent, bool bScaled, float Tolerance, bool bUpdateOverlaps)
{
  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return;
  }

  USceneComponent* RootComponent = PawnOwner->GetRootComponent();
  if (!IsValid(RootComponent))
  {
    gmc_ckne()
    return;
  }

  if (!HasValidRootCollisionScale())
  {
    gmc_ckne()
    return;
  }

  FHitResult TestHit{};
  const FVector TestExtent = GetValidExtent(CollisionShape, bScaled ? NewExtent : NewExtent * RootComponent->GetComponentScale());
  if (
    !IsValidPosition(
      CollisionShape,
      TestExtent,
      RootComponent->GetComponentLocation(),
      RootComponent->GetComponentQuat(),
      TestHit,
      RootComponent->GetCollisionObjectType(),
      Tolerance
    )
  )
  {
    return;
  }

  SetRootCollisionExtent(NewExtent, bScaled, bUpdateOverlaps);
}

void UGMC_MovementUtilityCmp::SetRootCollisionHalfHeight(float NewHalfHeight, bool bScaled, bool bUpdateOverlaps)
{
  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return;
  }

  if (!HasValidRootCollisionScale())
  {
    gmc_ckne()
    return;
  }

  gmc_ck(IsValid(PawnOwner))
  const auto& RootComponent = PawnOwner->GetRootComponent();
  gmc_ck(IsValid(RootComponent))
  const FVector Scale = RootComponent->GetComponentScale();

  const FVector Extent = GetRootCollisionExtent(false);
  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::Box:
    {
      const float HalfHeightZ = bScaled ? NewHalfHeight / Scale.Z : NewHalfHeight;
      SetRootCollisionExtent({Extent.X, Extent.Y, HalfHeightZ}, false, bUpdateOverlaps);
      return;
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      const float HalfHeightX = bScaled ? NewHalfHeight / Scale.X : NewHalfHeight;
      const float HalfHeightY = bScaled ? NewHalfHeight / Scale.Y : NewHalfHeight;
      SetRootCollisionExtent({HalfHeightX, HalfHeightY, Extent.Z}, false, bUpdateOverlaps);
      return;
    }
    case EGMC_CollisionShape::Sphere:
    {
      const float HalfHeightX = bScaled ? NewHalfHeight / Scale.X : NewHalfHeight;
      const float HalfHeightY = bScaled ? NewHalfHeight / Scale.Y : NewHalfHeight;
      const float HalfHeightZ = bScaled ? NewHalfHeight / Scale.Z : NewHalfHeight;
      SetRootCollisionExtent({HalfHeightX, HalfHeightY, HalfHeightZ}, false, bUpdateOverlaps);
      return;
    }
    default: gmc_ckne();
  }
  gmc_ckne()
}

void UGMC_MovementUtilityCmp::SetRootCollisionHalfHeightSafe(float NewHalfHeight, bool bScaled, float Tolerance, bool bUpdateOverlaps)
{
  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return;
  }

  if (!HasValidRootCollisionScale())
  {
    gmc_ckne()
    return;
  }

  gmc_ck(IsValid(PawnOwner))
  const auto& RootComponent = PawnOwner->GetRootComponent();
  gmc_ck(IsValid(RootComponent))
  const FVector Scale = RootComponent->GetComponentScale();

  const FVector Extent = GetRootCollisionExtent(false);
  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::Box:
    {
      const FVector TestExtent = {
        Extent.X * Scale.X,
        Extent.Y * Scale.Y,
        bScaled ? NewHalfHeight : NewHalfHeight * Scale.Z
      };

      FHitResult TestHit{};
      if (
        !IsValidPosition(
          CollisionShape,
          GetValidExtent(CollisionShape, TestExtent),
          RootComponent->GetComponentLocation(),
          RootComponent->GetComponentQuat(),
          TestHit,
          RootComponent->GetCollisionObjectType(),
          Tolerance
        )
      )
      {
        return;
      }

      const float HalfHeightZ = bScaled ? NewHalfHeight / Scale.Z : NewHalfHeight;
      const FVector NewExtent = {Extent.X, Extent.Y, HalfHeightZ};
      SetRootCollisionExtent(NewExtent, false, bUpdateOverlaps);
      return;
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      const FVector TestExtent = {
        bScaled ? NewHalfHeight : NewHalfHeight * Scale.X,
        bScaled ? NewHalfHeight : NewHalfHeight * Scale.Y,
        Extent.Z * Scale.Z
      };

      FHitResult TestHit{};
      if (
        !IsValidPosition(
          CollisionShape,
          GetValidExtent(CollisionShape, TestExtent),
          RootComponent->GetComponentLocation(),
          RootComponent->GetComponentQuat(),
          TestHit,
          RootComponent->GetCollisionObjectType(),
          Tolerance
        )
      )
      {
        return;
      }

      const float HalfHeightX = bScaled ? NewHalfHeight / Scale.X : NewHalfHeight;
      const float HalfHeightY = bScaled ? NewHalfHeight / Scale.Y : NewHalfHeight;
      const FVector NewExtent = {HalfHeightX, HalfHeightY, Extent.Z};
      SetRootCollisionExtent(NewExtent, false, bUpdateOverlaps);
      return;
    }
    case EGMC_CollisionShape::Sphere:
    {
      const FVector TestExtent = {
        bScaled ? NewHalfHeight : NewHalfHeight * Scale.X,
        bScaled ? NewHalfHeight : NewHalfHeight * Scale.Y,
        bScaled ? NewHalfHeight : NewHalfHeight * Scale.Z
      };

      FHitResult TestHit{};
      if (
        !IsValidPosition(
          CollisionShape,
          GetValidExtent(CollisionShape, TestExtent),
          RootComponent->GetComponentLocation(),
          RootComponent->GetComponentQuat(),
          TestHit,
          RootComponent->GetCollisionObjectType(),
          Tolerance
        )
      )
      {
        return;
      }

      const float HalfHeightX = bScaled ? NewHalfHeight / Scale.X : NewHalfHeight;
      const float HalfHeightY = bScaled ? NewHalfHeight / Scale.Y : NewHalfHeight;
      const float HalfHeightZ = bScaled ? NewHalfHeight / Scale.Z : NewHalfHeight;
      const FVector NewExtent = {HalfHeightX, HalfHeightY, HalfHeightZ};
      SetRootCollisionExtent(NewExtent, false, bUpdateOverlaps);
      return;
    }
    default: gmc_ckne();
  }
  gmc_ckne()
}

void UGMC_MovementUtilityCmp::SetRootCollisionLocation(const FVector& NewLocation, ETeleportType Teleport)
{
  if (!IsUpdatedComponentRootCollision())
  {
    return;
  }

  UpdatedComponent->SetWorldLocation(NewLocation, false, nullptr, Teleport);
}

FVector UGMC_MovementUtilityCmp::GetRootCollisionLocation() const
{
  if (!IsUpdatedComponentRootCollision())
  {
    return FVector{0.};
  }

  return UpdatedComponent->GetComponentLocation();
}

void UGMC_MovementUtilityCmp::SetRootCollisionLocationSafe(const FVector& NewLocation, float Tolerance, ETeleportType Teleport)
{
  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return;
  }

  const FVector InitialLocation = UpdatedComponent->GetComponentLocation();
  if (InitialLocation.Equals(NewLocation, 0.01))
  {
    return;
  }

  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  const FVector CollisionExtent = GetRootCollisionExtent(true);
  FHitResult TestHit{};
  if (
    !IsValidPosition(
      CollisionShape,
      CollisionExtent,
      NewLocation,
      UpdatedComponent->GetComponentRotation(),
      TestHit,
      UpdatedComponent->GetCollisionObjectType(),
      Tolerance
    )
  )
  {
    return;
  }

  SetRootCollisionLocation(NewLocation, Teleport);
}

void UGMC_MovementUtilityCmp::SetRootCollisionRotation(const FQuat& NewRotation, ETeleportType Teleport)
{
  if (!IsUpdatedComponentRootCollision())
  {
    return;
  }

  UpdatedComponent->SetWorldRotation(NewRotation, false, nullptr, Teleport);
}

void UGMC_MovementUtilityCmp::SetRootCollisionRotation(const FRotator& NewRotation, ETeleportType Teleport)
{
  SetRootCollisionRotation(NewRotation.Quaternion(), Teleport);
}

FRotator UGMC_MovementUtilityCmp::GetRootCollisionRotation() const
{
  if (!IsUpdatedComponentRootCollision())
  {
    return FRotator{0.};
  }

  return UpdatedComponent->GetComponentRotation();
}

void UGMC_MovementUtilityCmp::SetRootCollisionRotationSafe(const FQuat& NewRotation, float Tolerance, ETeleportType Teleport)
{
  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return;
  }

  const FQuat InitialRotation = UpdatedComponent->GetComponentQuat();
  if (InitialRotation.Equals(NewRotation, 0.01))
  {
    return;
  }

  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  const FVector CollisionExtent = GetRootCollisionExtent(true);
  FHitResult TestHit{};
  if (
    !IsValidPosition(
      CollisionShape,
      CollisionExtent,
      UpdatedComponent->GetComponentLocation(),
      NewRotation,
      TestHit,
      UpdatedComponent->GetCollisionObjectType(),
      Tolerance
    )
  )
  {
    return;
  }

  SetRootCollisionRotation(NewRotation, Teleport);
}

void UGMC_MovementUtilityCmp::SetRootCollisionRotationSafe(const FRotator& NewRotation, float Tolerance, ETeleportType Teleport)
{
  SetRootCollisionRotationSafe(NewRotation.Quaternion(), Tolerance, Teleport);
}

FCollisionShape UGMC_MovementUtilityCmp::GetFrom(EGMC_CollisionShape CollisionShape, const FVector& Extent) const
{
  const FVector ValidExtent = GetValidExtent(CollisionShape, Extent);
  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::HorizontalCapsule: return FCollisionShape::MakeCapsule(ValidExtent);
    case EGMC_CollisionShape::Box: return FCollisionShape::MakeBox(ValidExtent);
    case EGMC_CollisionShape::Sphere: return FCollisionShape::MakeSphere(ValidExtent.X);
    default: gmc_ckne();
  }
  gmc_ckne()
  return FCollisionShape();
}

FQuat UGMC_MovementUtilityCmp::AddToGMCCapsuleRotation(const FQuat& Rotation) const
{
  if (const auto& GMCCapsule = Cast<UGMC_CapsuleCmp>(UpdatedComponent))
  {
    return (Rotation.GetNormalized() * FQuat(GMCCapsule->GetGMCCapsuleRotation())).GetNormalized();
  }

  return Rotation;
}

FRotator UGMC_MovementUtilityCmp::AddToGMCCapsuleRotation(const FRotator& Rotation) const
{
  return AddToGMCCapsuleRotation(Rotation.Quaternion()).Rotator();
}

FVector UGMC_MovementUtilityCmp::GetValidExtent(EGMC_CollisionShape CollisionShape, const FVector& Extent) const
{
  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return FVector{0.};
  }

  FVector ValidExtent{0.};
  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      ValidExtent.X = ValidExtent.Y = FMath::Max3(UE_DOUBLE_KINDA_SMALL_NUMBER, Extent.X, Extent.Y);
      ValidExtent.Z = FMath::Max3(UE_DOUBLE_KINDA_SMALL_NUMBER, Extent.X, Extent.Z);
      break;
    }
    case EGMC_CollisionShape::Box:
    {
      ValidExtent.X = FMath::Max(UE_DOUBLE_KINDA_SMALL_NUMBER, Extent.X);
      ValidExtent.Y = FMath::Max(UE_DOUBLE_KINDA_SMALL_NUMBER, Extent.Y);
      ValidExtent.Z = FMath::Max(UE_DOUBLE_KINDA_SMALL_NUMBER, Extent.Z);
      break;
    }
    case EGMC_CollisionShape::Sphere:
    {
      ValidExtent.X = FMath::Max3(Extent.X, Extent.Y, Extent.Z);
      ValidExtent.X = ValidExtent.Y = ValidExtent.Z = FMath::Max(UE_DOUBLE_KINDA_SMALL_NUMBER, ValidExtent.X);
      break;
    }
    default: gmc_ckne();
  }

  return ValidExtent;
}

bool UGMC_MovementUtilityCmp::IsValidExtent(EGMC_CollisionShape CollisionShape, const FVector& Extent) const
{
  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return false;
  }

  // No negative, zero or nearly zero components.
  if (Extent.X < UE_DOUBLE_KINDA_SMALL_NUMBER || Extent.Y < UE_DOUBLE_KINDA_SMALL_NUMBER || Extent.Z < UE_DOUBLE_KINDA_SMALL_NUMBER)
  {
    return false;
  }

  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      // X and Y both refer to the radius.
      if (Extent.X != Extent.Y)
      {
        return false;
      }
      // The half height cannot be smaller than the radius.
      if (Extent.Z < Extent.X)
      {
        return false;
      }
      // The passed vector is a valid extent for a capsule.
      return true;
    }
    case EGMC_CollisionShape::Box:
    {
      // A box has no further restrictions, the passed vector is a valid extent for a box.
      return true;
    }
    case EGMC_CollisionShape::Sphere:
    {
      // All components refer to the radius.
      if (Extent.X != Extent.Y)
      {
        return false;
      }
      if (Extent.X != Extent.Z)
      {
        return false;
      }
      // The passed vector is a valid extent for a sphere.
      return true;
    }
    default: gmc_ckne();
  }
  gmc_ckne()
  return false;
}

float UGMC_MovementUtilityCmp::ComputeDistanceToRootCollisionBoundaryXY(const FVector& Direction) const
{
  if (!HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  const FVector NormalizedDirectionXY = FVector(Direction.X, Direction.Y, 0.).GetSafeNormal();
  if (NormalizedDirectionXY.IsZero())
  {
    return 0.f;
  }

  const EGMC_CollisionShape RootCollision = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  const FVector Extent = GetRootCollisionExtent(true);
  if (Extent.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER)) return 0.f;
  float DistanceToBoundaryXY{0.f};
  switch (RootCollision)
  {
    // For vertical capsules and spheres the offset is always the radius.
    case EGMC_CollisionShape::VerticalCapsule:
    case EGMC_CollisionShape::Sphere:
    {
      DistanceToBoundaryXY = Extent.X;
      break;
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      const float Radius = Extent.X;
      const float XExtentNoCap = Extent.Z - Radius;
      const FRotator CapsuleRotation = AddToGMCCapsuleRotation(FRotator::ZeroRotator);
      const FRotator CapsuleRotationYaw = {0., CapsuleRotation.Yaw, 0.};
      const float CosAlpha = FMath::Abs(CapsuleRotationYaw.RotateVector(UpdatedComponent->GetRightVector()) | NormalizedDirectionXY);
      const float Alpha = FMath::Acos(CosAlpha);
      const float Theta = FMath::Atan(XExtentNoCap / Radius);
      if ((Theta <= Alpha && Alpha <= UE_PI - Theta) || (UE_PI + Theta <= Alpha && Alpha <= 2.f * UE_PI - Theta))
      {
        // We need to consider the capsule-hemispheres.
        const float Beta = 0.5f * UE_PI - Alpha;
        DistanceToBoundaryXY = FMath::Cos(Beta) * (
          XExtentNoCap + FMath::Sqrt(
            FMath::Square(Radius) + (Radius - XExtentNoCap) * (Radius + XExtentNoCap) * FMath::Square(FMath::Tan(Beta))
          )
        );
      }
      else
      {
        // We don't need to consider the capsule-hemispheres.
        DistanceToBoundaryXY = Radius / CosAlpha;
      }
      DistanceToBoundaryXY = FMath::Abs(DistanceToBoundaryXY);
      break;
    }
    case EGMC_CollisionShape::Box:
    {
      const float XExtent = Extent.X;
      const float YExtent = Extent.Y;
      const float CosAlpha = FMath::Abs(UpdatedComponent->GetRightVector() | NormalizedDirectionXY);
      const float CosBeta = FMath::Abs(UpdatedComponent->GetForwardVector() | NormalizedDirectionXY);
      const float Alpha = FMath::Acos(CosAlpha);
      const float Theta = FMath::Atan(XExtent / YExtent);
      if ((Theta <= Alpha && Alpha <= PI - Theta) || (PI + Theta <= Alpha && Alpha <= 2.f * PI - Theta))
      {
        DistanceToBoundaryXY = XExtent / CosBeta;
      }
      else
      {
        DistanceToBoundaryXY = YExtent / CosAlpha;
      }
      DistanceToBoundaryXY = FMath::Abs(DistanceToBoundaryXY);
      break;
    }
    default: gmc_ckne();
  }

  return DistanceToBoundaryXY;
}

void UGMC_MovementUtilityCmp::RotateYawTowardsDirection(const FVector& Direction, float Rate, float DeltaTime)
{
  if (Direction.IsZero() || !IsUpdatedComponentRootCollision())
  {
    return;
  }

  const FVector TargetDirection = FVector::VectorPlaneProject(Direction, UpdatedComponent->GetUpVector()).GetSafeNormal();
  if (TargetDirection.IsZero())
  {
    return;
  }

  // Round the current rotation to avoid deviations between server and client when using FMath::FixedTurn which can yield differently signed results for inputs
  // that are not exactly the same (e.g. if the rotation yaw on the client was 90.000023, it could be 89.999992 on the server).
  const FRotator CurrentRotation = RoundRotator(UpdatedComponent->GetComponentRotation(), EGMC_FloatPrecisionBlueprint::TwoDecimals);
  const double TargetRotationYaw = FRotator::NormalizeAxis(TargetDirection.Rotation().Yaw);
  const double CurrentRotationYaw = FRotator::NormalizeAxis(CurrentRotation.Yaw);
  if (FMath::IsNearlyEqual(CurrentRotationYaw, TargetRotationYaw, 0.01))  return;

  if (Rate <= 0.f)
  {
    UpdatedComponent->SetRelativeRotation(FRotator(CurrentRotation.Pitch, TargetRotationYaw, CurrentRotation.Roll));
    return;
  }

  DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
  const float RotationDeltaYaw = FRotator::NormalizeAxis(Rate * DeltaTime);
  if (FMath::Abs(RotationDeltaYaw) >= FMath::Abs(TargetRotationYaw - CurrentRotationYaw))
  {
    // The rotation delta already exceeds the total difference between the current and target rotation.
    UpdatedComponent->SetRelativeRotation(FRotator(CurrentRotation.Pitch, TargetRotationYaw, CurrentRotation.Roll));
    return;
  }

  const float NewRotationYaw = FMath::FixedTurn(CurrentRotationYaw, TargetRotationYaw, RotationDeltaYaw);
  const FRotator RotationDelta = FRotator(0., NewRotationYaw - CurrentRotationYaw, 0.);
  UpdatedComponent->AddLocalRotation(RotationDelta);
}

bool UGMC_MovementUtilityCmp::RotateYawTowardsDirectionSafe(const FVector& Direction, float Rate, float DeltaTime)
{
  if (Direction.IsZero() || !IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return false;
  }

  const auto& CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  if (CollisionShape == EGMC_CollisionShape::VerticalCapsule || CollisionShape == EGMC_CollisionShape::Sphere)
  {
    // For uniform collision shapes the rotation does not matter so there is no need to check for collisions.
    RotateYawTowardsDirection(Direction, Rate, DeltaTime);
    return true;
  }

  const FVector TargetDirection = FVector::VectorPlaneProject(Direction, UpdatedComponent->GetUpVector()).GetSafeNormal();
  if (TargetDirection.IsZero())
  {
    return false;
  }

  // Round the current rotation to avoid deviations between server and client when using FMath::FixedTurn which can yield differently signed results for inputs
  // that are not exactly the same (e.g. if the rotation yaw on the client was 90.000023, it could be 89.999992 on the server).
  const FRotator CurrentRotation = RoundRotator(UpdatedComponent->GetComponentRotation(), EGMC_FloatPrecisionBlueprint::TwoDecimals);
  const double TargetRotationYaw = FRotator::NormalizeAxis(TargetDirection.Rotation().Yaw);
  const double CurrentRotationYaw = FRotator::NormalizeAxis(CurrentRotation.Yaw);
  if (FMath::IsNearlyEqual(CurrentRotationYaw, TargetRotationYaw, 0.01)) return true;

  FQuat ProposedRotation{EForceInit::ForceInit};
  if (Rate <= 0.f)
  {
    ProposedRotation = FRotator(CurrentRotation.Pitch, TargetRotationYaw, CurrentRotation.Roll).Quaternion();
  }
  else
  {
    DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
    const float RotationDeltaYaw = FRotator::NormalizeAxis(Rate * DeltaTime);
    if (FMath::Abs(RotationDeltaYaw) >= FMath::Abs(TargetRotationYaw - CurrentRotationYaw))
    {
      // The rotation delta already exceeds the total difference between the current and target rotation.
      ProposedRotation = FRotator(CurrentRotation.Pitch, TargetRotationYaw, CurrentRotation.Roll).Quaternion();
    }
    else
    {
      const float NewRotationYaw = FMath::FixedTurn(CurrentRotationYaw, TargetRotationYaw, RotationDeltaYaw);
      const FRotator RotationDelta = FRotator(0., NewRotationYaw - CurrentRotationYaw, 0.);
      ProposedRotation = (CurrentRotation + RotationDelta).Quaternion();
    }
  }

  gmc_ck(ProposedRotation.IsNormalized())

  FHitResult Hit{};
  const FVector CollisionExtent = GetRootCollisionExtent(true);
  const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
  const FVector PreAdjustLocation = UpdatedComponent->GetComponentLocation();
  bool bIsValidRotation = IsValidPosition(
    CollisionShape,
    CollisionExtent,
    PreAdjustLocation,
    ProposedRotation,
    Hit,
    CollisionChannel
  );

  if (Hit.bBlockingHit)
  {
    FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

    const float CollisionMaxWidth = GetMaxDistanceToRootCollisionBoundaryXY();
    const FVector RotationStart = CurrentRotation.Vector() * CollisionMaxWidth;
    const FVector RotationEnd = ProposedRotation.Vector() * CollisionMaxWidth;
    float AdjustDistance = FMath::Min((RotationStart - RotationEnd).Size(), CollisionMaxWidth) + UU_MILLIMETER;
    // We only adjust in the XY-plane.
    const FVector ImpactNormalXY = Hit.ImpactNormal.GetSafeNormal2D();
    FVector AdjustDirection{0.};
    if (!ImpactNormalXY.IsZero())
    {
      AdjustDirection = ImpactNormalXY;
      const float IntraShapeDistance = ComputeDistanceToRootCollisionBoundaryXY(-AdjustDirection);
      if (AdjustDistance > IntraShapeDistance)
      {
        // We never need to move further away from the obstruction than our collision shape is wide in the direction opposing adjustment.
        AdjustDistance = (AdjustDistance - IntraShapeDistance) + UU_MILLIMETER;
      }
    }
    else
    {
      // Sometimes we get bad impact normals (especially with box collisions) which can make the 2D impact normal a zero vector. In that case we have to
      // calculate the adjustment direction in another (worse) way.
      AdjustDirection = (UpdatedComponent->GetComponentLocation() - Hit.ImpactPoint).GetSafeNormal2D();
    }
    const FVector Adjustment = AdjustDirection * AdjustDistance;
    if (Adjustment.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
    {
      return false;
    }

    MoveUpdatedComponent(Adjustment, CurrentRotation, true, &Hit, ETeleportType::TeleportPhysics);
    if (Hit.bStartPenetrating)
    {
      ScopedMovement.RevertMove();
      return false;
    }

    bIsValidRotation = IsValidPosition(
      CollisionShape,
      CollisionExtent,
      UpdatedComponent->GetComponentLocation(),
      ProposedRotation,
      Hit,
      CollisionChannel
    );

    if (!bIsValidRotation)
    {
      ScopedMovement.RevertMove();
      return false;
    }
  }

  if (bIsValidRotation)
  {
    UpdatedComponent->SetRelativeRotation(ProposedRotation);
    return true;
  }

  return false;
}

bool UGMC_MovementUtilityCmp::IsUpdatedComponentRootCollision() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  const auto& RootComponent = PawnOwner->GetRootComponent();
  if (!IsValid(RootComponent))
  {
    return false;
  }

  if (RootComponent != UpdatedComponent)
  {
    FLog(Verbose, "Root collision invalid, the root component must be the updated component.")
    return false;
  }

  gmc_ck(UpdatedComponent == UpdatedPrimitive)

  return true;
}

bool UGMC_MovementUtilityCmp::HasValidRootCollisionShape() const
{
  const EGMC_CollisionShape GMCCollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  if (GMCCollisionShape >= EGMC_CollisionShape::Invalid)
  {
    FLog(Verbose, "Root collision invalid, the root component has an unsupported shape.")
    return false;
  }

  return true;
}

bool UGMC_MovementUtilityCmp::HasValidRootCollisionExtent() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  const auto& RootComponent = Cast<UShapeComponent>(PawnOwner->GetRootComponent());
  if (!IsValid(RootComponent))
  {
    return false;
  }

  if (!IsValidExtent(UGMC_MovementUtilityCmp::GetRootCollisionShape(), RootComponent->GetCollisionShape().GetExtent()))
  {
    FLog(Verbose, "Root collision invalid, the current shape extent is not a valid extent vector.")
    return false;
  }

  return true;
}

bool UGMC_MovementUtilityCmp::HasValidRootCollisionScale() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  const auto& RootComponent = PawnOwner->GetRootComponent();
  if (!IsValid(RootComponent))
  {
    return false;
  }

  const FVector Scale = RootComponent->GetComponentScale();
  if (Scale.X < UE_DOUBLE_KINDA_SMALL_NUMBER || Scale.Y < UE_DOUBLE_KINDA_SMALL_NUMBER || Scale.Z < UE_DOUBLE_KINDA_SMALL_NUMBER)
  {
    FLog(Verbose, "Root collision invalid, the root component has at least one scaling factor of zero.")
    return false;
  }

  return true;
}

float UGMC_MovementUtilityCmp::ComputeImmersionDepth() const
{
  SCOPE_CYCLE_COUNTER(STAT_ComputeImmersionDepth)

  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  float ImmersionDepth = 0.f;
  if (const auto& PhysicsVolume = GetPhysicsVolume())
  {
    if (PhysicsVolume->bWaterVolume)
    {
      float HalfHeightZ = GetRootCollisionHalfHeight(true);
      FVector CollisionHalfHeight = FVector(0., 0., HalfHeightZ);
      UBrushComponent* VolumeBrush = PhysicsVolume->GetBrushComponent();
      FHitResult HitResult{};
      if (VolumeBrush)
      {
        FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
        FVector TraceStart = CurrentLocation + CollisionHalfHeight;
        FVector TraceEnd = CurrentLocation - CollisionHalfHeight;
        FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ComputeImmersionDepth), true);
        // Trace from the top of our root collision to its bottom against the physics volume's brush component.
        VolumeBrush->LineTraceComponent(HitResult, TraceStart, TraceEnd, CollisionQueryParams);
      }
      // The hit result time tells us how deep we are immersed.
      ImmersionDepth = 1.f - HitResult.Time;
    }
  }

  // Immersion depth will always be zero when not in a fluid volume.
  return ImmersionDepth;
}

bool UGMC_MovementUtilityCmp::CanStepUp(const FHitResult& Hit) const
{
  if (!Hit.IsValidBlockingHit())
  {
    return false;
  }

  UPrimitiveComponent* HitComponent = Hit.Component.Get();
  if (!HitComponent)
  {
    return false;
  }

  // UPrimitiveComponent::CanCharacterStepUp will also call AActor::CanBeBaseForCharacter if needed so a separate check is not necessary.
  if (!HitComponent->CanCharacterStepUp(PawnOwner))
  {
    return false;
  }

  // UPrimitiveComponent::CanCharacterStepUp will already call AActor::CanBeBaseForCharacter on the owner of the hit component if appropriate, but it may be
  // possible that the hit actor is not the owner of the hit component.
  AActor* HitActor = Hit.GetActor();
  // The hit actor can be null (e.g. BSPs).
  if (HitActor)
  {
    if (!HitActor->CanBeBaseForCharacter(PawnOwner))
    {
      return false;
    }
  }

  return true;
}

FHitResult UGMC_MovementUtilityCmp::K2_SweepRootCollisionSingleByChannel_Direction(
  const FVector& Direction,
  float TraceLength,
  const FVector& Extent,
  const FRotator& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  return SweepRootCollisionSingleByChannel(Direction, TraceLength, Extent, Rotation.Quaternion(), CollisionChannel);
}

FHitResult UGMC_MovementUtilityCmp::SweepRootCollisionSingleByChannel(
  const FVector& Direction,
  float TraceLength,
  const FVector& Extent,
  const FQuat& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return FHitResult();
  }

  const auto& World = GetWorld();
  const FVector NormalizedDirection = Direction.GetSafeNormal();
  if (!World || NormalizedDirection.IsZero() || TraceLength <= 0.f)
  {
    return FHitResult();
  }

  const FTransform& Transform = UpdatedComponent->GetComponentTransform();
  const FVector TraceStart = Transform.GetLocation();
  const FVector TraceEnd = TraceStart + NormalizedDirection * TraceLength;
  return SweepRootCollisionSingleByChannel(TraceStart, TraceEnd, Extent, Rotation, CollisionChannel);
}

FHitResult UGMC_MovementUtilityCmp::K2_SweepRootCollisionSingleByChannel(
  const FVector& TraceStart,
  const FVector& TraceEnd,
  const FVector& Extent,
  const FRotator& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  return SweepRootCollisionSingleByChannel(TraceStart, TraceEnd, Extent, Rotation.Quaternion(), CollisionChannel);
}

FHitResult UGMC_MovementUtilityCmp::SweepRootCollisionSingleByChannel(
  const FVector& TraceStart,
  const FVector& TraceEnd,
  const FVector& Extent,
  const FQuat& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return FHitResult();
  }

  if (TraceStart == TraceEnd || !UpdatedComponent->GetCollisionEnabled())
  {
    return FHitResult();
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return FHitResult();
  }

  FHitResult HitResult{};
  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  const FCollisionShape TraceShape =
    GetFrom(CollisionShape, Extent.IsZero() ? GetRootCollisionExtent(true) : GetValidExtent(CollisionShape, Extent));
  const FQuat TraceRotation = (Rotation * AddToGMCCapsuleRotation(UpdatedComponent->GetComponentQuat())).GetNormalized();
  FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(SweepRootCollisionSingleByChannel), false, GetOwner());
  CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
  CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
  const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
  World->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, TraceRotation, CollisionChannel, TraceShape, CollisionQueryParams, CollisionResponseParams);
  return HitResult;
}

TArray<FHitResult> UGMC_MovementUtilityCmp::K2_SweepRootCollisionMultiByChannel_Direction(
  const FVector& Direction,
  float TraceLength,
  const FVector& Extent,
  const FRotator& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  return SweepRootCollisionMultiByChannel(Direction, TraceLength, Extent, Rotation.Quaternion(), CollisionChannel);
}

TArray<FHitResult> UGMC_MovementUtilityCmp::SweepRootCollisionMultiByChannel(
  const FVector& Direction,
  float TraceLength,
  const FVector& Extent,
  const FQuat& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return TArray<FHitResult>();
  }

  const auto& World = GetWorld();
  const FVector NormalizedDirection = Direction.GetSafeNormal();
  if (!World || NormalizedDirection.IsZero() || TraceLength <= 0.f)
  {
    return TArray<FHitResult>();
  }

  const FTransform& Transform = UpdatedComponent->GetComponentTransform();
  const FVector TraceStart = Transform.GetLocation();
  const FVector TraceEnd = TraceStart + NormalizedDirection * TraceLength;
  return SweepRootCollisionMultiByChannel(TraceStart, TraceEnd, Extent, Rotation, CollisionChannel);
}

TArray<FHitResult> UGMC_MovementUtilityCmp::K2_SweepRootCollisionMultiByChannel(
  const FVector& TraceStart,
  const FVector& TraceEnd,
  const FVector& Extent,
  const FRotator& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  return SweepRootCollisionMultiByChannel(TraceStart, TraceEnd, Extent, Rotation.Quaternion(), CollisionChannel);
}

TArray<FHitResult> UGMC_MovementUtilityCmp::SweepRootCollisionMultiByChannel(
  const FVector& TraceStart,
  const FVector& TraceEnd,
  const FVector& Extent,
  const FQuat& Rotation,
  ECollisionChannel CollisionChannel
) const
{
  TArray<FHitResult> HitResults{};

  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return HitResults;
  }

  if (TraceStart == TraceEnd || !UpdatedComponent->GetCollisionEnabled())
  {
    return HitResults;
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return HitResults;
  }

  const EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  const FCollisionShape TraceShape =
    GetFrom(CollisionShape, Extent.IsZero() ? GetRootCollisionExtent(true) : GetValidExtent(CollisionShape, Extent));
  const FTransform& Transform = UpdatedComponent->GetComponentTransform();
  const FQuat TraceRotation = (Rotation * AddToGMCCapsuleRotation(UpdatedComponent->GetComponentQuat())).GetNormalized();
  FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(SweepRootCollisionMultiByChannel), false, GetOwner());
  CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
  CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
  const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
  World->SweepMultiByChannel(HitResults, TraceStart, TraceEnd, TraceRotation, CollisionChannel, TraceShape, CollisionQueryParams, CollisionResponseParams);
  return HitResults;
}

TArray<FHitResult> UGMC_MovementUtilityCmp::LineTracesAlignedVertical(
  const FVector& Direction,
  float TraceLength,
  float MaxHeight,
  int32 Resolution,
  ECollisionChannel CollisionChannel
) const
{
  TArray<FHitResult> HitResults{};

  if (!IsUpdatedComponentRootCollision())
  {
    return HitResults;
  }

  if (!UpdatedComponent->GetCollisionEnabled())
  {
    return HitResults;
  }

  const auto& World = GetWorld();
  const FVector NormalizedDirection = Direction.GetSafeNormal();
  if (!World || TraceLength <= 0.f || NormalizedDirection.IsZero() || MaxHeight <= 0.f || Resolution <= 0)
  {
    return HitResults;
  }

  // Calculate the parameters.
  const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
  const FVector Bounds = UpdatedComponent->Bounds.BoxExtent;
  const FVector LowerBound = FVector(CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z - Bounds.Z);
  const FVector TracesMaxHeight = LowerBound + FVector(0., 0., MaxHeight);
  const float TracesInterval = (TracesMaxHeight - LowerBound).Size() / Resolution;

  // Execute the line traces.
  FHitResult HitResult{};
  FVector TraceStart = LowerBound;
  FVector TraceEnd = LowerBound + NormalizedDirection * TraceLength;
  FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(LineTracesAlignedVertical), false, GetOwner());
  CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
  CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
  const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
  for (int NumTrace = 0; NumTrace < Resolution; ++NumTrace)
  {
    World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, CollisionQueryParams, CollisionResponseParams);
    HitResults.Emplace(HitResult);
    TraceStart += FVector(0., 0., TracesInterval);
    TraceEnd += FVector(0., 0., TracesInterval);
  }

  return HitResults;
}

FHitResult UGMC_MovementUtilityCmp::AutoResolvePenetration()
{
  SCOPE_CYCLE_COUNTER(STAT_AutoResolvePenetration)

  const FQuat CurrentRotation = UpdatedComponent->GetComponentQuat();
  const FVector TestDelta = {0.01, 0.01, 0.01};
  FHitResult Hit{};

  const auto TestMove = [&](const FVector& Delta)
  {
    FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

    // Check for penetrations by applying a small location delta.
    MoveUpdatedComponent(Delta, CurrentRotation, true, &Hit, ETeleportType::TeleportPhysics);

    // Revert the movement again afterwards, we are only interested in the hit result.
    ScopedMovement.RevertMove();
  };

  TestMove(TestDelta);

  if (!Hit.bBlockingHit || Hit.IsValidBlockingHit())
  {
    // Apply the test delta in the opposite direction.
    TestMove(-TestDelta);

    if (!Hit.bBlockingHit || Hit.IsValidBlockingHit())
    {
      // The pawn is not in penetration, no action is needed.
      return Hit;
    }
  }

  const auto& RollbackActor = Cast<AGMC_RollbackActor>(Hit.GetActor());

  CFLog(!RollbackActor, VeryVerbose, "Pawn in penetration. Trying to resolve...")
  CFLog(RollbackActor, VeryVerbose,
    "Pawn in penetration with rollback actor (%s). Trying to resolve...", *RollbackActor->GetHumanReadableName())

  if (ResolvePenetrationWithRollbackActor(RollbackActor, Hit))
  {
    FLog(VeryVerbose, "Successfully resolved penetration with rollback actor. Returning...")
    return Hit;
  }

  CFLog(RollbackActor, VeryVerbose, "Could not resolve penetration with rollback actor.")
  FLog(VeryVerbose, "Executing up/down sweep.")

  SafeMoveUpdatedComponent(TestDelta, CurrentRotation, true, Hit, ETeleportType::TeleportPhysics);
  DEBUG_LOG_AUTO_RESOLVE_PENETRATION_UP_SWEEP
  SafeMoveUpdatedComponent(-TestDelta, CurrentRotation, true, Hit, ETeleportType::TeleportPhysics);
  DEBUG_LOG_AUTO_RESOLVE_PENETRATION_DOWN_SWEEP

  CFLog(!Hit.bStartPenetrating, VeryVerbose, "Penetration was successfully resolved. Returning...")
  CFLog(Hit.bStartPenetrating, VeryVerbose, "Could not resolve penetration. Returning...")

  return Hit;
}

bool UGMC_MovementUtilityCmp::ResolvePenetrationWithRollbackActor(AGMC_RollbackActor* RollbackActor, FHitResult& OutHit)
{
  OutHit = FHitResult{};
  if (!RollbackActor)
  {
    return false;
  }

  // Do not call SafeMoveUpdatedComponent or anything that ends up calling ResolvePenetrationImpl internally in this function (potential infinite recursion).

  FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

  const FQuat CurrentRotation = UpdatedComponent->GetComponentQuat();
  const FVector TotalVelocity = RollbackActor->GetLinearVelocity() + RollbackActor->ComputeTangentialVelocity(GetLowerBound());

  // Try to adjust by the amount that the rollback actor moved first, it is likely that the penetration was caused by the rollback actor moving into the pawn
  // (without the pawn being based on it).
  constexpr double Tolerance = 1.05;
  FVector CorrectionDelta = TotalVelocity * GetPhysDeltaTime() * Tolerance;
  if (CorrectionDelta.IsZero())
  {
    return false;
  }

  MoveUpdatedComponent(CorrectionDelta, CurrentRotation, false, nullptr, ETeleportType::TeleportPhysics);
  MoveUpdatedComponent(-CorrectionDelta, CurrentRotation, true, &OutHit, ETeleportType::TeleportPhysics);
  if (!OutHit.bBlockingHit || OutHit.IsValidBlockingHit() || OutHit.GetActor() != RollbackActor)
  {
    return true;
  }

  // If we are still stuck adjust in fixed increments.
  constexpr double AdjustmentStepSize = 2.5;
  CorrectionDelta = TotalVelocity.GetSafeNormal() * AdjustmentStepSize;
  if (CorrectionDelta.IsZero())
  {
    return false;
  }

  // Set some arbitrary limit on the distance that the pawn can be moved so we can't get stuck in an infinite loop.
  constexpr double MaxAdjustDistance = 10.;

  gmc_ck(OutHit.bBlockingHit)

  // Ensure there is a correction in Z direction, otherwise we can't get out if the rollback actor doesn't have any Z velocity.
  if (OutHit.ImpactPoint.Z > UpdatedComponent->GetComponentLocation().Z)
  {
    // The impact point on the upper side of the pawn, so we may need to correct downwards to get out of the penetration.
    CorrectionDelta.Z = FMath::Min(CorrectionDelta.Z, -AdjustmentStepSize);
  }
  else
  {
    // The impact point is on the lower side of the pawn, so we may need to correct upwards to get out of the penetration.
    CorrectionDelta.Z = FMath::Max(CorrectionDelta.Z, AdjustmentStepSize);
  }

  const double CorrectionDeltaSize = CorrectionDelta.Size();

  double TotalAdjustedDistance = 0.;
  bool bResolvedPenetration = false;
  while (true)
  {
    gmc_ck(AdjustmentStepSize > 0.)
    gmc_ck(FMath::Abs(CorrectionDelta.Z) >= AdjustmentStepSize)

    MoveUpdatedComponent(CorrectionDelta, CurrentRotation, false, nullptr, ETeleportType::TeleportPhysics);
    TotalAdjustedDistance += CorrectionDeltaSize;

    // Check if the penetration was resolved. We move downward by the adjustment step size because otherwise we could lose contact with rollback actors that
    // have upward velocity.
    MoveUpdatedComponent(FVector(0., 0., -AdjustmentStepSize), CurrentRotation, true, &OutHit, ETeleportType::TeleportPhysics);
    if (!OutHit.bBlockingHit || OutHit.IsValidBlockingHit() || OutHit.GetActor() != RollbackActor)
    {
      return true;
    }

    if (TotalAdjustedDistance >= MaxAdjustDistance)
    {
      return false;
    }
  }
}

bool UGMC_MovementUtilityCmp::ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation)
{
  const auto& RollbackActor = Cast<AGMC_RollbackActor>(Hit.GetActor());
  if (IsValid(RollbackActor))
  {
    FHitResult RollbackActorHit{};
    if (ResolvePenetrationWithRollbackActor(RollbackActor, RollbackActorHit))
    {
      return true;
    }

    if (RollbackActorHit.bStartPenetrating)
    {
      const bool ReturnValue = Super::ResolvePenetrationImpl(GetPenetrationAdjustment(RollbackActorHit), RollbackActorHit, NewRotation);

      if (!ReturnValue)
      {
        CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnStuckInGeometry, RollbackActorHit.Component.Get(), RollbackActorHit);
      }

      return ReturnValue;
    }
  }

  const bool ReturnValue = Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotation);

  if (!ReturnValue)
  {
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, OnStuckInGeometry, Hit.Component.Get(), Hit);
  }

  return ReturnValue;
}

FVector UGMC_MovementUtilityCmp::GetPenetrationAdjustment(const FHitResult& Hit) const
{
  if (!Hit.bStartPenetrating)
  {
    return FVector::ZeroVector;
  }

  FHitResult AdjustedHit = Hit;
  AdjustedHit.Normal = RoundVector(AdjustedHit.Normal, EGMC_FloatPrecisionBlueprint::TwoDecimals).GetSafeNormal();

  FVector Result = Super::GetPenetrationAdjustment(AdjustedHit);
  Result = Result.GetSafeNormal() * (Result.Size() + PENETRATION_ADJUSTMENT_TOLERANCE);

  return Result;
}

void UGMC_MovementUtilityCmp::OnStuckInGeometry_Implementation(UPrimitiveComponent* OtherComponent, const FHitResult& Hit)
{
  FLog(Verbose, "Pawn stuck in \"%s\".", *GetObjectNameWithOuter(OtherComponent))
}

bool UGMC_MovementUtilityCmp::IsWithinEdgeTolerance(const FVector& Location, const FVector& Point, float Tolerance) const
{
  const FVector LocationToPoint = Point - Location;
  const float CenterToEdgeXY = ComputeDistanceToRootCollisionBoundaryXY(LocationToPoint);
  const float DistanceFromCenterSq = LocationToPoint.SizeSquared2D();
  const float ReducedWidthSq = FMath::Square(FMath::Max(CenterToEdgeXY - Tolerance, Tolerance + UE_KINDA_SMALL_NUMBER));
  const bool bIsWithinEdgeTolerance = DistanceFromCenterSq < ReducedWidthSq;
  FLog(VeryVerbose, "Test point %s within edge tolerance.", bIsWithinEdgeTolerance ? TEXT("is") : TEXT("not"))
  return bIsWithinEdgeTolerance;
}

float UGMC_MovementUtilityCmp::LerpRootCollisionHalfHeight(
  float TargetHalfHeight,
  float InterpSpeed,
  float InterpTolerance,
  float DeltaTime,
  bool bAdjustPosition,
  EGMC_AdjustDirection AdjustDirection
)
{
  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  TargetHalfHeight = FMath::Clamp(TargetHalfHeight, 2.f * UE_KINDA_SMALL_NUMBER, UE_BIG_NUMBER);
  DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
  if (DeltaTime < MIN_DELTA_TIME) return 0.f;

  EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  FVector Extent = GetRootCollisionExtent(false);
  const float InitialHalfHeight = CollisionShape == EGMC_CollisionShape::HorizontalCapsule ? Extent.X : Extent.Z;
  if (FMath::IsNearlyEqual(InitialHalfHeight, TargetHalfHeight, 0.000001f)) return 0.f;
  float NewHalfHeight = FMath::FInterpConstantTo(InitialHalfHeight, TargetHalfHeight, DeltaTime, InterpSpeed);

  // Clamp the new half height to a valid extent value.
  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    {
      NewHalfHeight = FMath::Max(Extent.X, NewHalfHeight);
      break;
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      NewHalfHeight = FMath::Min(Extent.Z, NewHalfHeight);
      break;
    }
    case EGMC_CollisionShape::Box:
    case EGMC_CollisionShape::Sphere:
    {
      break;
    }
    default: gmc_ckne() return 0.f;
  }
  if (FMath::IsNearlyEqual(NewHalfHeight, InitialHalfHeight, 0.000001f)) return 0.f;

  const auto SetNewExtent = [&]() -> double {
    switch (CollisionShape)
    {
      case EGMC_CollisionShape::VerticalCapsule:
      case EGMC_CollisionShape::Box:
      {
        SetRootCollisionExtentSafe({Extent.X, Extent.Y, NewHalfHeight}, false, InterpTolerance, true);
        return FMath::Abs(GetRootCollisionExtent(false).Z - InitialHalfHeight);
      }
      case EGMC_CollisionShape::HorizontalCapsule:
      {
        SetRootCollisionExtentSafe({NewHalfHeight, NewHalfHeight, Extent.Z}, false, InterpTolerance, true);
        return FMath::Abs(GetRootCollisionExtent(false).X - InitialHalfHeight);
      }
      case EGMC_CollisionShape::Sphere:
      {
        SetRootCollisionExtentSafe({NewHalfHeight, NewHalfHeight, NewHalfHeight}, false, InterpTolerance, true);
        return FMath::Abs(GetRootCollisionExtent(false).X - InitialHalfHeight);
      }
      default: gmc_ckne();
    }
    gmc_ckne()
    return 0.;
  };

  // If we are not going to move the root collision just set the new extent.
  if (!bAdjustPosition)
  {
    return SetNewExtent();
  }

  const auto AdjustPosition = [this](float ZAdjustment)
  {
    FHitResult Hit{};
    SafeMoveUpdatedComponent({0., 0., ZAdjustment}, UpdatedComponent->GetComponentQuat(), true, Hit, ETeleportType::TeleportPhysics);
    return Hit;
  };

  // If the collision is going to get larger we should move first, then set the new extent. If the collision is going to get smaller we want to do it the other
  // way round.
  const float Adjustment = FMath::Abs(NewHalfHeight - InitialHalfHeight) * (AdjustDirection == EGMC_AdjustDirection::Up ? 1.f : -1.f);
  if (NewHalfHeight > InitialHalfHeight)
  {
    FScopedMovementUpdate ScopedMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

    if (AdjustPosition(Adjustment).Time < 1.f)
    {
      // The pawn was blocked while adjusting the position.
      ScopedMovement.RevertMove();
      return 0.f;
    }

    const float HalfHeightChange = SetNewExtent();
    if (HalfHeightChange == 0.f)
    {
      // The new extent would cause a blocking collision.
      ScopedMovement.RevertMove();
      return 0.f;
    }

    return HalfHeightChange;
  }

  const float HalfHeightChange = SetNewExtent();
  if (HalfHeightChange == 0.f)
  {
    // The new extent would cause a blocking collision.
    return 0.f;
  }

  // Since the collision extent got smaller and we don't move the pawn by more than the half height was changed a blocking collision should never occur in this
  // case.
  AdjustPosition(Adjustment);

  return HalfHeightChange;
}

float UGMC_MovementUtilityCmp::LerpRootCollisionWidth(
  float TargetWidth,
  float InterpSpeed,
  float InterpTolerance,
  float DeltaTime,
  bool bInterpBoxY
)
{
  if (!IsUpdatedComponentRootCollision() || !HasValidRootCollisionExtent())
  {
    return 0.f;
  }

  TargetWidth = FMath::Clamp(TargetWidth, 2.f * UE_KINDA_SMALL_NUMBER, UE_BIG_NUMBER);
  DeltaTime = DeltaTime >= MIN_DELTA_TIME ? DeltaTime : GetPhysDeltaTime();
  if (DeltaTime < MIN_DELTA_TIME) return 0.f;

  EGMC_CollisionShape CollisionShape = UGMC_MovementUtilityCmp::GetRootCollisionShape();
  FVector Extent = GetRootCollisionExtent(false);
  bInterpBoxY = CollisionShape == EGMC_CollisionShape::Box ? bInterpBoxY : false;
  const float InitialWidth = CollisionShape == EGMC_CollisionShape::HorizontalCapsule ? Extent.Z : bInterpBoxY ? Extent.Y : Extent.X;
  if (FMath::IsNearlyEqual(InitialWidth, TargetWidth, 0.000001f)) return 0.f;
  float NewWidth = FMath::FInterpConstantTo(InitialWidth, TargetWidth, DeltaTime, InterpSpeed);

  // Clamp the new width to a valid extent value.
  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    {
      NewWidth = FMath::Min(Extent.Z, NewWidth);
      break;
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      NewWidth = FMath::Max(Extent.X, NewWidth);
      break;
    }
    case EGMC_CollisionShape::Box:
    case EGMC_CollisionShape::Sphere:
    {
      break;
    }
    default: gmc_ckne() return 0.f;
  }
  if (FMath::IsNearlyEqual(NewWidth, InitialWidth, 0.000001f)) return 0.f;

  switch (CollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    {
      SetRootCollisionExtentSafe({NewWidth, NewWidth, Extent.Z}, false, InterpTolerance, true);
      return FMath::Abs(GetRootCollisionExtent(false).X - InitialWidth);
    }
    case EGMC_CollisionShape::Box:
    {
      if (bInterpBoxY)
      {
        SetRootCollisionExtentSafe({Extent.X, NewWidth, Extent.Z}, false, InterpTolerance, true);
        return FMath::Abs(GetRootCollisionExtent(false).Y - InitialWidth);
      }
      SetRootCollisionExtentSafe({NewWidth, Extent.Y, Extent.Z}, false, InterpTolerance, true);
      return FMath::Abs(GetRootCollisionExtent(false).X - InitialWidth);
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      SetRootCollisionExtentSafe({Extent.X, Extent.Y, NewWidth}, false, InterpTolerance, true);
      return FMath::Abs(GetRootCollisionExtent(false).Z - InitialWidth);
    }
    case EGMC_CollisionShape::Sphere:
    {
      SetRootCollisionExtentSafe({NewWidth, NewWidth, NewWidth}, false, InterpTolerance, true);
      return FMath::Abs(GetRootCollisionExtent(false).X - InitialWidth);
    }
    default: gmc_ckne();
  }
  gmc_ckne()
  return 0.f;
}

USceneComponent* UGMC_MovementUtilityCmp::SetRootCollisionShape(EGMC_CollisionShape NewCollisionShape, const FVector& Extent, bool bScaled, FName Name)
{
  SCOPE_CYCLE_COUNTER(STAT_SetRootCollisionShape)

  if (!IsValid(PawnOwner))
  {
    gmc_ckne()
    return nullptr;
  }

  const auto& OriginalRootComponent = PawnOwner->GetRootComponent();
  if (!IsValid(OriginalRootComponent))
  {
    gmc_ckne()
    return nullptr;
  }

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

  // The scale will be copied to the new collision component as well.
  const FVector Scale = OriginalRootComponent->GetComponentScale();

  const FVector ValidExtent = GetValidExtent(NewCollisionShape, bScaled ? Extent / Scale : Extent);
  if (NewCollisionShape == UGMC_MovementUtilityCmp::GetRootCollisionShape())
  {
    // The root component already has the requested collision shape, only update the extent (already scaled if applicable).
    SetRootCollisionExtent(ValidExtent, false, true);
    return OriginalRootComponent;
  }

  USceneComponent* NewRootComponent{nullptr};
  switch (NewCollisionShape)
  {
    case EGMC_CollisionShape::VerticalCapsule:
    {
      NewRootComponent = NewObject<UCapsuleComponent>(PawnOwner, UCapsuleComponent::StaticClass(), Name, RF_Transactional);
      break;
    }
    case EGMC_CollisionShape::HorizontalCapsule:
    {
      NewRootComponent = NewObject<UGMC_FlatCapsuleCmp>(PawnOwner, UGMC_FlatCapsuleCmp::StaticClass(), Name, RF_Transactional);
      break;
    }
    case EGMC_CollisionShape::Box:
    {
      NewRootComponent = NewObject<UBoxComponent>(PawnOwner, UBoxComponent::StaticClass(), Name, RF_Transactional);
      break;
    }
    case EGMC_CollisionShape::Sphere:
    {
      NewRootComponent = NewObject<USphereComponent>(PawnOwner, USphereComponent::StaticClass(), Name, RF_Transactional);
      break;
    }
    default: gmc_ckne() return OriginalRootComponent;
  }

  TArray<USceneComponent*> RootChildrenExclusive{};
  OriginalRootComponent->GetChildrenComponents(false, RootChildrenExclusive);
  PawnOwner->AddInstanceComponent(NewRootComponent);
  NewRootComponent->RegisterComponent();
  PawnOwner->Modify();
  NewRootComponent->AttachToComponent(OriginalRootComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
  for (USceneComponent* Child : RootChildrenExclusive)
  {
    Child->AttachToComponent(NewRootComponent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
  }
  const auto& OriginalRootComponentShape = Cast<UShapeComponent>(OriginalRootComponent);
  const auto& NewRootComponentShape = Cast<UShapeComponent>(NewRootComponent);
  // The original root component may not have been a UShapeComponent. In that case we don't copy any settings and just use the defaults.
  if (OriginalRootComponentShape && NewRootComponentShape)
  {
    CopyComponentSettings(OriginalRootComponentShape, NewRootComponentShape);
  }
  PawnOwner->SetRootComponent(NewRootComponent);
  SetUpdatedComponent(NewRootComponent);
  gmc_ck(UpdatedComponent && UpdatedPrimitive)

  // The extent was already scaled previously if applicable.
  SetRootCollisionExtent(ValidExtent, false, true);

  OriginalRootComponent->UnregisterComponent();
  OriginalRootComponent->DestroyComponent(false);
  return NewRootComponent;
}

USceneComponent* UGMC_MovementUtilityCmp::SetRootCollisionShapeSafe(
  EGMC_CollisionShape NewCollisionShape,
  const FVector& Extent,
  bool bScaled,
  FName Name,
  float Tolerance
)
{
  if (!IsValid(PawnOwner))
  {
    gmc_ckne()
    return nullptr;
  }

  const auto& OriginalRootComponent = PawnOwner->GetRootComponent();
  if (!IsValid(OriginalRootComponent))
  {
    gmc_ckne()
    return nullptr;
  }

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

  FHitResult TestHit{};
  const FVector TestExtent = GetValidExtent(NewCollisionShape, bScaled ? Extent : Extent * OriginalRootComponent->GetComponentScale());
  if (
    !IsValidPosition(
      NewCollisionShape,
      TestExtent,
      OriginalRootComponent->GetComponentLocation(),
      OriginalRootComponent->GetComponentQuat(),
      TestHit,
      OriginalRootComponent->GetCollisionObjectType(),
      Tolerance
    )
  )
  {
    return OriginalRootComponent;
  }

  return SetRootCollisionShape(NewCollisionShape, Extent, bScaled, Name);
}

bool UGMC_MovementUtilityCmp::IsValidPosition(
  EGMC_CollisionShape CollisionShape,
  const FVector& Extent,
  const FVector& Location,
  const FQuat& Rotation,
  FHitResult& OutHit,
  ECollisionChannel CollisionChannel,
  float Tolerance
) const
{
  SCOPE_CYCLE_COUNTER(STAT_IsValidPosition)

  if (CollisionShape >= EGMC_CollisionShape::Invalid)
  {
    gmc_ckne()
    return false;
  }

  if (!IsUpdatedComponentRootCollision())
  {
    return false;
  }

  if (!IsValidExtent(CollisionShape, Extent))
  {
    return false;
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return false;
  }

  if (Tolerance <= 0.f)
  {
    // A tolerance of 0 means we can consider any position valid.
    return true;
  }

  if (!UpdatedComponent->GetCollisionEnabled())
  {
    return true;
  }

  const FVector& ScaledExtent = Extent * Tolerance;
  const FCollisionShape& TraceShape = GetFrom(CollisionShape, ScaledExtent);
  const FQuat& TraceRotation = CollisionShape == EGMC_CollisionShape::HorizontalCapsule ? Rotation * UGMC_FlatCapsuleCmp::FLAT_ROTATION.Quaternion() : Rotation;
  FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(IsValidPosition), false, GetOwner());
  CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
  CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
  const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();

  const auto IsBlocked = [&](const FVector& Offset) {
    World->SweepSingleByChannel(
      OutHit,
      Location,
      Location + Offset,
      TraceRotation,
      CollisionChannel,
      TraceShape,
      CollisionQueryParams,
      CollisionResponseParams
    );

    if (OutHit.bBlockingHit)
    {
      // Not a valid location for the test shape.
      return true;
    }
    gmc_ck(!OutHit.bStartPenetrating)
    return false;
  };

  const FVector& TestDelta = {0.01, 0.01, 0.01};

  if (IsBlocked(TestDelta))
  {
    return false;
  }

  if (IsBlocked(-TestDelta))
  {
    return false;
  }

  return true;
}

bool UGMC_MovementUtilityCmp::IsValidPosition(
  EGMC_CollisionShape CollisionShape,
  const FVector& Extent,
  const FVector& Location,
  const FRotator& Rotation,
  FHitResult& OutHit,
  ECollisionChannel CollisionChannel,
  float Tolerance
) const
{
  return IsValidPosition(CollisionShape, Extent, Location, Rotation.Quaternion(), OutHit, CollisionChannel, Tolerance);
}

void UGMC_MovementUtilityCmp::CopyComponentSettings(UShapeComponent* Source, UShapeComponent* Target)
{
  if (!Source || !Target)
  {
    return;
  }

  const auto SourceBI = Source->GetBodyInstance(NAME_None, false);
  const auto TargetBI = Target->GetBodyInstance(NAME_None, false);

  // Transform.
  Target->SetWorldScale3D(Source->GetComponentScale());
  Target->SetMobility(Source->Mobility);

  // Shape.
  Target->ShapeColor = Source->ShapeColor;

  // Navigation.
  Target->bDynamicObstacle = Source->bDynamicObstacle;
  //Target->AreaClassOverride = Source->AreaClassOverride;
  //Target->bUseSystemDefaultObstacleAreaClass = Source->bUseSystemDefaultObstacleAreaClass;
  Target->bFillCollisionUnderneathForNavmesh = Source->bFillCollisionUnderneathForNavmesh;

  // HLOD.
  //Target->bEnableAutoLODGeneration = Source->bEnableAutoLODGeneration;
  //Target->HLODBatchingPolicy = Source->HLODBatchingPolicy;
  //Target->ExcludeFromHLODLevels = Source->ExcludeFromHLODLevels;

  // Component Tick.
  Target->SetComponentTickEnabled(Source->IsComponentTickEnabled());
  Target->PrimaryComponentTick.bStartWithTickEnabled = Source->PrimaryComponentTick.bStartWithTickEnabled;
  Target->SetComponentTickInterval(Source->GetComponentTickInterval());
  Target->SetTickableWhenPaused(Source->PrimaryComponentTick.bTickEvenWhenPaused);
  Target->PrimaryComponentTick.bAllowTickOnDedicatedServer = Source->PrimaryComponentTick.bAllowTickOnDedicatedServer;
  Target->SetTickGroup(Source->PrimaryComponentTick.TickGroup);

  // Rendering.
  Target->SetVisibility(Source->GetVisibleFlag(), false);
  Target->SetHiddenInGame(Source->bHiddenInGame);
  Target->bVisibleInReflectionCaptures = Source->bVisibleInReflectionCaptures;
  Target->bVisibleInRealTimeSkyCaptures = Source->bVisibleInRealTimeSkyCaptures;
  Target->SetVisibleInRayTracing(Source->bVisibleInRayTracing);
  Target->SetRenderInMainPass(Source->bRenderInMainPass);
  Target->SetRenderInDepthPass(Source->bRenderInDepthPass);
  Target->SetReceivesDecals(Source->bReceivesDecals);
  Target->SetOwnerNoSee(Source->bOwnerNoSee);
  Target->SetOnlyOwnerSee(Source->bOnlyOwnerSee);
  Target->bTreatAsBackgroundForOcclusion = Source->bTreatAsBackgroundForOcclusion;
  Target->bUseAsOccluder = Source->bUseAsOccluder;
  Target->SetRenderCustomDepth(Source->bRenderCustomDepth);
  Target->SetVisibleInSceneCaptureOnly(Source->bVisibleInSceneCaptureOnly);
  Target->SetHiddenInSceneCapture(Source->bHiddenInSceneCapture);
  Target->SetCustomDepthStencilValue(Source->CustomDepthStencilValue);
  Target->SetTranslucentSortPriority(Source->TranslucencySortPriority);
  Target->SetTranslucencySortDistanceOffset(Source->TranslucencySortDistanceOffset);
  Target->SetBoundsScale(Source->BoundsScale);
  Target->SetCustomDepthStencilWriteMask(Source->CustomDepthStencilWriteMask);
  Target->bUseAttachParentBound = Source->bUseAttachParentBound;

  // Physics.
  Target->SetSimulatePhysics(Source->IsSimulatingPhysics());
  if (SourceBI && TargetBI && SourceBI->bOverrideMass)
  {
    Target->SetMassOverrideInKg(NAME_None, SourceBI->GetMassOverride(), true);
  }
  Target->SetLinearDamping(Source->GetLinearDamping());
  Target->SetAngularDamping(Source->GetAngularDamping());
  Target->SetEnableGravity(Source->IsGravityEnabled());
  if (SourceBI && TargetBI)
  {
    TargetBI->bLockTranslation = SourceBI->bLockTranslation;
    TargetBI->bLockXTranslation = SourceBI->bLockXTranslation;
    TargetBI->bLockYTranslation = SourceBI->bLockYTranslation;
    TargetBI->bLockZTranslation = SourceBI->bLockZTranslation;
    TargetBI->bLockRotation = SourceBI->bLockRotation;
    TargetBI->bLockXRotation = SourceBI->bLockXRotation;
    TargetBI->bLockYRotation = SourceBI->bLockYRotation;
    TargetBI->bLockZRotation = SourceBI->bLockZRotation;
    Target->SetConstraintMode(SourceBI->DOFMode);
  }
  Target->bIgnoreRadialImpulse = Source->bIgnoreRadialImpulse;
  Target->bIgnoreRadialForce = Source->bIgnoreRadialForce;
  Target->bApplyImpulseOnDamage = Source->bApplyImpulseOnDamage;
  Target->bReplicatePhysicsToAutonomousProxy = Source->bReplicatePhysicsToAutonomousProxy;
  if (SourceBI && TargetBI)
  {
    TargetBI->bAutoWeld = SourceBI->bAutoWeld;
    TargetBI->bStartAwake = SourceBI->bStartAwake;
    TargetBI->COMNudge = SourceBI->COMNudge;
  }
  Target->SetMassScale(NAME_None, Source->GetMassScale());
  if (SourceBI && TargetBI)
  {
    TargetBI->bOverrideMaxAngularVelocity = SourceBI->bOverrideMaxAngularVelocity;
    if (TargetBI->bOverrideMaxAngularVelocity)
    {
      Target->SetPhysicsMaxAngularVelocityInRadians(SourceBI->GetMaxAngularVelocityInRadians());
    }
    TargetBI->SleepFamily = SourceBI->SleepFamily;
    //TargetBI->bInertiaConditioning = SourceBI->bInertiaConditioning;
    TargetBI->InertiaTensorScale = SourceBI->InertiaTensorScale;
    //TargetBI->bOverrideWalkableSlopeOnInstance = SourceBI->bOverrideWalkableSlopeOnInstance;
    //if (TargetBI->bOverrideWalkableSlopeOnInstance)
    //{
    //  Target->SetWalkableSlopeOverride(Source->GetWalkableSlopeOverride());
    //}
    TargetBI->CustomSleepThresholdMultiplier = SourceBI->CustomSleepThresholdMultiplier;
    TargetBI->StabilizationThresholdMultiplier = SourceBI->StabilizationThresholdMultiplier;
    TargetBI->bGenerateWakeEvents = SourceBI->bGenerateWakeEvents;
    TargetBI->bUpdateMassWhenScaleChanges = SourceBI->bUpdateMassWhenScaleChanges;
  }
  Target->SetShouldUpdatePhysicsVolume(Source->GetShouldUpdatePhysicsVolume());

  // Collision.
  if (SourceBI)
  {
    Target->SetNotifyRigidBodyCollision(SourceBI->bNotifyRigidBodyCollision);
  }
  //Target->SetPhysMaterialOverride(SourceBI->PhysMaterialOverride);
  Target->SetGenerateOverlapEvents(Source->GetGenerateOverlapEvents());
  Target->CanCharacterStepUpOn = Source->CanCharacterStepUpOn;
  Target->SetCollisionProfileName(Source->GetCollisionProfileName());
  if (Target->GetCollisionProfileName() == "Custom")
  {
    Target->SetCollisionEnabled(Source->GetCollisionEnabled());
    Target->SetCollisionObjectType(Source->GetCollisionObjectType());
    Target->SetCollisionResponseToChannels(Source->GetCollisionResponseToChannels());
  }
  if (SourceBI && TargetBI)
  {
    Target->SetUseCCD(SourceBI->bUseCCD);
    TargetBI->bIgnoreAnalyticCollisions = SourceBI->bIgnoreAnalyticCollisions;
    TargetBI->bSmoothEdgeCollisions = SourceBI->bSmoothEdgeCollisions;
  }
  Target->bAlwaysCreatePhysicsState = Source->bAlwaysCreatePhysicsState;
  Target->bMultiBodyOverlap = Source->bMultiBodyOverlap;
  Target->bTraceComplexOnMove = Source->bTraceComplexOnMove;
  Target->bReturnMaterialOnMove = Source->bReturnMaterialOnMove;
#if WITH_EDITORONLY_DATA
  Target->bConsiderForActorPlacementWhenHidden = Source->bConsiderForActorPlacementWhenHidden;
#endif
  Target->SetCanEverAffectNavigation(Source->CanEverAffectNavigation());

  // Tags.
  for (const auto& Tag : Source->ComponentTags)
  {
    Target->ComponentTags.Emplace(Tag);
  }

  // Component Replication.
  Target->SetIsReplicated(Source->GetIsReplicated());

  // Mobile.
  Target->bReceiveMobileCSMShadows = Source->bReceiveMobileCSMShadows;

  // Ray Tracing.
  Target->RayTracingGroupId = Source->RayTracingGroupId;
  Target->RayTracingGroupCullingPriority = Source->RayTracingGroupCullingPriority;

  // Move Ignore Objects.
  Target->MoveIgnoreActors = Source->CopyArrayOfMoveIgnoreActors();
  Target->MoveIgnoreComponents = Source->CopyArrayOfMoveIgnoreComponents();
}

float UGMC_MovementUtilityCmp::GetMontageBlendInTime(UAnimMontage* Montage) const
{
  return Montage ? Montage->GetDefaultBlendInTime() : 0.f;
}

float UGMC_MovementUtilityCmp::GetMontageBlendOutTime(UAnimMontage* Montage) const
{
  return Montage ? Montage->GetDefaultBlendOutTime() : 0.f;
}

void UGMC_MovementUtilityCmp::IsBlendingAtPosition(const FGMC_AnimMontageInstance& MontageInstance, bool& bOutIsBlendingIn, bool& bOutIsBlendingOut) const
{
  bOutIsBlendingIn = false;
  bOutIsBlendingOut = false;

  const auto& Montage = MontageInstance.GetMontage();
  const float Position = MontageInstance.GetPosition();

  if (!Montage || Position < 0.f || Position > Montage->GetPlayLength())
  {
    return;
  }

  const float BlendInTime = Montage->GetDefaultBlendInTime();
  const float BlendOutTime = Montage->GetDefaultBlendOutTime();

  if (BlendInTime <= 0.f && BlendOutTime <= 0.f)
  {
    return;
  }

  const int32 PositionSectionID = Montage->GetAnimCompositeSectionIndexFromPos(Position, UNUSED(float));

  float BlendInPosition{0.f};
  float BlendOutPosition{0.f};
  int32 BlendInPositionID{0};
  int32 BlendOutPositionID{0};

  if (BlendInTime > 0.f)
  {
    BlendInPosition = MontageInstance.GetBlendInFinishedPosition();
    BlendInPositionID = MontageInstance.GetBlendInFinishedPositionIndex();
  }

  if (BlendOutTime > 0.f)
  {
    BlendOutPosition = MontageInstance.GetBlendOutStartedPosition();
    BlendOutPositionID = MontageInstance.GetBlendOutStartedPositionIndex();
  }

  if (PositionSectionID == BlendInPositionID)
  {
    bOutIsBlendingIn = Position < BlendInPosition;
  }
  else
  {
    const auto& PreviousSections = MontageInstance.GetPreviousSections();
    int32 SectionID = BlendInPositionID;
    gmc_ck(Montage->IsValidSectionIndex(SectionID))

    TArray<int32> VisitedSections{};
    while (Montage->IsValidSectionIndex(SectionID))
    {
      if (VisitedSections.Contains(SectionID))
      {
        break;
      }
      VisitedSections.Add(SectionID);

      if (PositionSectionID == SectionID)
      {
        bOutIsBlendingIn = true;
        break;
      }

      if (SectionID == 0)
      {
        break;
      }

      SectionID = PreviousSections[SectionID];
    }
  }

  if (PositionSectionID == BlendOutPositionID)
  {
    bOutIsBlendingOut = Position > BlendOutPosition;
  }
  else
  {
    const auto& NextSections = MontageInstance.GetNextSections();
    int32 SectionID = BlendOutPositionID;
    gmc_ck(Montage->IsValidSectionIndex(SectionID))

    TArray<int32> VisitedSections{};
    while (Montage->IsValidSectionIndex(SectionID))
    {
      if (VisitedSections.Contains(SectionID))
      {
        break;
      }
      VisitedSections.Add(SectionID);

      if (PositionSectionID == SectionID)
      {
        bOutIsBlendingOut = true;
        break;
      }

      if (SectionID == MontageInstance.GetLastSectionIndex())
      {
        break;
      }

      SectionID = NextSections[SectionID];
    }
  }
}

FAnimMontageInstance* UGMC_MovementUtilityCmp::GetMontageInstance(USkeletalMeshComponent* Mesh, UAnimMontage* Montage) const
{
  if (!IsValid(Mesh) || !IsValid(Montage))
  {
    return nullptr;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return nullptr;
  }

  const auto& MontageInstances = AnimInstance->MontageInstances;
  for (int32 Index = MontageInstances.Num() - 1; Index >= 0; --Index)
  {
    if (const auto& MontageInstance = MontageInstances[Index])
    {
      const auto& MontageAsset = MontageInstance->Montage; // May be nullptr.
      gmc_ck(Montage)
      if (Montage == MontageAsset)
      {
        return MontageInstance;
      }
    }
  }

  return nullptr;
}

bool UGMC_MovementUtilityCmp::PrePlayMontage_NonBlocking(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  float StartPosition,
  float PlayRate,
  bool bWhenExtrapolating,
  EMontagePlayReturnType ReturnValueType,
  float& OutReturnValue,
  bool& bOutSimulated
) const
{
  OutReturnValue = 0.f;
  bOutSimulated = false;

  if (
    !IsValid(MontageToPlay) ||
    !IsValid(Mesh) ||
    (!bWhenExtrapolating && IsExtrapolating()) ||
    MontageToPlay->GetPlayLength() <= 0.f ||
    StartPosition < 0.f ||
    StartPosition > MontageToPlay->GetPlayLength()
  )
  {
    return false;
  }

  // Check the anim instance to ensure that that the montage can actually be played.
  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!IsValid(AnimInstance))
  {
    return false;
  }

  if (!MontageTracker.SetActiveMontage(MontageToPlay, StartPosition, PlayRate))
  {
    gmc_ckne()
    return false;
  }

  // Do not play montages visually for non-simulated moves of smoothed listen server pawns or during a client replay. Only the the observable version of a pawn
  // should trigger any animations.
  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    // No divide-by-zero check on the EMontagePlayReturnType::Duration branch to keep the behaviour exactly the same as when Montage_Play is called.
    const float MontageLength = MontageToPlay->GetPlayLength();
    OutReturnValue = ReturnValueType == EMontagePlayReturnType::MontageLength ? MontageLength : MontageLength / (PlayRate * MontageToPlay->RateScale);
    bOutSimulated = true;
    return false;
  }

  return true;
}

bool UGMC_MovementUtilityCmp::PrePlayMontage_Blocking(
  const FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  bool bInterruptOther,
  bool bCancelOnBlendIn,
  bool bCancelOnBlendOut
) const
{
  if (MontageTracker.HasActiveMontage())
  {
    if (bInterruptOther && MontageTracker.Montage != MontageToPlay)
    {
      return true;
    }

    if (!bCancelOnBlendIn && !bCancelOnBlendOut)
    {
      return false;
    }

    FGMC_AnimMontageInstance MontageInstance{MontageTracker.Montage, MontageTracker.MontagePosition, MontageTracker.MontagePlayRate, true};

    bool bIsBlendingIn{false};
    bool bIsBlendingOut{false};
    IsBlendingAtPosition(MontageInstance, bIsBlendingIn, bIsBlendingOut);

    return (bCancelOnBlendIn && bIsBlendingIn) || (bCancelOnBlendOut && bIsBlendingOut);
  }

  return true;
}

float UGMC_MovementUtilityCmp::PlayMontage_NonBlocking(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  float StartPosition,
  float PlayRate,
  bool bWhenExtrapolating,
  EMontagePlayReturnType ReturnValueType
) const
{
  float ReturnValue{0.f};

  if (
    PrePlayMontage_NonBlocking(
      Mesh,
      MontageTracker,
      MontageToPlay,
      StartPosition,
      PlayRate,
      bWhenExtrapolating,
      ReturnValueType,
      ReturnValue,
      UNUSED(bool)
    )
  )
  {
    gmc_ck(ReturnValue == 0.f)
    gmc_ck(IsValid(Mesh))
    gmc_ck(IsValid(MontageToPlay))
    const auto& AnimInstance = Mesh->GetAnimInstance();
    gmc_ck(IsValid(AnimInstance))
    ReturnValue = Montage_Play(AnimInstance, MontageToPlay, PlayRate, ReturnValueType, StartPosition, true);
    CFLog(ReturnValue == 0.f, Error, "Montage \"%s\" failed to play.", *MontageToPlay->GetName())
    gmc_ck(ReturnValue > 0.f)
    return ReturnValue;
  }

  return ReturnValue;
}

float UGMC_MovementUtilityCmp::PlayMontage_Blocking(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  float StartPosition,
  float PlayRate,
  bool bInterruptOther,
  bool bCancelOnBlendIn,
  bool bCancelOnBlendOut,
  bool bWhenExtrapolating,
  EMontagePlayReturnType ReturnValueType
) const
{
  if (
    PrePlayMontage_Blocking(
      MontageTracker,
      MontageToPlay,
      bInterruptOther,
      bCancelOnBlendIn,
      bCancelOnBlendOut
    )
  )
  {
    return PlayMontage_NonBlocking(
      Mesh,
      MontageTracker,
      MontageToPlay,
      StartPosition,
      PlayRate,
      bWhenExtrapolating,
      ReturnValueType
    );
  }

  return 0.f;
}

float UGMC_MovementUtilityCmp::PlayMontageWithBlendIn_NonBlocking(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  const FAlphaBlendArgs& BlendIn,
  float StartPosition,
  float PlayRate,
  bool bWhenExtrapolating,
  EMontagePlayReturnType ReturnValueType
) const
{
  float ReturnValue{0.f};

  if (
    PrePlayMontage_NonBlocking(
      Mesh,
      MontageTracker,
      MontageToPlay,
      StartPosition,
      PlayRate,
      bWhenExtrapolating,
      ReturnValueType,
      ReturnValue,
      UNUSED(bool)
    )
  )
  {
    gmc_ck(ReturnValue == 0.f)
    gmc_ck(IsValid(Mesh))
    gmc_ck(IsValid(MontageToPlay))
    const auto& AnimInstance = Mesh->GetAnimInstance();
    gmc_ck(IsValid(AnimInstance))
    ReturnValue = Montage_PlayWithBlendIn(AnimInstance, MontageToPlay, BlendIn, PlayRate, ReturnValueType, StartPosition, true);
    CFLog(ReturnValue == 0.f, Error, "Montage \"%s\" failed to play.", *MontageToPlay->GetName())
    gmc_ck(ReturnValue > 0.f)
    return ReturnValue;
  }

  return ReturnValue;
}

float UGMC_MovementUtilityCmp::PlayMontageWithBlendIn_Blocking(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  const FAlphaBlendArgs& BlendIn,
  float StartPosition,
  float PlayRate,
  bool bInterruptOther,
  bool bCancelOnBlendIn,
  bool bCancelOnBlendOut,
  bool bWhenExtrapolating,
  EMontagePlayReturnType ReturnValueType
) const
{
  if (
    PrePlayMontage_Blocking(
      MontageTracker,
      MontageToPlay,
      bInterruptOther,
      bCancelOnBlendIn,
      bCancelOnBlendOut
    )
  )
  {
    return PlayMontageWithBlendIn_NonBlocking(
      Mesh,
      MontageTracker,
      MontageToPlay,
      BlendIn,
      StartPosition,
      PlayRate,
      bWhenExtrapolating,
      ReturnValueType
    );
  }

  return 0.f;
}

float UGMC_MovementUtilityCmp::PlayMontageWithBlendSettings_NonBlocking(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  const FMontageBlendSettings& BlendInSettings,
  float StartPosition,
  float PlayRate,
  bool bWhenExtrapolating,
  EMontagePlayReturnType ReturnValueType
) const
{
  float ReturnValue{0.f};

  if (
    PrePlayMontage_NonBlocking(
      Mesh,
      MontageTracker,
      MontageToPlay,
      StartPosition,
      PlayRate,
      bWhenExtrapolating,
      ReturnValueType,
      ReturnValue,
      UNUSED(bool)
    )
  )
  {
    gmc_ck(ReturnValue == 0.f)
    gmc_ck(IsValid(Mesh))
    gmc_ck(IsValid(MontageToPlay))
    const auto& AnimInstance = Mesh->GetAnimInstance();
    gmc_ck(IsValid(AnimInstance))
    ReturnValue = Montage_PlayWithBlendSettings(AnimInstance, MontageToPlay, BlendInSettings, PlayRate, ReturnValueType, StartPosition, true);
    CFLog(ReturnValue == 0.f, Error, "Montage \"%s\" failed to play.", *MontageToPlay->GetName())
    gmc_ck(ReturnValue > 0.f)
    return ReturnValue;
  }

  return ReturnValue;
}

float UGMC_MovementUtilityCmp::PlayMontageWithBlendSettings_Blocking(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* MontageToPlay,
  const FMontageBlendSettings& BlendInSettings,
  float StartPosition,
  float PlayRate,
  bool bInterruptOther,
  bool bCancelOnBlendIn,
  bool bCancelOnBlendOut,
  bool bWhenExtrapolating,
  EMontagePlayReturnType ReturnValueType
) const
{
  if (
    PrePlayMontage_Blocking(
      MontageTracker,
      MontageToPlay,
      bInterruptOther,
      bCancelOnBlendIn,
      bCancelOnBlendOut
    )
  )
  {
    return PlayMontageWithBlendSettings_NonBlocking(
      Mesh,
      MontageTracker,
      MontageToPlay,
      BlendInSettings,
      StartPosition,
      PlayRate,
      bWhenExtrapolating,
      ReturnValueType
    );
  }

  return 0.f;
}

void UGMC_MovementUtilityCmp::StopMontage(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  float BlendOutTime,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  const auto& MontageToStop = MontageTracker.Montage;

  MontageTracker.ClearActiveMontage();

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_Stop(AnimInstance, BlendOutTime, MontageToStop);
}

void UGMC_MovementUtilityCmp::StopMontageWithBlendOut(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  const FAlphaBlendArgs& BlendOut,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  const auto& MontageToStop = MontageTracker.Montage;

  MontageTracker.ClearActiveMontage();

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_StopWithBlendOut(AnimInstance, BlendOut, MontageToStop);
}

void UGMC_MovementUtilityCmp::StopMontageWithBlendSettings(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  const FMontageBlendSettings& BlendOutSettings,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  const auto& MontageToStop = MontageTracker.Montage;

  MontageTracker.ClearActiveMontage();

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_StopWithBlendSettings(AnimInstance, BlendOutSettings, MontageToStop);
}

void UGMC_MovementUtilityCmp::PauseMontage(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  MontageTracker.bMontagePaused = true;

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_Pause(AnimInstance, MontageTracker.Montage);
}

void UGMC_MovementUtilityCmp::ResumeMontage(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  MontageTracker.bMontagePaused = false;

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_Resume(AnimInstance, MontageTracker.Montage);
}

void UGMC_MovementUtilityCmp::JumpToMontageSection(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  FName SectionName,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  const int32 SectionID = MontageTracker.Montage->GetSectionIndex(SectionName);
  if (!MontageTracker.Montage->IsValidSectionIndex(SectionID))
  {
    return;
  }

  const auto CalculatePos = [](const FCompositeSection& Section, float PosWithinCompositeSection)
  {
    float Offset = Section.GetTime();
    Offset += PosWithinCompositeSection;
    return Offset;
  };

  const FCompositeSection& CurrentSection = MontageTracker.Montage->GetAnimCompositeSection(SectionID);
  const float NewPosition = CalculatePos(CurrentSection, 0.f);
  MontageTracker.MontagePosition = NewPosition;

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_JumpToSection(AnimInstance, SectionName, MontageTracker.Montage);
}

void UGMC_MovementUtilityCmp::JumpToMontageSectionsEnd(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  FName SectionName,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  const int32 SectionID = MontageTracker.Montage->GetSectionIndex(SectionName);
  if (!MontageTracker.Montage->IsValidSectionIndex(SectionID))
  {
    return;
  }

  const auto CalculatePos = [](const FCompositeSection& Section, float PosWithinCompositeSection)
  {
    float Offset = Section.GetTime();
    Offset += PosWithinCompositeSection;
    return Offset;
  };

  const FCompositeSection& CurrentSection = MontageTracker.Montage->GetAnimCompositeSection(SectionID);
  const float NewPosition = CalculatePos(CurrentSection, MontageTracker.Montage->GetSectionLength(SectionID));
  MontageTracker.MontagePosition = NewPosition;

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_JumpToSectionsEnd(AnimInstance, SectionName, MontageTracker.Montage);
}

FName UGMC_MovementUtilityCmp::GetCurrentMontageSection(const FGMC_MontageTracker& MontageTracker, float& OutPositionWithinSection) const
{
  OutPositionWithinSection = 0.f;

  if (!MontageTracker.HasActiveMontage())
  {
    return NAME_None;
  }

  const int32 SectionIndex = MontageTracker.Montage->GetAnimCompositeSectionIndexFromPos(MontageTracker.MontagePosition, OutPositionWithinSection);

  return MontageTracker.Montage->GetSectionName(SectionIndex);
}

float UGMC_MovementUtilityCmp::GetMontagePlayRate(USkeletalMeshComponent* Mesh, const FGMC_MontageTracker& MontageTracker) const
{
  if (!IsValid(Mesh) || !MontageTracker.HasActiveMontage())
  {
    return 0.f;
  }

  return MontageTracker.MontagePlayRate;
}

void UGMC_MovementUtilityCmp::SetMontagePlayRate(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  float NewPlayRate,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  MontageTracker.MontagePlayRate = NewPlayRate;

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_SetPlayRate(AnimInstance, MontageTracker.Montage, NewPlayRate);
}

float UGMC_MovementUtilityCmp::GetMontagePosition(USkeletalMeshComponent* Mesh, const FGMC_MontageTracker& MontageTracker) const
{
  if (!IsValid(Mesh) || !MontageTracker.HasActiveMontage())
  {
    return -1.f;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return -1.f;
  }

  return MontageTracker.MontagePosition;
}

void UGMC_MovementUtilityCmp::SetMontagePosition(
  USkeletalMeshComponent* Mesh,
  FGMC_MontageTracker& MontageTracker,
  float NewPosition,
  bool bWhenExtrapolating
) const
{
  if (!IsValid(Mesh) || (!bWhenExtrapolating && IsExtrapolating()) || !MontageTracker.HasActiveMontage())
  {
    return;
  }

  if (NewPosition < 0.f || NewPosition > MontageTracker.Montage->GetPlayLength())
  {
    return;
  }

  const auto& AnimInstance = Mesh->GetAnimInstance();
  if (!AnimInstance)
  {
    return;
  }

  gmc_ck(NewPosition >= 0.f)
  gmc_ck(NewPosition <= MontageTracker.Montage->GetPlayLength())
  MontageTracker.MontagePosition = NewPosition;

  if (CL_IsReplaying() || (IsExecutingNonSimulatedMove() && IsSmoothedListenServerPawn()))
  {
    return;
  }

  Montage_SetPosition(AnimInstance, MontageTracker.Montage, NewPosition);
}

bool UGMC_MovementUtilityCmp::SetActiveMontage(
  FGMC_MontageTracker& MontageTracker,
  UAnimMontage* Montage,
  float Position,
  float PlayRate,
  bool bWhenExtrapolating
) const
{
  if (!bWhenExtrapolating && IsExtrapolating())
  {
    return false;
  }

  return MontageTracker.SetActiveMontage(Montage, Position, PlayRate);
}

void UGMC_MovementUtilityCmp::ClearActiveMontage(FGMC_MontageTracker& MontageTracker, bool bWhenExtrapolating) const
{
  if (!bWhenExtrapolating && IsExtrapolating())
  {
    return;
  }

  MontageTracker.ClearActiveMontage();
}

UAnimMontage* UGMC_MovementUtilityCmp::GetActiveMontage(const FGMC_MontageTracker& MontageTracker) const
{
  if (MontageTracker.HasActiveMontage())
  {
    return MontageTracker.Montage;
  }

  return nullptr;
}

UAnimMontage* UGMC_MovementUtilityCmp::GetActiveNonRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const
{
  if (MontageTracker.HasActiveNonRootMotionMontage())
  {
    return MontageTracker.Montage;
  }

  return nullptr;
}

UAnimMontage* UGMC_MovementUtilityCmp::GetActiveRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const
{
  if (MontageTracker.HasActiveRootMotionMontage())
  {
    return MontageTracker.Montage;
  }

  return nullptr;
}

bool UGMC_MovementUtilityCmp::HasActiveMontage(const FGMC_MontageTracker& MontageTracker) const
{
  return MontageTracker.HasActiveMontage();
}

bool UGMC_MovementUtilityCmp::HasActiveRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const
{
  return MontageTracker.HasActiveRootMotionMontage();
}

bool UGMC_MovementUtilityCmp::HasActiveNonRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const
{
  return MontageTracker.HasActiveNonRootMotionMontage();
}

void UGMC_MovementUtilityCmp::PausePathFollowing(FAIRequestID RequestID, bool bKeepVelocity)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  const auto& AIController = Cast<AAIController>(PawnOwner->GetController());
  if (!AIController)
  {
    return;
  }

  const auto& PathFollowingComponent = AIController->GetPathFollowingComponent();
  if (!PathFollowingComponent)
  {
    return;
  }

  // If bKeepVelocity is false this will call StopActiveMovement internally which will then zero all velocity related members.
  PathFollowingComponent->PauseMove(RequestID, bKeepVelocity ? EPathFollowingVelocityMode::Keep : EPathFollowingVelocityMode::Reset);
}

void UGMC_MovementUtilityCmp::ResumePathFollowing(FAIRequestID RequestID)
{
  if (!IsValid(PawnOwner))
  {
    return;
  }

  const auto& AIController = Cast<AAIController>(PawnOwner->GetController());
  if (!AIController)
  {
    return;
  }

  const auto& PathFollowingComponent = AIController->GetPathFollowingComponent();
  if (!PathFollowingComponent)
  {
    return;
  }

  PathFollowingComponent->ResumeMove(RequestID);
}

void UGMC_MovementUtilityCmp::StopActiveMovement()
{
  Super::StopActiveMovement();
  HaltMovement();
}

FAIRequestID UGMC_MovementUtilityCmp::GetCurrentPathFollowingRequest() const
{
  return FAIRequestID::CurrentRequest;
}

bool UGMC_MovementUtilityCmp::IsPathFollowing() const
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  const auto& AIController = Cast<AAIController>(PawnOwner->GetController());
  if (!AIController)
  {
    return false;
  }

  const auto& PathFollowingComponent = AIController->GetPathFollowingComponent();
  if (!PathFollowingComponent)
  {
    return false;
  }

  return PathFollowingComponent->HasValidPath();
}

void UGMC_MovementUtilityCmp::EnableAccelerationForPathFollowing(bool bEnable)
{
  bUseAccelerationForPaths = bEnable;
}

bool UGMC_MovementUtilityCmp::IsUsingAccelerationForPathFollowing() const
{
  return bUseAccelerationForPaths;
}

void UGMC_MovementUtilityCmp::SimpleMoveToLocation_GMC(const FVector& TargetLocation) const
{
  const auto& World = GetWorld();
  if (!IsValid(World) || !IsValid(PawnOwner))
  {
    return;
  }

  UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  if (!NavSystem)
  {
    return;
  }

  const auto& PathFollowingComponent = [&]()
  {
    const auto& AIController = Cast<AAIController>(PawnOwner->GetController());
    UPathFollowingComponent* PathFollowingComponent = nullptr;

    if (AIController)
    {
      return AIController->GetPathFollowingComponent();
    }

    PathFollowingComponent = PawnOwner->FindComponentByClass<UPathFollowingComponent>();

    if (!PathFollowingComponent)
    {
      PathFollowingComponent = NewObject<UPathFollowingComponent>(PawnOwner);
      PathFollowingComponent->RegisterComponentWithWorld(GetWorld());
      PathFollowingComponent->Initialize();
    }

    return PathFollowingComponent;
  }();

  if (!PathFollowingComponent)
  {
    return;
  }

  if (!PathFollowingComponent->IsPathFollowingAllowed())
  {
    return;
  }

  const bool bAlreadyAtGoal = PathFollowingComponent->HasReached(TargetLocation, EPathFollowingReachMode::OverlapAgent);

  if (PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
  {
    PathFollowingComponent->AbortMove(
      *NavSystem,
      FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest,
      FAIRequestID::AnyRequest,
      bAlreadyAtGoal ? EPathFollowingVelocityMode::Reset : EPathFollowingVelocityMode::Keep
    );
  }

  if (bAlreadyAtGoal)
  {
    PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
    return;
  }

  const FVector AgentFeetLocation = GetActorLocation_GMC() - GetRootCollisionHalfHeight(true);
  const ANavigationData* NavData = NavSystem->GetNavDataForProps(GetNavAgentPropertiesRef(), AgentFeetLocation);
  if (!NavData)
  {
    return;
  }

  FPathFindingQuery PathFindingQuery(PawnOwner, *NavData, AgentFeetLocation, TargetLocation);
  FPathFindingResult PathFindingResult = NavSystem->FindPathSync(PathFindingQuery);

  if (PathFindingResult.IsSuccessful())
  {
    PathFollowingComponent->RequestMove(FAIMoveRequest(TargetLocation), PathFindingResult.Path);
    return;
  }

  if (PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
  {
    PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Invalid);
    return;
  }
}

void UGMC_MovementUtilityCmp::SimpleMoveToActor_GMC(AActor* TargetActor) const
{
  if (!IsValid(TargetActor))
  {
    return;
  }

  SimpleMoveToLocation_GMC(TargetActor->GetActorLocation());
}

float UGMC_MovementUtilityCmp::GetAgentRadius(const UNavigationSystemV1* const NavigationSystem) const
{
  const FNavAgentProperties& AgentProps = GetNavAgentPropertiesRef();
  if (AgentProps.AgentRadius > UE_KINDA_SMALL_NUMBER)
  {
    return AgentProps.AgentRadius;
  }

  if (NavigationSystem)
  {
    if (const auto& MainNavData = NavigationSystem->MainNavData)
    {
      const auto& AgentRadius = MainNavData->GetConfig().AgentRadius;
      if (AgentRadius > UE_KINDA_SMALL_NUMBER)
      {
        return AgentRadius;
      }
    }
  }

  return GetMaxDistanceToRootCollisionBoundaryXY();
};

float UGMC_MovementUtilityCmp::GetNavWalkingSearchHeightScale(const UNavigationSystemV1* const NavigationSystem) const
{
  const FNavAgentProperties& AgentProps = GetNavAgentPropertiesRef();
  if (AgentProps.NavWalkingSearchHeightScale > UE_KINDA_SMALL_NUMBER)
  {
    return AgentProps.NavWalkingSearchHeightScale;
  }

  if (NavigationSystem)
  {
    if (const auto& MainNavData = NavigationSystem->MainNavData)
    {
      const auto& NavWalkingSearchHeightScale = MainNavData->GetConfig().NavWalkingSearchHeightScale;
      if (NavWalkingSearchHeightScale > UE_KINDA_SMALL_NUMBER)
      {
        return NavWalkingSearchHeightScale;
      }
    }
  }

  return 1.f;
};

float UGMC_MovementUtilityCmp::GetAgentHeight(const UNavigationSystemV1* const NavigationSystem) const
{
  const FNavAgentProperties& AgentProps = GetNavAgentPropertiesRef();
  if (AgentProps.AgentHeight > UE_KINDA_SMALL_NUMBER)
  {
    return AgentProps.AgentHeight;
  }

  if (NavigationSystem)
  {
    if (const auto& MainNavData = NavigationSystem->MainNavData)
    {
      const auto& AgentHeight = MainNavData->GetConfig().AgentHeight;
      if (AgentHeight > UE_KINDA_SMALL_NUMBER)
      {
        return AgentHeight;
      }
    }
  }

  return GetRootCollisionHalfHeight(true) * 2.f;
};

float UGMC_MovementUtilityCmp::GetAgentStepHeight(const UNavigationSystemV1* const NavigationSystem) const
{
  const FNavAgentProperties& AgentProps = GetNavAgentPropertiesRef();
  if (AgentProps.AgentStepHeight >= 0.f)
  {
    return AgentProps.AgentStepHeight;
  }

  if (NavigationSystem)
  {
    if (const auto& MainNavData = NavigationSystem->MainNavData)
    {
      const auto& AgentStepHeight = MainNavData->GetConfig().AgentStepHeight;
      if (AgentStepHeight >= 0.f)
      {
        return AgentStepHeight;
      }
    }
  }

  return -1.f;
};

bool UGMC_MovementUtilityCmp::FilterContext(bool bServerMove, bool bClientMove, bool bClientReplay, bool bSimulationInterp, bool bSimulationExtrap)
{
  if (bServerMove)
  {
    if (IsServerPawn() && IsExecutingNonSimulatedMove())
    {
      return true;
    }
  }

  if (bClientMove)
  {
    if (!CL_IsReplaying() && IsExecutingNonSimulatedMove() && IsPredictedAutonomousProxy())
    {
      return true;
    }
  }

  if (bClientReplay)
  {
    if (CL_IsReplaying() && IsExecutingNonSimulatedMove())
    {
      gmc_ck(IsPredictedAutonomousProxy())
      return true;
    }
  }

  if (bSimulationInterp)
  {
    if (IsSimulating() && GetLastInterpTargetTime() >= 0.)
    {
      gmc_ck(!IsExtrapolating())
      return true;
    }
  }

  if (bSimulationExtrap)
  {
    if (IsSimulating() && IsExtrapolating())
    {
      gmc_ck(GetLastExtrapTargetTime() >= 0.)
      return true;
    }
  }

  return false;
}

bool UGMC_MovementUtilityCmp::FilterOverlap(
  bool bServerMove,
  bool bClientMove,
  bool bClientReplay,
  bool bSimulationInterp,
  bool bSimulationExtrap,
  const UPrimitiveComponent* OtherComponent,
  FGMC_FilterOverlapAux& Aux,
  EGMC_OverlapType Type
)
{
  if (!IsValid(PawnOwner))
  {
    return false;
  }

  if (OtherComponent != PawnOwner->GetRootComponent())
  {
    return false;
  }

  auto& PawnStatus = Aux.Status.FindOrAdd(this);

  const auto Initialize = [this, Type](bool bInitToActive, auto& PawnStatus, auto& Aux)
  {
    if (!PawnStatus.bInitialized)
    {
      Aux.Reset(bInitToActive, this);
      PawnStatus.bInitialized = true;
    }
  };

  const auto Filter = [this, Type](bool& AuxFlag, bool bClientMove)
  {
    if (Type == EGMC_OverlapType::Begin)
    {
      if (AuxFlag)
      {
        return false;
      }

      AuxFlag = true;

      if (bClientMove)
      {
        CL_DoNotCombineNextMove();
      }

      return true;
    }
    else
    {
      gmc_ck(Type == EGMC_OverlapType::End)

      if (!AuxFlag)
      {
        return false;
      }

      AuxFlag = false;

      if (bClientMove)
      {
        CL_DoNotCombineNextMove();
      }

      return true;
    }
  };

  // No early returns, the aux flags must be updated for each context.
  bool ReturnValue = false;

  // Whether the pawn was moved outside of a GMC context.
  bool bExternalMove = true;

  if (bServerMove || bExternalMove)
  {
    if (IsServerPawn() && IsExecutingNonSimulatedMove())
    {
      bExternalMove = false;

      if (bServerMove)
      {
        Initialize(Type == EGMC_OverlapType::End, PawnStatus, Aux);
        ReturnValue |= Filter(PawnStatus.bServerMoveActive, false);
      }
    }
  }

  if (bClientMove || bExternalMove)
  {
    if (!CL_IsReplaying() && IsExecutingNonSimulatedMove() && IsPredictedAutonomousProxy())
    {
      bExternalMove = false;

      if (bClientMove)
      {
        Initialize(Type == EGMC_OverlapType::End, PawnStatus, Aux);
        ReturnValue |= Filter(PawnStatus.bClientMoveActive, true);
      }
    }
  }

  if (bClientReplay || bExternalMove)
  {
    if (CL_IsReplaying() && IsExecutingNonSimulatedMove())
    {
      gmc_ck(IsPredictedAutonomousProxy())
      bExternalMove = false;

      if (bClientReplay)
      {
        Initialize(Type == EGMC_OverlapType::End, PawnStatus, Aux);
        ReturnValue |= Filter(PawnStatus.bClientReplayActive, false);
      }
    }
  }

  if (bSimulationInterp || bExternalMove)
  {
    if (IsSimulating() && GetLastInterpTargetTime() >= 0.)
    {
      gmc_ck(!IsExtrapolating())
      bExternalMove = false;

      if (bSimulationInterp)
      {
        Initialize(Type == EGMC_OverlapType::End, PawnStatus, Aux);
        ReturnValue |= Filter(PawnStatus.bSimulationInterpActive, false);
      }
    }
  }

  if (bSimulationExtrap || bExternalMove)
  {
    if (IsSimulating() && IsExtrapolating())
    {
      gmc_ck(GetLastExtrapTargetTime() >= 0.)
      bExternalMove = false;

      if (bSimulationExtrap)
      {
        Initialize(Type == EGMC_OverlapType::End, PawnStatus, Aux);
        ReturnValue |= Filter(PawnStatus.bSimulationExtrapActive, false);
      }
    }
  }

  if (bExternalMove)
  {
    Initialize(Type == EGMC_OverlapType::Begin, PawnStatus, Aux);
  }

  return ReturnValue;
}

void UGMC_MovementUtilityCmp::ResetFilterOverlapAux(FGMC_FilterOverlapAux& Aux, bool bIsOverlapping)
{
  Aux.Reset(bIsOverlapping, this);
  gmc_ck(Aux.Status.Contains(this))
  Aux.Status.FindOrAdd(this).bInitialized = true;
}

void UGMC_MovementUtilityCmp::PackBoolsToByte(
  uint8& OutByte,
  bool InBit1,
  bool InBit2,
  bool InBit3,
  bool InBit4,
  bool InBit5,
  bool InBit6,
  bool InBit7,
  bool InBit8
)
{
  OutByte = 0;
  OutByte |= static_cast<uint8>(InBit1);
  OutByte |= static_cast<uint8>(InBit2) << 1;
  OutByte |= static_cast<uint8>(InBit3) << 2;
  OutByte |= static_cast<uint8>(InBit4) << 3;
  OutByte |= static_cast<uint8>(InBit5) << 4;
  OutByte |= static_cast<uint8>(InBit6) << 5;
  OutByte |= static_cast<uint8>(InBit7) << 6;
  OutByte |= static_cast<uint8>(InBit8) << 7;
}

void UGMC_MovementUtilityCmp::UnpackBoolsFromByte(
  const uint8& InByte,
  bool& OutBit1,
  bool& OutBit2,
  bool& OutBit3,
  bool& OutBit4,
  bool& OutBit5,
  bool& OutBit6,
  bool& OutBit7,
  bool& OutBit8
)
{
  OutBit1 = InByte & 0x1;
  OutBit2 = InByte & 0x2;
  OutBit3 = InByte & 0x4;
  OutBit4 = InByte & 0x8;
  OutBit5 = InByte & 0x10;
  OutBit6 = InByte & 0x20;
  OutBit7 = InByte & 0x40;
  OutBit8 = InByte & 0x80;
}

float UGMC_MovementUtilityCmp::GetAvgFPS()
{
  return GAverageFPS;
}

float UGMC_MovementUtilityCmp::GetAvgMS()
{
  return GAverageMS;
}

void UGMC_MovementUtilityCmp::NetLog(const FString& String)
{
  GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("%s"), *String);
}

void UGMC_MovementUtilityCmp::NetLogWarning(const FString& String)
{
  GMC_LOG(LogGMCMovement, PawnOwner, Warning, TEXT("%s"), *String);
}

void UGMC_MovementUtilityCmp::NetLogError(const FString& String)
{
  GMC_LOG(LogGMCMovement, PawnOwner, Error, TEXT("%s"), *String);
}

float UGMC_MovementUtilityCmp::Montage_Play(
  UAnimInstance* AnimInstance,
  UAnimMontage* MontageToPlay,
  float PlayRate,
  EMontagePlayReturnType ReturnValueType,
  float StartPosition,
  bool bStopAllMontages
) const
{
  return AnimInstance->Montage_Play(MontageToPlay, PlayRate, ReturnValueType, StartPosition, bStopAllMontages);
}

float UGMC_MovementUtilityCmp::Montage_PlayWithBlendIn(
  UAnimInstance* AnimInstance,
  UAnimMontage* MontageToPlay,
  const FAlphaBlendArgs& BlendIn,
  float PlayRate,
  EMontagePlayReturnType ReturnValueType,
  float StartPosition,
  bool bStopAllMontages
) const
{
  return AnimInstance->Montage_PlayWithBlendIn(MontageToPlay, BlendIn, PlayRate, ReturnValueType, StartPosition, bStopAllMontages);
}

float UGMC_MovementUtilityCmp::Montage_PlayWithBlendSettings(
  UAnimInstance* AnimInstance,
  UAnimMontage* MontageToPlay,
  const FMontageBlendSettings& BlendInSettings,
  float PlayRate,
  EMontagePlayReturnType ReturnValueType,
  float StartPosition,
  bool bStopAllMontages
) const
{
  return AnimInstance->Montage_PlayWithBlendSettings(MontageToPlay, BlendInSettings, PlayRate, ReturnValueType, StartPosition, bStopAllMontages);
}

void UGMC_MovementUtilityCmp::Montage_Stop(UAnimInstance* AnimInstance, float BlendOutTime, UAnimMontage* MontageToStop) const
{
  AnimInstance->Montage_Stop(BlendOutTime, MontageToStop);
}

void UGMC_MovementUtilityCmp::Montage_StopWithBlendOut(UAnimInstance* AnimInstance, const FAlphaBlendArgs& BlendOut, UAnimMontage* MontageToStop) const
{
  AnimInstance->Montage_StopWithBlendOut(BlendOut, MontageToStop);
}

void UGMC_MovementUtilityCmp::Montage_StopWithBlendSettings(
  UAnimInstance* AnimInstance,
  const FMontageBlendSettings& BlendOutSettings,
  UAnimMontage* MontageToStop
) const
{
  AnimInstance->Montage_StopWithBlendSettings(BlendOutSettings, MontageToStop);
}

void UGMC_MovementUtilityCmp::Montage_Pause(UAnimInstance* AnimInstance, UAnimMontage* MontageToPause) const
{
  AnimInstance->Montage_Pause(MontageToPause);
}

void UGMC_MovementUtilityCmp::Montage_Resume(UAnimInstance* AnimInstance, UAnimMontage* MontageToResume) const
{
  AnimInstance->Montage_Resume(MontageToResume);
}

void UGMC_MovementUtilityCmp::Montage_JumpToSection(UAnimInstance* AnimInstance, FName SectionName, UAnimMontage* Montage) const
{
  AnimInstance->Montage_JumpToSection(SectionName, Montage);
}

void UGMC_MovementUtilityCmp::Montage_JumpToSectionsEnd(UAnimInstance* AnimInstance, FName SectionName, UAnimMontage* Montage) const
{
  AnimInstance->Montage_JumpToSectionsEnd(SectionName, Montage);
}

void UGMC_MovementUtilityCmp::Montage_SetPlayRate(UAnimInstance* AnimInstance, UAnimMontage* Montage, float NewPlayRate) const
{
  AnimInstance->Montage_SetPlayRate(Montage, NewPlayRate);
}

void UGMC_MovementUtilityCmp::Montage_SetPosition(UAnimInstance* AnimInstance, UAnimMontage* Montage, float NewPosition) const
{
  AnimInstance->Montage_SetPosition(Montage, NewPosition);
}
