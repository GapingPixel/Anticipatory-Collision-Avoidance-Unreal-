// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "WorldTime.h"
#include "GMCPlayerController.h"
#include "GMCAssert.h"

AGMC_WorldTimeReplicator::AGMC_WorldTimeReplicator()
{
  bReplicates = true;
  bAlwaysRelevant = true;

  // Net updates will be forced based on the interval at which the replicated world time is updated on the server.
  NetUpdateFrequency = 0.f;
}

void AGMC_WorldTimeReplicator::BeginPlay()
{
  Super::BeginPlay();

  if (WorldTimeUpdateInterval <= 0.f || IsNetMode(NM_Standalone) || GetLocalRole() != ROLE_Authority)
  {
    return;
  }

  gmc_ck(bReplicates)
  gmc_ck(bAlwaysRelevant)
  gmc_ck(NetUpdateFrequency == 0.f)

  SV_StartUpdateTimer(WorldTimeUpdateInterval);
}

void AGMC_WorldTimeReplicator::EndPlay(EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  if (const auto& World = GetWorld())
  {
    World->GetTimerManager().ClearTimer(SV_TimerHandleUpdateTime);
  }
}

double AGMC_WorldTimeReplicator::GetRealWorldTimeSecondsReplicated() const
{
  return RealWorldTimeSecondsReplicated;
}

void AGMC_WorldTimeReplicator::SV_StartUpdateTimer(float UpdateInterval)
{
  if (GetLocalRole() != ROLE_Authority)
  {
    return;
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  World->GetTimerManager().SetTimer(
    SV_TimerHandleUpdateTime,
    this,
    &AGMC_WorldTimeReplicator::SV_UpdateRealWorldTimeSecondsReplicated,
    UpdateInterval,
    true,
    0.f
  );
}

void AGMC_WorldTimeReplicator::SV_StopUpdateTimer()
{
  if (GetLocalRole() != ROLE_Authority)
  {
    return;
  }

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  World->GetTimerManager().ClearTimer(SV_TimerHandleUpdateTime);
}

void AGMC_WorldTimeReplicator::SV_ResetUpdateTimer(float UpdateInterval)
{
  SV_StopUpdateTimer();
  SV_StartUpdateTimer(UpdateInterval);
}

void AGMC_WorldTimeReplicator::CL_OnRepRealWorldTimeSecondsReplicated()
{
  if (UGameInstance* GameInstance = GetGameInstance())
  {
    const auto& LocalController = Cast<AGMC_PlayerController>(GameInstance->GetFirstLocalPlayerController());
    if (LocalController)
    {
      LocalController->CL_SyncWithServerTime(RealWorldTimeSecondsReplicated);
    }
  }
}

void AGMC_WorldTimeReplicator::SV_UpdateRealWorldTimeSecondsReplicated()
{
  gmc_ck(GetLocalRole() == ROLE_Authority)

  if (const auto& World = GetWorld())
  {
    RealWorldTimeSecondsReplicated = World->GetRealTimeSeconds();

    // We need to replicate the server time as quickly as possible to the client to keep the discrepancy not caused by network latency minimal.
    ForceNetUpdate();
  }
}

void AGMC_WorldTimeReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  FDoRepLifetimeParams Params;
  Params.bIsPushBased = false;
  Params.RepNotifyCondition = REPNOTIFY_Always;
  Params.Condition = COND_None;

  DOREPLIFETIME_WITH_PARAMS_FAST(AGMC_WorldTimeReplicator, RealWorldTimeSecondsReplicated, Params);
}
