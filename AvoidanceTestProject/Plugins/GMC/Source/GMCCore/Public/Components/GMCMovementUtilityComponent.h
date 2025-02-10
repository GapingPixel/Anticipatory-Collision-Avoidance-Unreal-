// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCReplicationComponent.h"
#include "GMCAnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "PhysicsEngine/BodyInstance.h"
#include "GMCMovementUtilityComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("UGMC_MovementUtilityCmp"), STATGROUP_UGMC_MovementUtilityCmp, STATCAT_Advanced);

UENUM(BlueprintType)
enum class EGMC_RadialImpulseFalloff : uint8
{
  Constant UMETA(ToolTip = "The impulse is of constant strength, up to the limit of its range."),
  Linear UMETA(ToolTip = "The impulse gets linearly weaker the further away we are from the origin."),
  MAX UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EGMC_CollisionShape : uint8
{
  VerticalCapsule,
  HorizontalCapsule,
  Box,
  Sphere,
  Invalid,
  MAX UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EGMC_AdjustDirection : uint8
{
  Up,
  Down,
  MAX UMETA(Hidden),
};

USTRUCT(BlueprintType)
struct FGMC_FloorParams
{
  GENERATED_BODY()

  FGMC_FloorParams() = default;
  virtual ~FGMC_FloorParams() {}

  FGMC_FloorParams(const FHitResult& ShapeHit, const FHitResult& LineHit, const UGMC_ReplicationCmp* const Outer)
    : ShapeHitResult(ShapeHit),
      LineHitResult(LineHit),
      SourceWorldPawnTransform(Outer->GetActorTransform_GMC()),
      SourceActorBase(Outer->GetActorBase()),
      bDirtyManual(false),
      bHasData(true)
  {
    gmc_ck(IsValid(Outer))

    if (ShapeHit.IsValidBlockingHit())
    {
      const FVector& SourceLocation = SourceWorldPawnTransform.GetLocation();
      ShapeDistanceToFloor = FMath::Abs(SourceLocation.Z - ShapeHit.Location.Z);
    }

    if (LineHit.IsValidBlockingHit())
    {
      LineDistanceToFloor = LineHit.Distance;
    }

    if (SourceActorBase)
    {
      SourceWorldActorBaseTransform = SourceActorBase->GetComponentTransform();
    }
  }

  // The distance of the pawn's lower bound to the floor based on the shape trace.
  float GetShapeDistanceToFloor() const
  {
    UE_CLOG(
      !HasValidShapeData(),
      LogTemp,
      Warning,
      TEXT("Accessing uninitialized or invalid floor data with call to FFloorParams::GetShapeDistanceToFloor.")
    )
    return ShapeDistanceToFloor;
  }

  // The distance of the pawn's lower bound to the floor based on the line trace.
  float GetLineDistanceToFloor() const
  {
    UE_CLOG(
      !HasValidLineData(),
      LogTemp,
      Warning,
      TEXT("Accessing uninitialized or invalid floor data with call to FFloorParams::GetLineDistanceToFloor.")
    )
    return LineDistanceToFloor;
  }

  // The pawn transform the floor data is based on.
  FTransform GetSourceWorldPawnTransform() const
  {
    return SourceWorldPawnTransform;
  }

  // The actor base the floor data is based on.
  USceneComponent* GetSourceActorBase() const
  {
    return SourceActorBase;
  }

  // The actor base transform the floor data is based on (if SourceActorBase is not null).
  FTransform GetSourceWorldActorBaseTransform() const
  {
    return SourceWorldActorBaseTransform;
  }

  // Update the distance of the pawn to the floor from the shape trace.
  void SetShapeDistanceToFloor(float NewDistanceToFloor)
  {
    ShapeDistanceToFloor = FMath::Abs(NewDistanceToFloor);
  }

  // Update the distance of the pawn to the floor from the line trace.
  void SetLineDistanceToFloor(float NewDistanceToFloor)
  {
    LineDistanceToFloor = FMath::Abs(NewDistanceToFloor);
  }

  // Update the hit result for the shape trace.
  void SetShapeHit(const FHitResult& NewShapeHit)
  {
    ShapeHitResult = NewShapeHit;
  }

  // Update the hit result for the line trace.
  void SetLineHit(const FHitResult& NewLineHit)
  {
    LineHitResult = NewLineHit;
  }

  // Update the data that the current floor is based on.
  void RefreshSourceData(const UGMC_ReplicationCmp* Outer)
  {
    SourceWorldPawnTransform = Outer->GetActorTransform_GMC();
    SourceActorBase = Outer->GetActorBase();
    SourceWorldActorBaseTransform = SourceActorBase ? SourceActorBase->GetComponentTransform() : FTransform::Identity;
  }

  // The shape trace hit result.
  FHitResult ShapeHit() const
  {
    // Accessing the shape hit when it did not produce a valid blocking hit is not necessarily an error.
    UE_CLOG(
      !HasData(),
      LogTemp,
      Warning,
      TEXT("Accessing uninitialized data with call to FFloorParams::ShapeHit.")
    )
    return ShapeHitResult;
  }

  // The line trace hit result.
  FHitResult LineHit() const
  {
    // Accessing the line hit when it did not produce a valid blocking hit is not necessarily an error.
    UE_CLOG(
      !HasData(),
      LogTemp,
      Warning,
      TEXT("Accessing uninitialized data with call to FFloorParams::LineHit.")
    )
    return LineHitResult;
  }

  // Returns true if the struct currently holds a valid shape trace hit.
  bool HasValidShapeData() const
  {
    gmc_ck(ShapeHitResult.IsValidBlockingHit() ? ShapeDistanceToFloor >= 0.f && bHasData : ShapeDistanceToFloor == -1.f)
    return ShapeHitResult.IsValidBlockingHit();
  }

  // Returns true if the struct currently holds a valid line trace hit.
  bool HasValidLineData() const
  {
    gmc_ck(LineHitResult.IsValidBlockingHit() ? LineDistanceToFloor >= 0.f && bHasData : LineDistanceToFloor == -1.f)
    return LineHitResult.IsValidBlockingHit();
  }

  // Returns true if the floor data is out of date (or uninitialized).
  bool IsDirty(double Tolerance, const UGMC_ReplicationCmp* const Outer) const
  {
    gmc_ck(IsValid(Outer))

    if (!bHasData || bDirtyManual || Tolerance <= 0.)
    {
      return true;
    }

    const auto& CurrentActorBase = Outer->GetActorBase();

    if (CurrentActorBase != SourceActorBase)
    {
      return true;
    }

    if (!IsValid(CurrentActorBase))
    {
      if (IsLocationDirty(Tolerance, Outer))
      {
        return true;
      }

      if (IsRotationDirty(Tolerance, Outer))
      {
        return true;
      }

      if (IsScaleDirty(Tolerance, Outer))
      {
        return true;
      }

      return false;
    }

    if (IsBasedLocationDirty(Tolerance, Outer))
    {
      return true;
    }

    if (IsBasedRotationDirty(Tolerance, Outer))
    {
      return true;
    }

    if (IsBasedScaleDirty(Tolerance, Outer))
    {
      return true;
    }

    return false;
  }

  // Allows for marking the floor data as dirty manually.
  void MarkDirty(bool bMarkDirty)
  {
    bDirtyManual = bMarkDirty;
  }

  // Returns true if the struct was initialized with some form of data (i.e. was not default constructed).
  bool HasData() const
  {
    return bHasData;
  }

  // Clears the current floor parameters.
  void Reset()
  {
    ShapeHitResult = FHitResult{};
    LineHitResult = FHitResult{};
    ShapeDistanceToFloor = -1.f;
    LineDistanceToFloor = -1.f;
    SourceWorldPawnTransform = FTransform::Identity;
    SourceActorBase = nullptr;
    SourceWorldActorBaseTransform = FTransform::Identity;
    bDirtyManual = false;
    bHasData = false;
  }

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The shape trace hit result.
  FHitResult ShapeHitResult{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The line trace hit result.
  FHitResult LineHitResult{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The distance of the pawn's lower bound to the floor based on the shape trace (will be -1 if not valid).
  float ShapeDistanceToFloor{-1.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The distance of the pawn's lower bound to the floor based on the line trace (will be -1 if not valid).
  float LineDistanceToFloor{-1.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The pawn transform the current floor data is based on in world space.
  FTransform SourceWorldPawnTransform{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The actor base the current floor data is based on.
  USceneComponent* SourceActorBase{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The actor base transform the current floor data is based on in world space (if SourceActorBase is not null).
  FTransform SourceWorldActorBaseTransform{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // True if the floor data has been marked as dirty manually.
  bool bDirtyManual{false};

protected:

  virtual bool IsLocationDirty(double Tolerance, const UGMC_ReplicationCmp* const Outer) const
  {
    gmc_ck(!IsValid(SourceActorBase))

    const auto& CurrentWorldPawnLocation = Outer->GetActorTransform_GMC().GetLocation();
    return !CurrentWorldPawnLocation.Equals(SourceWorldPawnTransform.GetLocation(), Tolerance);
  }

  virtual bool IsRotationDirty(double Tolerance, const UGMC_ReplicationCmp* const Outer) const
  {
    gmc_ck(!IsValid(SourceActorBase))

    const auto& CurrentWorldPawnRotation = Outer->GetActorTransform_GMC().GetRotation();
    return !CurrentWorldPawnRotation.Equals(SourceWorldPawnTransform.GetRotation(), Tolerance);
  }

  virtual bool IsScaleDirty(double Tolerance, const UGMC_ReplicationCmp* const Outer) const
  {
    gmc_ck(!IsValid(SourceActorBase))

    const auto& CurrentWorldPawnScale = Outer->GetActorTransform_GMC().GetScale3D();
    return !CurrentWorldPawnScale.Equals(SourceWorldPawnTransform.GetScale3D(), Tolerance);
  }

  virtual bool IsBasedLocationDirty(double Tolerance, const UGMC_ReplicationCmp* const Outer) const
  {
    const auto& CurrentActorBase = Outer->GetActorBase();

    gmc_ck(IsValid(CurrentActorBase))
    gmc_ck(CurrentActorBase == SourceActorBase)

    const auto& CurrentWorldPawnLocation = Outer->GetActorTransform_GMC().GetLocation();
    const auto& CurrentBasedPawnLocation = Outer->GetBasedActorLocationFor(CurrentWorldPawnLocation, CurrentActorBase->GetComponentTransform());
    const auto& SourceBasedPawnLocation = Outer->GetBasedActorLocationFor(SourceWorldPawnTransform.GetLocation(), SourceWorldActorBaseTransform);

    return !CurrentBasedPawnLocation.Equals(SourceBasedPawnLocation, Tolerance);
  }

  virtual bool IsBasedRotationDirty(double Tolerance, const UGMC_ReplicationCmp* const Outer) const
  {
    const auto& CurrentActorBase = Outer->GetActorBase();

    gmc_ck(IsValid(CurrentActorBase))
    gmc_ck(CurrentActorBase == SourceActorBase)

    const auto& CurrentWorldPawnRotation = Outer->GetActorTransform_GMC().GetRotation();
    const auto& CurrentBasedPawnRotation = Outer->GetBasedActorRotationFor(CurrentWorldPawnRotation.Rotator(), CurrentActorBase->GetComponentTransform());
    const auto& SourceBasedPawnRotation = Outer->GetBasedActorRotationFor(SourceWorldPawnTransform.GetRotation().Rotator(), SourceWorldActorBaseTransform);

    return !CurrentBasedPawnRotation.Equals(SourceBasedPawnRotation, Tolerance);
  }

  virtual bool IsBasedScaleDirty(double Tolerance, const UGMC_ReplicationCmp* const Outer) const
  {
    const auto& CurrentActorBase = Outer->GetActorBase();

    gmc_ck(IsValid(CurrentActorBase))
    gmc_ck(CurrentActorBase == SourceActorBase)

    const auto& CurrentWorldPawnScale = Outer->GetActorTransform_GMC().GetScale3D();
    const auto& CurrentBasedPawnScale = Outer->GetBasedActorScaleFor(CurrentWorldPawnScale, CurrentActorBase->GetComponentTransform());
    const auto& SourceBasedPawnScale = Outer->GetBasedActorScaleFor(SourceWorldPawnTransform.GetScale3D(), SourceWorldActorBaseTransform);

    return !CurrentBasedPawnScale.Equals(SourceBasedPawnScale, Tolerance);
  }

private:

  bool bHasData{false};
};

USTRUCT(BlueprintType)
struct FGMC_FilterOverlapAux
{
  GENERATED_BODY()

  struct FStatus
  {
    bool bInitialized{false};

    bool bServerMoveActive{false};
    bool bClientMoveActive{false};
    bool bClientReplayActive{false};
    bool bSimulationInterpActive{false};
    bool bSimulationExtrapActive{false};
  };

  TMap<UGMC_ReplicationCmp*, FStatus> Status{};

  void Reset(const bool bIsOverlapping, UGMC_ReplicationCmp* Outer)
  {
    auto& PawnStatus = Status.FindOrAdd(Outer);

    if (bIsOverlapping)
    {
      PawnStatus.bServerMoveActive = true;
      PawnStatus.bClientMoveActive = true;
      PawnStatus.bClientReplayActive = true;
      PawnStatus.bSimulationInterpActive = true;
      PawnStatus.bSimulationExtrapActive = true;
    }
    else
    {
      PawnStatus.bServerMoveActive = false;
      PawnStatus.bClientMoveActive = false;
      PawnStatus.bClientReplayActive = false;
      PawnStatus.bSimulationInterpActive = false;
      PawnStatus.bSimulationExtrapActive = false;
    }
  }
};

UENUM(BlueprintType)
enum class EGMC_OverlapType : uint8
{
  Begin,
  End,
  MAX UMETA(Hidden),
};

/// Facilitates the implementation of movement components by providing a basic framework with common utility.
UCLASS(ClassGroup = "Movement", BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GMCCORE_API UGMC_MovementUtilityCmp : public UGMC_ReplicationCmp
{
  GENERATED_BODY()

public:

  UGMC_MovementUtilityCmp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

  void PredictionTick(
    const FGMC_PawnState& InputState,
    float DeltaTime,
    double Timestamp,
    int32 Iteration,
    bool bSubSteppedIteration,
    bool bLocal,
    bool bSimulated,
    bool bCombined
  ) override final;

  void SimulationTick(
    float DeltaTime,
    double SimTime,
    int32 TargetIdx,
    const TArray<int32>& SkippedIdx
  ) override final;

  void AncillaryTick(
    const FGMC_Move& Move,
    float DeltaTime,
    double Timestamp,
    bool bLocalMove,
    bool bCombinedClientMove,
    bool& bOutDoNotCombineNextClientMove
  ) override final;

  void ClearTransientData(bool bResetMoves = true) override;

  /// Generic prediction tick - implement your own predicted movement logic here.
  ///
  /// @param        DeltaTime    The delta time for the current move. Not necessarily the same as the current frame delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void GenPredictionTick(float DeltaTime);
  virtual void GenPredictionTick_Implementation(float DeltaTime) {}

  /// Generic simulation tick - implement additional cosmetic logic for simulated pawns here.
  ///
  /// @param        DeltaTime    The current frame delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "General Movement Component")
  void GenSimulationTick(float DeltaTime);
  virtual void GenSimulationTick_Implementation(float DeltaTime) {}

  /// Generic ancillary tick - implement additional (predicted) gameplay logic that should not be affected by client replays or sub-stepping here.
  ///
  /// @param        DeltaTime              The delta time for the current move. Not necessarily the same as the current frame delta time.
  /// @param        bLocalMove             Whether the current move is a local move.
  /// @param        bCombinedClientMove    Whether the current move is a combined client move (i.e. a move that was not newly enqueued this frame). Always false
  ///                                      on the server.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void GenAncillaryTick(float DeltaTime, bool bLocalMove, bool bCombinedClientMove);
  virtual void GenAncillaryTick_Implementation(float DeltaTime, bool bLocalMove, bool bCombinedClientMove) {}

  /// Allows for custom pre-processing of the delta time value before move execution. By default this applies the effective time dilation to the the delta time
  /// which may yield faulty or unexpected results in networked games if it has a value other than one.
  ///
  /// @param        RealDeltaTime    The current real-time delta seconds.
  /// @returns      float            The move delta time to use.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  float PreProcessMoveDeltaTime(float RealDeltaTime) const;
  virtual float PreProcessMoveDeltaTime_Implementation(float RealDeltaTime) const;

public:

  /// How much distance to add to penetration adjustments to account for rounding.
  static constexpr float PENETRATION_ADJUSTMENT_TOLERANCE = 2.5f;

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The mass to use for velocity calculations (in kg).
  float Mass{100.f};

  /// Returns the timestamp of the move currently being executed.
  ///
  /// @returns      float    The timestamp of the current move.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetMoveTimestamp() const;

  /// Returns the delta time for the current move execution (may be sub-stepped). The move delta time is not necessarily equal to the current frame delta time.
  ///
  /// @returns      float    The delta time of the current move.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float GetMoveDeltaTime() const;

  /// Returns the input state of the current move.
  ///
  /// @returns      const FGMC_PawnState&    The input state of the current move.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const FGMC_PawnState& GetMoveInputState() const;

  /// Returns the number of the current iteration of a move execution. Only relevant in the context of physics sub-stepping. Usually there is only 1 iteration
  /// to be executed but if the delta time of the move exceeds the max time step the move will be broken down into 2 or more iterations.
  ///
  /// @returns      int32    The number of the iteration that currently being executed. This can be between 1 (for the first iteration) and the configured max
  ///                        amount of iterations (inclusive).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 GetIterationNumber() const;

  /// Tells us whether we are currently executing a sub-stepped iteration. An iteration is sub-stepped if it is not the last one for a given move.
  ///
  /// @returns      bool    True if we are currently executing a move within an iteration that is not the final one, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsSubSteppedIteration() const;

  /// Tells us whether we are currently executing a local move. A move is local if it is executed by a local server pawn or a predicted autonomous proxy (but
  /// excluding replay moves).
  ///
  /// @returns      bool    True if we are currently executing a local move, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsLocalMove() const;

  /// Tells us whether we are currently executing a simulated move. A move is simulated if it is executed in a smoothing context.
  ///
  /// @returns      bool    True if we are currently executing a move for a simulated pawn, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsSimulatedMove() const;

  /// Tells us whether we are currently executing a combined client move. This always returns false on the server.
  ///
  /// @returns      bool    True if we are currently executing a combined client move, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsCombinedMove() const;

  /// The time at which the simulated pawn is currently being displayed.
  ///
  /// @returns      double    The current smoothing time.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetSmoothingTime() const;

  /// Returns the target index for the current smoothing iteration.
  ///
  /// @returns      int32    The current smoothing target index.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 GetSmoothingTargetIdx() const;

  /// Returns all skipped move indices (if any) for the current smoothing iteration from oldest (at index 0) to newest (at the last index).
  ///
  /// @returns      TArray<int32>    All skipped indices. Will have size 0 if no states were skipped or if bDetermineSkippedSmoothingStates is false.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  TArray<int32> GetSkippedSmoothingIndices() const;

  /// The delta time used for physics calculations by default.
  ///
  /// @returns      float    The currently set physics delta time.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float GetPhysDeltaTime() const;

  /// Returns the acceleration accumulated since the last physics reset. The acceleration value is transient and does not persist between ticks.
  ///
  /// @returns      FVector    The current acceleration.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetTransientAcceleration() const;

  /// Returns the force accumulated since the last physics reset. The force value is transient and does not persist between ticks.
  ///
  /// @returns      FVector    The current force.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetTransientForce() const;

  /// Returns the starting velocity that was saved with the last physics reset. Does not persist between ticks.
  ///
  /// @returns      FVector    The velocity saved with the last physics reset.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetStartVelocity() const;

  /// Returns the starting location that was saved with the last physics reset. Does not persist between ticks.
  ///
  /// @returns      FVector    The location saved with the last physics reset.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetStartLocation() const;

  /// Set the current velocity. Updates acceleration and force from the passed velocity as well.
  ///
  /// @param        NewVelocity    The new velocity to set.
  /// @param        DeltaTime      The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UpdateVelocity(const FVector& NewVelocity, float DeltaTime = 0.f);

  /// Set the current acceleration. Updates velocity and force from the passed acceleration as well. The acceleration value is transient and does not persist
  /// between ticks.
  ///
  /// @param        NewAcceleration    The new acceleration to set.
  /// @param        DeltaTime          The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UpdateAcceleration(const FVector& NewAcceleration, float DeltaTime = 0.f);

  /// Set the current force. Updates velocity and acceleration from the passed force as well. The force value is transient and does not persist between ticks.
  ///
  /// @param        NewForce     The new force to set.
  /// @param        DeltaTime    The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UpdateForce(const FVector& NewForce, float DeltaTime = 0.f);

  /// Add to the current velocity. Updates acceleration and force from the new total velocity as well.
  ///
  /// @param        AddVelocity    The velocity to add to the current velocity.
  /// @param        DeltaTime      The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void AddVelocity(const FVector& AddVelocity, float DeltaTime = 0.f);

  /// Add to the current acceleration. Updates velocity and force from the new total acceleration as well. The acceleration value is transient and does not
  /// persist between ticks.
  ///
  /// @param        AddAcceleration    The acceleration to add to the current acceleration.
  /// @param        DeltaTime          The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void AddAcceleration(const FVector& AddAcceleration, float DeltaTime = 0.f);

  /// Add to the current force. Updates velocity and acceleration from the new total force as well. The force value is transient and does not persist between
  /// ticks.
  ///
  /// @param        AddForce     The force to add to the current force.
  /// @param        DeltaTime    The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void AddForce(const FVector& AddForce, float DeltaTime = 0.f);

  /// Adds an impulse to the pawn. An impulse should only be applied for one single frame. If you want to add a continuous force to the pawn over several frames
  /// use AddForce.
  ///
  /// @param        Impulse       The impulse to apply.
  /// @param        bVelChange    When true, the mass of the pawn will not be taken into account.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void AddImpulse(const FVector& Impulse, bool bVelChange);

  /// Adds a radial force to the pawn.
  ///
  /// @param        Origin       The location where the force originates.
  /// @param        Radius       The radius of the force.
  /// @param        Strength     The strength of the force.
  /// @param        Falloff      The falloff function to use (constant or linear).
  /// @param        DeltaTime    The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void AddRadialForce(const FVector& Origin, float Radius, float Strength, EGMC_RadialImpulseFalloff Falloff, float DeltaTime = 0.f);
  void AddRadialForce(const FVector& Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff) override;

  /// Adds a radial impulse to the pawn. An impulse should only be applied for one single frame. If you want to add a continuous radial force to the pawn over
  /// several frames use AddRadialForce.
  ///
  /// @param        Origin        The location where the impulse originates.
  /// @param        Radius        The radius of the impulse.
  /// @param        Strength      The strength of the impulse.
  /// @param        Falloff       The falloff function to use (constant or linear).
  /// @param        bVelChange    When true, the mass of the pawn will not be taken into account.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void AddRadialImpulse(const FVector& Origin, float Radius, float Strength, EGMC_RadialImpulseFalloff Falloff, bool bVelChange);
  void AddRadialImpulse(const FVector& Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange) override;

  /// Sets a new physics delta time and optionally resets start location and velocity with the current pawn state and zeroes acceleration and force.
  ///
  /// @param        NewDeltaTime    The delta time to use for any future physics calculations.
  /// @param        bReset          Whether to update the start location and start velocity, and zero acceleration and force.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetPhysDeltaTime(float NewDeltaTime, bool bReset = true);

  /// Transforms the passed raw input vector to an "AbsoluteZ" input vector. This means that the Z component of the transformed input vector remains in world
  /// space, the X and Y components will be relative to the controller view. This is the type of input you would expect for a game that lets you control a
  /// character that is walking on the ground.
  ///
  /// @param        AbsoluteInputVector    The raw input vector with world-space components.
  /// @returns      FVector                The transformed input vector.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector TransformInputVectorAbsoluteZ(const FVector& AbsoluteInputVector) const;

  /// Transforms the passed raw input vector to an "AllRelative" input vector. This means that all components of the transformed input input vector are relative
  /// to the controller view i.e. forward is whatever direction the pawn is currently looking towards. This type of input is most commonly used for
  /// three-dimensional movement, e.g. flying or swimming.
  ///
  /// @param        AbsoluteInputVector    The raw input vector with world-space components.
  /// @returns      FVector                The transformed input vector.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector TransformInputVectorAllRelative(const FVector& AbsoluteInputVector) const;

  /// Updates the velocity based on the difference between the current location of the updated component and the currently set start location.
  ///
  /// @param        DeltaTime    The time over which the change in location occurred. If 0 the current physics delta time will be used.
  /// @param        MaxSize      The max magnitude the newly calculated velocity can have. If 0 the result will not be clamped.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void UpdateVelocityFromMovedDistance(float DeltaTime = 0.f, float MaxSize = 0.f);

  /// Whether the updated component has moved at all since the last physics reset.
  ///
  /// @returns      bool    True if the current updated component location differs from the currently set start location.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasMoved() const;

  /// Immediately stops movement by zeroing all motion values.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void HaltMovement();

  /// Returns the current total speed of the pawn.
  ///
  /// @returns      float    The current speed of the pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetSpeed() const;

  /// Returns the current horizontal speed of the pawn.
  ///
  /// @returns      float    The current horizontal speed  of the pawn (XY-velocity-components).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetSpeedXY() const;

  /// Returns the current vertical speed  of the pawn.
  ///
  /// @returns      float    The current vertical speed of the pawn (Z-velocity-component).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetSpeedZ() const;

  /// Adjust the velocity according to the hit normal.
  ///
  /// @param        Hit             The hit result of the collision.
  /// @param        bUseRelative    Whether rounding should happen relative to the current actor base.
  /// @param        DeltaTime       The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void AdjustVelocityFromHit(const FHitResult& Hit, bool bUseRelative, float DeltaTime);

  /// Adjust the velocity according to the passed normal.
  ///
  /// @param        Normal          The normal to use for adjustment.
  /// @param        bUseRelative    Whether rounding should happen relative to the current actor base.
  /// @param        DeltaTime       The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void AdjustVelocityFromNormal(const FVector& Normal, bool bUseRelative, float DeltaTime);

  /// Generates the same pseudorandom positive double on server and client.
  ///
  /// @param        Max      The max value allowed for generated numbers.
  /// @returns      float    A random integer in the range [0, Max) that is safe to use for a networked game.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual double GenerateNetRandomNumber(int32 Max) const;

  /// Generates the same pseudorandom angle in degrees on server and client.
  ///
  /// @returns      float    A random angle value in the range [0, 360) that is safe to use for a networked game.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual double GenerateNetRandomAngle() const;

  /// Rotates the updated component smoothly around its local yaw axis towards the passed direction.
  ///
  /// @param        Direction    The direction which the updated component should rotate towards (does not need to be normalized).
  /// @param        Rate         The rate of rotation i.e. how fast the updated component will rotate. If <= 0.f the target rotation will be applied directly.
  /// @param        DeltaTime    The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void RotateYawTowardsDirection(const FVector& Direction, float Rate, float DeltaTime = 0.f);

  /// Rotates the root collision component smoothly around its local yaw axis towards the passed direction while trying to adjust the pawn position to avoid
  /// collisions.
  ///
  /// @param        Direction    The direction which the updated component should rotate towards (does not need to be normalized).
  /// @param        Rate         The rate of rotation i.e. how fast the updated component will rotate. If <= 0.f the target rotation will be applied directly.
  /// @param        DeltaTime    The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @returns      bool         True if the new rotation was applied, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool RotateYawTowardsDirectionSafe(const FVector& Direction, float Rate, float DeltaTime = 0.f);

  /// Compute how deeply the updated component is immersed in a fluid volume. Range is from 0 (not in fluid) to 1 (fully immersed). Always returns 0 if the
  /// current physics volume does not have the water flag set. Fluids volumes should have bPhysicsOnContact enabled, otherwise this will always return 0 if the
  /// immersion depth is smaller than 0.5 (meaning the origin of the pawn is not inside the fluid volume). Assumes that the pawn enters and leaves the fluid
  /// volume vertically (i.e. through a horizontal volume face).
  ///
  /// @returns      float    The current immersion depth.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float ComputeImmersionDepth() const;

  /// Returns the passed vector resized to the desired length.
  ///
  /// @param        Vector     The vector to resize.
  /// @param        Size       The desired length.
  /// @returns      FVector    The resized vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector ResizeVector(const FVector& Vector, double Size);

  /// Checks if the two passed vectors are pointing in opposite directions.
  ///
  /// @param        D1      The first direction.
  /// @param        D2      The second direction.
  /// @returns      bool    True if the two directions are differing by 90 degrees or more, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static bool DirectionsDiffer(const FVector& D1, const FVector& D2);

  /// Checks if the two passed vector's XY components are pointing in opposite directions.
  ///
  /// @param        D1      The first direction.
  /// @param        D2      The second direction.
  /// @returns      bool    True if the two directions are differing by 90 degrees or more in the XY plane, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static bool DirectionsDifferXY(const FVector& D1, const FVector& D2);

  /// Checks if the two passed vector's Z components are pointing in opposite directions.
  ///
  /// @param        D1      The first direction.
  /// @param        D2      The second direction.
  /// @returns      bool    True if D1 and D2 are pointing in the same Z direction (up or down), false if they are pointing in opposite directions.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static bool DirectionsDifferZ(const FVector& D1, const FVector& D2);

  /// Round a single-precision floating point number.
  ///
  /// @param        FloatToRound    The floating point number to round.
  /// @param        Level           The quantization level.
  /// @returns      float           The rounded floating point number.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static float RoundFloat(float FloatToRound, EGMC_FloatPrecisionBlueprint Level);

  /// Round a double-precision floating point number.
  ///
  /// @param        DoubleToRound    The floating point number to round.
  /// @param        Level            The quantization level.
  /// @returns      double           The rounded floating point number.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static double RoundDouble(double DoubleToRound, EGMC_FloatPrecisionBlueprint Level);

  /// Round a vector.
  ///
  /// @param        VectorToRound    The vector to round.
  /// @param        Level            The quantization level.
  /// @returns      FVector          The rounded vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector RoundVector(const FVector& VectorToRound, EGMC_FloatPrecisionBlueprint Level);

  /// Round a rotator.
  ///
  /// @param        RotatorToRound    The rotator to round.
  /// @param        Level             The quantization level.
  /// @returns      FRotator          The rounded rotator.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FRotator RoundRotator(const FRotator& RotatorToRound, EGMC_FloatPrecisionBlueprint Level);

  /// Round a world vector relative to a base.
  ///
  /// @param        WorldVectorToRound    The vector to round in world space.
  /// @param        Base                  The base the passed vector should be relative to. If null absolute coordinates will be used.
  /// @param        Level                 The quantization level.
  /// @returns      FVector               The rounded world vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector RoundBasedVector(const FVector& WorldVectorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level);

  /// Round a world vector relative to a base. Only considers the base rotation.
  ///
  /// @param        WorldVectorToRound    The vector to round in world space.
  /// @param        Base                  The base the passed vector should be relative to. If null absolute coordinates will be used.
  /// @param        Level                 The quantization level.
  /// @returns      FVector               The rounded world vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector RoundBasedVectorByRotation(const FVector& WorldVectorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level);

  /// Round a world rotator relative to a base.
  ///
  /// @param        WorldRotatorToRound    The rotator to round in world space.
  /// @param        Base                   The base the passed rotator should be relative to. If null absolute coordinates will be used.
  /// @param        Level                  The quantization level.
  /// @returns      FRotator               The rounded world rotator.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FRotator RoundBasedRotator(const FRotator& WorldRotatorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level);

  /// Round a world rotator relative to a base. Ignores roll and pitch rotation of the base.
  ///
  /// @param        WorldRotatorToRound    The rotator to round in world space.
  /// @param        Base                   The base the passed rotator should be relative to. If null absolute coordinates will be used.
  /// @param        Level                  The quantization level.
  /// @returns      FRotator               The rounded world rotator (roll and pitch components unchanged).
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FRotator RoundBasedRotatorYaw(const FRotator& WorldRotatorToRound, USceneComponent* Base, EGMC_FloatPrecisionBlueprint Level);

  /// Transforms an absolute vector into a vector relative to a base.
  ///
  /// @param        WorldVector    The vector to transform in world space.
  /// @param        Base           The base the passed vector should be made relative to. If null absolute coordinates will be used.
  /// @returns      FVector        The relative vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetBasedVector(const FVector& WorldVector, USceneComponent* Base);

  /// Transforms an absolute vector into a vector relative to a base. Only considers the base rotation.
  ///
  /// @param        WorldVector    The vector to transform in world space.
  /// @param        Base           The base the passed vector should be made relative to. If null absolute coordinates will be used.
  /// @returns      FVector        The relative vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetBasedVectorByRotation(const FVector& WorldVector, USceneComponent* Base);

  /// Transforms an absolute rotator into a rotator relative to a base.
  ///
  /// @param        WorldRotator    The rotator to transform in world space.
  /// @param        Base            The base the passed rotator should be made relative to. If null absolute coordinates will be used.
  /// @returns      FRotator        The relative rotator.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FRotator GetBasedRotator(const FRotator& WorldRotator, USceneComponent* Base);

  /// Transforms an absolute rotator into a rotator relative to a base. Ignores roll and pitch rotation of the base.
  ///
  /// @param        WorldRotator    The rotator to transform in world space.
  /// @param        Base            The base the passed rotator should be made relative to. If null absolute coordinates will be used.
  /// @returns      FRotator        The relative rotator (roll and pitch components unchanged).
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FRotator GetBasedRotatorYaw(const FRotator& WorldRotator, USceneComponent* Base);

  /// Transforms a relative vector into an absolute vector.
  ///
  /// @param        RelativeVector    The relative vector.
  /// @param        Base              The base the passed vector is relative to. If null absolute coordinates will be used.
  /// @returns      FVector           The world vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetWorldVector(const FVector& RelativeVector, USceneComponent* Base);

  /// Transforms a relative vector into an absolute vector. Only considers the base rotation.
  ///
  /// @param        RelativeVector    The relative vector.
  /// @param        Base              The base the passed vector is relative to. If null absolute coordinates will be used.
  /// @returns      FVector           The world vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetWorldVectorByRotation(const FVector& RelativeVector, USceneComponent* Base);

  /// Transforms a relative rotator into an absolute rotator.
  ///
  /// @param        RelativeRotator    The relative rotator.
  /// @param        Base               The base the passed rotator is relative to. If null absolute coordinates will be used.
  /// @returns      FRotator           The world rotator.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FRotator GetWorldRotator(const FRotator& RelativeRotator, USceneComponent* Base);

  /// Returns the passed angular velocity as normalized rotator.
  ///
  /// @param        InAngularVelocity    The angular velocity vector.
  /// @returns      FRotator             The normalized angular velocity rotator.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FRotator ToAngularVelocityRotator(const FVector& InAngularVelocity);

  /// Returns the passed angular velocity as quaternion.
  ///
  /// @param        InAngularVelocity    The angular velocity vector.
  /// @returns      FQuat                The angular velocity quaternion.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FQuat ToAngularVelocityQuat(const FVector& InAngularVelocity);

  /// Check if the pawn's root component the updated component.
  ///
  /// @returns      bool    True if the root component is the updated component, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsUpdatedComponentRootCollision() const;

  /// Check if the pawn's root component is of a supported collision shape type.
  ///
  /// @returns      bool    True if the root component is a supported collision shape, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasValidRootCollisionShape() const;

  /// Check if the pawn's root component is has a valid collision shape extent. A return value of true implies that the root component also is of a supported
  /// collision shape type.
  ///
  /// @returns      bool    True if the root component has a valid collision shape extent, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasValidRootCollisionExtent() const;

  /// Check if the pawn's root component has a valid scale (no scaling factors that are zero or nearly zero).
  ///
  /// @returns      bool    True if the root component has valid scaling factors, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasValidRootCollisionScale() const;

  /// Query the collision shape of the pawn.
  ///
  /// @returns      EGMC_CollisionShape    The shape of the pawn's root collision.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual EGMC_CollisionShape GetRootCollisionShape() const;

  /// Check if the pawn's root collision shape is a vertical capsule.
  ///
  /// @returns      EGMC_CollisionShape    True if the pawn has a vertical capsule collision, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasVerticalCapsuleCollision() const;

  /// Check if the pawn's root collision shape is a flat capsule.
  ///
  /// @returns      EGMC_CollisionShape    True if the pawn has a flat capsule collision, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasHorizontalCapsuleCollision() const;

  /// Check if the pawn's root collision shape is a box.
  ///
  /// @returns      EGMC_CollisionShape    True if the pawn has a box collision, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasBoxCollision() const;

  /// Check if the pawn's root collision shape is a sphere.
  ///
  /// @returns      EGMC_CollisionShape    True if the pawn has a sphere collision, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasSphereCollision() const;

  /// Returns the extent of the root component's FCollisionShape in the following vector format:
  /// VerticalCapsule   = (Radius, Radius, HalfHeight)
  /// HorizontalCapsule = (Radius, Radius, HalfHeight)
  /// Box               = (HalfExtentX, HalfExtentY, HalfExtentZ)
  /// Sphere            = (Radius, Radius, Radius)
  ///
  /// Keep in mind that the actual height (along the Z-axis) of an un-rotated horizontal capsule is determined by the radius, not by the value of the HalfHeight
  /// member.
  ///
  /// Although it would be more logical for flat capsules to have the radius as Z-component we don't do it this way due to the manner in which FCollisionShape
  /// handles the extent vector and for easier interaction with existing engine functions.
  ///
  /// @param        bScaled    Whether the scale of the root component should be considered.
  /// @returns      FVector    The extent of the root collision.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetRootCollisionExtent(bool bScaled) const;

  /// Returns the half height of the root component. This is the extent in Z direction of the collision's default position (not rotated), and not necessarily
  /// the same as the HalfHeight property of a capsule (e.g. for a horizontal capsule this is the radius).
  ///
  /// @param        bScaled    Whether the scale of the root component should be considered.
  /// @returns      float      The half height of the un-rotated root collision in Z direction.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetRootCollisionHalfHeight(bool bScaled) const;

  /// Returns the half height of the vertical portion of the root collision (0 for horizontal capsules and spheres).
  ///
  /// @param        bScaled    Whether the scale of the root component should be considered.
  /// @returns      float      The half height of the vertical portion of the collision.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetRootCollisionOuterHalfHeight(bool bScaled) const;

  /// Returns the width of the root component. This is the extent in the XY plane from the center in the given direction. For uniform shapes the return value is
  /// always the radius (meaning the passed direction does not matter) but for horizontal capsules and box shapes this is the distance to the boundary of the
  /// collision in the passed direction.
  ///
  /// @param        Direction    The direction for which to calculate the width (only relevant for non-uniform shapes).
  /// @returns      float        The width of the root collision in the given direction.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetRootCollisionWidth(const FVector& Direction) const;

  /// Set the extent of the root component's FCollisionShape in the following vector format:
  /// VerticalCapsule   = (Radius, Radius, HalfHeight)
  /// HorizontalCapsule = (Radius, Radius, HalfHeight)
  /// Box               = (HalfExtentX, HalfExtentY, HalfExtentZ)
  /// Sphere            = (Radius, Radius, Radius)
  ///
  /// Keep in mind that the actual height (along the Z-axis) of an un-rotated horizontal capsule is determined by the radius, not by the value of the HalfHeight
  /// member.
  ///
  /// @param        NewExtent          The new extent the root collision should have. Component values may be altered if the given vector does not constitute a
  ///                                  valid extent (GetValidExtent).
  /// @param        bScaled            Whether the scale of the root component should be considered.
  /// @param        bUpdateOverlaps    If true, updates touching array for owning actor (if the root component is registered and collides).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionExtent(const FVector& NewExtent, bool bScaled, bool bUpdateOverlaps = true);

  /// Version of SetRootCollisionExtent that only applies the change if the new extent will not cause any blocking collision.
  ///
  /// @param        NewExtent          The new extent the root collision should have. Component values may be altered if the given vector does not constitute a
  ///                                  valid extent (see GetValidExtent).
  /// @param        Tolerance          When testing whether the new extent would cause a blocking collision, this argument is applied as a scaling factor to the
  ///                                  passed extent vector to allow for a more lenient (< 1) or stricter (> 1) test.
  /// @param        bScaled            Whether the scale of the root component should be considered.
  /// @param        bUpdateOverlaps    If true, updates touching array for owning actor (if the root component is registered and collides).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionExtentSafe(const FVector& NewExtent, bool bScaled, float Tolerance = 0.99f, bool bUpdateOverlaps = true);

  /// Sets the half height of the root component. This is the extent in Z direction of the collision's default position (not rotated), and not necessarily the
  /// same as the HalfHeight property of a capsule (e.g. for a horizontal capsule this is the radius).
  ///
  /// @param        NewHalfHeight      The new half height the root collision should have. May get clamped to ensure a valid extent.
  /// @param        bScaled            Whether the scale of the root component should be considered.
  /// @param        bUpdateOverlaps    If true, updates touching array for owning actor (if the root component is registered and collides).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionHalfHeight(float NewHalfHeight, bool bScaled, bool bUpdateOverlaps = true);

  /// Version of SetRootCollisionHalfHeight that only applies the change if the new extent with the passed half height value will not cause any blocking
  /// collision.
  ///
  /// @param        NewHalfHeight      The new half height the root collision should have. May get clamped to ensure a valid extent.
  /// @param        Tolerance          When testing whether the new extent would cause a blocking collision, this argument is applied as a scaling factor to
  ///                                  allow for a more lenient (< 1) or stricter (> 1) test. Keep in mind that the scaling is applied to the whole extent
  ///                                  vector, not just the half height (i.e. Z-extent component).
  /// @param        bScaled            Whether the scale of the root component should be considered.
  /// @param        bUpdateOverlaps    If true, updates touching array for owning actor (if the root component is registered and collides).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionHalfHeightSafe(float NewHalfHeight, bool bScaled, float Tolerance = 0.99f, bool bUpdateOverlaps = true);

  /// Sets the location of the root collision.
  ///
  /// @param        NewLocation    The new location of the root collision in world space.
  /// @param        Teleport       How a change in location should affect the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionLocation(const FVector& NewLocation, ETeleportType Teleport = ETeleportType::None);

  /// Returns the location of the root collision.
  ///
  /// @returns      FVector    The current location of the root collision in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetRootCollisionLocation() const;

  /// Version of SetRootCollisionLocation that only sets the new location if it will not cause any blocking collision.
  ///
  /// @param        NewLocation    The new location of the root collision in world space.
  /// @param        Tolerance      When testing whether the new location would cause a blocking collision, this argument is applied as a scaling factor to the
  ///                              collision extent to allow for a more lenient (< 1) or stricter (> 1) test.
  /// @param        Teleport       How a change in location should affect the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionLocationSafe(
    const FVector& NewLocation,
    float Tolerance = 0.99f,
    ETeleportType Teleport = ETeleportType::None
  );

  /// Sets the rotation of the root collision.
  ///
  /// @param        NewRotation    The new rotation of the root collision.
  /// @param        Teleport       How a change in rotation should affect the physics state.
  /// @returns      void
  virtual void SetRootCollisionRotation(const FQuat& NewRotation, ETeleportType Teleport = ETeleportType::None);

  /// Sets the rotation of the root collision.
  ///
  /// @param        NewRotation    The new rotation of the root collision.
  /// @param        Teleport       How a change in rotation should affect the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionRotation(const FRotator& NewRotation, ETeleportType Teleport = ETeleportType::None);

  /// Returns the rotation of the root collision.
  ///
  /// @returns      FRotator    The current rotation of the root collision.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetRootCollisionRotation() const;

  /// Version of SetRootCollisionRotation that only sets the new rotation if it will not cause any blocking collision.
  ///
  /// @param        NewRotation    The new rotation of the root collision.
  /// @param        Tolerance      When testing whether the new rotation would cause a blocking collision, this argument is applied as a scaling factor to the
  ///                              collision extent to allow for a more lenient (< 1) or stricter (> 1) test.
  /// @param        Teleport       How a change in rotation should affect the physics state.
  /// @returns      void
  virtual void SetRootCollisionRotationSafe(
    const FQuat& NewRotation,
    float Tolerance = 0.99f,
    ETeleportType Teleport = ETeleportType::None
  );

  /// Version of SetRootCollisionRotation that only sets the new rotation if it will not cause any blocking collision.
  ///
  /// @param        NewRotation    The new rotation of the root collision.
  /// @param        Tolerance      When testing whether the new rotation would cause a blocking collision, this argument is applied as a scaling factor to the
  ///                              collision extent to allow for a more lenient (< 1) or stricter (> 1) test.
  /// @param        Teleport       How a change in rotation should affect the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetRootCollisionRotationSafe(
    const FRotator& NewRotation,
    float Tolerance = 0.99f,
    ETeleportType Teleport = ETeleportType::None
  );

  /// Generate an FCollisionShape with the passed extent from an EGMC_CollisionShape value. The extent format is as follows:
  /// VerticalCapsule   = (Radius, Radius, HalfHeight)
  /// HorizontalCapsule = (Radius, Radius, HalfHeight)
  /// Box               = (HalfExtentX, HalfExtentY, HalfExtentZ)
  /// Sphere            = (Radius, Radius, Radius)
  ///
  /// Keep in mind that FCollisionShape does not differentiate between vertical and horizontal capsules.
  ///
  /// @param        CollisionShape     The collision shape we want an FCollisionShape for.
  /// @param        Extent             The extent the FCollisionShape should have. Component values may be altered if the given vector does not constitute a
  ///                                  valid extent (GetValidExtent).
  /// @returns      FCollisionShape    The FCollisionShape equivalent to the passed EGMC_CollisionShape.
  virtual FCollisionShape GetFrom(EGMC_CollisionShape CollisionShape, const FVector& Extent) const;

  /// Adds the passed rotation to the inherent rotation of the pawn's general capsule if applicable. Useful when executing traces for a flat capsule with an
  /// FCollisionShape capsule.
  ///
  /// @param        Rotation    The rotation value to add the flat capsule rotation to.
  /// @returns      FQuat       The passed rotation value rotated by the general capsule rotation. Simply returns the original rotation if the pawn does not
  ///                           have a UGMC_CapsuleCmp as root collision shape.
  virtual FQuat AddToGMCCapsuleRotation(const FQuat& Rotation) const;

  /// Adds the passed rotation to the inherent rotation of the pawn's general capsule if applicable. Useful when executing traces for a flat capsule with an
  /// FCollisionShape capsule.
  ///
  /// @param        Rotation    The rotation value to add the flat capsule rotation to.
  /// @returns      FQuat       The passed rotation value rotated by the general capsule rotation. Simply returns the original rotation if the pawn does not
  ///                           have a GMC capsule component as root collision shape.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator AddToGMCCapsuleRotation(const FRotator& Rotation) const;

  /// Returns the passed vector as a valid extent for the passed collision shape. A valid extent vector has no zero or nearly zero components, no negative
  /// components and has the same values for components that refer to the same property (e.g. for a sphere all components have the same value representing the
  /// radius). For capsules the half height cannot be smaller than the radius. If there are different values for the same property in the passed vector the
  /// largest value will be chosen.
  ///
  /// @param        CollisionShape    The target collision type (must be a valid shape).
  /// @param        Extent            The extent to validate.
  /// @returns      FVector           A vector that is a valid extent for the passed collision shape (see GetRootCollisionExtent for the format).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetValidExtent(EGMC_CollisionShape CollisionShape, const FVector& Extent) const;

  /// Checks if the passed vector is a valid extent for the passed collision shape. Refer to GetValidExtent for information on what constitutes a valid
  /// extent vector.
  ///
  /// @param        CollisionShape    The target collision type (must be a valid shape).
  /// @param        Extent            The extent to check.
  /// @returns      bool              True if the passed vector is a valid extent for the given collision shape (see GetRootCollisionExtent for the format),
  ///                                 false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsValidExtent(EGMC_CollisionShape CollisionShape, const FVector& Extent) const;

  /// Calculates the distance from the center of the root collision to its edge for the given direction in the XY-plane.
  ///
  /// @param        Direction    The direction to check (only the XY-components will be considered).
  /// @returns      float        The distance from the center of the root collision to its boundary in the given direction.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float ComputeDistanceToRootCollisionBoundaryXY(const FVector& Direction) const;

  /// Get the maximum 2D distance from the center to the outer boundary of the root collision.
  ///
  /// @returns      float    The maximum distance to the outer boundary of the collision.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMaxDistanceToRootCollisionBoundaryXY() const;

  /// Calculates the 2D location of the impact point as percentage between the center (= 0) and the outer boundary (= 1) of the pawn's root collision.
  ///
  /// @param        ImpactPoint    The location of the impact.
  /// @returns      float          The calculated percentage. Can be > 1 if the impact point lies outside of the collision boundaries.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float ComputeDistanceRootCollisionToImpactPercentXY(const FVector& ImpactPoint) const;

  /// Checks whether the passed floor parameters are current or require updating. The floor parameters are out of date when the pawn has moved from the position
  /// where the parameters were captured.
  ///
  /// @param        Floor        The floor parameters to check.
  /// @param        Tolerance    The threshold to use when determining whether the floor is out of date.
  /// @returns      bool         True if the passed floor parameters are out of date, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsFloorDirty(const FGMC_FloorParams& Floor, double Tolerance) const;

  /// Does a shape trace based on the type of collision that the pawn has starting at the root component's location.
  ///
  /// @param        Direction           The direction in which to trace (does not need to be normalized).
  /// @param        TraceLength         The length of the trace i.e. how far to sweep.
  /// @param        Extent              The extent the trace shape should have (see GetRootCollisionExtent for the format). Uses the current extent of the root
  ///                                   collision if a zero vector is passed.
  /// @param        Rotation            The rotation of the trace shape.
  /// @param        CollisionChannel    The collision channel to use for sweeping.
  /// @returns      FHitResult          The hit result of the sweep.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component", meta = (DisplayName = "SweepRootCollisionSingleByChannel"))
  virtual FHitResult K2_SweepRootCollisionSingleByChannel_Direction(
    const FVector& Direction,
    float TraceLength,
    const FVector& Extent = FVector::ZeroVector,
    const FRotator& Rotation = FRotator::ZeroRotator,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;
  virtual FHitResult SweepRootCollisionSingleByChannel(
    const FVector& Direction,
    float TraceLength,
    const FVector& Extent = FVector::ZeroVector,
    const FQuat& Rotation = FQuat::Identity,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;

  /// Does a shape trace based on the type of collision that the pawn has.
  ///
  /// @param        TraceStart          The start point of the trace.
  /// @param        TraceEnd            The target point of the trace.
  /// @param        Extent              The extent the trace shape should have (see GetRootCollisionExtent for the format). Uses the current extent of the root
  ///                                   collision if a zero vector is passed.
  /// @param        Rotation            The rotation of the trace shape.
  /// @param        CollisionChannel    The collision channel to use for sweeping.
  /// @returns      FHitResult          The hit result of the sweep.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component", meta = (DisplayName = "SweepRootCollisionSingleByChannel"))
  virtual FHitResult K2_SweepRootCollisionSingleByChannel(
    const FVector& TraceStart,
    const FVector& TraceEnd,
    const FVector& Extent = FVector::ZeroVector,
    const FRotator& Rotation = FRotator::ZeroRotator,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;
  virtual FHitResult SweepRootCollisionSingleByChannel(
    const FVector& TraceStart,
    const FVector& TraceEnd,
    const FVector& Extent = FVector::ZeroVector,
    const FQuat& Rotation = FQuat::Identity,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;

  /// Does a multi shape trace based on the type of collision that the pawn has starting at the root component's location.
  ///
  /// @param        Direction             The direction in which to trace (does not need to be normalized).
  /// @param        TraceLength           The length of the trace i.e. how far to sweep.
  /// @param        Extent                The extent the trace shape should have (see GetRootCollisionExtent for the format). Uses the current extent of the 
  ///                                     root collision if a zero vector is passed.
  /// @param        Rotation              The rotation of the trace shape.
  /// @param        CollisionChannel      The collision channel to use for sweeping.
  /// @returns      TArray<FHitResult>    The hit results of the sweep.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component", meta = (DisplayName = "SweepRootCollisionMultiByChannel"))
  virtual TArray<FHitResult> K2_SweepRootCollisionMultiByChannel_Direction(
    const FVector& Direction,
    float TraceLength,
    const FVector& Extent = FVector::ZeroVector,
    const FRotator& Rotation = FRotator::ZeroRotator,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;
  virtual TArray<FHitResult> SweepRootCollisionMultiByChannel(
    const FVector& Direction,
    float TraceLength,
    const FVector& Extent = FVector::ZeroVector,
    const FQuat& Rotation = FQuat::Identity,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;

  /// Does a multi shape trace based on the type of collision that the pawn has.
  ///
  /// @param        TraceStart            The start point of the trace.
  /// @param        TraceEnd              The target point of the trace.
  /// @param        Extent                The extent the trace shape should have (see GetRootCollisionExtent for the format). Uses the current extent of the
  ///                                     root collision if a zero vector is passed.
  /// @param        Rotation              The rotation of the trace shape.
  /// @param        CollisionChannel      The collision channel to use for sweeping.
  /// @returns      TArray<FHitResult>    The hit result of the sweep.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component", meta = (DisplayName = "SweepRootCollisionMultiByChannel"))
  virtual TArray<FHitResult> K2_SweepRootCollisionMultiByChannel(
    const FVector& TraceStart,
    const FVector& TraceEnd,
    const FVector& Extent = FVector::ZeroVector,
    const FRotator& Rotation = FRotator::ZeroRotator,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;
  virtual TArray<FHitResult> SweepRootCollisionMultiByChannel(
    const FVector& TraceStart,
    const FVector& TraceEnd,
    const FVector& Extent = FVector::ZeroVector,
    const FQuat& Rotation = FQuat::Identity,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;

  /// Executes multiple vertically aligned line traces from the bottom of the pawn's root collision up to the passed height.
  ///
  /// @param        Direction             The direction in which to trace (does not need to be normalized).
  /// @param        TraceLength           The length of the line traces.
  /// @param        MaxHeight             Only apply up to this height, starting from the lowest point of the root collision.
  /// @param        Resolution            How many traces to apply within the space between the bottom of the root collision and the passed height.
  /// @param        CollisionChannel      The collision channel to use for tracing.
  /// @returns      TArray<FHitResult>    The hit results of all traces with index 0 containing the hit result of the lowest line trace and
  ///                                     the last index containing the hit result of the highest line trace.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual TArray<FHitResult> LineTracesAlignedVertical(
    const FVector& Direction,
    float TraceLength,
    float MaxHeight,
    int32 Resolution,
    ECollisionChannel CollisionChannel = ECC_Pawn
  ) const;

  /// Tries to get the pawn unstuck when in penetration.
  ///
  /// @returns      FHitResult    The hit result of the most recent sweep.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FHitResult AutoResolvePenetration();

  /// Tries to resolve the penetration with a rollback actor by considering its velocity.
  ///
  /// @param        RollbackActor    The rollback actor that is currently encroaching on the pawn.
  /// @param        OutHit           The hit result of the adjustment.
  /// @returns      bool             True if the penetration could be resolved, false otherwise.
  virtual bool ResolvePenetrationWithRollbackActor(class AGMC_RollbackActor* RollbackActor, FHitResult& OutHit);

protected:

  /// Override to handle penetrations with rollback actors in a special way.
  ///
  /// @param        Adjustment     The requested adjustment, usually obtained from GetPenetrationAdjustment.
  /// @param        Hit            The hit result of the failed move.
  /// @param        NewRotation    The pawn rotation to set when moving.
  /// @returns      bool           True if the adjustment was successful and the original move should be retried, false otherwise.
  bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;

public:

  /// Overridden to produce better results across client and server by rounding the hit normal.
  ///
  /// @param        Hit        The hit result of the failed move.
  /// @returns      FVector    The adjustment to use after the failed move (zero vector if no attempt should be made).
  FVector GetPenetrationAdjustment(const FHitResult& Hit) const override;

  /// Called when the pawn is stuck in geometry.
  ///
  /// @param        OtherComponent    The component the pawn is stuck in.
  /// @param        Hit               The hit result of the failed move.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnStuckInGeometry(UPrimitiveComponent* OtherComponent, const FHitResult& Hit);
  virtual void OnStuckInGeometry_Implementation(UPrimitiveComponent* OtherComponent, const FHitResult& Hit);

  /// Checks if the passed component is movable.
  ///
  /// @param        Component    The component to check.
  /// @returns      bool         True if the passed component has the EComponentMobility::Movable flag set, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static bool IsMovable(UPrimitiveComponent* Component);

  /// Tries to retrieve the linear velocity of the passed component.
  ///
  /// @param        Component    The component to check.
  /// @returns      FVector      The linear velocity of the component.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetLinearComponentVelocity(UPrimitiveComponent* Component);

  /// Tries to retrieve the angular velocity of the passed component in radians per second.
  ///
  /// @param        Component    The component to check.
  /// @returns      FVector      The angular velocity of the component in rad/sec.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetAngularComponentVelocityInRadians(UPrimitiveComponent* Component);

  /// Tries to retrieve the angular velocity of the passed component in degrees per second.
  ///
  /// @param        Component    The component to check.
  /// @returns      FVector      The angular velocity of the component in deg/sec.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetAngularComponentVelocityInDegrees(UPrimitiveComponent* Component);

  /// Computes the tangential velocity at the passed location inflicted by the passed component.
  ///
  /// @param        WorldLocation    The location in world space that the tangential velocity should be calculated for.
  /// @param        Component        The component that exerts the force.
  /// @returns      FVector          The tangential velocity at the passed world location.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector ComputeTangentialVelocity(const FVector& WorldLocation, UPrimitiveComponent* Component);

  /// Calculates the combined velocity that would currently be imparted on the pawn from the passed component.
  ///
  /// @param        Component                 The component to check.
  /// @param        bGetLinearVelocity        Whether to consider the linear velocity of the passed component.
  /// @param        bGetTangentialVelocity    Whether to consider the angular velocity of the passed component.
  /// @returns      FVector                   The combined velocity to impart from the passed component.
  virtual FVector GetImpartVelocity(UPrimitiveComponent* Component, bool bGetLinearVelocity, bool bGetTangentialVelocity);

  /// Returns the position of the lowest point of the updated component.
  ///
  /// @returns      FVector    The lower bound of the updated component.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetLowerBound() const;

  /// Calculates the opposing force to the passed velocity caused by air resistance.
  ///
  /// @param        CurrentVelocity    The current velocity in m/s.
  /// @param        DragCoefficient    The drag coefficient.
  /// @returns      FVector            The force opposing the current velocity in N.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector CalculateAirResistance(const FVector& CurrentVelocity, float DragCoefficient);

  /// Calculates the opposing force to the passed velocity cause by rolling resistance.
  ///
  /// @param        CurrentVelocity       The current velocity in m/s.
  /// @param        Mass                  The mass of the pawn in kg.
  /// @param        GravityZ              The gravity Z-component in m/s^2.
  /// @param        RollingCoefficient    The rolling resistance coefficient.
  /// @returns      FVector               The force opposing the current velocity in N.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector CalculateRollingResistance(const FVector& CurrentVelocity, float PawnMass, float GravityZ, float RollingCoefficient);

  /// Returns the normal of the plane defined by the passed direction and the world up-vector.
  ///
  /// @param        Direction    The direction (does not need to be normalized).
  /// @returns      FVector      The cross product of the direction and the world up-vector.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static FVector GetPlaneNormalWithWorldZ(const FVector& Direction);

  /// Does a shape trace of the current root collision to update the floor parameters.
  ///
  /// @param        Floor               The floor parameters to update.
  /// @param        Direction           The direction in which to trace.
  /// @param        TraceLength         The length of the trace.
  /// @param        Tolerance           The tolerance to use for checking if the floor data is dirty.
  /// @param        ShapeExtentScale    Scaling factor applied to the root collision extent for the shape sweep.
  /// @param        bAutoAdjust         Whether the pawn position should be adjusted if the floor update fails due to initial penetrations.
  /// @param        bForceUpdate        If true, the floor will be updated even when the floor data is not dirty.
  /// @returns      bool                True if the floor was updated, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool UpdateFloor(
    UPARAM(Ref) FGMC_FloorParams& Floor,
    const FVector& Direction,
    float TraceLength,
    float Tolerance,
    float ShapeExtentScale,
    bool bAutoAdjust,
    bool bForceUpdate
  );

  /// Tries to adjust the root collision location for hits that started in penetration.
  ///
  /// @param        Hit              The hit that started in penetration.
  /// @param        MaxAdjustment    How far the root collision should be moved at most.
  /// @returns      bool             True if the adjustment was successful, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool ResolveRootCollisionPenetration(const FHitResult& Hit, float MaxAdjustment = 10.f);

  /// Checks if a given point is closer to the collision center than the tolerance allows. Usually used to discard hits that are very close to the edge of the
  /// vertical portion of the collision shape.
  ///
  /// @param        Location       The location of the root collision.
  /// @param        Point          The point to test.
  /// @param        Tolerance      The edge tolerance.
  /// @returns      bool           True if the point is further towards the center of the collision than the tolerance allows.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsWithinEdgeTolerance(const FVector& Location, const FVector& Point, float Tolerance) const;

  /// Linearly interpolates the current half height of the root collision towards the target value. The half height is the Z-extent of the unrotated collision
  /// shape so for flat capsules and spheres the height is the radius property of the class. The new half height is set through SetRootCollisionExtentSafe
  /// meaning no changes will be applied if the new extent would cause a blocking collision (in accordance with the passed interp tolerance). Does not consider
  /// the scale of the root component.
  ///
  /// @param        TargetHalfHeight    The target value for the interpolation. Must be greater than 0.
  /// @param        InterpSpeed         How quickly the target value should be reached.
  /// @param        InterpTolerance     Factor the test extent is scaled with (see SetRootCollisionExtentSafe).
  /// @param        DeltaTime           The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @param        bAdjustPosition     If true, the updated component will be moved up/down (depending on the value of AdjustDirection) by the amount the half
  ///                                   height has changed.
  /// @param        AdjustDirection     Whether the updated component should move up or down. Only relevant if bAdjustPosition is true.
  /// @returns      float               The absolute difference in half height that was actually applied to the root collision shape.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float LerpRootCollisionHalfHeight(
    float TargetHalfHeight,
    float InterpSpeed,
    float InterpTolerance = 0.99f,
    float DeltaTime = 0.f,
    bool bAdjustPosition = false,
    EGMC_AdjustDirection AdjustDirection = EGMC_AdjustDirection::Up
  );

  /// Linearly interpolates the current width of the root collision towards the target value. For vertical capsules and spheres the width is the radius, for
  /// flat capsules it is the half height property of the class. For box collisions the value operated on is the X-extent if bInterpBoxY is false, otherwise it
  /// is the Y-extent. The new width is set through SetRootCollisionExtentSafe meaning no changes will be applied if the new extent would cause a blocking
  /// collision (in accordance with the passed interp tolerance). Does not consider the scale of the root component.
  ///
  /// @param        TargetWidth        The target value for the interpolation. Must be greater than 0.
  /// @param        InterpSpeed        How quickly the target value should be reached.
  /// @param        InterpTolerance    Factor the test extent is scaled with (see SetRootCollisionExtentSafe).
  /// @param        DeltaTime          The delta time to use. If <= 0 the currently set physics delta time will be used.
  /// @param        bInterpBoxY        Only relevant if the root collision is a box shape. If true, the Y-extent of the box collision will be interpolated
  ///                                  instead of the X-extent.
  /// @returns      float              The absolute difference in width that was actually applied to the root collision shape.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float LerpRootCollisionWidth(
    float TargetWidth,
    float InterpSpeed,
    float InterpTolerance = 0.99f,
    float DeltaTime = 0.f,
    bool bInterpBoxY = false
  );

  /// Replaces the current root collision of the pawn with a new one of the passed shape and extent. The current component settings (for collision, physics,
  /// rendering etc.) will be adopted and the original root component will be destroyed. Only updates the extent if the root component already has the passed
  /// collision shape.
  ///
  /// @param        NewCollisionShape    The new collision type to use (must be a valid shape).
  /// @param        Extent               The extent of the new root component (see GetRootCollisionExtent for the format). Component values may be altered if
  ///                                    the given vector does not constitute a valid extent (see GetValidExtent).
  /// @param        bScaled              Whether the scale of the root component should be considered.
  /// @param        Name                 The name of the new root component (must be unique). The name will not be updated if the root component already has the
  ///                                    passed collision shape.
  /// @returns      USceneComponent*     The new root component. May be equal to the old root component if the pawn already had the passed collision shape.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual USceneComponent* SetRootCollisionShape(EGMC_CollisionShape NewCollisionShape, const FVector& Extent, bool bScaled, FName Name);

  /// Version of SetRootCollisionShape that only applies the change if the new shape will not cause any blocking collision.
  ///
  /// @param        NewCollisionShape    The new collision type to use (must be a valid shape).
  /// @param        Extent               The extent of the new root component (see GetRootCollisionExtent for the format). Component values may be altered if
  ///                                    the given vector does not constitute a valid extent (see GetValidExtent).
  /// @param        bScaled              Whether the scale of the root component should be considered.
  /// @param        Name                 The name of the new root component (must be unique). The name will not be updated if the root component already has the
  ///                                    passed collision shape.
  /// @param        Tolerance            When testing whether the new shape would cause a blocking collision, this argument is applied as a scaling factor to
  ///                                    the extent to allow for a more lenient (< 1) or stricter (> 1) test.
  /// @returns      USceneComponent*     The new root component. May be equal to the old root component if the pawn already had the passed collision shape.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual USceneComponent* SetRootCollisionShapeSafe(
    EGMC_CollisionShape NewCollisionShape,
    const FVector& Extent,
    bool bScaled,
    FName Name,
    float Tolerance = 0.99f
  );

  /// Tests whether the passed location would be a valid (collision-free) spot for the passed shape.
  ///
  /// @param        CollisionShape      The collision shape to test.
  /// @param        Extent              The extent of the collision to test. Must be a valid extent vector as defined by IsValidExtent.
  /// @param        Location            The test location.
  /// @param        Rotation            The test rotation.
  /// @param        OutHit              The hit result of the test.
  /// @param        CollisionChannel    The test collision channel.
  /// @param        Tolerance           Scale the passed extent by this factor to allow for a more lenient (< 1) or stricter (> 1) test.
  /// @returns      bool                True if the location is a valid spot for the test shape with the given rotation and extent. Always returns false if an
  ///                                   invalid extent vector was passed. Always returns true if the passed tolerance was <= 0 and the passed extent was valid.
  virtual bool IsValidPosition(
    EGMC_CollisionShape CollisionShape,
    const FVector& Extent,
    const FVector& Location,
    const FQuat& Rotation,
    FHitResult& OutHit,
    ECollisionChannel CollisionChannel = ECC_Pawn,
    float Tolerance = 1.f
  ) const;

  /// Tests whether the passed location would be a valid (collision-free) spot for the passed shape.
  ///
  /// @param        CollisionShape      The collision shape to test.
  /// @param        Extent              The extent of the collision to test. Must be a valid extent vector as defined by IsValidExtent.
  /// @param        Location            The test location.
  /// @param        Rotation            The test rotation.
  /// @param        OutHit              The hit result of the test.
  /// @param        CollisionChannel    The test collision channel.
  /// @param        Tolerance           Scale the passed extent by this factor to allow for a more lenient (< 1) or stricter (> 1) test.
  /// @returns      bool                True if the location is a valid spot for the test shape with the given rotation and extent. Always returns false if an
  ///                                   invalid extent vector was passed. Always returns true if the passed tolerance was <= 0 and the passed extent was valid.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsValidPosition(
    EGMC_CollisionShape CollisionShape,
    const FVector& Extent,
    const FVector& Location,
    const FRotator& Rotation,
    FHitResult& OutHit,
    ECollisionChannel CollisionChannel = ECC_Pawn,
    float Tolerance = 1.f
  ) const;

  /// Determines whether the updated component can be moved through movement physics.
  ///
  /// @returns      bool    True if the updated component can be moved, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool CanMove() const;
  virtual bool CanMove_Implementation() const;

  /// Check if the pawn can receive directional movement input.
  ///
  /// @returns      bool    True if this pawn receives move input, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasMoveInputEnabled() const;

  /// Check if the hit object is a valid base for a pawn to stand on.
  ///
  /// @param        Hit     The hit result to check.
  /// @returns      bool    True if the hit object is a valid base for a pawn, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool CanStepUp(const FHitResult& Hit) const;

  /// Used by SetRootCollisionShape. Copies the settings of the source component to the target component. Can be overridden for custom behaviour.
  ///
  /// @param        Source    The component to copy the settings from.
  /// @param        Target    The component to copy the settings to.
  /// @returns      void
  virtual void CopyComponentSettings(UShapeComponent* Source, UShapeComponent* Target);

  /// Returns the default blend in time of the passed montage.
  ///
  /// @param        Montage    The montage to get the blend in time for.
  /// @returns      float      The default blend in time of the montage.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMontageBlendInTime(UAnimMontage* Montage) const;

  /// Returns the default blend out time of the passed montage.
  ///
  /// @param        Montage    The montage to get the blend out time for.
  /// @returns      float      The default blend out time of the montage.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMontageBlendOutTime(UAnimMontage* Montage) const;

  /// Checks whether we are current blending in/out at the position of the passed montage instance.
  ///
  /// @param        MontageInstance     The montage instance to check (with extra data calculated).
  /// @param        OutIsBlendingIn     Whether we are blending in at the passed position for the passed montage.
  /// @param        OutIsBlendingOut    Whether we are blending out at the passed position for the passed montage.
  /// @returns      void
  virtual void IsBlendingAtPosition(const FGMC_AnimMontageInstance& MontageInstance, bool& bOutIsBlendingIn, bool& bOutIsBlendingOut) const;

  /// Returns the montage instance associated with the passed montage if it is currently active in the anim instance. Includes montages that are blending out.
  ///
  /// @param        Mesh                     The skeletal mesh of the owning pawn.
  /// @param        Montage                  The montage to get the montage instance for.
  /// @returns      FAnimMontageInstance*    The montage instance for the passed montage if it exists, nullptr otherwise.
  virtual FAnimMontageInstance* GetMontageInstance(USkeletalMeshComponent* Mesh, UAnimMontage* Montage) const;

  /// Handles basic checks and setup logic for playing montages.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        MontageToPlay         The montage to play.
  /// @param        StartPosition         The position to start the montage at.
  /// @param        PlayRate              The rate to play the montage at.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @param        ReturnValueType       Whether to return the absolute length of the montage or the effective duration (i.e. length / play rate).
  /// @param        OutReturnValue        The return value for the calling function (only != 0 if bOutSimulated is true).
  /// @param        bOutSimulated         True if the montage is activated during a client replay or the move execution of a smoothed listen server pawn.
  /// @returns      bool                  True if the montage should be played visually, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual bool PrePlayMontage_NonBlocking(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    float StartPosition,
    float PlayRate,
    bool bWhenExtrapolating,
    EMontagePlayReturnType ReturnValueType,
    float& OutReturnValue,
    bool& bOutSimulated
  ) const;

  /// Blocks or forwards montage play requests according to the passed arguments.
  ///
  /// @param        MontageTracker       The montage tracker associated with any active montages.
  /// @param        MontageToPlay        The montage to play.
  /// @param        bInterruptOther      Whether to always interrupt another (but not the same) montage that may already be playing.
  /// @param        bCancelOnBlendIn     Whether to cancel any (another or the same) montage if it is currently blending in (default blend in time).
  /// @param        bCancelOnBlendOut    Whether to cancel any (another or the same) montage if it is currently blending out (default blend out time).
  /// @returns      bool                 True if the request should be forwarded to PrePlayMontage_NonBlocking, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual bool PrePlayMontage_Blocking(
    const FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    bool bInterruptOther,
    bool bCancelOnBlendIn,
    bool bCancelOnBlendOut
  ) const;

  /// Plays the passed montage. This function is non-blocking, additional calls will not be ignored.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        MontageToPlay         The montage to play.
  /// @param        StartPosition         The position to start the montage at.
  /// @param        PlayRate              The rate to play the montage at.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @param        ReturnValueType       Whether to return the absolute length of the montage or the effective duration (i.e. length / play rate).
  /// @returns      float                 The length or duration of the montage (based on the passed ReturnValueType argument), or 0 if failed to play.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual float PlayMontage_NonBlocking(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    bool bWhenExtrapolating = false,
    EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength
  ) const;

  /// Plays the passed montage. This function is blocking, additional calls may be ignored.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        MontageToPlay         The montage to play.
  /// @param        StartPosition         The position to start the montage at.
  /// @param        PlayRate              The rate to play the montage at.
  /// @param        bInterruptOther       Whether to always interrupt another (but not the same) montage that may already be playing.
  /// @param        bCancelOnBlendIn      Whether to cancel any (another or the same) montage if it is currently blending in (default blend in time).
  /// @param        bCancelOnBlendOut     Whether to cancel any (another or the same) montage if it is currently blending out (default blend out time).
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @param        ReturnValueType       Whether to return the absolute length of the montage or the effective duration (i.e. length / play rate).
  /// @returns      float                 The length or duration of the montage (based on the passed ReturnValueType argument), or 0 if failed to play.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual float PlayMontage_Blocking(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    bool bInterruptOther = true,
    bool bCancelOnBlendIn = false,
    bool bCancelOnBlendOut = true,
    bool bWhenExtrapolating = false,
    EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength
  ) const;

  /// Plays the passed montage. This function is non-blocking, additional calls will not be ignored.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        MontageToPlay         The montage to play.
  /// @param        BlendIn               The blend in to use.
  /// @param        StartPosition         The position to start the montage at.
  /// @param        PlayRate              The rate to play the montage at.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @param        ReturnValueType       Whether to return the absolute length of the montage or the effective duration (i.e. length / play rate).
  /// @returns      float                 The length or duration of the montage (based on the passed ReturnValueType argument), or 0 if failed to play.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual float PlayMontageWithBlendIn_NonBlocking(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    const FAlphaBlendArgs& BlendIn,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    bool bWhenExtrapolating = false,
    EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength
  ) const;

  /// Plays the passed montage. This function is blocking, additional calls may be ignored.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        MontageToPlay         The montage to play.
  /// @param        BlendIn               The blend in to use.
  /// @param        StartPosition         The position to start the montage at.
  /// @param        PlayRate              The rate to play the montage at.
  /// @param        bInterruptOther       Whether to always interrupt another (but not the same) montage that may already be playing.
  /// @param        bCancelOnBlendIn      Whether to cancel any (another or the same) montage if it is currently blending in (default blend in time).
  /// @param        bCancelOnBlendOut     Whether to cancel any (another or the same) montage if it is currently blending out (default blend out time).
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @param        ReturnValueType       Whether to return the absolute length of the montage or the effective duration (i.e. length / play rate).
  /// @returns      float                 The length or duration of the montage (based on the passed ReturnValueType argument), or 0 if failed to play.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual float PlayMontageWithBlendIn_Blocking(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    const FAlphaBlendArgs& BlendIn,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    bool bInterruptOther = true,
    bool bCancelOnBlendIn = false,
    bool bCancelOnBlendOut = true,
    bool bWhenExtrapolating = false,
    EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength
  ) const;

  /// Plays the passed montage. This function is non-blocking, additional calls will not be ignored.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        MontageToPlay         The montage to play.
  /// @param        BlendInSettings       The blend in settings to use.
  /// @param        StartPosition         The position to start the montage at.
  /// @param        PlayRate              The rate to play the montage at.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @param        ReturnValueType       Whether to return the absolute length of the montage or the effective duration (i.e. length / play rate).
  /// @returns      float                 The length or duration of the montage (based on the passed ReturnValueType argument), or 0 if failed to play.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual float PlayMontageWithBlendSettings_NonBlocking(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    const FMontageBlendSettings& BlendInSettings,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    bool bWhenExtrapolating = false,
    EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength
  ) const;

  /// Plays the passed montage. This function is blocking, additional calls may be ignored.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        MontageToPlay         The montage to play.
  /// @param        BlendInSettings       The blend in settings to use.
  /// @param        StartPosition         The position to start the montage at.
  /// @param        PlayRate              The rate to play the montage at.
  /// @param        bInterruptOther       Whether to always interrupt another (but not the same) montage that may already be playing.
  /// @param        bCancelOnBlendIn      Whether to cancel any (another or the same) montage if it is currently blending in (default blend in time).
  /// @param        bCancelOnBlendOut     Whether to cancel any (another or the same) montage if it is currently blending out (default blend out time).
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @param        ReturnValueType       Whether to return the absolute length of the montage or the effective duration (i.e. length / play rate).
  /// @returns      float                 The length or duration of the montage (based on the passed ReturnValueType argument), or 0 if failed to play.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual float PlayMontageWithBlendSettings_Blocking(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* MontageToPlay,
    const FMontageBlendSettings& BlendInSettings,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    bool bInterruptOther = true,
    bool bCancelOnBlendIn = false,
    bool bCancelOnBlendOut = true,
    bool bWhenExtrapolating = false,
    EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength
  ) const;

  /// Stops the currently active montage.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        BlendOutTime          The time over which to blend out.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void StopMontage(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    float BlendOutTime,
    bool bWhenExtrapolating = false
  ) const;

  /// Stops the currently active montage with values from the passed AlphaBlendArgs (other settings will come from the montage asset).
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        BlendOut              The blend out to use.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void StopMontageWithBlendOut(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    const FAlphaBlendArgs& BlendOut,
    bool bWhenExtrapolating = false
  ) const;

  /// Stops the currently active montage with fully custom blend out settings.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        BlendOutSettings      The blend out to use.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void StopMontageWithBlendSettings(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    const FMontageBlendSettings& BlendOutSettings,
    bool bWhenExtrapolating = false
  ) const;

  /// Pauses the currently active montage.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void PauseMontage(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    bool bWhenExtrapolating = false
  ) const;

  /// Resumes the currently active montage after pausing.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void ResumeMontage(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    bool bWhenExtrapolating = false
  ) const;

  /// Makes the currently active montage jump to the passed section.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        SectionName           The section to jump to.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void JumpToMontageSection(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    FName SectionName,
    bool bWhenExtrapolating = false
  ) const;

  /// Makes the currently active montage jump to the end of the passed section.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        SectionName           The section for which to jump to the end of.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void JumpToMontageSectionsEnd(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    FName SectionName,
    bool bWhenExtrapolating = false
  ) const;

  /// Returns the name of the current section of the currently active montage.
  ///
  /// @param        MontageTracker              The montage tracker associated with active montages.
  /// @param        OutPositionWithinSection    The position we are at relative to the current montage section.
  /// @returns      FName                       The name of the current montage section, or "None" if not valid.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FName GetCurrentMontageSection(const FGMC_MontageTracker& MontageTracker, float& OutPositionWithinSection) const;

  /// Returns the play rate of the currently active montage.
  ///
  /// @param        Mesh              The skeletal mesh of the owning pawn.
  /// @param        MontageTracker    The montage tracker associated with active montages.
  /// @returns      float             The play rate of the root motion montage, or 0 if not valid (but 0 can also be the actual play rate value).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMontagePlayRate(USkeletalMeshComponent* Mesh, const FGMC_MontageTracker& MontageTracker) const;

  /// Sets the play rate of the currently active montage.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        NewPlayRate           The new play rate to use.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void SetMontagePlayRate(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    float NewPlayRate = 1.f,
    bool bWhenExtrapolating = false
  ) const;

  /// Returns the position of the currently active montage.
  ///
  /// @param        Mesh              The skeletal mesh of the owning pawn.
  /// @param        MontageTracker    The montage tracker associated with active montages.
  /// @returns      float             The current position of the montage, or -1 if not valid.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMontagePosition(USkeletalMeshComponent* Mesh, const FGMC_MontageTracker& MontageTracker) const;

  /// Sets the position of the currently active montage.
  ///
  /// @param        Mesh                  The skeletal mesh of the owning pawn.
  /// @param        MontageTracker        The montage tracker associated with active montages.
  /// @param        NewPosition           The new position to set the montage to. Must be >= 0 and < montage play length.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void SetMontagePosition(
    USkeletalMeshComponent* Mesh,
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    float NewPosition,
    bool bWhenExtrapolating = false
  ) const;

  /// Sets the parameters for the passed montage in the passed montage tracker. Does not affect animations.
  ///
  /// @param        MontageTracker        The montage tracker to set.
  /// @param        Montage               The montage to set as active.
  /// @param        Position              The montage position.
  /// @param        PlayRate              The montage play rate.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      bool                  True if the passed were successfully applied to the montage tracker, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual bool SetActiveMontage(
    UPARAM(Ref) FGMC_MontageTracker& MontageTracker,
    UAnimMontage* Montage,
    float Position = 0.f,
    float PlayRate = 1.f,
    bool bWhenExtrapolating = false
  ) const;

  /// Clears the currently set parameters from the passed montage tracker. Does not affect animations.
  ///
  /// @param        MontageTracker        The montage tracker to clear.
  /// @param        bWhenExtrapolating    Whether to also execute the operation when extrapolating. Only relevant for simulated pawns.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void ClearActiveMontage(UPARAM(Ref) FGMC_MontageTracker& MontageTracker, bool bWhenExtrapolating = false) const;

  /// Returns the currently active montage from the passed montage tracker.
  ///
  /// @param        MontageTracker    The montage tracker associated with active montages.
  /// @returns      UAnimMontage*     The currently active montage from the passed montage tracker if any, nullptr otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual UAnimMontage* GetActiveMontage(const FGMC_MontageTracker& MontageTracker) const;

  /// Returns the currently active montage from the passed montage tracker if it doesn't have root motion.
  ///
  /// @param        MontageTracker    The montage tracker associated with active montages.
  /// @returns      UAnimMontage*     The currently active montage without root motion from the passed montage tracker if any, nullptr otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual UAnimMontage* GetActiveNonRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const;

  /// Returns the currently active montage from the passed montage tracker if it has root motion.
  ///
  /// @param        MontageTracker    The montage tracker associated with active montages.
  /// @returns      UAnimMontage*     The currently active root motion montage from the passed montage tracker if any, nullptr otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual UAnimMontage* GetActiveRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const;

  /// Whether the passed montage tracker currently has an active montage set.
  ///
  /// @param        MontageTracker    The montage tracker to check.
  /// @returns      bool              True if the passed montage tracker currently has an active montage set, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasActiveMontage(const FGMC_MontageTracker& MontageTracker) const;

  /// Whether the passed montage tracker currently has an active montage with root motion set.
  ///
  /// @param        MontageTracker    The montage tracker to check.
  /// @returns      bool              True if the passed montage tracker currently has an active montage with root motion set, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasActiveRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const;

  /// Whether the passed montage tracker currently has an active montage without root motion set.
  ///
  /// @param        MontageTracker    The montage tracker to check.
  /// @returns      bool              True if the passed montage tracker currently has an active montage without root motion set, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasActiveNonRootMotionMontage(const FGMC_MontageTracker& MontageTracker) const;

  /// Pause a path following request.
  ///
  /// @param        RequestID        The request to pause.
  /// @param        bKeepVelocity    Whether to keep the current velocity of the pawn or reset it (see StopActiveMovement).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void PausePathFollowing(FAIRequestID RequestID, bool bKeepVelocity);

  /// Resume a path following request after it has been paused.
  ///
  /// @param        RequestID    The request to resume.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ResumePathFollowing(FAIRequestID RequestID);

  /// Returns the current path following request ID.
  ///
  /// @returns      FAIRequestID    The current path following request ID.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FAIRequestID GetCurrentPathFollowingRequest() const;

  /// Checks if the pawn is currently following any path locally.
  ///
  /// @returns      bool    Whether this pawn currently has a valid path to follow locally.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsPathFollowing() const;

  /// Set whether path following should use acceleration input.
  ///
  /// @param        bEnable    Whether acceleration for path following should be enabled.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void EnableAccelerationForPathFollowing(bool bEnable);

  /// Returns whether path following is using acceleration input.
  ///
  /// @returns      bool    Whether acceleration for path following is currently enabled.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsUsingAccelerationForPathFollowing() const;

  /// Issues a path following request to the target location. Does not require the pawn to have a controller.
  ///
  /// @param        TargetLocation    The location to move to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void SimpleMoveToLocation_GMC(const FVector& TargetLocation) const;

  /// Issues a path following request to the current location of the target actor. Does not require the pawn to have a controller.
  ///
  /// @param        TargetActor    The actor to move to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  virtual void SimpleMoveToActor_GMC(AActor* TargetActor) const;

  /// Returns the agent radius. If the configured value on the movement component is <= 0, the value from the config will be retrieved. If that is also <= 0,
  /// the root collision radius will be returned.
  ///
  /// @param        NavigationSystem    Reference to the navigation system.
  /// @returns      float               The agent radius.
  virtual float GetAgentRadius(const class UNavigationSystemV1* const NavigationSystem) const;

  /// Returns the agent's nav walking search height scale. If the configured value on the movement component is <= 0, the value from the config will be
  /// retrieved. If that is also <= 0, 1 will be returned.
  ///
  /// @param        NavigationSystem    Reference to the navigation system.
  /// @returns      float               The agent's nav walking search height scale.
  virtual float GetNavWalkingSearchHeightScale(const UNavigationSystemV1* const NavigationSystem) const;

  /// Returns the total agent height. If the configured value on the movement component is <= 0, the value from the config will be retrieved. If that is also
  /// <= 0, the root collision height will be returned.
  ///
  /// @param        NavigationSystem    Reference to the navigation system.
  /// @returns      float               The total agent height (not scaled by the agent's nav walking search height scale).
  virtual float GetAgentHeight(const UNavigationSystemV1* const NavigationSystem) const;

  /// Returns the agent step height. If the configured value on the movement component is < 0, the value from the config will be retrieved. If that is also < 0,
  /// -1 will be returned.
  ///
  /// @param        NavigationSystem    Reference to the navigation system.
  /// @returns      float               The agent step height.
  virtual float GetAgentStepHeight(const UNavigationSystemV1* const NavigationSystem) const;

  /// Stops movement by zeroing velocity.
  ///
  /// @returns      void
  void StopActiveMovement() override;

  /// Filters the current context according to the passed flags.
  ///
  /// @param        bServerMove          Whether to return true during move execution of a server pawn (remotely or locally controlled).
  /// @param        bClientMove          Whether to return true during move execution of the autonomous proxy (implies that prediction is enabled).
  /// @param        bClientReplay        Whether to return true during client replays of the autonomous proxy (implies that prediction is enabled).
  /// @param        bSimulationInterp    Whether to return true during smoothing when interpolating.
  /// @param        bSimulationExtrap    Whether to return true during smoothing when extrapolating.
  /// @returns      bool                 True if one of the passed context flags currently applies, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool FilterContext(bool bServerMove, bool bClientMove, bool bClientReplay, bool bSimulationInterp, bool bSimulationExtrap);

  /// Filter function to be used for overlap events (will prevent repeated overlaps in a networked context).
  ///
  /// @param        bServerMove          Whether to return true during move execution of a server pawn (remotely or locally controlled).
  /// @param        bClientMove          Whether to return true during move execution of the autonomous proxy (implies that prediction is enabled). If this
  ///                                    applies CL_DoNotCombineNextMove will be called to prevent another overlap on the next move execution.
  /// @param        bClientReplay        Whether to return true during client replays of the autonomous proxy (implies that prediction is enabled).
  /// @param        bSimulationInterp    Whether to return true during smoothing when interpolating.
  /// @param        bSimulationExtrap    Whether to return true during smoothing when extrapolating.
  /// @param        OtherComponent       The overlapping component. Only overlaps of the pawn's root component will be filtered.
  /// @param        Aux                  Helper struct required to ensure correct behaviour (do not modify). The same struct must be passed for the begin and
  ///                                    end overlap events.
  /// @param        Type                 The type of overlap that is being filtered.
  /// @returns      bool                 True if one of the passed context flags currently applies, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool FilterOverlap(
    bool bServerMove,
    bool bClientMove,
    bool bClientReplay,
    bool bSimulationInterp,
    bool bSimulationExtrap,
    const UPrimitiveComponent* OtherComponent,
    UPARAM(Ref) FGMC_FilterOverlapAux& Aux,
    EGMC_OverlapType Type
  );

  /// Resets the aux struct for filtering overlaps.
  ///
  /// @param        Aux                The associated aux struct for filtering overlaps.
  /// @param        bIsOverlapping     Whether the pawn is currently inside the overlap volume.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ResetFilterOverlapAux(UPARAM(Ref) FGMC_FilterOverlapAux& Aux, bool bIsOverlapping);

  /// Writes a series of boolean values to a byte as bit-mask. When replicating several bools it is often more bandwidth efficient to pack them into a byte.
  ///
  /// @param        OutByte    The masked byte.
  /// @param        InBit1     The value bit 1 should have.
  /// @param        InBit2     The value bit 2 should have.
  /// @param        InBit3     The value bit 3 should have.
  /// @param        InBit4     The value bit 4 should have.
  /// @param        InBit5     The value bit 5 should have.
  /// @param        InBit6     The value bit 6 should have.
  /// @param        InBit7     The value bit 7 should have.
  /// @param        InBit8     The value bit 8 should have.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static void PackBoolsToByte(
    uint8& OutByte,
    bool InBit1 = false,
    bool InBit2 = false,
    bool InBit3 = false,
    bool InBit4 = false,
    bool InBit5 = false,
    bool InBit6 = false,
    bool InBit7 = false,
    bool InBit8 = false
  );

  /// Reads a byte as bit-mask of boolean values. When replicating several bools it is often more bandwidth efficient to pack them into a byte.
  ///
  /// @param        InByte     The byte to be read.
  /// @param        OutBit1    The value of bit 1.
  /// @param        OutBit2    The value of bit 2.
  /// @param        OutBit3    The value of bit 3.
  /// @param        OutBit4    The value of bit 4.
  /// @param        OutBit5    The value of bit 5.
  /// @param        OutBit6    The value of bit 6.
  /// @param        OutBit7    The value of bit 7.
  /// @param        OutBit8    The value of bit 8.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  static void UnpackBoolsFromByte(
    const uint8& InByte,
    bool& OutBit1,
    bool& OutBit2,
    bool& OutBit3,
    bool& OutBit4,
    bool& OutBit5,
    bool& OutBit6,
    bool& OutBit7,
    bool& OutBit8
  );

  /// Returns the average frames per second as calculated internally by the engine.
  ///
  /// @returns      float    The average frames per second.
  UFUNCTION(BlueprintCallable, BlueprintPure = true, Category = "General Movement Component")
  static float GetAvgFPS();

  /// Returns the average frame time as calculated internally by the engine.
  ///
  /// @returns      float    The average frame time.
  UFUNCTION(BlueprintCallable, BlueprintPure = true, Category = "General Movement Component")
  static float GetAvgMS();

  /// Prints the given string to the log with additional debug information pertaining to the network context. Should only be called from GMC components when
  /// there is a valid reference to the owning pawn defined. Does not operate in builds that have logging disabled.
  ///
  /// @param        String    The message to log.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void NetLog(const FString& String = FString(TEXT("Hello")));

  /// Prints the given string to the log as a warning with additional debug information pertaining to the network context. Should only be called from GMC
  /// components when there is a valid reference to the owning pawn defined. Does not operate in builds that have logging disabled.
  ///
  /// @param        String    The message to log.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void NetLogWarning(const FString& String = FString(TEXT("Hello")));

  /// Prints the given string to the log as an error with additional debug information pertaining to the network context. Should only be called from GMC
  /// components when there is a valid reference to the owning pawn defined. Does not operate in builds that have logging disabled.
  ///
  /// @param        String    The message to log.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void NetLogError(const FString& String = FString(TEXT("Hello")));

  /// Virtual wrappers for UE's built-in montage functions.
  virtual float Montage_Play(UAnimInstance* AnimInstance, UAnimMontage* MontageToPlay, float PlayRate, EMontagePlayReturnType ReturnValueType, float StartPosition, bool bStopAllMontages) const;
  virtual float Montage_PlayWithBlendIn(UAnimInstance* AnimInstance, UAnimMontage* MontageToPlay, const FAlphaBlendArgs& BlendIn, float PlayRate, EMontagePlayReturnType ReturnValueType, float StartPosition, bool bStopAllMontages) const;
  virtual float Montage_PlayWithBlendSettings(UAnimInstance* AnimInstance, UAnimMontage* MontageToPlay, const FMontageBlendSettings& BlendInSettings, float PlayRate, EMontagePlayReturnType ReturnValueType, float StartPosition, bool bStopAllMontages) const;
  virtual void Montage_Stop(UAnimInstance* AnimInstance, float BlendOutTime, UAnimMontage* MontageToStop) const;
  virtual void Montage_StopWithBlendOut(UAnimInstance* AnimInstance, const FAlphaBlendArgs& BlendOut, UAnimMontage* MontageToStop) const;
  virtual void Montage_StopWithBlendSettings(UAnimInstance* AnimInstance, const FMontageBlendSettings& BlendOutSettings, UAnimMontage* MontageToStop) const;
  virtual void Montage_Pause(UAnimInstance* AnimInstance, UAnimMontage* MontageToPause) const;
  virtual void Montage_Resume(UAnimInstance* AnimInstance, UAnimMontage* MontageToResume) const;
  virtual void Montage_JumpToSection(UAnimInstance* AnimInstance, FName SectionName, UAnimMontage* Montage) const;
  virtual void Montage_JumpToSectionsEnd(UAnimInstance* AnimInstance, FName SectionName, UAnimMontage* Montage) const;
  virtual void Montage_SetPlayRate(UAnimInstance* AnimInstance, UAnimMontage* Montage, float NewPlayRate) const;
  virtual void Montage_SetPosition(UAnimInstance* AnimInstance, UAnimMontage* Montage, float NewPosition) const;

private:

  double MoveTimestamp{0.};
  float MoveDeltaTime{0.f};

  FGMC_PawnState MoveInputState{};

  double CurrentSmoothingTime{0.};
  int32 CurrentSmoothingTargetIdx{0};
  TArray<int32> CurrentSmoothingSkippedIdx{};

  float PhysDeltaTime{0.f};
  FVector Acceleration{0.};
  FVector Force{0.};
  FVector Velocity0{0.};
  FVector Location0{0.};

  int32 MoveIteration{0};
  bool bIsSubSteppedMoveIteration{false};
  bool bIsLocalMove{false};
  bool bIsSimulatedMove{false};
  bool bIsCombinedMove{false};
};
