// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCOrganicMovementComponent.h"
#include "InputAction.h"
#include "GMCGoldSrcMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EGMC_JumpMode : uint8
{
  Manual UMETA(ToolTip = "The player can only jump if the jump button is pressed while already on the ground."),
  SemiAuto UMETA(ToolTip = "The player can jump by holding the jump button, but the button must be released between jumps."),
  FullAuto UMETA(ToolTip = "The player can continuously jump by holding the jump button."),
  MAX UMETA(Hidden)
};

/// Movement component loosely based on the movement physics found in classic shooter games such as Quake and Half-Life.
UCLASS(ClassGroup = "Movement", HideCategories = ("Movement|Tempo"), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GMCCORE_API UGMC_GoldSrcMovementCmp : public UGMC_OrganicMovementCmp
{
  GENERATED_BODY()

  friend class UGMC_GoldSrcConfigMenu;

public:

  UGMC_GoldSrcMovementCmp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  ///~ Begin UActorComponent Interface
  void BeginPlay() override;
  ///~ End UActorComponent Interface

  ///~ Begin UMovementComponent Interface
  void HaltMovement() override;
  float GetMaxSpeed() const override;
  ///~ End UMovementComponent Interface

  ///~ Begin UNavMovementComponent Interface
  bool IsCrouching() const override;
  ///~ End UNavMovementComponent Interface

  /// Whether the pawn has just jumped. Only intended as animation trigger, should not be used for movement logic (use OnJumped instead).
  ///
  /// @returns      bool    Whether the pawn has just jumped.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool HasJustJumped() const;

  /// Whether the pawn has just landed on the ground. Only intended as animation trigger, should not be used for movement logic (use OnLanded or
  /// HasJustLanded_Internal instead).
  ///
  /// @returns      bool    Whether the pawn has just landed.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool HasJustLanded() const;

  /// Whether the pawn is currently sprinting.
  ///
  /// @returns      bool    Whether the pawn is sprinting.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsSprinting() const;

  /// Returns the default half height of the pawn's root collision.
  ///
  /// @returns      float    The half height the root collision when the pawn spawned.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float GetDefaultHalfHeight() const;

  /// Returns the half height of the pawn's root collision when fully crouched.
  ///
  /// @returns      float    The half height the root collision when the pawn is fully crouched.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float GetCrouchedHalfHeight() const;

  /// Whether the pawn is fully crouched. Only returns true if the root collision's half height is equal to CrouchedHalfHeight.
  ///
  /// @returns      bool    True if the pawn is currently fully crouched, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsFullyCrouched() const;

  /// Whether the pawn is currently in the process of crouching down. Only returns true if the root collision's half height is smaller than DefaultHalfHeight
  /// and greater than CrouchedHalfHeight, and the crouch button is being pressed.
  ///
  /// @returns      bool    True if the pawn is currently in the process of crouching down, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsCrouchingDown() const;

  /// Whether the pawn is currently in the process of uncrouching. Only returns true if the root collision's half height is greater than CrouchedHalfHeight and
  /// smaller than DefaultHalfHeight, and the crouch button is not being pressed.
  ///
  /// @returns      bool    True if the pawn is currently in the process of uncrouching, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsUncrouching() const;

  /// Whether the pawn is using fly mode.
  ///
  /// @returns      bool    True when currently flying, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsUsingFlyMode() const;

  /// Whether the pawn is using no-clip mode.
  ///
  /// @returns      bool    True when currently using no-clip, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsUsingNoClipMode() const;

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_WantsToSprint{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_WantsToCrouch{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_WantsToJump{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_CanJump{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_JustLandedPostPhysics{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_JustJumped{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_JustLanded{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_IsSprinting{-1};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  int32 BI_MaxSpeed{-1};

protected:

  ///~ Begin UGMC_ReplicationCmp Interface
  void BindReplicationData_Implementation() override;
  void SetupPlayerInputComponent_Implementation(UInputComponent* PlayerInputComponent) override;
  bool SV_OnProxyMoveInitialized_Implementation(FGMC_Move& Move, float DeltaTime, double Timestamp) override;
  bool OnCumulativeMoveInitialized_Implementation(FGMC_PawnState& InputState, EGMC_InterpolationStates SimStates, float DeltaTime, double Timestamp) override;
  ///~ End UGMC_ReplicationCmp Interface

  ///~ Begin UGMC_OrganicMovementCmp Interface
  void PreMovementUpdate_Implementation(float DeltaSeconds) override;
  void ClampToValidValues() override;
  void OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode) override;
  void MovementUpdate_Implementation(float DeltaSeconds) override;
  bool UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds) override;
  void PostPhysicsUpdate_Implementation(float DeltaSeconds) override;
  void PhysicsCustom_Implementation(float DeltaSeconds) override;
  FVector PreProcessInputVector_Implementation(FVector InRawInputVector) override;
  void CalculateVelocity(float DeltaSeconds) override;
  void ApplyInputVelocity(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) override;
  void ApplyDeceleration(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) override;
  void LimitSpeed(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) override;
  float GetInputAcceleration() const override;
  float GetBrakingDeceleration() const override;
  float GetOverMaxSpeedDeceleration() const override;
  void ProcessLanded(const FHitResult& Hit, FGMC_FloorParams& Floor, bool bUpdateFloor, float DeltaSeconds) override;
  void OnLanded_Implementation(const FVector& ImpactVelocity) override;
  void PostMovementUpdate_Implementation(float DeltaSeconds) override;
  float CalculatePathFollowingBrakingDistance_Implementation(float InMaxSpeed, float DeltaSeconds) const;
  ///~ End UGMC_OrganicMovementCmp Interface

///=============================================================================================================================================================
/// Movement Physics

protected:

  /// Custom movement mode aliases.
  constexpr EGMC_MovementMode MovementModeFlying() const { return EGMC_MovementMode::Custom1; }
  constexpr EGMC_MovementMode MovementModeNoClip() const { return EGMC_MovementMode::Custom2; }

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The half height of the pawn's root collision when just spawned.
  float DefaultHalfHeight{0.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The collision type that the root component had when it just spawned.
  TEnumAsByte<ECollisionEnabled::Type> DefaultCollisionType{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// True while the user is pressing the sprint button, false otherwise.
  bool bWantsToSprint{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// True while the user is pressing the crouch button, false otherwise.
  bool bWantsToCrouch{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// True while the user is pressing the jump button, false otherwise.
  bool bWantsToJump{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Flag to determine whether a jump is currently allowed.
  bool bCanJump{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Flag set when the pawn is sprinting.
  bool bIsSprinting{false};

  /// Whether the pawn has just jumped. Only intended as animation trigger, should not be used for logic (use OnJumped instead).
  bool bJustJumped{false};

  /// Whether the pawn has just landed. Since OnLanded can be called from multiple places we need more than just one variable to track the state accurately.
  /// These are set and cleared at different times during move execution. Use HasJustLanded for querying the state with respect to the current context.
  /// bJustLandedPrePhysics and bJustLandedPostPhysicsLock do not need to be bound.
  bool bJustLandedPrePhysics{false};
  bool bJustLandedPostPhysics{false};
  bool bJustLandedPostPhysicsLock{false};
  bool bJustLanded{false};

  /// Starts the sprint action.
  ///
  /// @returns      void
  virtual void StartSprintAction(const FInputActionInstance& InputAction);

  /// Stops the sprint action.
  ///
  /// @returns      void
  virtual void StopSprintAction(const FInputActionInstance& InputAction);

  /// Starts the crouch action.
  ///
  /// @returns      void
  virtual void StartCrouchAction(const FInputActionInstance& InputAction);

  /// Stops the crouch action.
  ///
  /// @returns      void
  virtual void StopCrouchAction(const FInputActionInstance& InputAction);

  /// Starts the jump action.
  ///
  /// @returns      void
  virtual void StartJumpAction(const FInputActionInstance& InputAction);

  /// Stops the jump action.
  ///
  /// @returns      void
  virtual void StopJumpAction(const FInputActionInstance& InputAction);

  /// Implements movement physics similar to those used in GoldSource games.
  ///
  /// @param        WishMove        The directional input of the player.
  /// @param        WishVelocity    The velocity generated from the directional input.
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void PhysicsGoldSrc(const FVector& WishMove, FVector WishVelocity, float DeltaSeconds);

  /// Calculates a new velocity for the pawn when on the ground.
  ///
  /// @param        WishDirection    The direction the player wants to move in.
  /// @param        WishSpeed        The speed the player wants to have based on the input.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void WalkMove(FVector WishDirection, float WishSpeed, float DeltaSeconds);

  /// Calculates a new velocity for the pawn when in the air.
  ///
  /// @param        WishDirection    The direction the player wants to move in.
  /// @param        WishSpeed        The speed the player wants to have based on the input.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void AirMove(FVector WishDirection, float WishSpeed, float DeltaSeconds);

  /// Calculates a new velocity for the pawn when in water.
  ///
  /// @param        WishDirection    The direction the player wants to move in.
  /// @param        WishSpeed        The speed the player wants to have based on the input.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void WaterMove(FVector WishDirection, float WishSpeed, float DeltaSeconds);

  /// Applies acceleration to calculate a new velocity for the pawn.
  ///
  /// @param        WishDirection    The direction the player wants to move in.
  /// @param        WishSpeed        The speed the player wants to have based on the input.
  /// @param        CurrentSpeed     The current speed of the pawn.
  /// @param        DeltaSeconds     The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void Accelerate(FVector WishDirection, float WishSpeed, float CurrentSpeed, float DeltaSeconds);

  /// Applies friction while moving on the ground.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      float           The new speed of the pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float ApplyFloorFriction(float DeltaSeconds);

  /// Returns the friction value to use when moving on the ground.
  ///
  /// @returns      float    The floor friction to use.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetFloorFriction() const;

  /// Applies friction while moving in a fluid.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      float           The new speed of the pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float ApplyWaterFriction(float DeltaSeconds);

  /// Makes the pawn slide along a usually walkable surface if its velocity is high enough.
  ///
  /// @param        MinRequiredZ    The minimum Z of the projected velocity required to slide.
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void Slide(float MinRequiredZ, float DeltaSeconds);

  /// Whether the pawn has just landed on the ground. Can be used for movement logic. For simulated pawns use HasJustLanded.
  ///
  /// @returns      bool    Whether the pawn has just landed.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool HasJustLanded_Internal() const;

  /// Increases the movement speed of the pawn to SprintSpeed if desired.
  ///
  /// @param        bRquestedSprint    Whether the player has pressed the sprint button.
  /// @param        DeltaSeconds       The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void Sprint(bool bRequestedSprint, float DeltaSeconds);
  virtual void Sprint_Implementation(bool bRequestedSprint, float DeltaSeconds);

  /// Whether the pawn is currently allowed to sprint (irrespective of whether the player wants to sprint or not).
  ///
  /// @returns      bool    True if the pawn can currently sprint, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool CanSprint() const;
  virtual bool CanSprint_Implementation() const;

  /// Smoothly shrinks the collision half height towards CrouchedHalfHeight or increases it towards DefaultHalfHeight.
  ///
  /// @param        bRequestedCrouch    Whether the player has pressed the crouch button.
  /// @param        DeltaSeconds        The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void Crouch(bool bRequestedCrouch, float DeltaSeconds);
  virtual void Crouch_Implementation(bool bRequestedCrouch, float DeltaSeconds);

  /// Whether the pawn is currently allowed to crouch (irrespective of whether the player wants to crouch or not).
  ///
  /// @returns      bool    True if the pawn can currently crouch, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool CanCrouch() const;
  virtual bool CanCrouch_Implementation() const;

  /// Whether the pawn is currently allowed to come out of a crouch.
  ///
  /// @returns      bool    True if the pawn can currently uncrouch, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool CanUncrouch() const;
  virtual bool CanUncrouch_Implementation() const;

  /// Handles the jumping logic.
  ///
  /// @param        bRequestedJump    Whether the player has pressed the jump button.
  /// @param        DeltaSeconds      The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void Jump(bool bRequestedJump, float DeltaSeconds);
  virtual void Jump_Implementation(bool bRequestedJump, float DeltaSeconds);

  /// Whether the pawn is currently allowed to jump (irrespective of whether the player wants to jump or not).
  ///
  /// @returns      bool    True if the pawn can currently jump, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool CanJump() const;
  virtual bool CanJump_Implementation() const;

  /// Handles the logic for jumping out of water.
  ///
  /// @param        DeltaSeconds    The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void WaterJump(float DeltaSeconds);
  virtual void WaterJump_Implementation(float DeltaSeconds);

  /// Calculates and applies the jump speed boost.
  ///
  /// @param        DeltaSeconds      The delta time to use.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void ApplyJumpSpeedBoost(float DeltaSeconds);
  virtual void ApplyJumpSpeedBoost_Implementation(float DeltaSeconds);

  /// Called when the pawn is just about to jump. The movement mode at this point is still grounded but will will be changed to airborne in the next frame.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void OnJumped();
  virtual void OnJumped_Implementation() {}

///=============================================================================================================================================================
/// Properties

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
  TObjectPtr<UInputAction> SprintAction{nullptr};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
  TObjectPtr<UInputAction> CrouchAction{nullptr};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Input")
  TObjectPtr<UInputAction> JumpAction{nullptr};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "1", UIMax = "1000"))
  /// Acceleration of the pawn when not in the air.
  float Acceleration{10.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "100", UIMax = "2000"))
  /// Soft cap on the movement speed of the pawn.
  float MaxSpeed{600.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "0", UIMax = "500"))
  /// How quickly the pawn comes to a stop when there is no directional input.
  float StopSpeed{200.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "100", UIMax = "5000"))
  /// The speed the pawn should have when sprinting.
  float SprintSpeed{900.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "100", UIMax = "2000"))
  /// How fast the pawn can walk when crouching while grounded.
  float CrouchWalkSpeed{400.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "100", UIMax = "1000"))
  /// How fast the pawn can (un)crouch.
  float CrouchInterpSpeed{500.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0.0001", UIMin = "1"))
  /// What the half height of the pawn's root collision should be when fully crouched. Crouching is only possible if the root collision is a vertical capsule or
  /// a box. Will be clamped if larger than the default root collision half height.
  float CrouchedHalfHeight{60.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "1", UIMax = "10"))
  /// The amount of friction applied when moving on the ground.
  float FloorFriction{5.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "1", UIMax = "5"))
  /// Multiplier applied to the floor friction when the pawn is close to a drop-off.
  float EdgeFrictionScale{2.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "10", UIMax = "100"))
  /// How high a ledge must be at the least for edge friction to be applied.
  float EdgeFrictionMinRequiredHeight{50.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground")
  /// If false, the edge friction multiplier will be applied as soon as the outer edge of the collision shape is over the ledge. If true, the edge friction
  /// multiplier will be applied only after the center of the pawn's collision shape is already over the ledge.
  bool bUseAltEdgeFriction{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground")
  /// Determines the behaviour of the jumping mechanic.
  EGMC_JumpMode JumpMode{EGMC_JumpMode::SemiAuto};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "300", UIMax = "2000"))
  /// The strength of the pawn's jump.
  float JumpForce{500.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground")
  /// Whether the pawn should receive a forward speed boost when jumping. Allows gaining speed through "Accelerated Back Hopping" but also prevents strafe
  /// jumping.
  bool bApplyJumpSpeedBoost{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "0.01", UIMax = "1", EditCondition = "bApplyJumpSpeedBoost"))
  /// The percentage of the max speed applied as jump speed boost when the pawn is sprinting or fully crouched.
  float SmallJumpSpeedBoostFactor{0.1f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|On Ground", meta =
    (ClampMin = "0", UIMin = "0.01", UIMax = "1", EditCondition = "bApplyJumpSpeedBoost"))
  /// The percentage of the max speed applied as jump speed boost when the pawn is neither sprinting nor fully crouched.
  float LargeJumpSpeedBoostFactor{0.5f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|In Air", meta =
    (ClampMin = "0", UIMin = "0", UIMax = "1000"))
  /// Acceleration of the pawn when in the air.
  float AirAcceleration{20.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|In Air", meta =
    (ClampMin = "0", UIMin = "0", UIMax = "1"))
  /// Multiplier limiting the pawn's speed while in the air.
  float AirControl{0.1f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|In Water", meta =
    (ClampMin = "0", UIMin = "0", UIMax = "1"))
  /// Multiplier limiting the pawn's speed while in water.
  float WaterControl{0.5f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|In Water", meta =
    (ClampMin = "0", UIMin = "0", UIMax = "500"))
  /// How quickly the pawn sinks in water when there is no directional input.
  float SinkSpeed{150.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|In Water", meta =
    (ClampMin = "0", UIMin = "300", UIMax = "2000"))
  /// The strength of the pawn's jump when trying to jump out of water.
  float WaterJumpForce{700.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|In Water", meta =
    (ClampMin = "0", UIMin = "0", UIMax = "2000"))
  /// The strength of the forward push when the pawn's trying to jump out of water.
  float WaterJumpPush{600.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|GoldSrc|In Water")
  /// If false, swimming controls with use an "AbsoluteZ" input scheme. If true, swimming controls will be fully relative to the player's view rotation.
  bool bUseAltSwimControls{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay, meta =
    (ClampMin = "0", UIMin = "0"))
  /// The absolute max speed that the pawn can achieve under any circumstances. Setting this to 0 disables the hard cap.
  float SpeedHardCap{10000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, the crouch walk speed will already be applied while partially crouching.
  bool bImmediateCrouchWalkSpeed{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", AdvancedDisplay)
  /// If true, the configured mass of the pawn will affect the height of the jump.
  bool bConsiderMassOnJump{false};
};
