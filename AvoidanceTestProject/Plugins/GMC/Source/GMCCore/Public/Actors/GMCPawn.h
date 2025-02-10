// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCReplicationComponent.h"
#include "GameFramework/Pawn.h"
#include "GMCPawn.generated.h"

/// Pawn class intended to be used with UGMC_ReplicationCmp.
UCLASS(BlueprintType, Blueprintable)
class GMCCORE_API AGMC_Pawn : public APawn
{
  GENERATED_BODY()

public:

  AGMC_Pawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void PostInitializeComponents() override;
  void BeginPlay() override;
  bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
  void NotifyRestarted() override;
  void NotifyControllerChanged() override;
  void PossessedBy(AController* NewController) override;
  void UnPossessed() override;
  void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
  void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
  UPawnMovementComponent* GetMovementComponent() const override;
  FVector GetVelocity() const override;
  UPrimitiveComponent* GetMovementBase() const override;
  FRotator GetBaseAimRotation() const override;

  /// Returns this pawn's replication component.
  ///
  /// @returns      UGMC_ReplicationCmp*    The replication component of this pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  UGMC_ReplicationCmp* GetReplicationComponent() const;

  /// Can be used to force the replication component to reserialize the move data for this pawn on the next replication update.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_MarkForNetReserialization();

  /// Whether the replication component should reserialize the move data for this pawn on the next replication update.
  ///
  /// @returns      bool    True if this pawn needs net reserialization, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_NeedsNetReserialization() const;

  /// Initializes/resets the pawn's move data.
  ///
  /// @returns      void
  void InitializeMoves();

protected:

  void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

  /// Called on the server and owning client when the controller for this pawn has changed.
  ///
  /// @param        NewController    The new controller.
  /// @param        OldController    The previous controller.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnControllerChanged(AController* NewController, AController* OldController);
  virtual void OnControllerChanged_Implementation(AController* NewController, AController* OldController);

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// Cached reference to the GMC aggregator (if present).
  TObjectPtr<class AGMC_Aggregator> GMCAggregator{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The replication component of this pawn.
  TObjectPtr<UGMC_ReplicationCmp> ReplicationComponent{nullptr};

private:

  bool SV_bNeedsNetReserialization{false};

  UPROPERTY(BlueprintReadWrite, Transient, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  FGMC_Move LocalMove{};
  friend FGMC_Move& UGMC_ReplicationCmp::LocalMove();
  friend const FGMC_Move& UGMC_ReplicationCmp::LocalMove() const;

  UFUNCTION()
  void CL_OnRepAPMove();

  UPROPERTY(BlueprintReadWrite, Transient, ReplicatedUsing = CL_OnRepAPMove, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  FGMC_Move APMove{};
  friend FGMC_Move& UGMC_ReplicationCmp::APMove();
  friend const FGMC_Move& UGMC_ReplicationCmp::APMove() const;

  UFUNCTION()
  void CL_OnRepSPMove();

  UPROPERTY(BlueprintReadWrite, Transient, ReplicatedUsing = CL_OnRepSPMove, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  FGMC_Move SPMove{};
  friend FGMC_Move& UGMC_ReplicationCmp::SPMove();
  friend const FGMC_Move& UGMC_ReplicationCmp::SPMove() const;

  UFUNCTION(Server, Reliable, WithValidation)
  void SV_ReceiveMovesReliable(const TArray<FGMC_Move>& RemoteMoves);
  void SV_ReceiveMovesReliable_Implementation(const TArray<FGMC_Move>& RemoteMoves);
  bool SV_ReceiveMovesReliable_Validate(const TArray<FGMC_Move>& RemoteMoves);

  UFUNCTION(Server, Unreliable, WithValidation)
  void SV_ReceiveMovesUnreliable(const TArray<FGMC_Move>& RemoteMoves);
  void SV_ReceiveMovesUnreliable_Implementation(const TArray<FGMC_Move>& RemoteMoves);
  bool SV_ReceiveMovesUnreliable_Validate(const TArray<FGMC_Move>& RemoteMoves);

  UFUNCTION(Server, Reliable, WithValidation)
  void SV_ReceiveClientSendStatus(bool bIsUsingUnreliableMoves);
  void SV_ReceiveClientSendStatus_Implementation(bool bIsUsingUnreliableMoves);
  bool SV_ReceiveClientSendStatus_Validate(bool bIsUsingUnreliableMoves);

  friend void UGMC_ReplicationCmp::CL_SendMoves();
  friend void UGMC_ReplicationCmp::FComponentStatus::CL_SetUseUnreliableClientMoves(int32 NumPendingReliablePackets, UGMC_ReplicationCmp* const Outer);
};
