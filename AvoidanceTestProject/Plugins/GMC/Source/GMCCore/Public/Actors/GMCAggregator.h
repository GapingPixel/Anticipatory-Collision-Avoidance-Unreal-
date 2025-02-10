// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GMCAggregator.generated.h"

DECLARE_STATS_GROUP(TEXT("AGMC_Aggregator"), STATGROUP_AGMC_Aggregator, STATCAT_Advanced);

/// Allows for efficient ticking and retrieval of GMC related actors and components.
UCLASS(BlueprintType, Blueprintable)
class GMCCORE_API AGMC_Aggregator : public AActor
{
  GENERATED_BODY()

public:

  AGMC_Aggregator(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void Tick(float DeltaTime) override;

  /// Returns the aggregator (if present). There must never be more than one instance of this actor in a world.
  ///
  /// @returns      AGMC_Aggregator*    The aggregator if one exists, otherwise nullptr.
  UFUNCTION(BlueprintCallable, BlueprintPure = true, Category = "General Movement Component", meta = (WorldContext = "Context"))
  static AGMC_Aggregator* GetGMCAggregator(UObject* Context);

  /// Add a controller actor to the aggregator.
  ///
  /// @param        Controller    The object to add.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void RegisterController(class AController* Controller);

  /// Add a pawn actor to the aggregator.
  ///
  /// @param        Pawn    The object to add.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void RegisterPawn(class APawn* Pawn);

  /// Add a movement component to the aggregator.
  ///
  /// @param        MovementComponent    The object to add.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void RegisterMovementComponent(class UMovementComponent* MovementComponent);

  /// Add a rollback actor to the aggregator.
  ///
  /// @param        RollbackActor    The object to add.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void RegisterRollbackActor(class AGMC_RollbackActor* RollbackActor);

  /// Add a mesh component to the aggregator.
  ///
  /// @param        Mesh    The object to add.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void RegisterMeshComponent(class UMeshComponent* Mesh);

  /// Remove a controller actor from the aggregator.
  ///
  /// @param        Controller    The object to remove.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UnregisterController(AController* Controller);

  /// Remove a pawn actor from the aggregator.
  ///
  /// @param        Pawn    The object to remove.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UnregisterPawn(APawn* Pawn);

  /// Remove a movement component from the aggregator.
  ///
  /// @param        MovementComponent    The object to remove.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UnregisterMovementComponent(UMovementComponent* MovementComponent);

  /// Remove a rollback actor from the aggregator.
  ///
  /// @param        RollbackActor    The object to remove.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UnregisterRollbackActor(AGMC_RollbackActor* RollbackActor);

  /// Remove a mesh component from the aggregator.
  ///
  /// @param        Mesh    The object to remove.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void UnregisterMeshComponent(UMeshComponent* Mesh);

  /// Sorts all currently registered controller actors.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SortControllers();

  /// Sorts all currently registered pawn actors.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SortPawns();

  /// Sorts all currently registered movement components.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SortMovementComponents();

  /// Sorts all currently registered rollback actors.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SortRollbackActors();

  /// Sorts all currently registered mesh components.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SortMeshComponents();

  /// Returns all currently registered controller actors.
  ///
  /// @returns      const TArray<AController*>&    The array containing all registered controller actors.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const TArray<AController*>& GetControllers() const;

  /// Returns all currently registered pawn actors.
  ///
  /// @returns      const TArray<APawn*>&    The array containing all registered pawn actors.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const TArray<APawn*>& GetPawns() const;

  /// Returns all currently registered movement components.
  ///
  /// @returns      const TArray<UMovementComponent*>&    The array containing all registered movement components.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const TArray<UMovementComponent*>& GetMovementComponents() const;

  /// Returns all currently registered rollback actors.
  ///
  /// @returns      const TArray<AGMC_RollbackActor*>&    The array containing all registered rollback actors.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const TArray<AGMC_RollbackActor*>& GetRollbackActors() const;

  /// Returns all currently registered mesh components.
  ///
  /// @returns      const TArray<UMeshComponent*>&    The array containing all registered mesh components.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const TArray<UMeshComponent*>& GetMeshComponents() const;

  /// Enables/disables the tick functions of all currently registered objects.
  ///
  /// @param        bInEnable    Whether to enable or disable the tick functions.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetTickFunctionsEnabled(bool bInEnable);

  /// Called when the aggregate tick is toggled.
  ///
  /// @param        bEnabled    Whether the aggregate tick was enabled or disabled.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  void OnAggregateTickToggled(bool bEnabled);
  virtual void OnAggregateTickToggled_Implementation(bool bEnabled);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tick")
  /// If true, all currently registered objects will have their tick functions disabled and will be ticked from the aggregator instead.
  bool bEnableAggregateTick{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
  /// Do not toggle at runtime. Some tick group combinations may cause faulty behaviour.
  bool bAggregateControllers{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
  /// Do not toggle at runtime. Some tick group combinations may cause faulty behaviour.
  bool bAggregatePawns{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
  /// Do not toggle at runtime. Some tick group combinations may cause faulty behaviour.
  bool bAggregateMovementComponents{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
  /// Do not toggle at runtime. Some tick group combinations may cause faulty behaviour.
  bool bAggregateRollbackActors{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
  /// Do not toggle at runtime. Some tick group combinations may cause faulty behaviour.
  bool bAggregateMeshComponents{true};

protected:

  /// Determines the order number of the passed controller.
  ///
  /// @param        Controller    The object to get the order number for.
  /// @returns      int32         The order number of the passed object.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual int32 GetControllerOrderNumber(const AController* Controller);

  /// Determines the order number of the passed pawn.
  ///
  /// @param        Pawn     The object to get the order number for.
  /// @returns      int32    The order number of the passed object.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual int32 GetPawnOrderNumber(const APawn* Pawn);

  /// Determines the order number of the passed movement component.
  ///
  /// @param        MovementComponent    The object to get the order number for.
  /// @returns      int32                The order number of the passed object.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual int32 GetMovementComponentOrderNumber(const UMovementComponent* MovementComponent);

  /// Determines the order number of the passed rollback actor.
  ///
  /// @param        RollbackActor    The object to get the order number for.
  /// @returns      int32            The order number of the passed object.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual int32 GetRollbackActorOrderNumber(const AGMC_RollbackActor* RollbackActor);

  /// Determines the order number of the passed mesh component.
  ///
  /// @param        MeshComponent    The object to get the order number for.
  /// @returns      int32            The order number of the passed object.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual int32 GetMeshComponentOrderNumber(const UMeshComponent* MeshComponent);

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Holds all currently registered controllers.
  TArray<AController*> Controllers{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Holds all currently registered pawns.
  TArray<APawn*> Pawns{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Holds all currently registered movement components.
  TArray<UMovementComponent*> MovementComponents{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Holds all currently registered rollback actors.
  TArray<AGMC_RollbackActor*> RollbackActors{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Holds all currently registered mesh components.
  TArray<UMeshComponent*> MeshComponents{};

private:

  bool VerifyOrder(int32 CurrentOrderNumber, int32& InOutPreviousOrderNumber) const;

  bool bWasEnabledLastFrame{false};

  bool bIsFirstUpdate{false};
};
