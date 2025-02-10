// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SyncMeta.h"
#include "FloatPrecision.h"
#include "Engine/EngineTypes.h"
#include "Containers/StaticArray.h"
#include "Smoothing.generated.h"

UENUM(BlueprintType)
enum class EGMC_InterpolationMode : uint8
{
  None,
  MatchLatest,
  FixedDelay,
  AdaptiveDelay,
  UniformSimulation,
  CumulativeSimulation
};

UENUM(BlueprintType)
enum class EGMC_InterpolationStates : uint8
{
  Input,
  Output
};

UENUM(BlueprintType)
enum class EGMC_NetworkTolerance : uint8
{
  Auto,
  Low,
  Normal,
  High,
  VeryHigh,
  Manual
};

inline GMCReplication::ESimState ToNativeState(EGMC_InterpolationStates States)
{
  switch (States)
  {
    case EGMC_InterpolationStates::Input:
    {
      return GMCReplication::ESimState::Input;
    }
    case EGMC_InterpolationStates::Output:
    {
      return GMCReplication::ESimState::Output;
    }
    default:
    {
      gmc_ckne();
    }
  }
  gmc_ckne()
  return GMCReplication::ESimState{};
}

UENUM(BlueprintType)
enum class EGMC_ExtrapolationMode : uint8
{
  None,
  DeadReckoning,
  UniformSimulation,
  CumulativeSimulation
};

UENUM(BlueprintType)
enum class EGMC_InterpolationFunction : uint8
{
  NearestNeighbour,
  StartValue,
  TargetValue,
  Linear,
  Cubic,
  Custom1,
  Custom2,
  Custom3,
  Custom4,
  Custom5,
  Custom6,
  Custom7,
  Custom8
};

USTRUCT(BlueprintType)
struct FGMC_MatchLatestPersistentParams
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0.1", UIMax = "0.2"))
  /// The max time delta over which the smoothing algorithm is allowed to interpolate between two server state updates.
  float MaxDeltaTime{0.12f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.1"))
  /// The min time delta over which the smoothing algorithm must interpolate between two server state updates.
  float MinDeltaTime{0.025f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "1", UIMax = "10"))
  /// Scaling factor for the time delta to interpolate over. Will be clamped to the configured min/max values.
  float DeltaTimeDilation{5.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "-1", UIMin = "-1", UIMax = "2500"))
  /// The min required distance between the interpolation states to teleport to the target state directly. Set to -1 to disable this functionality.
  float TeleportThreshold{1000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether interpolation should be based on input or output states.
  EGMC_InterpolationStates InterpStates{EGMC_InterpolationStates::Output};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The timestamp of the interpolation target state.
  double TargetStateTimestamp{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current simulation time.
  double SimTime{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current target delta to move.
  float TargetDelta{0.f};

  void Reset()
  {
    TargetStateTimestamp = 0.;
    SimTime = 0.;
    TargetDelta = 0.f;
  }
};

struct GMCCORE_API FGMC_MatchLatestTransientParams
{
  float MaxDeltaTime;
  float MinDeltaTime;
  float DeltaTimeDilation;
  float TeleportThreshold;
  EGMC_InterpolationStates InterpStates;
  double TargetStateTimestamp;
  double SimTime;
  float TargetDelta;

  int32 PrevIdx;
  int32 LastIdx;
  float Alpha;

  uint8 bInterpThisUpdate:1;
  uint8 bExtrapThisUpdate:1;

  void Reset()
  {
    PrevIdx = -1;
    LastIdx = -1;
    Alpha = -1.f;

    bInterpThisUpdate = false;
    bExtrapThisUpdate = false;
  }

  FGMC_MatchLatestTransientParams(const FGMC_MatchLatestPersistentParams& StaticParams)
  {
    MaxDeltaTime = StaticParams.MaxDeltaTime;
    MinDeltaTime = StaticParams.MinDeltaTime;
    DeltaTimeDilation = StaticParams.DeltaTimeDilation;
    TeleportThreshold = StaticParams.TeleportThreshold;
    InterpStates = StaticParams.InterpStates;
    TargetStateTimestamp = StaticParams.TargetStateTimestamp;
    SimTime = StaticParams.SimTime;
    TargetDelta = StaticParams.TargetDelta;
    Reset();
  }

  void Log(const class UGMC_ReplicationCmp* Outer, const FString& PrefixStr = TEXT("")) const;
};

USTRUCT(BlueprintType)
struct FGMC_FixedDelayPersistentParams
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0.1", UIMax = "2"))
  /// How far back in time (in seconds) simulated pawns are displayed on the local machine. Lower values are more "real-time" but may produce unsatisfactory
  /// results with bad network conditions. Larger values enable a greater tolerance for packet loss, latency spikes and higher ping but simulated pawns are
  /// farther in the past on the local machine. Since timestamps are based on the world time of the machine where the original move was created you need to
  /// consider the longest time it can take for one packet to get from one client to another (i.e. the two highest client latencies added up) as well as the
  /// frame times of all machines involved. When running a server that accepts only one client connection (e.g. co-op games where one player hosts as a listen
  /// server and only one other player can join as a client) you only have to consider the latency of that one connection.
  float FixedDelay{0.15f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  /// The time over which to smoothly return to an interpolated pawn state after extrapolation.
  float ExtrapolationRecoveryTime{0.3f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether extrapolation should be allowed when simulating physics.
  bool bAllowPhysicsExtrapolation{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "-1", UIMin = "-1", UIMax = "2500"))
  /// The min required distance between the interpolation states to teleport to the target state directly. Set to -1 to disable this functionality.
  float TeleportThreshold{1000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether interpolation should be based on input or output states.
  EGMC_InterpolationStates InterpStates{EGMC_InterpolationStates::Output};

  void Reset()
  {
    // Nothing to clear.
  }
};

struct GMCCORE_API FGMC_FixedDelayTransientParams
{
  float FixedDelay;
  float ExtrapolationRecoveryTime;
  bool bAllowPhysicsExtrapolation;
  float TeleportThreshold;
  EGMC_InterpolationStates InterpStates;

  int32 StartIdx;
  int32 TargetIdx;
  double SimTime;
  float Alpha;

  uint8 bInterpThisUpdate:1;
  uint8 bExtrapThisUpdate:1;

  void Reset()
  {
    StartIdx = -1;
    TargetIdx = -1;
    SimTime = 0.;
    Alpha = -1.f;

    bInterpThisUpdate = false;
    bExtrapThisUpdate = false;
  }

  FGMC_FixedDelayTransientParams(const FGMC_FixedDelayPersistentParams& StaticParams)
  {
    FixedDelay = StaticParams.FixedDelay;
    ExtrapolationRecoveryTime = StaticParams.ExtrapolationRecoveryTime;
    bAllowPhysicsExtrapolation = StaticParams.bAllowPhysicsExtrapolation;
    TeleportThreshold = StaticParams.TeleportThreshold;
    InterpStates = StaticParams.InterpStates;
    Reset();
  }

  void Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr = TEXT("")) const;
};

USTRUCT(BlueprintType)
struct FGMC_AdaptiveDelayPersistentParams
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0.1", UIMin = "1", UIMax = "10"))
  /// The interval at which the adaptive delay is synced between server and client. Regardless of this setting, new parameters may still get transmitted more
  /// than once per interval if they change by more than the set tolerance or when using a dynamic buffer time.
  float SyncInterval{2.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0.1", UIMax = "5"))
  /// If true, the client will request the buffer time dynamically from the server based on the current network conditions. Works best for good connections
  /// (little packet loss) and higher net update frequencies.
  bool bUseDynamicBufferTime{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0.1", UIMax = "2"))
  /// How much additional time should be added to the calculated base simulation delay to account for variable network conditions, packet loss, frame rate, etc.
  /// When using a dynamic buffer time this will merely determine the initial value.
  float BufferTime{0.13f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0.000001", UIMin = "0.05", UIMax = "0.2"))
  /// Tolerance within which a newly calculated simulation delay will be considered equal to the current one. Only if the new delay value differs by more than
  /// the set tolerance the simulation delay will be updated.
  float Tolerance{0.1f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.5"))
  /// The time over which to smoothly return to an interpolated pawn state after extrapolation.
  float ExtrapolationRecoveryTime{0.3f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether extrapolation should be allowed when simulating physics.
  bool bAllowPhysicsExtrapolation{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "-1", UIMin = "-1", UIMax = "2500"))
  /// The min required distance between the interpolation states to teleport to the target state directly. Set to -1 to disable this functionality.
  float TeleportThreshold{1000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether interpolation should be based on input or output states.
  EGMC_InterpolationStates InterpStates{EGMC_InterpolationStates::Output};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current simulation delay. Automatically adapts to the client's ping.
  float CurrentDelay{0.f};

  /// How large the sync interval has to be at least.
  static constexpr float MIN_SYNC_INTERVAL = 0.1f;

  /// How much random variance should be added to the sync interval to avoid calling RPCs for all pawns at the exact same time.
  static constexpr float SYNC_INTERVAL_VARIANCE = 0.4f;

  /// How large the total delay (buffer time + latency) has to be at least. Values that are too small never produce good results even at very low latency
  /// because replication updates are not perfectly uniform.
  static constexpr float MIN_TOTAL_DELAY = 0.05f;

  /// How large the total delay (buffer time + latency) can be at most.
  static constexpr float MAX_TOTAL_DELAY = 2.f;

  /// How large the buffer time has to be at least.
  static constexpr float MIN_BUFFER_TIME = 0.05f;

  /// How large the buffer time can be at most. Especially important when using a dynamic buffer time so the client can't request very large values (will be
  /// clamped on the server).
  static constexpr float MAX_BUFFER_TIME = 1.f;

  /// Compression settings for syncing the adaptive delay parameters.
  static constexpr EGMC_FloatPrecision DELAY_VALUE_COMPRESSION = TwoDecimals;
  static constexpr EGMC_FloatPrecision DYNAMIC_BUFFER_TIME_COMPRESSION = TwoDecimals;
  static constexpr EGMC_FloatPrecision SYNC_TIME_COMPRESSION = TwoDecimals;

  /// Number of previous smoothing parameters to save for rollback.
  static constexpr int32 NUM_SAVED_PARAMS = 20;

  struct FAux
  {
    float DelayValue{0.f}; // Total delay value (also contains the buffer time).
    float DynamicBufferTime{0.f}; // The applied dynamic buffer time (only relevant when bUseDynamicBufferTime is enabled).
    double SyncTime{0.}; // The time at which to apply the new total delay value.
  };

  /// Holds the current and past parameters for adaptive delay smoothing with a timer that tells us when it's time to sync the most recent values with the
  /// client based on the configured update interval. Parameters may still get sent more than once per interval if they change by more than the set tolerance.
  struct FTimedParams
  {
    TStaticArray<FAux, NUM_SAVED_PARAMS> Parameters;
    float UpdateTimer{0.f};
    float RequestedDynamicBufferTime{0.f}; // If 0 no buffer time was recently requested (or bUseDynamicBufferTime is disabled).
    float AppliedDynamicBufferTime{0.f}; // The last applied requested buffer time. If 0 none was applied before (or bUseDynamicBufferTime is disabled).
  };

  /// Maps out which adaptive delay parameters belong to which client controller. We need to save a certain number of past parameters to be able to retrieve the
  /// correct simulation delay during rollback on the server. Higher indices contain newer values. Destroyed player controllers are never deleted from the map,
  /// test for validity before access.
  TMap<TWeakObjectPtr<APlayerController>, FTimedParams> SV_PerClientParams;

  /// Tells us when it is time to update the delay for the local simulation of a smoothed remotely controlled server pawn based on the configured update
  /// interval. The delay may still get updated more than once per interval if it changes by more than the set tolerance.
  float SV_SmoothedRemoteServerPawnUpdateTimer{0.f};

  /// Used to save the current and past delay(s) locally. We need to save a certain number of past values to retrieve the correct simulation delay during
  /// rollback on the client. Higher indices contain newer parameters.
  TStaticArray<FAux, NUM_SAVED_PARAMS> CL_LocalParams;

  void Reset()
  {
    CurrentDelay = 0.f;
    SV_PerClientParams.Reset();
    SV_SmoothedRemoteServerPawnUpdateTimer = 0.f;
    CL_LocalParams = TStaticArray<FAux, NUM_SAVED_PARAMS>{};
  }

  void SV_PreTickServerPawn(float DeltaTime)
  {
    for (auto& Entry : SV_PerClientParams)
    {
      Entry.Value.UpdateTimer += DeltaTime;
    }
  }

  void SV_PreTickSmoothedRemoteServerPawn(float DeltaTime)
  {
    SV_SmoothedRemoteServerPawnUpdateTimer += DeltaTime;
  }

  float SV_GetInternalSyncInterval() const
  {
    const float InternalSyncInterval =
      FMath::Clamp(SyncInterval + FMath::FRandRange(-SYNC_INTERVAL_VARIANCE, SYNC_INTERVAL_VARIANCE), MIN_SYNC_INTERVAL, UE_BIG_NUMBER);

    gmc_ck(SyncInterval >= MIN_SYNC_INTERVAL)
    gmc_ck(InternalSyncInterval >= MIN_SYNC_INTERVAL)

    return InternalSyncInterval;
  }
};

struct GMCCORE_API FGMC_AdaptiveDelayTransientParams
{
  float SyncInterval;
  bool bUseDynamicBufferTime;
  float BufferTime;
  float Tolerance;
  float ExtrapolationRecoveryTime;
  bool bAllowPhysicsExtrapolation;
  float TeleportThreshold;
  EGMC_InterpolationStates InterpStates;
  float CurrentDelay;

  int32 StartIdx;
  int32 TargetIdx;
  double SimTime;
  float Alpha;

  uint8 bInterpThisUpdate:1;
  uint8 bExtrapThisUpdate:1;

  void Reset()
  {
    StartIdx = -1;
    TargetIdx = -1;
    SimTime = 0.;
    Alpha = -1.f;

    bInterpThisUpdate = false;
    bExtrapThisUpdate = false;
  }

  FGMC_AdaptiveDelayTransientParams(const FGMC_AdaptiveDelayPersistentParams& StaticParams)
  {
    SyncInterval = StaticParams.SyncInterval;
    bUseDynamicBufferTime = StaticParams.bUseDynamicBufferTime;
    BufferTime = StaticParams.BufferTime;
    Tolerance = StaticParams.Tolerance;
    ExtrapolationRecoveryTime = StaticParams.ExtrapolationRecoveryTime;
    bAllowPhysicsExtrapolation = StaticParams.bAllowPhysicsExtrapolation;
    TeleportThreshold = StaticParams.TeleportThreshold;
    InterpStates = StaticParams.InterpStates;
    CurrentDelay = StaticParams.CurrentDelay;
    Reset();
  }

  void Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr = TEXT("")) const;
};

USTRUCT()
struct GMCCORE_API FGMC_AdaptiveDelayServerPacket
{
  GENERATED_BODY()

  UGMC_ReplicationCmp* TargetComponent{nullptr};
  float DelayValue{0.f};
  float DynamicBufferTime{0.f};
  double SyncTime{0.};

  bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

  FGMC_AdaptiveDelayServerPacket() = default;

  FGMC_AdaptiveDelayServerPacket(UGMC_ReplicationCmp* TargetComponent, float DelayValue, float DynamicBufferTime, double SyncTime)
  : TargetComponent(TargetComponent),
    DelayValue(DelayValue),
    DynamicBufferTime(DynamicBufferTime),
    SyncTime(SyncTime)
  {}
};

template<>
struct TStructOpsTypeTraits<FGMC_AdaptiveDelayServerPacket> : public TStructOpsTypeTraitsBase2<FGMC_AdaptiveDelayServerPacket>
{
  enum { WithNetSerializer = true };
};

USTRUCT()
struct GMCCORE_API FGMC_AdaptiveDelayClientPacket
{
  GENERATED_BODY()

  UGMC_ReplicationCmp* TargetComponent{nullptr};
  float DynamicBufferTime{0.f};

  bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

  FGMC_AdaptiveDelayClientPacket() = default;

  FGMC_AdaptiveDelayClientPacket(UGMC_ReplicationCmp* TargetComponent, float DynamicBufferTime)
  : TargetComponent(TargetComponent),
    DynamicBufferTime(DynamicBufferTime)
  {}
};

template<>
struct TStructOpsTypeTraits<FGMC_AdaptiveDelayClientPacket> : public TStructOpsTypeTraitsBase2<FGMC_AdaptiveDelayClientPacket>
{
  enum { WithNetSerializer = true };
};

USTRUCT(BlueprintType)
struct FGMC_UniformSimulationPersistentParams
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (UIMin = "0", UIMax = "0.5"))
  /// How much tolerance in seconds there should be before switching to extrapolation when simulated interpolation is also used.
  float InterpTolerance{0.3f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0.000001", UIMin = "0.1"))
  /// The time interval in which to subdivide the delta time of a move for simulated pawns.
  float MaxTimeStep{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "1", UIMin = "1"))
  /// How many iterations of a sub-stepped move execution are allowed at most for simulation.
  int32 MaxIterations{3};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether simulation should be based on input or output states.
  EGMC_InterpolationStates SimStates{EGMC_InterpolationStates::Output};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The timestamp of the previous move simulation.
  double PreviousMoveTimestamp{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The time since we last got a new move for simulation.
  float TimeSinceLastNewMove{0.f};

  void Reset()
  {
    PreviousMoveTimestamp = 0.;
    TimeSinceLastNewMove = 0.f;
  }
};

struct GMCCORE_API FGMC_UniformSimulationTransientParams
{
  float InterpTolerance;
  float MaxTimeStep;
  int32 MaxIterations;
  EGMC_InterpolationStates SimStates;
  double PreviousMoveTimestamp;
  float TimeSinceLastNewMove;

  int32 LastIdx;
  float SimDeltaTime;
  double SimTime;
  uint8 bNewMove:1;

  uint8 bInterpThisUpdate:1;
  uint8 bExtrapThisUpdate:1;

  void Reset()
  {
    LastIdx = -1;
    SimDeltaTime = -1.f;
    SimTime = 0.;
    bNewMove = false;

    bInterpThisUpdate = false;
    bExtrapThisUpdate = false;
  }

  FGMC_UniformSimulationTransientParams(const FGMC_UniformSimulationPersistentParams& StaticParams)
  {
    InterpTolerance = StaticParams.InterpTolerance;
    MaxTimeStep = StaticParams.MaxTimeStep;
    MaxIterations = StaticParams.MaxIterations;
    SimStates = StaticParams.SimStates;
    PreviousMoveTimestamp = StaticParams.PreviousMoveTimestamp;
    TimeSinceLastNewMove = StaticParams.TimeSinceLastNewMove;
    Reset();
  }

  void Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr = TEXT("")) const;
};

USTRUCT(BlueprintType)
struct FGMC_CumulativeSimulationPersistentParams
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (UIMin = "0", UIMax = "0.5"))
  /// How much tolerance in seconds there should be before switching to extrapolation when simulated interpolation is also used.
  float InterpTolerance{0.3f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0.000001", UIMin = "0.1"))
  /// The time interval in which to subdivide the delta time of a move for simulated pawns.
  float MaxTimeStep{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "1", UIMin = "1"))
  /// How many iterations of a sub-stepped move execution are allowed at most for simulation.
  int32 MaxIterations{3};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// If true, the local simulation will continue to run on its own while extrapolating until we have a recent enough state available again to interpolate.
  bool bUseDissociativeExtrapolation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether simulation should be based on input or output states.
  EGMC_InterpolationStates SimStates{EGMC_InterpolationStates::Output};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The timestamp of the previous move simulation.
  double PreviousMoveTimestamp{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The time since we last got a new move for simulation.
  float TimeSinceLastNewMove{0.f};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The accumulated delta time since the last new move.
  float AccDeltaTime{-1.f};

  void Reset()
  {
    PreviousMoveTimestamp = 0.;
    TimeSinceLastNewMove = 0.f;
    AccDeltaTime = -1.f;
  }
};

struct GMCCORE_API FGMC_CumulativeSimulationTransientParams
{
  float InterpTolerance;
  float MaxTimeStep;
  int32 MaxIterations;
  bool bUseDissociativeExtrapolation;
  EGMC_InterpolationStates SimStates;
  double PreviousMoveTimestamp;
  float TimeSinceLastNewMove;
  float AccDeltaTime;

  int32 LastIdx;
  float SimDeltaTime;
  double SimTime;
  uint8 bNewMove:1;

  uint8 bInterpThisUpdate:1;
  uint8 bExtrapThisUpdate:1;

  void Reset()
  {
    LastIdx = -1;
    SimDeltaTime = -1.f;
    SimTime = 0.;
    bNewMove = false;

    bInterpThisUpdate = false;
    bExtrapThisUpdate = false;
  }

  FGMC_CumulativeSimulationTransientParams(const FGMC_CumulativeSimulationPersistentParams& StaticParams)
  {
    InterpTolerance = StaticParams.InterpTolerance;
    MaxTimeStep = StaticParams.MaxTimeStep;
    MaxIterations = StaticParams.MaxIterations;
    bUseDissociativeExtrapolation = StaticParams.bUseDissociativeExtrapolation;
    SimStates = StaticParams.SimStates;
    PreviousMoveTimestamp = StaticParams.PreviousMoveTimestamp;
    TimeSinceLastNewMove = StaticParams.TimeSinceLastNewMove;
    AccDeltaTime = StaticParams.AccDeltaTime;
    Reset();
  }

  void Log(const UGMC_ReplicationCmp* Outer, const FString& PrefixStr = TEXT("")) const;
};

USTRUCT(BlueprintType)
struct FGMC_SmoothComponentParams
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  /// The scene component to smooth. Set through the replication component, not directly.
  TObjectPtr<USceneComponent> Component{nullptr};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0.05", UIMax = "0.1"))
  /// The max time delta over which the smoothing algorithm is allowed to interpolate between two transforms.
  float MaxDeltaTime{0.06f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "0", UIMax = "0.05"))
  /// The min time delta over which the smoothing algorithm must interpolate between two transforms.
  float MinDeltaTime{0.01f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "1", UIMax = "5"))
  /// Scaling factor for the time delta to interpolate over. Will be clamped to the configured min/max values.
  float DeltaTimeDilation{3.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "-1", UIMin = "-1", UIMax = "2500"))
  /// The min required distance between two location values to teleport to the target transform directly. Set to -1 to disable this functionality.
  float TeleportThreshold{1000.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// Whether we should skip updating kinematic bones if the component is a skeletal mesh with a physics state.
  bool bSkipBoneUpdate{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  /// The teleport type to use for the physics state of the component.
  ETeleportType TeleportPhysics{ETeleportType::TeleportPhysics};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The relative offset of the component to the owning actor's root component without any smoothing applied.
  FTransform ComponentOffset{};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The timestamp of the interpolation target transform.
  double TargetTransformTimestamp{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current simulation time.
  double SimTime{0.};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// The current target delta to move.
  float TargetDelta{0.f};

  void Reset()
  {
    ComponentOffset = FTransform::Identity;
    TargetTransformTimestamp = 0.;
    SimTime = 0.;
    TargetDelta = 0.f;
  }
};

USTRUCT(BlueprintType)
struct FGMC_SmoothCorrection
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether smoothed client corrections should be used.
  bool bEnable{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "0.5"))
  // The time to interpolate over during a correction.
  float InterpolationTime{0.2f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to smooth a correction of the actor location.
  bool bSmoothActorLocation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to smooth a correction of the actor rotation.
  bool bSmoothActorRotation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to smooth a correction of the actor scale.
  bool bSmoothActorScale{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether to smooth a correction of the control rotation.
  bool bSmoothControlRotation{true};
};

USTRUCT(BlueprintType)
struct FGMC_SimulationThrottle
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component")
  // Whether simulation throttling should be used.
  bool bEnable{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "1000", UIMax = "10000"))
  // The distance from the local player up to which simulated pawns will still be smoothed every frame.
  double MaxSmoothingDistance{5000.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "0", UIMin = "1000", UIMax = "10000"))
  // The distance over which simulation frequency will drop exponentially once the max smoothing distance has been exceeded.
  double SmoothingFallOffDistance{5000.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement Component", meta = (ClampMin = "1", UIMin = "5", UIMax = "100"))
  // How many frames should be skipped at most between simulations.
  int32 MaxSkippedSmoothingFrames{30};
};
