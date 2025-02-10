// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SyncCore.h"
#include "MacroForEach.h"
#include "Engine/ScopedMovementUpdate.h"
#include "SyncInterface.generated.h"

namespace DataOp
{
  enum EGMC_DataOp
  {
    None       = (int32)GMCReplication::EDataOp::None,
    Save       = (int32)GMCReplication::EDataOp::Save,
    Quantize   = (int32)GMCReplication::EDataOp::Quantize,
    Apply      = (int32)GMCReplication::EDataOp::Apply,
    AllDataOps = (int32)GMCReplication::EDataOp::AllDataOps,
  };
}

namespace DataFilter
{
  enum EGMC_DataFilter
  {
    None                      = (int32)GMCReplication::EDataFilter::None,
    SV_ReplicateForSimulation = (int32)GMCReplication::EDataFilter::SV_ReplicateForSimulation,
    SV_ReplicateForPrediction = (int32)GMCReplication::EDataFilter::SV_ReplicateForPrediction,
    SV_ReplicateForCorrection = (int32)GMCReplication::EDataFilter::SV_ReplicateForCorrection,
    SV_Validate               = (int32)GMCReplication::EDataFilter::SV_Validate,
    SV_UseClientValue         = (int32)GMCReplication::EDataFilter::SV_UseClientValue,
    SV_ForceUpdateOnChange    = (int32)GMCReplication::EDataFilter::SV_ForceUpdateOnChange,
    CL_SendToServer           = (int32)GMCReplication::EDataFilter::CL_SendToServer,
    CL_Validate               = (int32)GMCReplication::EDataFilter::CL_Validate,
    Wildcard1                 = (int32)GMCReplication::EDataFilter::Wildcard1,
    Wildcard2                 = (int32)GMCReplication::EDataFilter::Wildcard2,
    Wildcard3                 = (int32)GMCReplication::EDataFilter::Wildcard3,
    Wildcard4                 = (int32)GMCReplication::EDataFilter::Wildcard4,
    AllDataFilters            = (int32)GMCReplication::EDataFilter::AllDataFilters,
  };
}

namespace DataFilterMode
{
  enum EGMC_DataFilterMode
  {
    Inclusive = (int32)GMCReplication::EDataFilterMode::Inclusive,
    Exclusive = (int32)GMCReplication::EDataFilterMode::Exclusive,
  };
}

#define USE_SCOPED_APPLY 1

#define ADD_SYNC_TYPE(Name)\
  GMCReplication::TSyncType<GMCReplication::F##Name> Name;

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_PawnState
{
  GENERATED_BODY()

  FOR_EACH(ADD_SYNC_TYPE, ALL_SYNC_TYPES)
};

#undef ADD_SYNC_TYPE

#define ADD_MEMBER_ALIAS(Name)\
  GMCReplication::TMemberAlias<GMCReplication::F##Name> Name;

struct GMCCORE_API FGMC_MemberAliases
{
  // Integrated sync types are defined directly within the pawn state, they don't need member aliases.

  FOR_EACH(ADD_MEMBER_ALIAS, GENERIC_SYNC_TYPES)
};

#undef ADD_MEMBER_ALIAS

#define INIT_INTEGRATED_DEFAULTS(Name)\
  State.Name.Initialize(\
    TranslateToSyncSettings(\
      DefaultReplicationSettings.DefaultPredictionSettings.Name,\
      DefaultReplicationSettings.DefaultCombineSettings.Name,\
      DefaultReplicationSettings.DefaultSimulationSettings.Name,\
      DefaultReplicationSettings.DefaultInterpolationFunctions.Name\
    ), SimulationState, SimulationType, Component\
  );
#define INIT_INTEGRATED_TAGS(Name)\
  State.Name.Initialize(\
    TranslateToSyncSettings(\
      GetNonDefaultReplicationSettings(DefaultReplicationSettings.SyncTags.Name, SyncTagsData)\
    ), SimulationState, SimulationType, Component\
  );

#define INIT_GENERIC(Name)\
  State.Name.Initialize(AliasData.Name, SimulationState, SimulationType, Component);

inline void InitializeSyncData(
  FGMC_PawnState& State,
  const FGMC_DefaultReplicationSettings& DefaultReplicationSettings,
  const FGMC_SyncTagsData& SyncTagsData,
  const FGMC_MemberAliases& AliasData,
  GMCReplication::ESimState SimulationState,
  GMCReplication::ESimType SimulationType,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(INIT_INTEGRATED_DEFAULTS, INTEGRATED_SYNC_TYPES_DEFAULTS)
  //FOR_EACH(INIT_INTEGRATED_TAGS, INTEGRATED_SYNC_TYPES_TAGS)
  FOR_EACH(INIT_GENERIC, GENERIC_SYNC_TYPES)
}

#undef INIT_INTEGRATED_DEFAULTS
#undef INIT_INTEGRATED_TAGS
#undef INIT_GENERIC

#define PROCESS_INTEGRATED(Name)\
  State.Name.Process(Directive, bUseRelative, Component);
#define PROCESS_GENERIC(Name)\
  State.Name.Process(AliasData.Name, Directive, Component);

inline void ProcessSyncData(
  FGMC_PawnState& State,
  GMCReplication::FDataOpDirective Directive,
  const FGMC_MemberAliases& AliasData,
  bool bUseRelative,
  UGMC_ReplicationCmp* const Component
)
{
#if USE_SCOPED_APPLY
  const bool bApply = Directive.GetDataOps() & DataOp::Apply;
  FScopedMovementUpdate(bApply ? ((UActorComponent*)Component)->GetOwner()->GetRootComponent() : nullptr);
#endif

  FOR_EACH(PROCESS_INTEGRATED, INTEGRATED_SYNC_TYPES)
  FOR_EACH(PROCESS_GENERIC, GENERIC_SYNC_TYPES)
}

inline void ProcessSyncData_IntegratedOnly(
  FGMC_PawnState& State,
  GMCReplication::FDataOpDirective Directive,
  const FGMC_MemberAliases& AliasData,
  bool bUseRelative,
  UGMC_ReplicationCmp* const Component
)
{
#if USE_SCOPED_APPLY
  const bool bApply = Directive.GetDataOps() & DataOp::Apply;
  FScopedMovementUpdate(bApply ? ((UActorComponent*)Component)->GetOwner()->GetRootComponent() : nullptr);
#endif

  FOR_EACH(PROCESS_INTEGRATED, INTEGRATED_SYNC_TYPES)
}

inline void ProcessSyncData_GenericOnly(
  FGMC_PawnState& State,
  GMCReplication::FDataOpDirective Directive,
  const FGMC_MemberAliases& AliasData,
  bool bUseRelative,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(PROCESS_GENERIC, GENERIC_SYNC_TYPES)
}

#undef PROCESS_INTEGRATED
#undef PROCESS_GENERIC

#define COPY_VALUES(Name)\
  Source.Name.CopyValuesTo(Destination.Name, Filters, (GMCReplication::EDataFilterMode)FilterMode, Component);

inline void CopySyncDataValues(
  const FGMC_PawnState& Source,
  FGMC_PawnState& Destination,
  int32 Filters,
  int32 FilterMode,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(COPY_VALUES, ALL_SYNC_TYPES)
}

#undef COPY_VALUES

#define COPY_LAST_SERIALIZED(Name)\
  Source.Name.CopyLastSerializedValuesTo(Destination.Name, Filters, (GMCReplication::EDataFilterMode)FilterMode, Component);

inline void CopySyncDataLastSerializedValues(
  const FGMC_PawnState& Source,
  FGMC_PawnState& Destination,
  int32 Filters,
  int32 FilterMode,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(COPY_LAST_SERIALIZED, ALL_SYNC_TYPES)
}

#undef COPY_LAST_SERIALIZED

#define COPY_SUSPENDED(Name)\
  Source.Name.CopySuspendedFlagsTo(Destination.Name, Filters, (GMCReplication::EDataFilterMode)FilterMode, Component);

inline void CopySyncDataSuspendedFlags(
  const FGMC_PawnState& Source,
  FGMC_PawnState& Destination,
  int32 Filters,
  int32 FilterMode,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(COPY_SUSPENDED, ALL_SYNC_TYPES)
}

#undef COPY_SUSPENDED

#define SHOULD_COMBINE(Name)\
  if (!State.Name.ShouldCombineValue(PreviousState.Name, Component)) return false;

inline bool CheckSyncDataShouldCombineMove(const FGMC_PawnState& State, const FGMC_PawnState& PreviousState, UGMC_ReplicationCmp* const Component)
{
  FOR_EACH(SHOULD_COMBINE, ALL_SYNC_TYPES)

  return true;
}

#undef SHOULD_COMBINE

#define SET_COMBINED(Name)\
  State.Name.SetCombinedValue(StateToCombine.Name, Component);

inline void SetSyncDataCombinedMove(FGMC_PawnState& State, const FGMC_PawnState& StateToCombine, UGMC_ReplicationCmp* const Component)
{
  FOR_EACH(SET_COMBINED, ALL_SYNC_TYPES)
}

#undef SET_COMBINED

#define IS_VALID_SERVER_INTEGRATED(Name)\
  if (\
    !State.Name.IsValidCheckCurrentServerValue(\
      Filters,\
      (GMCReplication::EDataFilterMode)FilterMode,\
      bUseRelative,\
      OutFirstDeviatingSyncType,\
      OutFirstDeviatingSyncTypeIndex,\
      Component\
    )\
  )\
  {\
    if (!bDebugForceFullSyncDataValidation)\
    {\
      return false;\
    }\
    bIsValid = false;\
  }
#define IS_VALID_SERVER_GENERIC(Name)\
  if (\
    !State.Name.IsValidCheckCurrentServerValue(\
      AliasData.Name,\
      Filters,\
      (GMCReplication::EDataFilterMode)FilterMode,\
      OutFirstDeviatingSyncType,\
      OutFirstDeviatingSyncTypeIndex,\
      Component\
    )\
  )\
  {\
    if (!bDebugForceFullSyncDataValidation)\
    {\
      return false;\
    }\
    bIsValid = false;\
  }

inline bool ValidateSyncDataAgainstCurrentServerState(
  const FGMC_PawnState& State,
  int32 Filters,
  int32 FilterMode,
  const FGMC_MemberAliases& AliasData,
  bool bUseRelative,
  EGMC_SyncType& OutFirstDeviatingSyncType,
  int32& OutFirstDeviatingSyncTypeIndex,
  UGMC_ReplicationCmp* const Component,
  bool bDebugForceFullSyncDataValidation
)
{
  OutFirstDeviatingSyncType = EGMC_SyncType::MAX;
  OutFirstDeviatingSyncTypeIndex = -1;

  bool bIsValid = true;

  FOR_EACH(IS_VALID_SERVER_INTEGRATED, INTEGRATED_SYNC_TYPES)
  FOR_EACH(IS_VALID_SERVER_GENERIC, GENERIC_SYNC_TYPES)

  return bIsValid;
}

#undef IS_VALID_SERVER_INTEGRATED
#undef IS_VALID_SERVER_GENERIC

#define IS_VALID_CLIENT(Name)\
  if (\
    !State.Name.IsValidCheckOtherClientValue(\
      OtherState.Name,\
      Filters,\
      (GMCReplication::EDataFilterMode)FilterMode,\
      OutFirstDeviatingSyncType,\
      OutFirstDeviatingSyncTypeIndex,\
      Component\
    )\
  )\
  {\
    if (!bDebugForceFullSyncDataValidation)\
    {\
      return false;\
    }\
    bIsValid = false;\
  }

inline bool ValidateSyncDataAgainstOtherClientState(
  const FGMC_PawnState& State,
  const FGMC_PawnState& OtherState,
  int32 Filters,
  int32 FilterMode,
  EGMC_SyncType& OutFirstDeviatingSyncType,
  int32& OutFirstDeviatingSyncTypeIndex,
  UGMC_ReplicationCmp* const Component,
  bool bDebugForceFullSyncDataValidation
)
{
  OutFirstDeviatingSyncType = EGMC_SyncType::MAX;
  OutFirstDeviatingSyncTypeIndex = -1;

  bool bIsValid = true;

  FOR_EACH(IS_VALID_CLIENT, ALL_SYNC_TYPES)

  return bIsValid;
}

#undef IS_VALID_CLIENT

#define SHOULD_FORCE_NET_UPDATE(Name)\
  if (State.Name.ShouldForceNetUpdate(PreviousState.Name, Component)) return true;

inline bool CheckSyncDataForcedNetUpdate(
  const FGMC_PawnState& State,
  const FGMC_PawnState& PreviousState,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(SHOULD_FORCE_NET_UPDATE, ALL_SYNC_TYPES)

  return false;
}

#undef SHOULD_FORCE_NET_UPDATE

#define NET_SERIALIZE(Name)\
  State.Name.NetSerializeValue(Ar, Map, TargetConnection, NetInfo, MetaData, bForceFullSerialization);

inline void NetSerializeSyncData(
  FGMC_PawnState& State,
  FArchive& Ar,
  UPackageMap* Map,
  const AActor* const TargetConnection,
  const FGMC_NetInfo& NetInfo,
  const FGMC_MetaData& MetaData,
  bool bForceFullSerialization
)
{
  FOR_EACH(NET_SERIALIZE, ALL_SYNC_TYPES)
}

#undef NET_SERIALIZE

#define TO_RELATIVE(Name)\
  State.Name.ToRelativeValue(TargetBase, Filters, (GMCReplication::EDataFilterMode)FilterMode, Component);

inline void ToRelativeSyncData(
  FGMC_PawnState& State,
  const USceneComponent* const TargetBase,
  int32 Filters,
  int32 FilterMode,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(TO_RELATIVE, INTEGRATED_SYNC_TYPES)
}

#undef TO_RELATIVE

#define TO_ABSOLUTE(Name)\
  State.Name.ToAbsoluteValue(SourceBase, Filters, (GMCReplication::EDataFilterMode)FilterMode, Component);

inline void ToAbsoluteSyncData(
  FGMC_PawnState& State,
  const USceneComponent* const SourceBase,
  int32 Filters,
  int32 FilterMode,
  UGMC_ReplicationCmp* const Component
)
{
  FOR_EACH(TO_ABSOLUTE, INTEGRATED_SYNC_TYPES)
}

#undef TO_ABSOLUTE

#define LOG(Name)\
  State.Name.Log(Component, PrefixStr);

inline void LogSyncData(
  const FGMC_PawnState& State,
  UGMC_ReplicationCmp* const Component,
  const FString& PrefixStr = TEXT("")
)
{
#if !NO_LOGGING

  FOR_EACH(LOG, ALL_SYNC_TYPES)

#endif
}

#undef LOG

#define INTERPOLATE(Name)\
  InterpolatedState.Name.SetInterpolatedValue(\
    StartState.Name,\
    TargetState.Name,\
    PreviousState.Name,\
    NextState.Name,\
    StartState.ActorBase.Read(),\
    TargetState.ActorBase.Read(),\
    PreviousState.ActorBase.Read(),\
    NextState.ActorBase.Read(),\
    Ratio,\
    bUseRelative,\
    Filters,\
    (GMCReplication::EDataFilterMode)FilterMode,\
    Component\
  );

inline FGMC_PawnState InterpolateSyncData(
  const FGMC_PawnState& StartState,
  const FGMC_PawnState& TargetState,
  const FGMC_PawnState& PreviousState,
  const FGMC_PawnState& NextState,
  double Ratio,
  GMCReplication::ESimState SimState,
  bool bUseRelative,
  int32 Filters,
  int32 FilterMode,
  const FGMC_DefaultReplicationSettings& DefaultReplicationSettings,
  const FGMC_SyncTagsData& SyncTagsData,
  const FGMC_MemberAliases& AliasData,
  UGMC_ReplicationCmp* const Component
)
{
  FGMC_PawnState InterpolatedState{};
  InitializeSyncData(
    InterpolatedState,
    DefaultReplicationSettings,
    SyncTagsData,
    AliasData,
    SimState,
    GMCReplication::ESimType::None,
    Component
  );

  FOR_EACH(INTERPOLATE, ALL_SYNC_TYPES)

  return InterpolatedState;
}

inline FGMC_PawnState InterpolateSyncData_NoPhysics(
  const FGMC_PawnState& StartState,
  const FGMC_PawnState& TargetState,
  const FGMC_PawnState& PreviousState,
  const FGMC_PawnState& NextState,
  double Ratio,
  GMCReplication::ESimState SimState,
  bool bUseRelative,
  int32 Filters,
  int32 FilterMode,
  const FGMC_DefaultReplicationSettings& DefaultReplicationSettings,
  const FGMC_SyncTagsData& SyncTagsData,
  const FGMC_MemberAliases& AliasData,
  UGMC_ReplicationCmp* const Component
)
{
  FGMC_PawnState InterpolatedState{};
  InitializeSyncData(
    InterpolatedState,
    DefaultReplicationSettings,
    SyncTagsData,
    AliasData,
    SimState,
    GMCReplication::ESimType::None,
    Component
  );

  // Ignore physics values, these are interpolated separately (LinearVelocity, AngularVelocity, ActorLocation, ActorRotation).
  FOR_EACH(INTERPOLATE, INTEGRATED_SYNC_TYPES_NO_PHYSICS)
  FOR_EACH(INTERPOLATE, GENERIC_SYNC_TYPES)

  return InterpolatedState;
}

inline FGMC_PawnState InterpolateSyncData_ExtrapolationRecovery(
  const FGMC_PawnState& StartState,
  const FGMC_PawnState& TargetState,
  const FGMC_PawnState& PreviousState,
  const FGMC_PawnState& NextState,
  double Ratio,
  GMCReplication::ESimState SimState,
  bool bUseRelative,
  int32 Filters,
  int32 FilterMode,
  const FGMC_DefaultReplicationSettings& DefaultReplicationSettings,
  const FGMC_SyncTagsData& SyncTagsData,
  const FGMC_MemberAliases& AliasData,
  UGMC_ReplicationCmp* const Component
)
{
  FGMC_PawnState InterpolatedState{};
  InitializeSyncData(
    InterpolatedState,
    DefaultReplicationSettings,
    SyncTagsData,
    AliasData,
    SimState,
    GMCReplication::ESimType::None,
    Component
  );

  // Only interpolate specific values important for extrapolation recovery.
  INTERPOLATE(ActorLocation)
  INTERPOLATE(ActorRotation)
  INTERPOLATE(ActorScale)
  INTERPOLATE(ControlRotation)

  return InterpolatedState;
}

#undef INTERPOLATE
