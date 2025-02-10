// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCMovementUtilityComponent.h"
#include "InputAction.h"
#include "Animation/AnimMetaData.h"
#include "AI/RVOAvoidanceInterface.h"
#include "AI/Navigation/NavigationTypes.h"
#include "AI/Navigation/NavigationAvoidanceTypes.h"
#include "GMCOrganicMovementComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("UGMC_OrganicMovementCmp"), STATGROUP_UGMC_OrganicMovementCmp, STATCAT_Advanced);

UENUM(BlueprintType)
enum class EGMC_MovementMode : uint8
{
  None UMETA(ToolTip = "No movement."),
  Grounded UMETA(ToolTip = "Moving along a surface under the effect of gravity."),
  Airborne UMETA(ToolTip = "Moving through the air with or without being affected by gravity."),
  Buoyant UMETA(ToolTip = "Moving through a fluid volume under the effect of buoyancy."),
  Custom1,
  Custom2,
  Custom3,
  Custom4,
  Custom5,
  Custom6,
  Custom7,
  Custom8,
  Custom9,
  Custom10,
  Custom11,
  Custom12,
};

inline FString GetMovementModeAsString(uint8 MovementMode)
{
  switch (MovementMode)
  {
    case 0:
      return TEXT("None");
    case 1:
      return TEXT("Grounded");
    case 2:
      return TEXT("Airborne");
    case 3:
      return TEXT("Buoyant");
    case 4:
      return TEXT("Custom 1");
    case 5:
      return TEXT("Custom 2");
    case 6:
      return TEXT("Custom 3");
    case 7:
      return TEXT("Custom 4");
    case 8:
      return TEXT("Custom 5");
    case 9:
      return TEXT("Custom 6");
    case 10:
      return TEXT("Custom 7");
    case 11:
      return TEXT("Custom 8");
    case 12:
      return TEXT("Custom 9");
    case 13:
      return TEXT("Custom 10");
    case 14:
      return TEXT("Custom 11");
    case 15:
      return TEXT("Custom 12");
    default:
      return TEXT("");
  }
}

UENUM(BlueprintType)
enum class EGMC_BasedMovementSource : uint8
{
  None UMETA(ToolTip = "The pawn will never consider any components as base."),
  Velocity UMETA(ToolTip = "The pawn will follow components based on their velocity."),
  Relative UMETA(ToolTip = "The pawn will move relative to components it is based on. Base changes are not predicted in this mode."),
  MAX UMETA(Hidden),
};

USTRUCT(BlueprintType)
struct FGMC_BasedMovementVelocity
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000"))
  // The max height at which a component will still be considered as a base. Setting this to 0 will only set the base while grounded.
  float MaxHeight{0.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, a detected component will only be accepted as base if the hit point on that component was below the pawn.
  bool bSurfacesBelowPawnOnly{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, a detected component will only be accepted as base if a walkable surface was hit on that component.
  bool bWalkableSurfacesOnly{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to impart linear velocity when leaving the current base. Velocity is never imparted from a base that is simulating physics. Does not consider the
  // configured based max height (meaning velocity is imparted as soon as the pawn leaves the ground).
  bool bImpartLinearVelocity{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to impart tangential velocity when leaving the current base. Velocity is never imparted from a base that is simulating physics. Does not consider
  // the configured based max height (meaning velocity is imparted as soon as the pawn leaves the ground).
  bool bImpartTangentialVelocity{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the mass of the pawn will be taken into account when imparting the velocity of a base the pawn has just left.
  bool bConsiderMassOnImpartVelocity{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the pawn will inherit yaw rotation changes of the base component.
  bool bFollowBaseRotation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the outermost attach parent will be used as the base component if applicable.
  bool bUseAttachParentAsBase{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, this pawn will ignore any other pawns that are based on it during movement.
  bool bIgnoreBasedPawns{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the pawn will not have any physics interactions with an object it is currently based on.
  bool bNoPhysicsInteractionWithBase{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, simulated versions of this pawn will use relative based movement.
  bool bForceRelativeBasedMovementForSimulation{true};
};

USTRUCT(BlueprintType)
struct FGMC_BasedMovementRelative
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000"))
  // The max height at which a component will still be considered as a base. Setting this to 0 will only set the base while grounded.
  float MaxHeight{2000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, a detected component will only be accepted as base if the hit point on that component was below the pawn.
  bool bSurfacesBelowPawnOnly{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, a detected component will only be accepted as base if a walkable surface was hit on that component.
  bool bWalkableSurfacesOnly{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to impart linear velocity when leaving the current base. Velocity is never imparted from a base that is simulating physics. Does not consider the
  // configured based max height (meaning velocity is imparted as soon as the pawn leaves the ground).
  bool bImpartLinearVelocity{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to impart tangential velocity when leaving the current base. Velocity is never imparted from a base that is simulating physics. Does not consider
  // the configured based max height (meaning velocity is imparted as soon as the pawn leaves the ground).
  bool bImpartTangentialVelocity{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the mass of the pawn will be taken into account when imparting the velocity of a base the pawn has just left.
  bool bConsiderMassOnImpartVelocity{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "3"))
  // How long a detected component must remain valid to be accepted as a new base. As the transition is handled exclusively by the server it may appear to take
  // longer than the set value on the client.
  float BaseChangeTransitionTime{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the outermost attach parent will be used as the base component if applicable.
  bool bUseAttachParentAsBase{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, this pawn will ignore any other pawns that are based on it during movement.
  bool bIgnoreBasedPawns{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the pawn will not have any physics interactions with an object it is currently based on.
  bool bNoPhysicsInteractionWithBase{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the client will send additional information about the actor base rotation to the server. This can make the transition from one base to another
  // significantly smoother (at the cost of increased bandwidth usage).
  bool bSendActorBaseRotation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the actor that we are currently based on will be unrotated and moved to the equalization location for the move execution of client-owned pawns.
  // This can significantly improve prediction results for bases that have roll and/or pitch rotation. However, in addition to the increased performance
  // overhead there may also have far-reaching consequences for the game logic. Only enable this option if you understand the implications.
  bool bEqualizeBaseActor{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "90"))
  // If the roll or pitch angle of the base (in degrees) is greater than the configured value, the base won't be equalized.
  float MaxEqualizationAngle{45.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the base will remain at its current location during equalization. Note that the unrotated base may intersect other objects.
  bool bEqualizeInPlace{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Location the current base actor will be moved to for equalization. This should be a location in the map that is unobstructed and generally never occupied
  // by any other object. Only used if the base is not equalized in place.
  FVector EqualizationLocation{FVector::ZeroVector};
};

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_BasedMovement
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The reference point for how the pawn should follow a dynamic base component.
  EGMC_BasedMovementSource Source{EGMC_BasedMovementSource::Relative};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The reference point for how the pawn should follow a dynamic base component.
  FGMC_BasedMovementVelocity Velocity{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The reference point for how the pawn should follow a dynamic base component.
  FGMC_BasedMovementRelative Relative{};

  bool IsVelocity() const;
  bool IsForcedRelativeSimulation() const;
  bool IsRelative() const;
  bool IsNone() const;

  float GetMaxHeight() const;
  bool GetSurfacesBelowPawnOnly() const;
  bool GetWalkableSurfacesOnly() const;
  bool GetImpartLinearVelocity() const;
  bool GetImpartTangentialVelocity() const;
  bool GetConsiderMassOnImpartVelocity() const;
  bool GetUseAttachParentAsBase() const;
  bool GetIgnoreBasedPawns() const;
  bool GetNoPhysicsInteractionWithBase() const;
  bool GetFollowBaseRotation() const;
  bool GetForceRelativeBasedMovementForSimulation() const;
  float GetBaseChangeTransitionTime() const;
  bool GetSendActorBaseRotation() const;
  bool GetEqualizeBaseActor() const;
  bool GetEqualizeInPlace() const;
  FVector GetEqualizationLocation() const;
  float GetMaxEqualizationAngle() const;
};

struct GMCCORE_API FGMC_SavedRelBasedMovement
{
  friend struct FGMC_RelBasedMovementAux;

  USceneComponent* GetSavedBase(bool bSimulated) const { return bSimulated ? BaseSimulated.Get() : Base.Get(); }

  FVector GetSavedRelativeLinearVelocity(bool bSimulated) const { return bSimulated ? PawnRelativeSimulated.LinearVelocity : PawnRelative.LinearVelocity; }
  FVector GetSavedRelativeActorLocation(bool bSimulated) const { return bSimulated ? PawnRelativeSimulated.Location : PawnRelative.Location; }
  FRotator GetSavedRelativeActorRotation(bool bSimulated) const { return bSimulated ? PawnRelativeSimulated.Rotation : PawnRelative.Rotation; }
  FRotator GetSavedRelativeControlRotation(bool bSimulated) const { return bSimulated ? PawnRelativeSimulated.ControlRotation : PawnRelative.ControlRotation; }

  FVector GetSavedWorldLinearVelocity(bool bSimulated) const { return bSimulated ? PawnWorldSimulated.LinearVelocity : PawnWorld.LinearVelocity; }
  FVector GetSavedWorldActorLocation(bool bSimulated) const { return bSimulated ? PawnWorldSimulated.Location : PawnWorld.Location; }
  FRotator GetSavedWorldActorRotation(bool bSimulated) const { return bSimulated ? PawnWorldSimulated.Rotation : PawnWorld.Rotation; }
  FRotator GetSavedWorldControlRotation(bool bSimulated) const { return bSimulated ? PawnWorldSimulated.ControlRotation : PawnWorld.ControlRotation; }

  FVector GetSavedBaseLocation(bool bSimulated) const { return bSimulated ? BaseWorldSimulated.Location : BaseWorld.Location; }
  FQuat GetSavedBaseRotation(bool bSimulated) const { return bSimulated ? BaseWorldSimulated.Rotation : BaseWorld.Rotation; }

  FVector GetSavedRelativeSmoothComponentLocation(bool bSimulated) const { return bSimulated ? SmoothComponentRelativeSimulated.Location : SmoothComponentRelative.Location; }
  FRotator GetSavedRelativeSmoothComponentRotation(bool bSimulated) const { return bSimulated ? SmoothComponentRelativeSimulated.Rotation : SmoothComponentRelative.Rotation; }

  void Save(bool bSimulated, UGMC_ReplicationCmp* const Outer);
  void Reset(bool bSimulated);

private:

  struct PawnData
  {
    FVector LinearVelocity{FVector::ZeroVector};
    FVector Location{FVector::ZeroVector};
    FRotator Rotation{FRotator::ZeroRotator};
    FRotator ControlRotation{FRotator::ZeroRotator};
  };

  struct BaseData
  {
    FVector Location{FVector::ZeroVector};
    FQuat Rotation{FQuat::Identity};
  };

  struct SmoothComponentData
  {
    FVector Location{FVector::ZeroVector};
    FRotator Rotation{FRotator::ZeroRotator};
  };

  TWeakObjectPtr<USceneComponent> Base{nullptr};
  PawnData PawnRelative{};
  PawnData PawnWorld{};
  BaseData BaseWorld{};
  SmoothComponentData SmoothComponentRelative{};

  TWeakObjectPtr<USceneComponent> BaseSimulated{nullptr};
  PawnData PawnRelativeSimulated{};
  PawnData PawnWorldSimulated{};
  BaseData BaseWorldSimulated{};
  SmoothComponentData SmoothComponentRelativeSimulated{};

  void SavePawnRelative(bool bSimulated, UGMC_ReplicationCmp* const Outer);
  void SavePawnWorld(bool bSimulated, UGMC_ReplicationCmp* const Outer);
  void SaveBaseWorld(bool bSimulated, UGMC_ReplicationCmp* const Outer);
  void SaveSmoothComponentRelative(bool bSimulated, UGMC_ReplicationCmp* const Outer);
};

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_RelBasedMovementAux
{
  GENERATED_BODY()

  // These variables must never be bound.
  // The properties are not used for simulation (SavedTransforms is used for cumulative simulation).

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  TObjectPtr<USceneComponent> SV_TransitionBase{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  float SV_TransitionTimer{-1.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FTransform SavedBaseEqualizationTransform{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  TObjectPtr<USceneComponent> SavedEqualizationBase{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  bool bDidEqualizeBase{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  TObjectPtr<USceneComponent> SavedChangeBase{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  TObjectPtr<USceneComponent> CL_OriginalReplayMoveBase{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FRotator CL_OriginalReplayMoveBaseRotation{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FRotator ActorBaseRotation{0.};

  // The saved transforms are updated at very specific points in the logic. For local and remote moves they are saved before and after move execution. For
  // client replays they are saved before and after the full replay (but not for each individual replay move). For simulated pawns they are saved at the end of
  // every simulation tick. Since these values are local only it is generally not recommended to use them during move execution.
  FGMC_SavedRelBasedMovement SavedTransforms{};

  void PreMoveExecution(bool bLocalMove, UGMC_OrganicMovementCmp* const Outer, float DeltaSeconds);
  void PostMoveExecution(bool bLocalMove, UGMC_OrganicMovementCmp* const Outer);
  void CL_PreReplay(UGMC_OrganicMovementCmp* const Outer);
  void CL_PostReplay(UGMC_OrganicMovementCmp* const Outer);
  void SV_HandleBaseTransition(UGMC_OrganicMovementCmp* const Outer, float DeltaSeconds);
  void EqualizeBase(bool bEqualize, bool bMoveAllPawns, UGMC_OrganicMovementCmp* const Outer);
  void OnCumulativeMoveInitialized(FGMC_PawnState& InputState, UGMC_OrganicMovementCmp* const Outer);
  void UpdateSavedRelativeTransform(bool bSimulated, UGMC_OrganicMovementCmp* const Outer);
  void HandleMoveIgnoreActors(bool bSimulated, UGMC_OrganicMovementCmp* const Outer);
  void SV_ReconcileClientAuthValues(bool bPreMoveExecution, UGMC_OrganicMovementCmp* const Outer);
  void CL_PreReplayMoveExecution(const FGMC_Move& ReplayMove, UGMC_OrganicMovementCmp* const Outer);
  void CL_ReconcileClientAuthValues(bool bPreMoveExecution, UGMC_OrganicMovementCmp* const Outer);
  void Reset();
};

struct FGMC_NavMeshWalkingAux
{
  FNavLocation LastValidTargetNavLocation{};
  bool bWasNavMeshWalkingLastUpdate{};
  FVector GetDistanceToNavMesh(const FVector& Location) const;
  void ResetLastValidTargetNavLocation();
};

USTRUCT(BlueprintType)
struct FGMC_MontagePrediction
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The prediction mode to use for the montage reference. Cannot be changed at runtime.
  EGMC_PredictionMode MontageReferencePredictionMode{EGMC_PredictionMode::ServerAuth_Output_ClientValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The prediction mode to use for the montage position. Cannot be changed at runtime.
  EGMC_PredictionMode MontagePositionPredictionMode{EGMC_PredictionMode::ServerAuth_Output_ClientValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The prediction mode to use for the montage play rate. Cannot be changed at runtime.
  EGMC_PredictionMode MontagePlayRatePredictionMode{EGMC_PredictionMode::ServerAuth_Output_ClientValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The prediction mode to use for the montage "paused" flag. Cannot be changed at runtime.
  EGMC_PredictionMode MontagePausedPredictionMode{EGMC_PredictionMode::ServerAuth_Output_ClientValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The combine mode to use for the montage reference. Cannot be changed at runtime.
  EGMC_CombineMode MontageReferenceCombineMode{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The combine mode to use for the montage position. Cannot be changed at runtime.
  EGMC_CombineMode MontagePositionCombineMode{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The combine mode to use for the montage play rate. Cannot be changed at runtime.
  EGMC_CombineMode MontagePlayRateCombineMode{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The combine mode to use for the montage "paused" flag. Cannot be changed at runtime.
  EGMC_CombineMode MontagePausedCombineMode{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  // The time over which to blend the animation after a replay when montage data was corrected.
  float CorrectionBlendTime{0.1f};
};

USTRUCT(BlueprintType)
struct FGMC_MontageSimulation
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether the reference to the active montage should be replicated. Cannot be changed at runtime.
  bool bReplicateMontageReference{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether the position of the active montage should be replicated. Cannot be changed at runtime.
  bool bReplicateMontagePosition{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether the play rate of the active montage should be replicated. Cannot be changed at runtime.
  bool bReplicateMontagePlayRate{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether the "paused" flag of the active montage should be replicated. Cannot be changed at runtime.
  bool bReplicateMontagePaused{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The interpolation method to use when replicating the montage reference. Cannot be changed at runtime.
  EGMC_InterpolationFunction MontageReferenceInterpolation{EGMC_InterpolationFunction::NearestNeighbour};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The interpolation method to use when replicating the montage position. Cannot be changed at runtime.
  EGMC_InterpolationFunction MontagePositionInterpolation{EGMC_InterpolationFunction::NearestNeighbour};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The interpolation method to use when replicating the montage play rate. Cannot be changed at runtime.
  EGMC_InterpolationFunction MontagePlayRateInterpolation{EGMC_InterpolationFunction::NearestNeighbour};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // The interpolation method to use when replicating the montage "paused" flag. Cannot be changed at runtime.
  EGMC_InterpolationFunction MontagePausedInterpolation{EGMC_InterpolationFunction::NearestNeighbour};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, replicated montages will automatically be played for simulated pawns.
  bool bAutoSimulateMontages{true};
};

USTRUCT(BlueprintType)
struct FGMC_MontageReplication
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  // The settings for predicting montages.
  FGMC_MontagePrediction MontagePrediction{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  // The settings for simulating montages.
  FGMC_MontageSimulation MontageSimulation{};
};

USTRUCT(BlueprintType)
struct FGMC_RootMotionExtractionSettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, root motion will not be applied while the montage is blending in.
  bool bDisableRootMotionDuringBlendIn{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, root motion will not be applied while the montage is blending out.
  bool bDisableRootMotionDuringBlendOut{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, any extracted root motion in the world XY plane will be ignored.
  bool bIgnoreRootMotionXY{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, any extracted root motion along the world Z axis will be ignored.
  bool bIgnoreRootMotionZ{true};
};

USTRUCT(BlueprintType)
struct FGMC_RootMotionVelocitySettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, input acceleration will be applied with root motion.
  bool bApplyInputWithRootMotion{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, additional rotations will be applied with root motion.
  bool bApplyRotationWithRootMotion{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, external forces will be applied with root motion.
  bool bApplyExternalForcesWithRootMotion{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, deceleration will be applied with root motion.
  bool bApplyDecelerationWithRootMotion{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // If true, the speed of the pawn will be limited with root motion.
  bool bLimitSpeedWithRootMotion{false};
};

USTRUCT(BlueprintType)
struct FGMC_SimulationSettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether the new montage should cancel the same active one if it is currently blending in.
  bool bCancelSelfOnBlendIn{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether the new montage should cancel the same active one if it is currently blending out.
  bool bCancelSelfOnBlendOut{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "-1", UIMin = "0", UIMax = "0.5"))
  // The min required difference between the start and active position to cancel the montage. Set to -1 to disable canceling based on position difference.
  float CancelMinPositionDiff{0.2f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  // How much of the montage (in percent) still has to be left based on the start position for it to be played.
  float MinRemainingRatio{0.1f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  // Prevents the montage from erroneously playing again by blocking it for the specified duration after it has finished. Does not apply if canceled during
  // blend in/out.
  float CompletionTimeout{0.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  // How long the montage will continue playing when no new valid montage reference is being received.
  float CompletionTolerance{0.2f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "-1", UIMin = "0", UIMax = "0.5"))
  // The time over which to blend out the montage when it is canceled due to exceeding the completion tolerance time. A value of -1 means that the default blend
  // out time of the montage should be used.
  float StopBlendTime{-1.f};
};

UCLASS(ClassGroup = "Movement", BlueprintType, Blueprintable)
class GMCCORE_API UGMC_MontageMetaData : public UAnimMetaData
{
  GENERATED_BODY()

public:

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  // The root motion extraction settings to use.
  FGMC_RootMotionExtractionSettings RootMotionExtraction{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  // The root motion velocity settings to use.
  FGMC_RootMotionVelocitySettings RootMotionVelocity{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  // The simulation settings to use when auto simulating replicated montages.
  FGMC_SimulationSettings Simulation{};
};

DECLARE_DELEGATE(FGMC_OnMontageStart)
DECLARE_DELEGATE(FGMC_OnMontageBlendInComplete)
DECLARE_DELEGATE(FGMC_OnMontageBlendOutBegin)
DECLARE_DELEGATE(FGMC_OnMontageComplete)
DECLARE_DELEGATE_OneParam(FGMC_OnMontageNotifyBegin, FName)
DECLARE_DELEGATE_OneParam(FGMC_OnMontageNotifyEnd, FName)

/// Movement component intended for animate actors such as humans and animals. Typically organic movement is characterized by quick acceleration to a maximum
/// speed (where most of the movement happens) and quick deceleration to a stop again. Physical forces like air resistance are being ignored with this type of
/// movement due to their negligible influence. This implementation is not limited to bipeds or any particular collision shape.
UCLASS(ClassGroup = "Movement", BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GMCCORE_API UGMC_OrganicMovementCmp : public UGMC_MovementUtilityCmp, public IRVOAvoidanceInterface
{
  GENERATED_BODY()

  friend class UGMC_PlayMontageCallbackProxy;
  friend FGMC_RelBasedMovementAux;

public:

  UGMC_OrganicMovementCmp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void BeginPlay() override;
  void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
  void NotifyBumpedPawn(APawn* BumpedPawn) override;
  void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;
  FVector GetPenetrationAdjustment(const FHitResult& Hit) const override;
  FVector ComputeSlideVector(const FVector& Delta, float Time, const FVector& Normal, const FHitResult& Hit) const override;
  float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact = false) override;
  void TwoWallAdjust(FVector& Delta, const FHitResult& Hit, const FVector& OldHitNormal) const override;
  void HaltMovement() override;
  bool UpdateFloor(FGMC_FloorParams& Floor, const FVector& Direction, float TraceLength, float Tolerance, float ShapeExtentScale, bool bAutoAdjust, bool bForceUpdate) override;
  bool CanMove_Implementation() const override;
  USceneComponent* SetRootCollisionShape(EGMC_CollisionShape NewCollisionShape, const FVector& Extent, bool bScaled, FName Name = {}/*not used*/) override;
  void SetRootCollisionExtent(const FVector& NewExtent, bool bScaled, bool bUpdateOverlaps = true) override;
  EGMC_CollisionShape GetRootCollisionShape() const override;
  FVector GetRootCollisionExtent(bool bScaled) const override;

#if WITH_EDITOR

  void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

  /// Prefixes for the names given to a dynamically spawned root components.
  inline static const TCHAR* ROOT_NAME_DYNAMIC_CAPSULE = TEXT("RCSDynamicCapsule");
  inline static const TCHAR* ROOT_NAME_DYNAMIC_FLAT_CAPSULE = TEXT("RCSDynamicFlatCapsule");
  inline static const TCHAR* ROOT_NAME_DYNAMIC_BOX = TEXT("RCSDynamicBox");
  inline static const TCHAR* ROOT_NAME_DYNAMIC_SPHERE = TEXT("RCSDynamicSphere");
  /// If we reach a velocity magnitude lower or equal to this value when braking, velocity is zeroed.
  static constexpr float BRAKE_TO_STOP_VELOCITY = UE_KINDA_SMALL_NUMBER;
  /// The minimum velocity magnitude the pawn should have when falling off a ledge.
  static constexpr float MIN_LEDGE_FALL_OFF_VELOCITY = 500.f;
  /// The minimum size of the deceleration vector when braking. Prevents the velocity from becoming very small when it approaches 0.
  static constexpr float MIN_DECELERATION = 5.f;
  /// When in a fluid, buoyant force will only be applied if the pawn's immersion exceeds this tolerance so it remains more stable at the water line.
  static constexpr float IMMERSION_DEPTH_TOLERANCE = 0.04f;
  /// Scaling factor applied to the fluid friction of a physics volume when the pawn is in water.
  static constexpr float FLUID_FRICTION_SCALE = 8.f;
  /// Scaling factor applied to the braking deceleration when the pawn is in water.
  static constexpr float FLUID_DECELERATION_SCALE = 0.001f;
  /// How close to the water line the pawn still needs to be when trying to exit a fluid volume in order for FluidMinExitSpeed to be considered. Measured as a
  /// percentage of the collision height.
  static constexpr float EXIT_FLUID_IMMERSION_TOLERANCE = 0.1f;
  /// The minimum distance the pawn should maintain to the floor when grounded.
  static constexpr float MIN_DISTANCE_TO_FLOOR = 1.9f;
  /// The maximum distance the pawn should maintain to the floor when grounded.
  static constexpr float MAX_DISTANCE_TO_FLOOR = 2.4f;
  /// Reject impacts that are this close to the edge of the vertical portion of the collision shape when performing vertical sweeps.
  static constexpr float EDGE_TOLERANCE = 0.15f;
  /// The min size of the step up location delta to be applied so we don't fail the step up due to an unwalkable surface when starting from a resting position.
  static constexpr float MIN_STEP_UP_DELTA = 5.f;
  /// The min trace length to be used when checking for the actor base. As this happens before move execution the pawn could be grounded but still not close to
  /// the floor until after the movement mode was updated.
  static constexpr float MIN_ACTOR_BASE_TRACE_LENGTH = 200.f;
  /// The minimum location distance required between start and target state to smooth out a simulated base change.
  static constexpr float SMOOTH_SIMULATED_BASE_CHANGE_MIN_LOC_DIFF = 150.f;
  /// The maximum number of retries allowed to get a successful point projection during nav mesh walking.
  static constexpr int32 NAV_MESH_PROJECTION_MAX_RETRIES = 5;
  /// The maximum XY deviation targeted for a point projection during nav mesh walking.
  static constexpr double NAV_MESH_PROJECTION_TOLERANCE_XY = 10.;

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_RawInputVector{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_CurrentRootCollisionShape{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_CurrentRootCollisionExtent{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_MovementMode{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_ReceivedUpwardForce{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_HasRootMotion{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_MontageTracker_Montage{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_MontageTracker_MontagePosition{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_MontageTracker_MontagePlayRate{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_MontageTracker_MontagePaused{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_RelBasedMovementAux_ActorBaseRotation{-1};

protected:

  void BindReplicationData_Implementation() override;
  void WorldTickStart_Implementation(float DeltaTime) override;
  void WorldTickEnd_Implementation(float DeltaTime) override;
  bool CL_ShouldUseSmoothCorrections() const override;
  void ClearTransientData(bool bResetMoves = true) override;
  void SetupPlayerInputComponent_Implementation(UInputComponent* PlayerInputComponent) override;
  bool SV_OnProxyMoveInitialized_Implementation(FGMC_Move& Move, float DeltaTime, double Timestamp) override;
  bool OnCumulativeMoveInitialized_Implementation(FGMC_PawnState& InputState, EGMC_InterpolationStates SimStates, float DeltaTime, double Timestamp) override;
  bool ShouldRollBackGMCPawn_Implementation(const AGMC_Pawn* PawnToTest) const override;
  bool ShouldRollBackGMCActor_Implementation(const class AGMC_RollbackActor* ActorToTest) const override;
  bool ShouldTickGMCActor_Implementation(const AGMC_RollbackActor* RollbackActor) const override;
  void OnServerAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings) override;
  void OnClientAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings) override;
  void OnClientPredictionEnabled_Implementation() override;
  void OnClientPredictionDisabled_Implementation() override;
  void OnGMCEnabled_Implementation() override;
  void OnGMCDisabled_Implementation() override;
  void PreLocalMoveExecution_Implementation(const FGMC_Move& LocalMove) override;
  void SV_PreRemoteMoveExecution_Implementation(const FGMC_Move& RemoteMove) override;
  void PostLocalMoveExecution_Implementation(const FGMC_Move& ExecutedMove) override;
  void SV_PostRemoteMoveExecution_Implementation(const FGMC_Move& RemoteMove) override;
  void CL_PreReplayMoveExecution_Implementation(const FGMC_Move& ReplayMove) override;
  void CL_PreAdoptStateForReplay(const FGMC_Move& APMove, const FGMC_Move& SourceMove) override;
  void CL_PreReplay_Implementation() override;
  void CL_PostReplay_Implementation() override;
  void GenPredictionTick_Implementation(float DeltaTime) override;
  void GenSimulationTick_Implementation(float DeltaTime) override;
  void PreSmoothing_Implementation(int32 TargetIndex, bool bExtrapolating, bool bRollback, EGMC_InterpolationStates InterpStates) override;
  void SaveMatchLatestSimulationStartState(FGMC_PawnState& InOutSimulationStartState) override;
  void SaveSmoothComponentSimulationStartTransform(USceneComponent* SmoothComponent, FTransform& OutAbsoluteStartTransform) override;
  void OnSyncDataApplied_Implementation(const FGMC_PawnState& State, EGMC_NetContext Context) override;

  /// Prevent changes of the Z velocity.
  FVector GetWorldLinearVelocityFor(const FVector& InRelativeLinearVelocity, const FTransform& InBaseTransform) const;
  FVector GetBasedLinearVelocityFor(const FVector& InLinearVelocity, const FTransform& InBaseTransform) const override;

  /// Prevent rotations around the pitch and roll axes for the root collision.
  FRotator GetWorldActorRotationFor(const FRotator& InRelativeActorRotation, const FTransform& InBaseTransform) const override;
  FRotator GetBasedActorRotationFor(const FRotator& InActorRotation, const FTransform& InBaseTransform) const override;

  /// Prevent rotations around the roll axis for the control rotation.
  FRotator GetWorldControlRotationFor(const FRotator& InRelativeControlRotation, const FTransform& InBaseTransform) const override;
  FRotator GetBasedControlRotationFor(const FRotator& InControlRotation, const FTransform& InBaseTransform) const override;

///=============================================================================================================================================================
/// Movement Physics

private:

  /// The velocity at which the pawn hit the ground when landing after being airborne.
  FVector LandingVelocity{0.};

  /// Required to call CheckReceivedUpwardForce in PostMovementUpdate.
  FVector VelocityBeforeMovementUpdate{0.};

  /// Helper function to determine if base equalizations should be visualized.
  bool DebugShouldVisualizeBaseEqualization() const;

public:

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Whether the pawn received some form of upward force that should be considered when the movement mode is updated during the next tick. This flag gets reset
  /// automatically after it was processed (see UpdateMovementModeDynamic).
  bool bReceivedUpwardForce{false};

  /// Sets a reference to the skeletal mesh component of the owning pawn. This is automatically done once when beginning play (taking the first skeletal mesh in
  /// the pawn's scene component hierarchy), but if the mesh is changed at any point after that the reference needs to be updated manually. Setting a skeletal
  /// mesh reference will add the movement component as a tick prerequisite component for the mesh.
  ///
  /// @param        Mesh    The skeletal mesh to use. Passing nullptr clears the reference.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetSkeletalMeshReference(USkeletalMeshComponent* Mesh);

  /// Returns the currently set skeletal mesh reference.
  ///
  /// @returns      USkeletalMeshComponent*    The skeletal mesh component of the owning pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual USkeletalMeshComponent* GetSkeletalMeshReference() const;

  /// Returns information about the floor currently located underneath the pawn.
  ///
  /// @returns      const FGMC_FloorParams&    The current floor.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual const FGMC_FloorParams& GetCurrentFloor() const;

protected:

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Reference to the skeletal mesh of the owning pawn.
  TObjectPtr<USkeletalMeshComponent> SkeletalMesh{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The current movement mode of the pawn. Default values: 0 = None, 1 = Grounded, 2 = Airborne, 3 = Buoyant. Supports up to 16 replicated states. Use
  /// SetMovementMode to prompt a call to OnMovementModeChanged. If you don't want the event to be triggered the movement mode can be set directly. We don't use
  /// the enum (see EGMC_MovementMode) directly for the movement mode to be able to simply bind it as half-byte for replication.
  uint8 MovementMode{0};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Holds information about the floor currently located underneath the pawn. Note that the pawn does not necessarily have to be grounded for this to be valid
  /// (see FloorTraceLength).
  FGMC_FloorParams CurrentFloor{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Holds whatever the value of MaxDesiredSpeed was when the pawn spawned.
  float DefaultSpeed{0.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Contains auxiliary information for relative based movement.
  FGMC_RelBasedMovementAux RelBasedMovementAux{};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current collision shape of the pawn.
  uint8 CurrentRootCollisionShape{0};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current extent of the pawn's root collision (unscaled).
  FVector CurrentRootCollisionExtent{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The raw directional user input.
  FVector RawInputVector{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The input vector that is used for all physics calculations.
  FVector ProcessedInputVector{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// Minimum Z value of the normal of a walkable surface. Computed from the walkable floor angle.
  float WalkableFloorZ{0.f};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current immersion depth of the pawn. Range is from 0 (not in fluid) to 1 (fully immersed).
  float CurrentImmersionDepth{0.f};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The 2D direction in which the pawn should fall off a ledge when exceeding LedgeFallOffThreshold. Will be a zero vector if the pawn is currently not in the
  /// process of falling off.
  FVector LedgeFallOffDirection{0.};

  /// Applies the directional input of the player.
  ///
  /// @returns      void
  virtual void ApplyDirectionalInput(const FInputActionInstance& InputAction);

  /// Applies the view input of the player.
  ///
  /// @returns      void
  virtual void ApplyViewInput(const FInputActionInstance& InputAction);

  /// Used to maintain consistency between the root collision component and the bound data members.
  ///
  /// @returns      void
  virtual void MaintainRootCollisionCoherency();

  /// Clamps selected data members to their respective valid range when the actor is spawned and before movement is executed.
  ///
  /// @returns      void
  virtual void ClampToValidValues();

  /// Main function for moving the pawn and updating all associated data.
  ///
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  virtual void PerformMovement(float DeltaSeconds);

  /// Executes the movement physics based on the current movement mode for this tick.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void RunPhysics(float DeltaSeconds);

  /// Allows for pre-processing of the raw input vector. Called after the movement mode was updated.
  ///
  /// @param        RawInputVector    The original input vector reflecting the raw input data.
  /// @returns      FVector           The actual input vector to be used for all physics calculations.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  FVector PreProcessInputVector(FVector InRawInputVector);
  virtual FVector PreProcessInputVector_Implementation(FVector InRawInputVector);

  /// Only called while the updated component is simulating physics. No other movement code apart from what is implemented here will be executed while physics
  /// simulation is active.
  ///
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PhysicsSimulationUpdate(float DeltaSeconds);
  virtual void PhysicsSimulationUpdate_Implementation(float DeltaSeconds);

  /// Called before any kind of movement related update has happened. This is the only movement event that is called even if the pawn cannot move.
  ///
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PreMovementUpdate(float DeltaSeconds);
  virtual void PreMovementUpdate_Implementation(float DeltaSeconds);

  /// Called after movement was performed and root motion was applied. Do not play montages from this event.
  ///
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PostMovementUpdate(float DeltaSeconds);
  virtual void PostMovementUpdate_Implementation(float DeltaSeconds);

  /// Called immediately before switching on the current movement mode and executing the appropriate physics. At this point the movement mode and floor have
  /// been updated, and the input vector has been pre-processed.
  ///
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PrePhysicsUpdate(float DeltaSeconds);
  virtual void PrePhysicsUpdate_Implementation(float DeltaSeconds) {}

  /// Called immediately after the movement physics have run.
  ///
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PostPhysicsUpdate(float DeltaSeconds);
  virtual void PostPhysicsUpdate_Implementation(float DeltaSeconds);

  /// Called at the end of the current movement update. This is the preferred entry point for subclasses to implement custom logic if automatic handling of the
  /// default movement modes is desired.
  ///
  /// @param        DeltaSeconds    The current move delta time (may not be equal to the frame delta time).
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void MovementUpdate(float DeltaSeconds);
  virtual void MovementUpdate_Implementation(float DeltaSeconds) {}

  /// Handles the physics for grounded movement.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void PhysicsGrounded(float DeltaSeconds);

  /// Handles the physics for airborne movement.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void PhysicsAirborne(float DeltaSeconds);

  /// Handles the physics for movement in a fluid.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void PhysicsBuoyant(float DeltaSeconds);

  /// Handles the physics for custom movement modes.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PhysicsCustom(float DeltaSeconds);
  virtual void PhysicsCustom_Implementation(float DeltaSeconds) {}

  /// Determines whether the pawn's position can be adjusted during a floor update to resolve penetrations.
  ///
  /// @returns      bool    True if the pawn can be adjusted during a floor update, false otherwise.
  virtual bool ShouldDoFloorUpdateAdjust() const;

  /// Calculates the new movement velocity for based on the current pawn state. Velocity from root motion animations is calculated separately.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void CalculateVelocity(float DeltaSeconds);

  /// Whether direct movement should be performed for bots. Only relevant when UNavMovementComponent::bUseAccelerationForPaths is set to false.
  ///
  /// @returns      bool    True to call BotDirectMove to calculate the move velocity, false to proceed with the regular velocity calculation instead.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool ShouldPerformDirectMove() const;
  virtual bool ShouldPerformDirectMove_Implementation() const;

  /// Event for calculating and setting the move velocity of bots when direct movement is used for path following. Direct movement (which can be performed when
  /// UNavMovementComponent::bUseAccelerationForPaths is set to false) is not based on move input, instead path following provides a velocity to reach the goal
  /// directly.
  ///
  /// @param        PathVelocity          The velocity requested by path following.
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void BotDirectMove(const FVector& PathVelocity, const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);
  virtual void BotDirectMove_Implementation(const FVector& PathVelocity, const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);

  /// Apply velocity from move input.
  ///
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ApplyInputVelocity(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);

  /// Change the pawn's orientation.
  ///
  /// @param        bIsDirectBotMove      Whether the pawn is a bot executing direct movement (i.e. the pawn is not using acceleration values
  ///                                     to calculate its velocity).
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ApplyRotation(bool bIsDirectBotMove, const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);

  /// Apply velocity from external forces (e.g. gravity).
  ///
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ApplyExternalForces(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);

  /// Apply acceleration to make the pawn fall off a ledge if required.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void ApplyLedgeFallOffAcceleration(float DeltaSeconds);

  /// Add forces that oppose the current velocity. Might cause the velocity to be clamped to 0 (important to consider when maintaining framerate independence).
  ///
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ApplyDeceleration(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);

  /// Slows the pawn down to max speed if necessary. Might cause the velocity to be clamped (important to consider when maintaining framerate independence).
  ///
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void LimitSpeed(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);

  /// Enforces the max desired speed in the XY-plane.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void LimitSpeedXY(float DeltaSeconds);

  /// Enforces the max desired speed in all directions.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void LimitSpeedXYZ(float DeltaSeconds);

  /// Limits the max speed in downward Z-direction to the terminal velocity of the current physics volume if the pawn is falling down.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void LimitAirborneSpeedTerminalVelocity(float DeltaSeconds);

  /// Called from CalculateVelocity for custom velocity computations. This is called before any clamping of the velocity happens and should be preferred to
  /// overriding the CalculateVelocity function directly. Not called for bots executing a direct move.
  ///
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void CalculateVelocityCustom(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds);
  virtual void CalculateVelocityCustom_Implementation(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) {}

  /// Recalculates the current immersion depth of the pawn and updates CurrentImmersionDepth with the new value.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void UpdateImmersionDepth();

  /// Used to prevent the pawn from leaving a fluid volume in certain circumstances. We do not want the pawn to leave a fluid volume if it is just experiencing
  /// a minor upward force and is still close to the water line, otherwise there is a risk of repeatedly switching in and out of the buoyant state.
  ///
  /// @param        Floor                 The current floor parameters.
  /// @param        ImmersionTolerance    How close to the water line the pawn must still be for an adjustment to happen (as a percentage of the current root
  ///                                     collision height).
  /// @param        SpeedTolerance        No adjustment will be made if the pawn's current upward speed is equal to or greater than the passed value. Passing 0
  ///                                     effectively disables this function.
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      bool                  True if an adjustment was made and the pawn should remain in a buoyant state, false otherwise.
  virtual bool CheckLeaveFluid(const FGMC_FloorParams& Floor, float ImmersionTolerance, float SpeedZTolerance, float DeltaSeconds);

  /// Sets bReceivedUpwardForce based on the velocity before and after MovementUpdate and the value of the current processed input vector. Called after the
  /// movement update has run and root motion was applied.
  ///
  /// @param        PreviousVelocity    The velocity value from before MovementUpdate.
  /// @returns      void
  virtual void CheckReceivedUpwardForce(const FVector& PreviousVelocity);

  /// Calculates the acceleration generated from the input vector and the configured input acceleration.
  ///
  /// @returns      FVector    The calculated input acceleration.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FVector ComputeInputAcceleration() const;
  virtual FVector ComputeInputAcceleration_Implementation() const;

  /// Rounds the world input vector relative to the current actor base (if any).
  ///
  /// @param        WorldInputVector    The input vector in world space.
  /// @param        Level               The quantization level.
  /// @returns      FVector             The rounded input vector.
  virtual FVector RoundInputVector(const FVector& WorldInputVector, EGMC_FloatPrecisionBlueprint Level) const;

  /// Returns the current input modifier (e.g. from an analog stick) i.e. the size of the input vector (clamped to a max size of 1). When grounded, the negative
  /// Z component of the input vector is not factored in.
  ///
  /// @returns      float    The current input modifier in the range [0,1].
  virtual float ComputeInputModifier() const;

  /// Calculates the buoyancy force a pawn should experience when immersed in a fluid.
  ///
  /// @param        PawnMass               The mass of the pawn in kg.
  /// @param        Gravity                The gravity Z-component in cm/s^2.
  /// @param        BuoyancyCoefficient    Coefficient conflating quantities that we don't want to consider individually (liquid density,
  ///                                      body volume, etc.).
  /// @returns      FVector                The calculated buoyancy force.
  virtual float ComputeBuoyantForce(float PawnMass, float GravityZ, float BuoyancyCoefficient) const;

  /// Clamps the passed vector to the min deceleration in XY-direction and Z-direction.
  ///
  /// @param        Deceleration    The current deceleration vector.
  /// @returns      FVector         The clamped deceleration vector.
  virtual FVector ClampToMinDeceleration(const FVector& Deceleration) const;

  /// Returns the outermost attach parent of the passed component.
  ///
  /// @param        Component           The component to check.
  /// @returns      USceneComponent*    The outermost attach parent of the passed component.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual USceneComponent* GetOutermostAttachParent(USceneComponent* Component) const;

  /// Sets the type of movement that should be used for following movable bases. Do not call this function during move execution, the based movement source can
  /// only be changed externally.
  ///
  /// @param        NewSource    The new based movement source to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetBasedMovementSource(EGMC_BasedMovementSource NewSource);

  /// Determines what the movement base should be for the current update.
  ///
  /// @returns      UPrimitiveComponent*    The new actor base.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  UPrimitiveComponent* FindActorBase();
  virtual UPrimitiveComponent* FindActorBase_Implementation();

  /// Performs the trace to find the movement base each update.
  ///
  /// @returns      FHitResult    The hit result of the trace.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FHitResult TraceForActorBase();
  virtual FHitResult TraceForActorBase_Implementation();

  /// Manages prerequisite ticks when the actor base changes. The base actor must tick before the movement component (and will then, by extension, also tick
  /// before the mesh).
  ///
  /// @param        NewBase         The current base of the pawn (null means no base).
  /// @param        PreviousBase    The previous base of the pawn (null means no base).
  /// @param        bSimulated      Whether the base change is being applied for simulation.
  /// @returns      void
  virtual void HandleActorBaseChange(USceneComponent* NewBase, USceneComponent* PreviousBase, bool bSimulated);

  /// Sets the prerequisite ticks when the actor base changes.
  ///
  /// @param        bAdd    Whether to add or remove the prerequisite ticks.
  /// @param        Base    The base to add or remove the prerequisite ticks for.
  /// @returns      void
  virtual void SetBasedMovementPrerequisiteTicks(bool bAdd, USceneComponent* Base);

  /// Sets the tick group when the actor base changes.
  ///
  /// @param        NewBase    The new movement base (may be nullptr).
  /// @returns      void
  virtual void SetBasedMovementTickGroup(USceneComponent* NewBase);

  /// Called from PreMovementUpdate when the actor base has changed.
  ///
  /// @param        NewBase         The current base of the pawn (null means no base).
  /// @param        PreviousBase    The previous base of the pawn (null means no base).
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void OnActorBaseChanged(UPrimitiveComponent* NewBase, UPrimitiveComponent* PreviousBase);
  virtual void OnActorBaseChanged_Implementation(UPrimitiveComponent* NewBase, UPrimitiveComponent* PreviousBase) {}

  /// Makes the pawn follow the current base component by its velocity.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void MoveWithBaseVelocity(float DeltaSeconds);
  virtual void MoveWithBaseVelocity_Implementation(float DeltaSeconds);

  /// Handles the pre-movement setup when "Velocity" is used as based movement source.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void UpdateBasedMovementVelocity(float DeltaSeconds);

  /// Called when the movement mode changes from grounded to airborne and the velocity of the movement base should be imparted. Allows for user-defined
  /// post-processing of the calculated velocity to impart. Only called if CanImpartVelocityFromBase returned true.
  ///
  /// @param        CurrentBase         The component of which the velocity should be imparted.
  /// @param        VelocityToImpart    The impart-velocity that was calculated up to this point.
  /// @returns      FVector             The final velocity to impart.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  FVector PostProcessVelocityToImpart(UPrimitiveComponent* CurrentBase, const FVector& VelocityToImpart);
  virtual FVector PostProcessVelocityToImpart_Implementation(UPrimitiveComponent* CurrentBase, const FVector& VelocityToImpart) { return VelocityToImpart; }

  /// Called at the end of CalculateVelocity. Allows for user-defined post-processing of the calculated pawn velocity (by modifying the value of
  /// UMovementComponent::Velocity). Also called for bots executing a direct move.
  ///
  /// @param        RootMotionMetaData    The velocity meta data settings for the current root motion (if any).
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PostProcessPawnVelocity(const FGMC_RootMotionVelocitySettings& RootMotionMetaData);
  virtual void PostProcessPawnVelocity_Implementation(const FGMC_RootMotionVelocitySettings& RootMotionMetaData) {}

  /// Updates the pawn's location and rotation relative to the base component when based movement is relative.
  ///
  /// @param        bSimulated    Whether simulated values should be retrieved from the saved transforms struct.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void MoveWithBaseRelative(bool bSimulated);
  virtual void MoveWithBaseRelative_Implementation(bool bSimulated);

  /// Determines whether a pawn should be moved for base equalization.
  ///
  /// @param        PawnToTest    The pawn to check.
  /// @returns      bool          True if the passed pawn should be moved for base equalization, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool ShouldMoveGMCPawnForBaseEqualization(const AGMC_Pawn* PawnToTest) const;
  virtual bool ShouldMoveGMCPawnForBaseEqualization_Implementation(const AGMC_Pawn* PawnToTest) const;

  /// Revert the base and all equalized pawns to their regular transforms. ReEqualizeBase must be called afterwards.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void UnEqualizeBase();

  /// Undo a previous call to UnEqualizeBase.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ReEqualizeBase();

  /// Handles the pre-movement setup when "Relative" is used as based movement source.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void UpdateBasedMovementRelative(float DeltaSeconds);

  /// Handles the pre-movement setup when "None" is used as based movement source.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void UpdateBasedMovementNone(float DeltaSeconds);

  /// Only returns the actor base if relative synchronisation is used for this pawn.
  ///
  /// @returns      USceneComponent*    The current actor base if relative synchronisation is used, null otherwise.
  virtual USceneComponent* GetRelativeActorBase() const;

  /// Updates the movement mode dynamically (i.e. with regard to the current movement mode). Returning false indicates that the movement mode should still be
  /// updated statically (see UpdateMovementModeStatic) afterwards, returning true will skip the static update.
  ///
  /// @param        Floor           The current floor parameters.
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      bool            If false, the movement mode will still be updated statically afterwards.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool UpdateMovementModeDynamic(UPARAM(Ref) FGMC_FloorParams& Floor, float DeltaSeconds);
  virtual bool UpdateMovementModeDynamic_Implementation(UPARAM(Ref) FGMC_FloorParams& Floor, float DeltaSeconds);

  /// Updates the movement mode statically (i.e. independent of the current movement mode).
  ///
  /// @param        Floor           The current floor parameters.
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      bool            True if the movement mode was updated, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool UpdateMovementModeStatic(UPARAM(Ref) FGMC_FloorParams& Floor, float DeltaSeconds);
  virtual bool UpdateMovementModeStatic_Implementation(UPARAM(Ref) FGMC_FloorParams& Floor, float DeltaSeconds);

  /// Called after the movement mode was updated dynamically and/or statically. Do not confuse this function with OnMovementModeChanged.
  ///
  /// @param        PreviousMovementMode    The movement mode we changed from (i.e. the value of MovementMode before it was updated).
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnMovementModeUpdated(EGMC_MovementMode PreviousMovementMode);
  virtual void OnMovementModeUpdated_Implementation(EGMC_MovementMode PreviousMovementMode);

  /// Move the updated component along the floor while grounded. Movement velocity will always remain horizontal.
  ///
  /// @param        LocationDelta    The location delta to apply.
  /// @param        Floor            The current floor.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      FVector          The applied location delta.
  virtual FVector MoveAlongFloor(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds);

  /// Move the updated component along the nav mesh while grounded. Movement velocity will always remain horizontal.
  ///
  /// @param        LocationDelta    The location delta to apply.
  /// @param        Floor            The current floor.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      FVector          The applied location delta.
  virtual FVector MoveAlongNavMesh(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds);

  /// Move the updated component by the given location delta while in the air.
  ///
  /// @param        LocationDelta    The location delta to apply.
  /// @param        Floor            The current floor.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      bool             True if the pawn landed on a walkable surface, false otherwise.
  virtual bool MoveThroughAir(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds);

  /// Move the updated component by the given location delta while swimming through a fluid volume.
  ///
  /// @param        LocationDelta    The location delta to apply.
  /// @param        Floor            The current floor.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      bool             True if the pawn left the fluid volume, false otherwise.
  virtual bool MoveThroughFluid(const FVector& LocationDelta, FGMC_FloorParams& Floor, float DeltaSeconds);

  /// Helper function for moving through a fluid volume.
  ///
  /// @param        LocationDelta    The location delta to apply.
  /// @param        Hit              The hit result of the movement.
  /// @param        Floor            The current floor.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      float            The remaining percentage of the location delta if the pawn has left the water during movement.
  virtual float Swim(const FVector& LocationDelta, FHitResult& Hit, FGMC_FloorParams& Floor, float DeltaSeconds);

  /// Computes a vector that moves parallel to the hit ramp.
  ///
  /// @param        LocationDelta    The attempted location delta.
  /// @param        RampHit          The ramp that was hit.
  /// @returns      FVector          The new movement vector that moves parallel to the hit surface.
  virtual FVector ComputeRampVector(const FVector& LocationDelta, const FHitResult& RampHit) const;

  /// Handles stepping up barriers that do not exceed the max step up height.
  ///
  /// @param        LocationDelta    The attempted location delta.
  /// @param        BarrierHit       The barrier that was hit.
  /// @param        Floor            The current floor.
  /// @returns      bool             True if the pawn has scaled the barrier, false otherwise.
  virtual bool StepUp(const FVector& LocationDelta, const FHitResult& BarrierHit, const FGMC_FloorParams& Floor);

  /// Changes the current movement mode from grounded to airborne by moving the pawn away from the ground by the max floor distance.
  ///
  /// @param        Floor    The current floor.
  /// @returns      void
  virtual void RaiseFromGround(FGMC_FloorParams& Floor);

  /// Maintains the pawn's distance to the floor.
  ///
  /// @param        Floor          The current floor (must be up to date).
  /// @param        bAnySurface    If true, ignores whether a floor surface is walkable or not.
  /// @returns      bool           Whether the adjustment was successful and the pawn is within the set limits, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool MaintainDistanceToFloor(UPARAM(Ref) FGMC_FloorParams& Floor, bool bAnySurface);

  /// Determines the location of the water line. Respects the current movement state meaning the resulting position will be adjusted slightly towards the pawn's
  /// current physics volume.
  ///
  /// @param        LocationInWater       The location of the pawn inside the fluid volume.
  /// @param        LocationOutOfWater    The location of the pawn outside the fluid volume.
  /// @param        ImmersionDepth        The current immersion depth of the pawn.
  /// @returns      FVector               The location of the water line.
  virtual FVector FindWaterLine(const FVector& LocationInWater, const FVector& LocationOutOfWater, float ImmersionDepth) const;

  /// Handles the movement mode change from buoyant to airborne.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void ProcessLeftFluid(float DeltaSeconds);

  /// Wrapper around AdjustVelocityFromHit that prevents the pawn from gaining upward velocity from the hit adjustment.
  ///
  /// @param        Hit             The hit result of the collision.
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void AdjustVelocityFromHitAirborne(const FHitResult& Hit, float DeltaSeconds);
  virtual void AdjustVelocityFromHitAirborne_Implementation(const FHitResult& Hit, float DeltaSeconds);

  /// Checks if the hit surface is a valid landing spot for the pawn (when trying to land after being airborne).
  ///
  /// @param        Hit         The hit surface.
  /// @returns      bool        True if the passed location is a valid landing spot, false otherwise.
  virtual bool IsValidLandingSpot(const FHitResult& Hit);

  /// Handles the movement mode change from airborne to grounded.
  ///
  /// @param        Hit             The hit result after landing.
  /// @param        DeltaSeconds    The delta time to use.
  /// @param        bUpdateFloor    Whether the current floor should be updated.
  /// @returns      void
  virtual void ProcessLanded(const FHitResult& Hit, FGMC_FloorParams& Floor, bool bUpdateFloor, float DeltaSeconds);

  /// Called when the movement mode changes from airborne to grounded.
  ///
  /// @param        ImpactVelocity    The velocity at which the pawn hit the ground.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void OnLanded(const FVector& ImpactVelocity);
  virtual void OnLanded_Implementation(const FVector& ImpactVelocity) {}

  /// Calculates a new XY velocity for the pawn based on the fall control percentage and the input acceleration. Only applied while in the air and under the
  /// influence of gravity.
  ///
  /// @param        Control              The degree of control the pawn should have as a percentage of the passed acceleration.
  /// @param        InputAcceleration    The current input acceleration of the pawn.
  /// @param        DeltaSeconds         The delta time to use.
  /// @returns      void
  virtual void ApplyFallControl(float Control, const FVector& InputAcceleration, float DeltaSeconds);

  /// Check whether the pawn is perching further beyond a ledge than allowed by LedgeFallOffThreshold. For box collision the threshold is assumed to be either
  /// 1 (if LedgeFallOffThreshold >= 0.5) or 0 (if LedgeFallOffThreshold < 0.5).
  ///
  /// @param        ImpactPoint       The impact point to test for (usually from the hit result of the floor sweep).
  /// @param        PawnLowerBound    The lower bound of the pawn's collision.
  /// @param        PawnCenter        The center of the pawn's collision.
  /// @returns      bool              True if the pawn is exceeding the threshold, false otherwise.
  virtual bool IsExceedingFallOffThreshold(const FVector& ImpactPoint, const FVector& PawnLowerBound, const FVector& PawnCenter) const;

  /// Prevents the pawn from boosting up slopes when airborne by limiting the computed slide vector.
  ///
  /// @param        SlideResult    The computed slide vector.
  /// @param        Delta          The attempted movement location delta.
  /// @param        Time           The amount of the delta to apply.
  /// @param        Normal         The hit normal.
  /// @param        Hit            The original hit result that was used to compute the slide vector.
  /// @returns      FVector        The adjusted slide vector.
  virtual FVector PreventAirborneSlopeBoosting(
    const FVector& SlideResult,
    const FVector& Delta,
    float Time,
    const FVector& Normal,
    const FHitResult& Hit
  ) const;

  /// Determines whether the velocity of the movement base can be imparted.
  ///
  /// @param        CurrentBase    The current movement base of the pawn.
  /// @returns      bool           True if the base velocity can be imparted, false otherwise.
  virtual bool CanImpartVelocityFromBase(UPrimitiveComponent* CurrentBase) const;

  /// Called when the movement mode is changed through a call to SetMovementMode. Do not confuse this function with OnMovementModeUpdated.
  ///
  /// @param        PreviousMovementMode    The previous movement mode we changed from.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void OnMovementModeChanged(EGMC_MovementMode PreviousMovementMode);
  virtual void OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode);

  /// Can be overridden to return the input acceleration for custom movement modes. Called directly from GetInputAcceleration if the current movement mode is
  /// none of the default ones.
  ///
  /// @returns      float    The current input acceleration.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  float GetInputAccelerationCustom() const;
  virtual float GetInputAccelerationCustom_Implementation() const { return 0.f; }

  /// Can be overridden to return the braking deceleration for custom movement modes. Called directly from GetBrakingDeceleration if the current movement mode
  /// is none of the default ones.
  ///
  /// @returns      float    The current braking deceleration.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  float GetBrakingDecelerationCustom() const;
  virtual float GetBrakingDecelerationCustom_Implementation() const { return 0.f; }

  /// Can be overridden to return the over-max-speed-deceleration for custom movement modes. Called directly from GetOverMaxSpeedDeceleration if the current
  /// movement mode is none of the default ones.
  ///
  /// @returns      float    The deceleration applied when the pawn exceeds the max desired speed.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  float GetOverMaxSpeedDecelerationCustom() const;
  virtual float GetOverMaxSpeedDecelerationCustom_Implementation() const { return 0.f; }

  /// Delegate called when the root collision touches another primitive component.
  ///
  /// @param        OverlappedComponent    The overlapped component.
  /// @param        OtherActor             The other actor.
  /// @param        OtherComponent         The other component.
  /// @param        OtherBodyIndex         The other body index.
  /// @param        bFromSweep             Whether the delegate was called from a sweep.
  /// @param        SweepResult            The hit result of the sweep.
  /// @returns      void
  UFUNCTION()
  virtual void RootCollisionTouched(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
  );

  /// Applies impact forces to the hit component when using physics interaction.
  ///
  /// @param        Impact                The hit result of the impact.
  /// @param        ImpactAcceleration    The acceleration of the pawn at the time of impact.
  /// @param        ImpactVelocity        The velocity of the pawn at the time of impact.
  /// @returns      void
  virtual void ApplyImpactPhysicsForces(const FHitResult& Impact, const FVector& ImpactAcceleration, const FVector& ImpactVelocity);

  /// Applies a downward force when walking on top of physics objects.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void ApplyDownwardForce(float DeltaSeconds);

  /// Applies a repulsion force to touched physics objects.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void ApplyRepulsionForce(float DeltaSeconds);

  /// Checks if physics interactions should be applied to the passed component.
  ///
  /// @param        Component    The component to check.
  /// @returns      bool         Whether any physics interaction should be applied to the passed component.
  virtual bool ShouldApplyPhysicsInteraction(UPrimitiveComponent* Component);

public:

  /// Returns the current max speed the pawn is allowed to have. This does not simply retrieve MaxDesiredSpeed. The final speed may be influenced by other
  /// factors such as the velocity requested from path following or a currently applied analog input modifier.
  ///
  /// @returns      float    The current max speed the pawn should have.
  float GetMaxSpeed() const override;

  /// Returns the raw directional user input.
  ///
  /// @returns      FVector    The raw user input vector.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetRawInputVector() const;

  /// Returns the pre-processed input vector. The processed input is not available until after the movement mode was updated meaning PrePhysicsUpdate is the
  /// earliest event for which the return value can be non-zero.
  ///
  /// @returns      FVector    The processed input vector used for all physics calculations.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetProcessedInputVector() const;

  /// Returns the current movement mode of the pawn.
  ///
  /// @returns      EGMC_MovementMode    The current movement mode.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual EGMC_MovementMode GetMovementMode() const;

  /// Returns the currently set input acceleration for the current movement mode.
  ///
  /// @returns      float    The current input acceleration.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetInputAcceleration() const;

  /// Returns the currently set braking deceleration for the current movement mode.
  ///
  /// @returns      float    The current braking deceleration.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetBrakingDeceleration() const;

  /// Returns the currently set deceleration applied when the pawn exceeds the max desired speed for the current movement mode.
  ///
  /// @returns      float    The current braking deceleration.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetOverMaxSpeedDeceleration() const;

  /// Returns the current ground friction.
  ///
  /// @returns      float    The current ground friction.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetGroundFriction() const;

  /// Checks if the hit object is a walkable surface.
  ///
  /// @param        Hit     The hit result to be checked.
  /// @returns      bool    True if the surface is walkable, false if not.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HitWalkableFloor(const FHitResult& Hit) const;

  /// Update the current movement mode. Calls OnMovementModeChanged if NewMovementMode is different from the current movement mode.
  ///
  /// @param        NewMovementMode    The new movement mode.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetMovementMode(EGMC_MovementMode NewMovementMode);

  /// Update the current movement mode. Calls OnMovementModeChanged if NewMovementMode is different from the current movement mode.
  ///
  /// @param        NewMovementMode    The new movement mode.
  /// @returns      void
  virtual void SetMovementMode(uint8 NewMovementMode);

  /// Update the current movement mode with the equivalent values from the built-in enum.
  ///
  /// @param        NewMovementMode    The new movement mode.
  /// @returns      void
  virtual void SetMovementMode(EMovementMode NewMovementMode);

  /// Returns the current immersion depth of the pawn (0 = not in a fluid volume, 1 = fully immersed).
  ///
  /// @returns      float    The current immersion depth of the pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetCurrentImmersionDepth() const;

  /// Updates the saved transform of the pawn relative to the current base. Should be called if the pawn is moved outside of the movement logic while based on
  /// another component.
  ///
  /// @param        bSimulated    Whether to update the simulated or prediction values.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void UpdateRelativeBasedMovementTransform(bool bSimulated);

  /// Check if the pawn can receive input.
  ///
  /// @returns      bool    True if this pawn receives input, false otherwise.
  bool HasMoveInputEnabled() const override;

  /// Returns the max step up height of the pawn.
  ///
  /// @returns      float    The max step up height.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMaxStepUpHeight() const;

  /// Returns the max step down height of the pawn.
  ///
  /// @returns      float    The max step down height.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMaxStepDownHeight() const;

  /// Blocks the automatic pose tick on the skeletal mesh by ensuring that USkeletalMeshComponent::ShouldTickPose will return false.
  ///
  /// @returns      void
  virtual void BlockSkeletalMeshPoseTick() const;

  /// Enables the automatic pose tick on the skeletal mesh again after it was disabled with BlockSkeletalMeshPoseTick.
  ///
  /// @returns      void
  virtual void EnableSkeletalMeshPoseTick() const;

  /// Set the max walkable slope for the pawn by angle. Automatically updates the walkable floor Z.
  ///
  /// @param        NewWalkableFloorAngle    The new walkable floor angle in degrees.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetWalkableFloorAngle(float NewWalkableFloorAngle);

  /// Set the max walkable slope for the pawn by Z value. Automatically updates the walkable floor angle.
  ///
  /// @param        NewWalkableFloorZ    The new walkable floor Z value.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetWalkableFloorZ(float NewWalkableFloorZ);

  /// Checks whether the current velocity is exceeding the given max speed.
  ///
  /// @param        MaxSpeed    The current max speed of the pawn.
  /// @returns      bool        True if the current velocity is exceeding the given max speed, false otherwise.
  bool IsExceedingMaxSpeed(float MaxSpeed) const override;

  /// Returns the current (scaled) gravity Z component. Upward gravity is not supported, a value larger than 0 will be clamped.
  ///
  /// @returns      float    The current gravity Z component.
  float GetGravityZ() const override;

  /// Returns the current gravity as vector in the format (0, 0, GravityZ).
  ///
  /// @returns      FVector    The current gravity acceleration vector.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetGravity() const;

  /// Completely disables kinematic movement. This will set the movement mode to None.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void DisableMovement();

  /// Checks whether we are affected by gravity.
  ///
  /// @returns      bool    True if the pawn is influenced by gravity, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsAffectedByGravity() const;

  /// Checks whether we currently have the airborne movement state.
  ///
  /// @returns      bool    True if airborne, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsAirborne() const;

  /// Checks whether the pawn's Z velocity is currently exceeding the threshold for remaining grounded/being able to land.
  ///
  /// @returns      bool    True if the Z velocity is larger than MaxGroundedVelocityZ, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsExceedingMaxGroundedVelocityZ() const;

  /// Checks whether we currently have the airborne movement state, which is the equivalent of MOVE_Falling of the engine's built-in enum. It is recommended to
  /// use IsAirborne instead of this function.
  ///
  /// @returns      bool    True if airborne, false otherwise.
  bool IsFalling() const override;

  /// Checks whether we currently have the grounded movement state.
  ///
  /// @returns      bool    True if grounded, false otherwise.
  bool IsMovingOnGround() const override;

  /// Checks whether we currently have the buoyant movement state.
  ///
  /// @returns      bool    True if swimming, false otherwise.
  bool IsSwimming() const override;

  /// Checks whether we are currently flying. We are flying if there is no gravity being applied to the pawn and we are airborne.
  ///
  /// @returns      bool    True if flying, false otherwise.
  bool IsFlying() const override;

///=============================================================================================================================================================
/// Montages

private:

  /// Delegate activated when a new montage starts. Called after OnMontageStarted.
  FGMC_OnMontageStart MontageStartDelegate{};

  /// Delegate activated when the active montage has finished blending in. Called after OnMontageBlendIn.
  FGMC_OnMontageBlendInComplete MontageBlendInDelegate{};

  /// Delegate activated when the active montage has started blending out. Called after OnMontageBlendOut.
  FGMC_OnMontageBlendOutBegin MontageBlendOutDelegate{};

  /// Delegate activated when the active montage has finished. Called after OnMontageCompleted.
  FGMC_OnMontageComplete MontageCompleteDelegate{};

  /// Delegate activated when a notify was triggered on the active montage. Called after OnMontageNotifyBegin.
  FGMC_OnMontageNotifyBegin MontageNotifyBeginDelegate{};

  /// Delegate activated when a notify state has ended on the active montage. Called after OnMontageNotifyEnd.
  FGMC_OnMontageNotifyEnd MontageNotifyEndDelegate{};

  /// The montage tracker values saved before a client replay.
  FGMC_MontageTracker CL_SavedPreReplayMontageTracker{};

public:

  /// Allows for setting a delegate that will be called when a new montage is starting.
  ///
  /// @param        InOnMontageStart    The delegate to set.
  /// @param        MontageTracker      The montage tracker associated with the montage.
  /// @returns      void
  virtual void SetMontageStartDelegate(FGMC_OnMontageStart& InOnMontageStart, const FGMC_MontageTracker& InMontageTracker);

  /// Allows for setting a delegate that will be called when the active montage has finished blending in.
  ///
  /// @param        InOnMontageBlendIn    The delegate to set.
  /// @param        MontageTracker        The montage tracker associated with the montage.
  /// @returns      void
  virtual void SetMontageBlendInDelegate(FGMC_OnMontageBlendInComplete& InOnMontageBlendIn, const FGMC_MontageTracker& InMontageTracker);

  /// Allows for setting a delegate that will be called when the active montage has started blending out.
  ///
  /// @param        InOnMontageBlendOut    The delegate to set.
  /// @param        MontageTracker         The montage tracker associated with the montage.
  /// @returns      void
  virtual void SetMontageBlendOutDelegate(FGMC_OnMontageBlendOutBegin& InOnMontageBlendOut, const FGMC_MontageTracker& InMontageTracker);

  /// Allows for setting a delegate that will be called when a notify was triggered on the active montage.
  ///
  /// @param        InOnMontageNotifyBegin    The delegate to set.
  /// @param        MontageTracker            The montage tracker associated with the montage.
  /// @returns      void
  virtual void SetMontageNotifyBeginDelegate(FGMC_OnMontageNotifyBegin& InOnMontageNotifyBegin, const FGMC_MontageTracker& InMontageTracker);

  /// Allows for setting a delegate that will be called when a notify state has ended on the active montage.
  ///
  /// @param        InOnMontageNotifyEnd    The delegate to set.
  /// @param        MontageTracker          The montage tracker associated with the montage.
  /// @returns      void
  virtual void SetMontageNotifyEndDelegate(FGMC_OnMontageNotifyEnd& InOnMontageNotifyEnd, const FGMC_MontageTracker& InMontageTracker);

  /// Allows for setting a delegate that will be called when the active montage has completed.
  ///
  /// @param        InOnMontageComplete    The delegate to set.
  /// @param        MontageTracker         The montage tracker associated with the montage.
  /// @returns      void
  virtual void SetMontageCompleteDelegate(FGMC_OnMontageComplete& InOnMontageComplete, const FGMC_MontageTracker& InMontageTracker);

  /// Returns the scaling factor applied to any animation root motion translation on this pawn.
  ///
  /// @returns      float    The scaling factor for animation root motion.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetAnimRootMotionTranslationScale() const;

  /// Sets the scaling factor applied to any animation root motion translation on this pawn.
  ///
  /// @param        Scale    The scaling factor to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetAnimRootMotionTranslationScale(float Scale = 1.f);

  /// Whether we currently have any root motion to consider.
  ///
  /// @returns      bool    True if we have root motion to consider, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool HasRootMotion() const;

  /// Returns the GMC meta data of the passed montage if present.
  ///
  /// @param        Montage                            The montage to get the meta data for.
  /// @returns      UGMC_RootMotionMontageMetaData*    The GMC meta data of the passed montage if any, nullptr otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure = true, Category = "General Movement Component")
  virtual UGMC_MontageMetaData* GetGMCMontageMetaData(UAnimMontage* Montage) const;

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Helper struct to track data from montages for prediction and replication purposes.
  FGMC_MontageTracker MontageTracker{};

protected:

  /// Updates montage data and applies root motion from animations if required.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void MontageUpdate(float DeltaSeconds);

  /// Returns the root motion extraction meta data of the passed montage.
  ///
  /// @param        Montage                              The montage to get the meta data for.
  /// @returns      FGMC_RootMotionExtractionSettings    The extraction meta data of the passed montage. Contains default settings if the passed montage has no
  ///                                                    GMC meta data (or nullptr was passed).
  virtual FGMC_RootMotionExtractionSettings GetRootMotionExtractionMetaData(UAnimMontage* Montage) const;

  /// Returns the root motion velocity meta data of the passed montage.
  ///
  /// @param        Montage                            The montage to get the meta data for.
  /// @returns      FGMC_RootMotionVelocitySettings    The velocity meta data of the passed montage. Contains default settings if the passed montage has no GMC
  ///                                                  meta data (or nullptr was passed).
  virtual FGMC_RootMotionVelocitySettings GetRootMotionVelocityMetaData(UAnimMontage* Montage) const;

  /// Returns the simulation meta data of the passed montage.
  ///
  /// @param        Montage                    The montage to get the meta data for.
  /// @returns      FGMC_SimulationSettings    The simulation meta data of the passed montage. Contains default settings if the passed montage has no GMC meta
  ///                                          data (or nullptr was passed).
  virtual FGMC_SimulationSettings GetSimulationMetaData(UAnimMontage* Montage) const;

  /// Applies processing to the extracted root motion parameters based on montage meta data.
  ///
  /// @param        MontageInstance          The montage instance for the currently active montage (with extra data calculated).
  /// @param        InOutRootMotionParams    The extracted root motion parameters.
  /// @param        ExtractionSettings       The root motion extraction settings for the current montage.
  /// @param        DeltaSeconds             The delta time to use.
  /// @returns      void
  virtual void ProcessRootMotionMetaData(
    const FGMC_AnimMontageInstance& MontageInstance,
    FRootMotionMovementParams& InOutRootMotionParams,
    const FGMC_RootMotionExtractionSettings& ExtractionSettings,
    float DeltaSeconds
  );

  /// Performs processing of the root motion before the final velocity is calculated. The passed root motion is still in local space at this point and must be
  /// converted to world space before the function returns.
  ///
  /// @param        MontageInstance          The montage instance for the currently active montage (with extra data calculated).
  /// @param        InOutRootMotionParams    The extracted root motion parameters.
  /// @param        DeltaSeconds             The delta time to use.
  /// @returns      void
  virtual void PreProcessRootMotion(const FGMC_AnimMontageInstance& MontageInstance, FRootMotionMovementParams& InOutRootMotionParams, float DeltaSeconds);

  /// Applies rotation from animation root motion to the updated component. The pawn position will only be adjusted to account for collisions when rotating
  /// around the yaw axis. Roll- and pitch-rotations will be set if they won't cause any blocking collisions, but no adjustment to the pawn position will be
  /// made.
  ///
  /// @param        ExtractionSettings    The root motion extraction settings for the current montage.
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  virtual void ApplyAnimRootMotionRotation(const FGMC_RootMotionExtractionSettings& ExtractionSettings, float DeltaSeconds);

  /// Calculates the velocity from animation root motion.
  ///
  /// @param        ExtractionSettings    The root motion extraction settings for the current montage.
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      void
  virtual void CalculateAnimRootMotionVelocity(const FGMC_RootMotionExtractionSettings& ExtractionSettings, float DeltaSeconds);

  /// Handles events triggered from montages.
  ///
  /// @param        MontageTracker              The montage tracker associated with active montages.
  /// @param        bFinishedBlendIn            Whether the blend in was completed this update.
  /// @param        bStartedBlendOut            Whether the blend out has started this update.
  /// @param        MontageNotifyBeginEvents    All montage notify events that were triggered this update.
  /// @param        MontageNotifyEndEvents      All montage notify state events that ended this update.
  /// @param        DeltaSeconds                The current move delta time.
  /// @returns      void
  virtual void CallMontageEvents(
    FGMC_MontageTracker& InOutMontageTracker,
    bool bFinishedBlendIn,
    bool bStartedBlendOut,
    const TArray<const FAnimNotifyEvent*>& MontageNotifyBeginEvents,
    const TArray<const FAnimNotifyEvent*>& MontageNotifyEndEvents,
    float DeltaSeconds
  );

  /// Changes the visual montage state in the anim instance according to the updated montage tracker values after a client replay.
  ///
  /// @param        PreReplayMontageTracker    The saved client montage tracker values before adopting the server state for replay.
  /// @returns      void
  virtual void CL_CheckMontageStatusAfterReplay(const FGMC_MontageTracker& PreReplayMontageTracker);

  /// Simulates a montage for an interpolated pawn.
  ///
  /// @param        Mesh                     The skeletal mesh of the owning pawn.
  /// @param        MontageToPlay            The montage to play.
  /// @param        DeltaSeconds             The current frame delta time.
  /// @param        StartPosition            The position to start the montage at.
  /// @param        PlayRate                 The rate to play the montage at.
  /// @param        StartSection             The section to start the montage at (will nullify the start position if set).
  /// @param        bPaused                  Whether the montage is currently paused.
  /// @param        bCancelSelfOnBlendIn     Whether to cancel the same active montage if it is currently blending in (default blend in time).
  /// @param        bCancelSelfOnBlendOut    Whether to cancel the same active montage if it is currently blending out (default blend out time).
  /// @param        CancelMinPositionDiff    The min difference required between the start position and the active position to cancel the active montage.
  /// @param        MinRemainingRatio        How much of the passed montage (in percent) still has to be left with regard to the start position for it to play.
  /// @param        CompletionTimeout        How long a montage that was just completed (but not canceled) should time out before it can be played again.
  /// @param        CompletionTolerance      How long the active montage will continue playing when no new valid montage reference is being passed.
  /// @param        StopBlendTime            The blend out time to use when the completion tolerance was exceeded (default blend out time if -1).
  /// @param        bStopAllMontages         Whether to stop all other montages when the passed montage is played.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void PlayMontageSimulated(
    USkeletalMeshComponent* Mesh,
    UAnimMontage* MontageToPlay,
    float DeltaSeconds,
    float StartPosition = 0.f,
    float PlayRate = 1.f,
    FName StartSection = NAME_None,
    bool bPaused = false,
    bool bCancelSelfOnBlendIn = false,
    bool bCancelSelfOnBlendOut = true,
    float CancelMinPositionDiff = 0.2f,
    float MinRemainingRatio = 0.1f,
    float CompletionTimeout = 0.f,
    float CompletionTolerance = 0.2f,
    float StopBlendTime = -1.f,
    bool bStopAllMontages = true
  );

  /// Handles the setup logic for playing simulated montages.
  ///
  /// @param        Mesh                     The skeletal mesh of the owning pawn.
  /// @param        MontageToPlay            The montage to play.
  /// @param        DeltaSeconds             The current frame delta time.
  /// @param        StartPosition            The position to start the montage at.
  /// @param        bPaused                  Whether the montage is currently paused.
  /// @param        bCancelSelfOnBlendIn     Whether to cancel the same active montage if it is currently blending in (default blend in time).
  /// @param        bCancelSelfOnBlendOut    Whether to cancel the same active montage if it is currently blending out (default blend out time).
  /// @param        CancelMinPositionDiff    The min difference required between the start position and the active position to cancel the active montage.
  /// @param        MinRemainingRatio        How much of the passed montage (in percent) still has to be left with regard to the start position for it to play.
  /// @param        CompletionTimeout        How long a montage that was just completed (but not canceled) should time out before it can be played again.
  /// @param        CompletionTolerance      How long the active montage will continue playing when no new valid montage reference is being passed.
  /// @param        StopBlendTime            The blend out time to use when the completion tolerance was exceeded (default blend out time if -1).
  /// @returns      bool                     True if the montage should be played, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool PrePlayMontageSimulated(
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
  );

  /// Allows for post-processing of the extracted root motion velocity.
  ///
  /// @param        RootMotionVelocity    The extracted root motion velocity.
  /// @param        DeltaSeconds          The delta time to use.
  /// @returns      FVector               The final root motion velocity to use.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  FVector PostProcessAnimRootMotionVelocity(const FVector& RootMotionVelocity, float DeltaSeconds);
  virtual FVector PostProcessAnimRootMotionVelocity_Implementation(const FVector& RootMotionVelocity, float DeltaSeconds) { return RootMotionVelocity; }

  /// Called when a new montage starts playing. This event is save to use for gameplay in a networked context and does not depend on any animation data.
  ///
  /// @param        Montage         The associated montage.
  /// @param        Position        The current montage position.
  /// @param        PlayRate        The current montage play rate.
  /// @param        bInterrupted    Whether the new montage interrupted an active one.
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  virtual void OnMontageStarted(UAnimMontage* Montage, float Position, float PlayRate, bool bInterrupted, float DeltaSeconds) {}

  /// Called when the active montage has reached the default blend in time. This event is save to use for gameplay in a networked context and does not depend on
  /// any animation data.
  ///
  /// @param        Montage         The associated montage.
  /// @param        Position        The current montage position.
  /// @param        PlayRate        The current montage play rate.
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  virtual void OnMontageBlendIn(UAnimMontage* Montage, float Position, float PlayRate, float DeltaSeconds) {}

  /// Called when the active montage has reached the default blend out time. This event is save to use for gameplay in a networked context and does not depend
  /// on any animation data.
  ///
  /// @param        Montage         The associated montage.
  /// @param        Position        The current montage position.
  /// @param        PlayRate        The current montage play rate.
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  virtual void OnMontageBlendOut(UAnimMontage* Montage, float Position, float PlayRate, float DeltaSeconds) {}

  /// Called when the active montage has finished (i.e. played out fully to the end). This event is save to use for gameplay in a networked context and does not
  /// depend on any animation data.
  ///
  /// @param        Montage         The associated montage.
  /// @param        Position        The current montage position.
  /// @param        PlayRate        The current montage play rate.
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  virtual void OnMontageCompleted(UAnimMontage* Montage, float Position, float PlayRate, float DeltaSeconds) {}

  /// Called when a notify was triggered on the active montage. This event is save to use for gameplay in a networked context and does not depend on any
  /// animation data.
  ///
  /// @param        NotifyName      The name of the notify that was triggered.
  /// @param        Montage         The associated montage.
  /// @param        Position        The current montage position.
  /// @param        PlayRate        The current montage play rate.
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  virtual void OnMontageNotifyBegin(const FName& NotifyName, UAnimMontage* Montage, float Position, float PlayRate, float DeltaSeconds) {}

  /// Called when a notify state has ended on the active montage. This event is save to use for gameplay in a networked context and does not depend on any
  /// animation data.
  ///
  /// @param        NotifyName      The name of the notify that ended.
  /// @param        Montage         The associated montage.
  /// @param        Position        The current montage position.
  /// @param        PlayRate        The current montage play rate.
  /// @param        DeltaSeconds    The current move delta time.
  /// @returns      void
  virtual void OnMontageNotifyEnd(const FName& NotifyName, UAnimMontage* Montage, float Position, float PlayRate, float DeltaSeconds) {}

  UPROPERTY(Transient)
  /// The root motion parameters extracted from the currently active root motion montage (if any).
  FRootMotionMovementParams RootMotionParams{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Whether we have any root motion to consider for the current update.
  bool bHasRootMotion{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Scaling factor applied to animation root motion translation on this pawn.
  float AnimRootMotionTranslationScale{1.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The previously played simulated montage.
  TObjectPtr<UAnimMontage> PreviousMontageSimulated{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Tracks the elapsed time to enforce the configured simulated montage timeout.
  float BlockMontageTimerSimulated{0.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Tracks the elapsed time to enforce the configured simulated completion tolerance.
  float EndMontageTimerSimulated{0.f};

///=============================================================================================================================================================
/// Simulation

protected:

  /// Called when a simulated pawn was updated. This is the preferred entry point for subclasses to implement custom logic for simulation. All functions with
  /// the "Simulated" postfix are called from the simulation tick and should only be used for non-gameplay critical logic (effects, animations, etc.). They are
  /// not called on a dedicated server.
  ///
  /// @param        DeltaSeconds    The current delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "General Movement Component")
  void MovementUpdateSimulated(float DeltaSeconds);
  virtual void MovementUpdateSimulated_Implementation(float DeltaSeconds) {}

  /// Returns the movement mode the pawn had during the previous simulation tick.
  ///
  /// @returns      EGMC_MovementMode    The previous movement mode.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual EGMC_MovementMode GetPreviousMovementModeSimulated() const;

  /// Returns the control rotation the pawn had during the previous simulation tick.
  ///
  /// @returns      FRotator    The previous control rotation in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetPreviousControlRotationSimulated() const;

  /// Called when the movement mode changes from airborne to grounded during simulation. This is called after MovementUpdateSimulated and
  /// OnMovementModeChangedSimulated (if applicable) have run.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "General Movement Component")
  void OnLandedSimulated();
  virtual void OnLandedSimulated_Implementation() {}

  /// Called when the movement mode changes during simulation. This is called before OnLandedSimulated (if applicable) but after MovementUpdateSimulated has
  /// run.
  ///
  /// @param        PreviousMovementMode    The previous movement mode we changed from.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "General Movement Component")
  void OnMovementModeChangedSimulated(EGMC_MovementMode PreviousMovementMode);
  virtual void OnMovementModeChangedSimulated_Implementation(EGMC_MovementMode PreviousMovementMode) {}

  /// Smooths out the transition between different actor bases when using relative based movement.
  ///
  /// @param        TargetIndex       The smoothing target index in the move history.
  /// @param        bExtrapolating    Whether we are going to extrapolate.
  /// @param        InterpStates      The type of interpolation states that are going to be used.
  /// @returns      void
  virtual void SmoothActorBaseChangeSimulated(int32 TargetIndex, bool bExtrapolating, EGMC_InterpolationStates InterpStates);

  /// Called when the actor base has changed during simulation.
  ///
  /// @param        NewBase         The current base of the pawn (null means no base).
  /// @param        PreviousBase    The previous base of the pawn (null means no base).
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void OnActorBaseChangedSimulated(UPrimitiveComponent* NewBase, UPrimitiveComponent* PreviousBase);
  virtual void OnActorBaseChangedSimulated_Implementation(UPrimitiveComponent* NewBase, UPrimitiveComponent* PreviousBase) {}

private:

  /// The movement mode of a simulated pawn during the previous simulation tick.
  EGMC_MovementMode PreviousMovementModeSimulated{};

  /// The control rotation of a simulated pawn during the previous simulation tick.
  FRotator PreviousControlRotationSimulated{0.};

///=============================================================================================================================================================
/// AI

public:

  ///~ Begin UNavMovementComponent Interface
  void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
  void RequestPathMove(const FVector& MoveInput) override;
  bool CanStartPathFollowing() const override;
  bool CanStopPathFollowing() const override;
  float GetPathFollowingBrakingDistance(float MaxSpeed) const override;
  ///~ End UNavMovementComponent Interface

  UPROPERTY(BlueprintReadOnly, Transient, Category = "General Movement Component")
  /// The velocity requested by path following for direct movement.
  FVector RequestedVelocity{0.};

protected:

  /// Whether max speed was requested by path following.
  bool bRequestedMoveWithMaxSpeed{false};

  /// Helper data for nav mesh walking.
  FGMC_NavMeshWalkingAux NavMeshWalkingAux{};

  /// Adjust the nav movement output according to the nav agent properties.
  ///
  /// @param        NavOutput    The output from the nav movement component to be adjusted (the move input or a requested velocity).
  /// @returns      FVector      The adjusted vector.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FVector NavAgentPropsAdjustment(const FVector& NavOutput);
  virtual FVector NavAgentPropsAdjustment_Implementation(const FVector& NavOutput);

  /// Calculates the current braking distance of the pawn for path following.
  ///
  /// @param        MaxSpeed        The current max speed of the pawn.
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      float           The calculated braking distance.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  float CalculatePathFollowingBrakingDistance(float MaxSpeed, float DeltaSeconds) const;
  virtual float CalculatePathFollowingBrakingDistance_Implementation(float MaxSpeed, float DeltaSeconds) const;

  /// Whether nav mesh walking is being used during grounded movement.
  ///
  /// @returns      bool    True if nav mesh walking is currently being used, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool IsNavMeshWalking() const;
  virtual bool IsNavMeshWalking_Implementation() const;

  /// Called when the projection of the current target location onto the nav mesh failed during nav mesh walking.
  ///
  /// @param        TargetLocation             The current target location that could not be projected.
  /// @param        MoveDelta                  The move delta that should have been applied.
  /// @param        LastValidTargetLocation    The last successfully projected target location (if available, zero vector otherwise).
  /// @param        SearchExtent               The search extent used during the failed projection.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void OnNavMeshProjectionFailed(
    const FVector& TargetLocation,
    const FVector& MoveDelta,
    const FVector& LastValidTargetLocation,
    const FVector& SearchExtent
  );
  virtual void OnNavMeshProjectionFailed_Implementation(
    const FVector& TargetLocation,
    const FVector& MoveDelta,
    const FVector& LastValidTargetLocation,
    const FVector& SearchExtent
  );

  /// Tries to project the passed target location onto the nav mesh. Adjusts the move delta to prevent the pawn from going off the nav mesh if necessary.
  ///
  /// @param        InOutMoveDelta          The current move delta (may get adjusted).
  /// @param        OutTargetNavLocation    The projected target location (if successful).
  /// @param        SearchExtent            The search extent to use for projection.
  /// @param        MaxRetries              Max number of retries to perform if the projection fails.
  /// @param        NavData                 The navigation data to use.
  /// @param        ToleranceXY             Tries to keep the XY distance between the projected and original within this tolerance.
  /// @returns      bool                    True if the projection was successful, false otherwise. A successful projection may still exceed the XY tolerance.
  virtual bool ProjectPointOntoNavMesh(
    FVector& InOutMoveDelta,
    FNavLocation& OutTargetNavLocation,
    const FVector& SearchExtent,
    int32 MaxRetries,
    double ToleranceXY,
    const class INavigationDataInterface* NavData
  );

public:

  ///~ Begin IRVOAvoidanceInterface
  void SetRVOAvoidanceUID(int32 UID) override;
  int32 GetRVOAvoidanceUID() override;
  void SetRVOAvoidanceWeight(float Weight) override;
  float GetRVOAvoidanceWeight() override;
  FVector GetRVOAvoidanceOrigin() override;
  float GetRVOAvoidanceRadius() override;
  float GetRVOAvoidanceHeight() override;
  float GetRVOAvoidanceConsiderationRadius() override;
  FVector GetVelocityForRVOConsideration() override;
  int32 GetAvoidanceGroupMask() override;
  int32 GetGroupsToAvoidMask() override;
  int32 GetGroupsToIgnoreMask() override;
  void SetAvoidanceGroupMask(int32 GroupFlags) override;
  void SetGroupsToAvoidMask(int32 GroupFlags) override;
  void SetGroupsToIgnoreMask(int32 GroupFlags) override;
  ///~ End IRVOAvoidanceInterface

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetAvoidanceGroupMask(const FNavAvoidanceMask& GroupMask);

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetGroupsToAvoidMask(const FNavAvoidanceMask& GroupMask);

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetGroupsToIgnoreMask(const FNavAvoidanceMask& GroupMask);

  /// Locks the avoidance velocity for the specified duration.
  ///
  /// @param        Avoidance    The avoidance manager.
  /// @param        Duration     Duration of the lock.
  /// @returns      void
  virtual void SetAvoidanceVelocityLock(class UAvoidanceManager* Avoidance, float Duration);

  /// Returns the navigation data for this pawn.
  ///
  /// @returns      INavigationDataInterface*    Pointer to the navigation data (if present).
  virtual INavigationDataInterface* GetNavData() const;

  /// Returns the last successfully projected nav location used as a target during nav mesh walking.
  ///
  /// @returns      FNavLocation    The last known good target nav location (zero vector if not set).
  virtual FNavLocation GetLastValidTargetNavLocation() const;

  /// Returns the last successfully projected location used as a target during nav mesh walking.
  ///
  /// @returns      FVector    The last known good target location (zero vector if not set).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component", DisplayName = "Get Last Valid Target Nav Location")
  virtual FVector K2_GetLastValidTargetNavLocation() const;

  /// Returns whether the pawn was nav mesh walking previously.
  ///
  /// @returns      bool    True if the pawn was anv mesh walking during the last movement update, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool WasNavMeshWalkingLastUpdate() const;

  /// Returns a vector pointing to the nav mesh based on the last known valid target location.
  ///
  /// @param        Location    The location to check.
  /// @returns      FVector     A vector pointing to the nav mesh for the passed location (zero vector if no saved valid location is available).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetDistanceToNavMesh(const FVector& Location) const;

  /// Resets the node ref and location for the last known valid target location from nav mesh walking.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ResetLastValidTargetNavLocation();

protected:

  UPROPERTY(BlueprintReadOnly, Transient, Category = "General Movement Component")
  /// UID generated by the avoidance manager.
  int32 AvoidanceUID{0};

  /// Allows for user-defined post-processing of the calculated avoidance velocity.
  ///
  /// @param        NewVelocity    In: the calculated avoidance velocity. Out: the post-processed avoidance velocity.
  /// @returns      void
  virtual void PostProcessAvoidanceVelocity(FVector& NewVelocity) {}

  /// Whether RVO avoidance should be computed for this pawn.
  ///
  /// @returns      bool    True if this pawn is a server bot and has avoidance enabled.
  virtual bool ShouldComputeAvoidance();

  /// Updates the current velocity with the avoidance velocity.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  virtual void CalculateAvoidanceVelocity(float DeltaSeconds);

  /// Resets all data members involved in avoidance calculations to their default values.
  ///
  /// @returns      void
  virtual void ResetAvoidanceData();

private:

  /// Whether we are currently using avoidance.
  bool bIsUsingAvoidanceInternal{false};

  /// Whether the avoidance was already updated this frame.
  bool bWasAvoidanceUpdated{false};

  /// The remaining time of the avoidance velocity lock.
  float AvoidanceLockTimer{0.f};

  /// Forced avoidance velocity used when the there is an avoidance lock duration.
  FVector AvoidanceLockVelocity{0.};

  /// Registers the movement component with the avoidance manager.
  ///
  /// @returns      void
  void EnableRVOAvoidance();

  /// Checks whether enabling/disabling of avoidance was requested by the user.
  ///
  /// @returns      void
  void CheckAvoidance();

  /// Updates data in the RVO avoidance manager at the end of the tick if necessary.
  ///
  /// @returns      void
  void UpdateAvoidance();

///=============================================================================================================================================================
/// Properties

public:

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
  TObjectPtr<UInputAction> DirectionalInput{nullptr};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
  TObjectPtr<UInputAction> ViewInput{nullptr};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// The amount of acceleration applied to the pawn by the controller's directional input when grounded.
  float InputAccelerationGrounded{6000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// The amount of acceleration applied to the pawn by the controller's directional input when airborne.
  float InputAccelerationAirborne{1800.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// The amount of acceleration applied to the pawn by the controller's directional input when in a fluid volume.
  float InputAccelerationBuoyant{1000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// How much deceleration is applied to the pawn when grounded.
  float BrakingDecelerationGrounded{3000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// How much deceleration is applied to the pawn when airborne.
  float BrakingDecelerationAirborne{200.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// How much deceleration is applied to the pawn when in a fluid volume.
  float BrakingDecelerationBuoyant{1000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// The max speed the pawn should have. When this limit is exceeded, the appropriate over-max-speed-deceleration will be applied.
  float MaxDesiredSpeed{800.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// How much deceleration is applied when the pawn is grounded and is exceeding the max desired speed.
  float OverMaxSpeedDecelerationGrounded{10000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// How much deceleration is applied when the pawn is airborne and is exceeding the max desired speed. In negative Z direction the max speed is determined by
  /// the terminal velocity of the current physics volume.
  float OverMaxSpeedDecelerationAirborne{1200.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Tempo", meta = (ClampMin = "0", UIMin = "0"))
  /// How much deceleration is applied when the pawn is in a fluid and is exceeding the max desired speed.
  float OverMaxSpeedDecelerationBuoyant{3000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0", UIMin = "0", UIMax = "1"))
  /// If greater than 0 airborne input acceleration will be applied in a different way to allow for more precise maneuvering. The fall control is a multiplier
  /// for the configured value of InputAccelerationAirborne and will determine how much control the pawn can exert over its XY direction while in the air and
  /// under the influence of gravity. The fall control has no effect on the deceleration behaviour (which is determined by BrakingDecelerationAirborne and
  /// OverMaxSpeedDecelerationAirborne).
  float FallControl{0.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
  /// When true, the pawn will smoothly rotate around the yaw axis to face the current input direction.
  bool bOrientToInputDirection{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
  /// When true, the pawn will smoothly rotate around the yaw axis to face the current control rotation direction. This setting takes precedence over
  /// bOrientToInputDirection if both options are set.
  bool bOrientToControlRotationDirection{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0", UIMin = "0"))
  /// When orienting the pawn's rotation to the input or control rotation direction, this is the rate of rotation.
  float RotationRate{650.f};

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Operation", meta = (ClampMin = "0", ClampMax = "90", UIMin = "0", UIMax = "75"))
  /// Max angle in degrees of a surface the pawn can still walk on. Should only be set through SetWalkableFloorAngle.
  float WalkableFloorAngle{45.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
  /// If true, we will maintain the regular ground speed when walking up or down slopes by rescaling the location delta. If false, no scaling will be applied
  /// and the pawn will effectively move faster on slopes due to the additional Z component in the location delta.
  bool bRescaleSlopeDelta{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0", UIMin = "0", UIMax = "100"))
  /// The maximum height the pawn can step up to while grounded.
  float MaxStepUpHeight{50.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0", UIMin = "0", UIMax = "100"))
  /// When walking down a slope or off a ledge, the pawn will remain grounded if the floor underneath is closer than this threshold.
  float MaxStepDownHeight{50.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "-1", UIMin = "500", UIMax = "2000"))
  /// The max upward velocity the pawn should be able to absorb while grounded. While the Z velocity exceeds this value the pawn will not be able to land on the
  /// ground. Set to -1 to disable this functionality completely.
  float MaxGroundedVelocityZ{800.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
  /// Whether the pawn is able to walk off ledges which exceed the max step down height when grounded.
  bool bCanWalkOffLedges{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta =
    (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", EditCondition = "bCanWalkOffLedges"))
  /// When standing on a ledge the pawn will fall off if its collision shape perches further beyond the end of the ledge than the set threshold allows. This is
  /// percentage based with the center of the collision being 0 (i.e. the pawn will fall off as early as possible) and the outer boundary of the collision being
  /// 1 (i.e. the pawn will fall off as late as possible). For box collisions the threshold is internally treated as either 1 (if >= 0.5) or 0 (if < 0.5).
  float LedgeFallOffThreshold{0.5f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0.0001", ClampMax = "1", UIMin = "0.1", UIMax = "1"))
  /// How deeply we need to be immersed in a fluid to enter the buoyant movement state with 1 being fully immersed in the fluid volume. The fluid volume should
  /// have the "Physics on Contact" flag enabled for this to work correctly.
  float BuoyantStateMinImmersion{0.8f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0.0001", ClampMax = "1", UIMin = "0.1", UIMax = "1"))
  /// How deeply we need to be immersed in a fluid while still grounded to experience a slow down from the fluid volume. Will be clamped if the value exceeds
  /// the buoyant state min immersion. The fluid volume should have the "Physics on Contact" flag enabled for this to work correctly.
  float PartialImmersionThreshold{0.5f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0", UIMin = "0"))
  /// Factor that determines how much slow down we experience while partially immersed in a fluid during grounded movement.
  float PartialImmersionSlowDown{10.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation", meta = (ClampMin = "0", UIMin = "0"))
  /// When in a buoyant state this is the minimum upward speed (i.e. positive Z-velocity) the pawn needs to have to be able to exit the fluid volume. Disabling
  /// this functionality (by setting the value to 0) may cause the pawn to repeatedly switch in and out of the buoyant state when close to the water line.
  float FluidMinExitSpeed{20.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
  /// If true, the pawn will use the nav mesh directly for movement while grounded.
  bool bUseNavMeshWalking{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
  /// Determines how movement is handled when based on another component.
  FGMC_BasedMovement BasedMovement{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Physics", meta = (ClampMin = "0", UIMin = "0"))
  /// Scale the effects of gravity acting on this pawn by this factor. Upward gravity is not supported.
  float GravityScale{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Physics", meta = (ClampMin = "0", UIMin = "0", UIMax = "1"))
  /// Friction of the ground i.e. how slippery the surface is. Only applies to grounded movement.
  float GroundFriction{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Physics", meta = (UIMin = "-0.5", UIMax = "0.5"))
  /// Fluid buoyancy.
  float Buoyancy{0.05f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// When true, input modification is ignored and we always accelerate up to the full max speed even if the analog stick is not at full tilt.
  bool bIgnoreInputModifier{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay, meta =
    (ClampMin = "1", UIMin = "10", EditCondition = "!bIgnoreInputModifier"))
  /// The walking speed that we should accelerate up to when walking at minimum analog stick tilt.
  float MinAnalogWalkSpeed{100.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, the contact normal of a hit will also be considered to check whether a floor is walkable when landing. This will often allow the pawn to land on
  /// hard edges that usually produce opposing impact normals.
  bool bLandOnEdges{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, stepping up obstacles will be more efficient, but results may be inferior in some situations. Works best with a low max step up height.
  bool bSimplifyStepUp{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If false, grounded movement will be disabled when there's no gravity acting on the pawn.
  bool bGroundingWithoutGravity{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, the pawn will not be able to accelerate up unwalkable slopes.
  bool bPreventSlopeBoosting{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, safe rotations will be used during the internal movement physics. Only relevant when a flat capsule or box is used as root collision.
  bool bUseSafeRotations{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "10"))
  /// How far downwards the trace should go when updating the floor.
  float FloorTraceLength{500.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0"))
  /// Scaling factor applied to the root collision extent when tracing for the floor.
  float FloorTraceShapeScale{0.99f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0"))
  /// How far the pawn must move to invalidate the current floor data. Setting this to 0 will cause the floor to be updated continuously.
  float FloorUpdateTolerance{0.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// Whether the configured floor update tolerance should be considered during nav mesh walking.
  bool bUseFloorUpdateToleranceForNavMeshWalking{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0"))
  /// Max distance allowed for depenetration when moving out of anything but pawns.
  float MaxDepenetrationWithGeometry{100.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0"))
  /// Max distance allowed for depenetration when moving out of other pawns.
  float MaxDepenetrationWithPawn{100.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, the skeletal mesh will be smoothed after simulation.
  bool bSmoothMesh{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, the pose of the skeletal mesh will never be ticked for kinematic movement on a dedicated server. This is different from merely setting the
  /// visibility based tick on the mesh to "OnlyTickPoseWhenRendered", which will still tick the pose for root motion montages.
  bool bDisablePoseTickOnDedicatedServer{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// The settings to use when networking montages.
  FGMC_MontageReplication MontageReplication{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction")
  /// Whether the pawn should interact with physics objects in the world.
  bool bEnablePhysicsInteraction{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (UIMin = "0", EditCondition = "bEnablePhysicsInteraction"))
  /// Multiplier for the force that is applied to physics objects that are touched by the pawn.
  float TouchForceScale{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (ClampMin = "0", UIMin = "0", EditCondition = "bEnablePhysicsInteraction"))
  /// The minimum force applied to physics objects touched by the pawn.
  float MinTouchForce{0.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (ClampMin = "0", UIMin = "0", EditCondition = "bEnablePhysicsInteraction"))
  /// The maximum force applied to physics objects touched by the pawn.
  float MaxTouchForce{250.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (EditCondition = "bEnablePhysicsInteraction"))
  /// If true, TouchForceScale is applied per kilogram of mass of the affected object.
  bool bScaleTouchForceToMass{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (UIMin = "0", EditCondition = "bEnablePhysicsInteraction"))
  /// Multiplier for the force that is applied when the player collides with a blocking physics object.
  float PushForceScale{750000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (UIMin = "0", EditCondition = "bEnablePhysicsInteraction"))
  /// Multiplier for the initial impulse force applied when the pawn bounces into a blocking physics object.
  float InitialPushForceScale{500.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (EditCondition = "bEnablePhysicsInteraction"))
  /// If true, PushForceScale is applied per kilogram of mass of the affected object.
  bool bScalePushForceToMass{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (EditCondition = "bEnablePhysicsInteraction"))
  /// If true, the applied push force will try to get the touched physics object to the same velocity as the pawn, not faster. This will only ever scale the
  /// force down and will never apply more force than calculated with regard to PushForceScale.
  bool bScalePushForceToVelocity{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (EditCondition = "bEnablePhysicsInteraction"))
  /// If true, the push force location is adjusted using PushForceZOffsetFactor. If false, the impact point is used.
  bool bUsePushForceZOffset{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (UIMin = "-1", UIMax = "1", EditCondition = "bEnablePhysicsInteraction && bUsePushForceZOffset"))
  /// Z-offset for the location the force is applied to the touched physics object (0 = center, 1 = top, -1 = bottom).
  float PushForceZOffsetFactor{-0.75f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (UIMin = "0", EditCondition = "bEnablePhysicsInteraction"))
  /// Multiplier for the gravity force applied to physics objects the pawn is walking on.
  float DownwardForceScale{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Interaction", meta =
    (ClampMin = "0", UIMin = "0", EditCondition = "bEnablePhysicsInteraction"))
  /// The force applied constantly per kilogram of mass of the pawn to all overlapping components.
  float RepulsionForce{2.5f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVO Avoidance")
  /// Whether avoidance should be used for bots.
  bool bUseAvoidance{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVO Avoidance")
  /// Indicates RVO behavior.
  float AvoidanceWeight{0.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVO Avoidance")
  /// The radius around the pawn for which to consider avoidance of other agents.
  float AvoidanceConsiderationRadius{500.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVO Avoidance")
  /// This pawn's avoidance group.
  FNavAvoidanceMask AvoidanceGroup{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVO Avoidance")
  /// This pawn will avoid other agents that belong to one of the groups specified in the mask.
  FNavAvoidanceMask GroupsToAvoid{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVO Avoidance")
  /// This pawn will ignore other agents that belong to one of the groups specified in the mask.
  FNavAvoidanceMask GroupsToIgnore{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMovement", meta = (DisplayAfter = "bUseFixedBrakingDistanceForPaths"))
  /// If true, the magnitude of the requested velocity will be used as max speed for direct bot movement.
  bool bUseRequestedVelocityMaxSpeed{false};
};
