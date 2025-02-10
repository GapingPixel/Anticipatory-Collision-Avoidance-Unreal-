// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCReplicationComponent.h"
#include "GameFramework/Actor.h"
#include "GMCRollbackActor.generated.h"

USTRUCT(BlueprintType)
struct FGMC_RollbackState
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FTransform Transform{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FVector LinearVelocity{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FVector AngularVelocity{0.};
};

/// Base class for actors that have no net role on the client but should still be synchronised for gameplay. All actors of this type can be rolled back to a
/// previous deterministic state for move execution by UGMC_ReplicationCmp.
UCLASS(ABSTRACT, ClassGroup = "Movement", BlueprintType, Blueprintable)
class GMCCORE_API AGMC_RollbackActor : public AActor
{
  GENERATED_BODY()

public:

  AGMC_RollbackActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void BeginPlay() override;
  void Tick(float DeltaTime) override;
  FVector GetVelocity() const override;

  /// Returns the linear velocity of the actor in cm/sec.
  ///
  /// @returns      FVector    The linear velocity of the actor.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetLinearVelocity() const;

  /// Returns the angular velocity of the actor in deg/sec.
  ///
  /// @returns      FVector    The angular velocity of the actor.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetAngularVelocityInDegrees() const;

  /// Returns the angular velocity of the actor in rad/sec.
  ///
  /// @returns      FVector    The angular velocity of the actor.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetAngularVelocityInRadians() const;

  /// Sets the linear velocity of the actor.
  ///
  /// @param        NewVelocity    The new linear velocity the actor should have in cm/sec.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetLinearVelocity(const FVector& NewVelocity);

  /// Sets the angular velocity of the rollback actor.
  ///
  /// @param        NewVelocity    The new angular velocity the actor should have in deg/sec.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetAngularVelocityInDegrees(const FVector& NewVelocity);

  /// Sets the angular velocity of the rollback actor.
  ///
  /// @param        NewVelocity    The new angular velocity the actor should have in rad/sec.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetAngularVelocityInRadians(const FVector& NewVelocity);

  /// Calculates the tangential velocity that would act on an object at the passed position. Only relevant if the actor has any angular velocity.
  ///
  /// @param        WorldLocation    The location to calculate the tangential velocity at in world space.
  /// @returns      FVector          The tangential velocity at the passed location in cm/sec.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector ComputeTangentialVelocity(const FVector& WorldLocation) const;

  /// Returns the current synchronised world time i.e. when called on the client the network delay is already accounted for.
  ///
  /// @returns      double    The current world time.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual double GetTime() const;

  /// Overridable function to update the state of the rollback actor (called every frame). This function should be implemented in a way that the state of the
  /// actor can be recreated solely based on the passed arguments.
  ///
  /// @param        Time         The time of the move (or the current world time for regular tick updates).
  /// @param        DeltaTime    The delta time to use.
  /// @param        Move         The move that is about to be executed (empty move with timestamp -1 for regular tick updates).
  /// @param        Context      The context in which the actor state is being updated.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void UpdateState(double Time, float DeltaTime, const FGMC_Move& Move, EGMC_NetContext Context);
  virtual void UpdateState_Implementation(double Time, float DeltaTime, const FGMC_Move& Move, EGMC_NetContext Context) {}

  /// Saves the current state of the actor before rollback.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void SaveState(EGMC_NetContext Context);
  virtual void SaveState_Implementation(EGMC_NetContext Context);

  /// Restores the currently saved state of the actor after rollback.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void LoadState(EGMC_NetContext Context);
  virtual void LoadState_Implementation(EGMC_NetContext Context);

  /// Setter for bTicked.
  ///
  /// @param        bNewValue    The new value.
  /// @returns      void
  void SetTicked(bool bNewValue);

  /// Checks whether UpdateState has already been called on this actor during the current frame.
  ///
  /// @returns      bool    True if the rollback actor have already been ticked this frame, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "Movement")
  bool HasTicked() const;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking")
  /// If true, this actor will never be rolled back by a pawn for move execution and won't be ticked from the replication component. This will effectively make
  /// it behave like a regular (non-rollback) actor.
  bool bExcludeFromRollback{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking")
  /// Disables all Blueprint events called for this actor.
  bool bNoBlueprintEvents{false};

protected:

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  /// The linear velocity of the actor in cm/sec.
  FVector LinearVelocity{0.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  /// The angular velocity of the actor in deg/sec.
  FVector AngularVelocity{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The linear velocity the actor started with when it was spawned.
  FVector BaseLinearVelocity{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The angular velocity the actor started with when it was spawned.
  FVector BaseAngularVelocity{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The transform the actor started with when it was spawned.
  FTransform BaseTransform{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Used to buffer the state of the actor for rollback.
  FGMC_RollbackState SavedRollbackState{};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// Cached reference to the GMC aggregator (if present).
  TObjectPtr<class AGMC_Aggregator> GMCAggregator{nullptr};

  /// Manages the prerequisite ticks for this actor.
  ///
  /// @returns      void
  virtual void ManagePrerequisiteTicks();

private:

  /// Whether this actor's state has already been updated this frame.
  bool bTicked{false};
};
