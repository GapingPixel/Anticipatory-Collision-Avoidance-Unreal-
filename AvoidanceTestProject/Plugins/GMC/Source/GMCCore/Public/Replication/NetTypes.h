// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SyncMeta.h"
#include "NetTypes.generated.h"

USTRUCT(BlueprintType)
struct FGMC_ClientInfo
{
  GENERATED_BODY()

  bool bWasNetRelevant{false};
};

USTRUCT(BlueprintType)
struct FGMC_NetInfo
{
  GENERATED_BODY()

  GMCReplication::ESimType NetType{};

  TWeakObjectPtr<class UGMC_ReplicationCmp> OwningComponent{};

  TMap<TWeakObjectPtr<const AActor>, FGMC_ClientInfo> ClientReceivers{};
};

USTRUCT(BlueprintType)
struct FGMC_MetaData
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  double Timestamp{-1.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  float DeltaTime{-1.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  double ServerAuthPhysicsTimestamp{-1.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  bool bIsUsingServerAuthPhysics{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  bool bIsUsingClientAuthPhysics{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  bool bPredictedClientMove{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  bool bValidClientMove{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  bool bPlayerOwned{false};
};

UENUM(BlueprintType)
enum class EGMC_NetContext : uint8
{
  NoContextInformation,
  LocalServerPawn_PreMoveExecution,
  LocalServerPawn_PostMoveExecution,
  LocalClientPawn_PreMoveExecution,
  LocalClientPawn_PostMoveExecution,
  LocalClientPawn_PreMoveExecutionNoPrediction,
  LocalClientPawn_PostMoveExecutionNoPrediction,
  LocalClientPawn_ReplayAborted,
  LocalClientPawn_ServerStateAdoptedForReplay,
  LocalClientPawn_PreReplay,
  LocalClientPawn_PreReplayMoveExecution,
  LocalClientPawn_PostReplayMoveExecution,
  LocalClientPawn_PostReplay,
  LocalClientPawn_Simulation,
  LocalClientPawn_ExtrapolationAborted,
  LocalClientPawn_PrePlayerTickNoPrediction,
  LocalClientPawn_PostPlayerTickNoPrediction,
  LocalClientPawn_PredictionEnabled,
  LocalClientPawn_PredictionDisabled,
  RemoteServerPawn_PreMoveProcessing,
  RemoteServerPawn_PreMoveExecution,
  RemoteServerPawn_PostMoveExecution,
  RemoteServerPawn_PostMoveProcessing,
  RemoteServerPawn_Smoothing,
  RemoteServerPawn_UnSmoothing,
  RemoteServerPawn_Simulation,
  RemoteServerPawn_ExtrapolationAborted,
  RemoteClientPawn_Simulation,
  RemoteClientPawn_ExtrapolationAborted,
  ManualRewind,
  PostManualRewind,
  RollbackSwap,
  RegularTickUpdate,
  MAX UMETA(Hidden),
};
