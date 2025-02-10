// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "Smoothing.h"
#include "GMCReplicationComponent.h"
#include "Compression.h"
#include "GMCLog.h"

void FGMC_MatchLatestTransientParams::Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr) const
{
  GMC_LOG(
    LogGMCReplication,
    Outer->GetOwner(),
    Log,
    TEXT("%s")
    TEXT("MaxDeltaTime = %f  ")
    TEXT("MinDeltaTime = %f  ")
    TEXT("DeltaTimeDilation = %f  ")
    TEXT("TeleportThreshold = %f  ")
    TEXT("InterpStates = %s  ")
    TEXT("TargetStateTimestamp = %f  ")
    TEXT("SimTime = %f  ")
    TEXT("TargetDelta = %f  ")
    TEXT("PrevIdx = %d  ")
    TEXT("LastIdx = %d  ")
    TEXT("Alpha = %f  ")
    TEXT("bInterpThisUpdate = %d  ")
    TEXT("bExtrapThisUpdate = %d"),
    *PrefixStr,
    MaxDeltaTime,
    MinDeltaTime,
    DeltaTimeDilation,
    TeleportThreshold,
    InterpStates == EGMC_InterpolationStates::Input ? TEXT("Input") : TEXT("Output"),
    TargetStateTimestamp,
    SimTime,
    TargetDelta,
    PrevIdx,
    LastIdx,
    Alpha,
    bInterpThisUpdate,
    bExtrapThisUpdate
  )
}

void FGMC_FixedDelayTransientParams::Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr) const
{
  GMC_LOG(
    LogGMCReplication,
    Outer->GetOwner(),
    Log,
    TEXT("%s")
    TEXT("FixedDelay = %f  ")
    TEXT("ExtrapolationRecoveryTime = %f  ")
    TEXT("bAllowPhysicsExtrapolation = %d  ")
    TEXT("TeleportThreshold = %f  ")
    TEXT("InterpStates = %s  ")
    TEXT("StartIdx = %d  ")
    TEXT("TargetIdx = %d  ")
    TEXT("SimTime = %f  ")
    TEXT("Alpha = %f  ")
    TEXT("bInterpThisUpdate = %d  ")
    TEXT("bExtrapThisUpdate = %d"),
    *PrefixStr,
    FixedDelay,
    ExtrapolationRecoveryTime,
    bAllowPhysicsExtrapolation,
    TeleportThreshold,
    InterpStates == EGMC_InterpolationStates::Input ? TEXT("Input") : TEXT("Output"),
    StartIdx,
    TargetIdx,
    SimTime,
    Alpha,
    bInterpThisUpdate,
    bExtrapThisUpdate
  )
}

void FGMC_AdaptiveDelayTransientParams::Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr) const
{
  GMC_LOG(
    LogGMCReplication,
    Outer->GetOwner(),
    Log,
    TEXT("%s")
    TEXT("SyncInterval = %f  ")
    TEXT("bUseDynamicBufferTime = %d  ")
    TEXT("BufferTime = %f  ")
    TEXT("Tolerance = %f  ")
    TEXT("ExtrapolationRecoveryTime = %f  ")
    TEXT("bAllowPhysicsExtrapolation = %d  ")
    TEXT("TeleportThreshold = %f  ")
    TEXT("InterpStates = %s  ")
    TEXT("CurrentDelay = %f  ")
    TEXT("StartIdx = %d  ")
    TEXT("TargetIdx = %d  ")
    TEXT("SimTime = %f  ")
    TEXT("Alpha = %f  ")
    TEXT("bInterpThisUpdate = %d  ")
    TEXT("bExtrapThisUpdate = %d"),
    *PrefixStr,
    SyncInterval,
    bUseDynamicBufferTime,
    BufferTime,
    Tolerance,
    ExtrapolationRecoveryTime,
    bAllowPhysicsExtrapolation,
    TeleportThreshold,
    InterpStates == EGMC_InterpolationStates::Input ? TEXT("Input") : TEXT("Output"),
    CurrentDelay,
    StartIdx,
    TargetIdx,
    SimTime,
    Alpha,
    bInterpThisUpdate,
    bExtrapThisUpdate
  )
}

void FGMC_UniformSimulationTransientParams::Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr) const
{
  GMC_LOG(
    LogGMCReplication,
    Outer->GetOwner(),
    Log,
    TEXT("%s")
    TEXT("InterpTolerance = %f  ")
    TEXT("MaxTimeStep = %f  ")
    TEXT("MaxIterations = %d  ")
    TEXT("SimStates = %s  ")
    TEXT("PreviousMoveTimestamp = %f  ")
    TEXT("TimeSinceLastNewMove = %f  ")
    TEXT("LastIdx = %d  ")
    TEXT("SimDeltaTime = %f  ")
    TEXT("SimTime = %f  ")
    TEXT("bNewMove = %d  ")
    TEXT("bInterpThisUpdate = %d  ")
    TEXT("bExtrapThisUpdate = %d"),
    *PrefixStr,
    InterpTolerance,
    MaxTimeStep,
    MaxIterations,
    SimStates == EGMC_InterpolationStates::Input ? TEXT("Input") : TEXT("Output"),
    PreviousMoveTimestamp,
    TimeSinceLastNewMove,
    LastIdx,
    SimDeltaTime,
    SimTime,
    bNewMove,
    bInterpThisUpdate,
    bExtrapThisUpdate
  )
}

void FGMC_CumulativeSimulationTransientParams::Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr) const
{
  GMC_LOG(
    LogGMCReplication,
    Outer->GetOwner(),
    Log,
    TEXT("%s")
    TEXT("InterpTolerance = %f  ")
    TEXT("MaxTimeStep = %f  ")
    TEXT("MaxIterations = %d  ")
    TEXT("bUseDissociativeExtrapolation = %d  ")
    TEXT("SimStates = %s  ")
    TEXT("PreviousMoveTimestamp = %f  ")
    TEXT("TimeSinceLastNewMove = %f  ")
    TEXT("AccDeltaTime = %f  ")
    TEXT("LastIdx = %d  ")
    TEXT("SimDeltaTime = %f  ")
    TEXT("SimTime = %f  ")
    TEXT("bNewMove = %d  ")
    TEXT("bInterpThisUpdate = %d  ")
    TEXT("bExtrapThisUpdate = %d"),
    *PrefixStr,
    InterpTolerance,
    MaxTimeStep,
    MaxIterations,
    bUseDissociativeExtrapolation,
    SimStates == EGMC_InterpolationStates::Input ? TEXT("Input") : TEXT("Output"),
    PreviousMoveTimestamp,
    TimeSinceLastNewMove,
    AccDeltaTime,
    LastIdx,
    SimDeltaTime,
    SimTime,
    bNewMove,
    bInterpThisUpdate,
    bExtrapThisUpdate
  )
}

bool FGMC_AdaptiveDelayServerPacket::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
  Ar << TargetComponent;

  if (Ar.IsSaving())
  {
    GMCCompression::SerializeFloat(DelayValue, FGMC_AdaptiveDelayPersistentParams::DELAY_VALUE_COMPRESSION, Ar);
    GMCCompression::SerializeFloat(DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION, Ar);
    GMCCompression::SerializeFloat(SyncTime, FGMC_AdaptiveDelayPersistentParams::SYNC_TIME_COMPRESSION, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeFloat(DelayValue, FGMC_AdaptiveDelayPersistentParams::DELAY_VALUE_COMPRESSION, Ar);
    GMCCompression::DeserializeFloat(DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION, Ar);
    GMCCompression::DeserializeFloat(SyncTime, FGMC_AdaptiveDelayPersistentParams::SYNC_TIME_COMPRESSION, Ar);
  }

  return (bOutSuccess = true);
}

bool FGMC_AdaptiveDelayClientPacket::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
  Ar << TargetComponent;

  if (Ar.IsSaving())
  {
    DynamicBufferTime =
      FMath::Clamp(DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME, FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME);
    GMCCompression::SerializeFloat(DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeFloat(DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::DYNAMIC_BUFFER_TIME_COMPRESSION, Ar);
    DynamicBufferTime =
      FMath::Clamp(DynamicBufferTime, FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME, FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME);
  }

  return (bOutSuccess = true);
}
