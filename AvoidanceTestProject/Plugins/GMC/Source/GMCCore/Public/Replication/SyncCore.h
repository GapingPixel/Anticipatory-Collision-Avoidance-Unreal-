// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "NetTypes.h"
#include "SyncTypes.h"
#include "SyncSettings.h"
#include "GMCLog.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

class UGMC_ReplicationCmp;

namespace GMCReplication
{

template<typename T>
struct TSyncTypeData
{
  T Value{};
  TMap<TWeakObjectPtr<AActor>, T> LastSerializedValue{};
  bool bSuspended{false};
  bool bWasSuspended{false};
  FSyncSettings Settings{};
};

template<typename T>
struct TMemberAliasData
{
  T* ValueRef{};
  FSyncSettings Settings{};
};

template<typename T>
using TUnderlyingSyncTypeReference = decltype(DeclVal<T>().Get());

template<typename T>
using TUnderlyingSyncType = typename TRemoveReference<TUnderlyingSyncTypeReference<T>>::Type;

template<typename T>
struct TSyncTypeBase;

template<typename T>
struct TSyncTypeSingle;

template<typename T>
struct TSyncTypeMulti;

template<typename T>
struct TSyncType;

template<typename T>
struct TMemberAlias
{
  using UType = TUnderlyingSyncType<T>;

  friend TSyncTypeBase<T>;
  friend TSyncTypeSingle<T>;
  friend TSyncTypeMulti<T>;
  friend TSyncType<T>;

protected:

  TMemberAliasData<UType>& GetData(int32 DataIndex)
  {
    return AliasData[DataIndex];
  }

  const TMemberAliasData<UType>& GetData(int32 DataIndex) const
  {
    return AliasData[DataIndex];
  }

  int32 GetNumData() const
  {
    return AliasData.Num();
  }

public:

  const UType& Read(int32 Index) const
  {
    return *GetData(Index).ValueRef;
  }

  void Write(const UType& Value, int32 Index)
  {
    *GetData(Index).ValueRef = Value;
  }

  int32 Num() const
  {
    return GetNumData();
  }

  void BindMember(UType& VariableToBind, const FSyncSettings& Settings, int32& BindingIndex)
  {
    TMemberAliasData<UType> NewData;
    NewData.ValueRef = &VariableToBind;
    NewData.Settings = Settings;
    BindingIndex = AliasData.Emplace(MoveTemp(NewData));
  }

private:

  TArray<TMemberAliasData<UType>> AliasData;
};

template<typename T>
struct TSyncTypeBase
{
  using UType = TUnderlyingSyncType<T>;

  friend TSyncTypeSingle<T>;
  friend TSyncTypeMulti<T>;
  friend TSyncType<T>;

  inline static constexpr int32 IndexSingle = -1;
  inline static constexpr int32 NumSingle = 1;

protected:

  virtual TSyncTypeData<UType>& GetData(int32 DataIndex) = 0;

  virtual const TSyncTypeData<UType>& GetData(int32 DataIndex) const = 0;

  virtual int32 GetNumData() const = 0;

  virtual void InitDefault(
    UType& Value,
    ::UGMC_ReplicationCmp* const Component
  ) = 0;

  virtual void Quantize(
    UType& Value,
    ::UGMC_ReplicationCmp* const Component
  ) const = 0;

  virtual bool ShouldCombine(
    const UType& Value,
    const UType& PreviousValue,
    ::UGMC_ReplicationCmp* const Component
  ) const = 0;

  virtual void Combine(
    UType& Value,
    const UType& ValueToCombine,
    ::UGMC_ReplicationCmp* const Component
  ) = 0;

  virtual bool IsEqual(
    const UType& Value,
    const UType& OtherValue,
    ::UGMC_ReplicationCmp* const Component
  ) const = 0;

  virtual bool IsValid(
    const UType& Value,
    const UType& OtherValue,
    ::UGMC_ReplicationCmp* const Component
  ) const = 0;

  virtual UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const class USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const = 0;

  virtual void Serialize(
    UType& Value,
    FArchive& Ar,
    UPackageMap* Map,
    const FGMC_NetInfo& NetInfo,
    const FGMC_MetaData& MetaData
  ) const = 0;

  virtual FString GetLogName(::UGMC_ReplicationCmp* const Component) const = 0;
  virtual FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const = 0;
  virtual EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const = 0;

public:

  const UType& Read(int32 Index = IndexSingle) const
  {
    return GetData(Index).Value;
  }

  void Write(const UType& Value, int32 Index = IndexSingle)
  {
    GetData(Index).Value = Value;
  }

  int32 Num() const
  {
    return GetNumData();
  }

  bool ShouldCombineValue(
    const TSyncTypeBase<T>& Previous,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove)
    gmc_ck(GetSimulationState() == ESimState::Input)

    gmc_ck(GetNumData() == Previous.GetNumData())
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      const auto& Data = GetData(Index);
      const auto& DataPrevious = Previous.GetData(Index);

      if (!ShouldCombineInternal(GetClientSettings(Data), Data.Value, DataPrevious.Value, Component))
      {
        return false;
      }
    }

    return true;
  }

  void SetCombinedValue(
    const TSyncTypeBase<T>& ToCombine,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove)
    gmc_ck(GetSimulationState() == ESimState::Input)

    gmc_ck(GetNumData() == ToCombine.GetNumData())
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      auto& Data = GetData(Index);
      const auto& DataToCombine = ToCombine.GetData(Index);
      const auto& Settings = GetServerSettings(Data);
      gmc_ck(Settings.Compare(ToCombine.GetServerSettings(DataToCombine)))

      if (Settings.InputState.bUseClientValue)
      {
        gmc_ck(GetClientSettings(Data).InputState.bSendToServer)
        Data.Value = DataToCombine.Value;
      }
      else
      {
        Combine(Data.Value, DataToCombine.Value, Component);
      }
    }
  }

  bool ShouldForceNetUpdate(
    const TSyncTypeBase<T>& Previous,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(GetSimulationType() == ESimType::SPMove)

    gmc_ck(GetNumData() == Previous.GetNumData())
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      const auto& Data = GetData(Index);
      const auto& DataPrevious = Previous.GetData(Index);
      const auto& Settings = GetServerSettings(Data);

      if (!ShouldForceNetUpdate(Settings) || !ShouldReplicateForSimulation(Settings) || IsEqual(Data.Value, DataPrevious.Value, Component))
      {
        continue;
      }

      return true;
    }

    return false;
  }

  void SetInterpolatedValue(
    const TSyncTypeBase<T>& StartValue,
    const TSyncTypeBase<T>& TargetValue,
    const TSyncTypeBase<T>& PreviousValue,
    const TSyncTypeBase<T>& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    int32 Filters,
    EDataFilterMode FilterMode,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    gmc_ck(GetSimulationType() == ESimType::None)
    gmc_ck(GetSimulationState() != ESimState::None)

    gmc_ck(GetNumData() == StartValue.GetNumData())
    gmc_ck(GetNumData() == TargetValue.GetNumData())
    gmc_ck(GetNumData() == PreviousValue.GetNumData())
    gmc_ck(GetNumData() == NextValue.GetNumData())
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      auto& Data = GetData(Index);

      if (!ShouldProcess(Filters, FilterMode, Data))
      {
        continue;
      }

      Data.Value = Interpolate(
        GetLocalSettings(Data).InterpFunc,
        StartValue.GetData(Index).Value,
        TargetValue.GetData(Index).Value,
        PreviousValue.GetData(Index).Value,
        NextValue.GetData(Index).Value,
        StartActorBase,
        TargetActorBase,
        PreviousActorBase,
        NextActorBase,
        Ratio,
        bUseRelative,
        Component
      );
    }
  }

  void NetSerializeValue(
    FArchive& Ar,
    UPackageMap* Map,
    const AActor* const TargetConnection,
    const FGMC_NetInfo& NetInfo,
    const FGMC_MetaData& MetaData,
    bool bForceFullSerialization
  )
  {
    gmc_ck(GetSimulationState() == ESimState::Input || GetSimulationState() == ESimState::Output)

    switch (GetSimulationType())
    {
      case ESimType::LocalMove:
      {
        NetSerializeClientToServer(Ar, Map, TargetConnection, NetInfo, MetaData, bForceFullSerialization);
        return;
      }
      case ESimType::APMove:
      case ESimType::SPMove:
      {
        NetSerializeServerToClient(Ar, Map, TargetConnection, NetInfo, MetaData, bForceFullSerialization);
        return;
      }
      default:
      {
        gmc_ckne();
      }
    }
    gmc_ckne()
  }

  void SetSuspended(
    bool bSuspend,
    int32 Index = IndexSingle
  )
  {
    gmc_ck(GetSimulationType() != ESimType::None)
    gmc_ck(GetSimulationState() != ESimState::None)

    GetData(Index).bSuspended = bSuspend;
  }

  void CopyValuesTo(
    TSyncTypeBase<T>& Other,
    int32 Filters,
    EDataFilterMode FilterMode,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(GetNumData() == Other.GetNumData())
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      const auto& Data = GetData(Index);

      if (!ShouldProcess(Filters, FilterMode, Data))
      {
        continue;
      }

      auto& DataOther = Other.GetData(Index);
      DataOther.Value = Data.Value;
    }
  }

  void CopyLastSerializedValuesTo(
    TSyncTypeBase<T>& Other,
    int32 Filters,
    EDataFilterMode FilterMode,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(GetNumData() == Other.GetNumData())
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      const auto& Data = GetData(Index);

      if (!ShouldProcess(Filters, FilterMode, Data))
      {
        continue;
      }

      auto& DataOther = Other.GetData(Index);
      DataOther.LastSerializedValue = Data.LastSerializedValue;
    }
  }

  void CopySuspendedFlagsTo(
    TSyncTypeBase<T>& Other,
    int32 Filters,
    EDataFilterMode FilterMode,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(GetNumData() == Other.GetNumData())
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      const auto& Data = GetData(Index);

      if (!ShouldProcess(Filters, FilterMode, Data))
      {
        continue;
      }

      auto& DataOther = Other.GetData(Index);
      DataOther.bSuspended = Data.bSuspended;
      DataOther.bWasSuspended = Data.bWasSuspended;
    }
  }

  virtual ~TSyncTypeBase() {}

protected:

  void InitializeBase(ESimState SimState, ESimType SimType)
  {
    SimulationState = SimState;
    SimulationType = SimType;
  }

  ESimState GetSimulationState() const
  {
    return SimulationState;
  }

  ESimType GetSimulationType() const
  {
    return SimulationType;
  }

  FClientSettings GetClientSettings(const TSyncTypeData<UType>& Data) const
  {
    return Data.Settings.ClientSettings;
  }

  FServerSettings GetServerSettings(const TSyncTypeData<UType>& Data) const
  {
    return Data.Settings.ServerSettings;
  }

  FLocalSettings GetLocalSettings(const TSyncTypeData<UType>& Data) const
  {
    return Data.Settings.LocalSettings;
  }

  bool ShouldSendToServer(const FClientSettings& Settings) const
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove)

    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bSendToServer)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bSendToServer)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldReplicateToClient(const FServerSettings& Settings, bool bPredictedMove, bool bValidMove) const
  {
    gmc_ck(GetSimulationType() == ESimType::APMove || GetSimulationType() == ESimType::SPMove)
    gmc_ck(GetSimulationType() == ESimType::SPMove ? !bPredictedMove && !bValidMove : true)

    if (bPredictedMove)
    {
      gmc_ck(GetSimulationType() == ESimType::APMove)

      if (ShouldReplicateForPrediction(Settings))
      {
        return true;
      }
      if (!bValidMove && ShouldReplicateForCorrection(Settings))
      {
        return true;
      }
    }
    else
    {
      if (ShouldReplicateForSimulation(Settings))
      {
        return true;
      }
    }

    return false;
  }

  bool ShouldReplicateForSimulation(const FServerSettings& Settings) const
  {
    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bReplicateForSimulation)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bReplicateForSimulation)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldReplicateForPrediction(const FServerSettings& Settings) const
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove || GetSimulationType() == ESimType::APMove)

    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bReplicateForPrediction)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bReplicateForPrediction)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldReplicateForCorrection(const FServerSettings& Settings) const
  {
    gmc_ck(GetSimulationType() == ESimType::APMove)

    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bReplicateForCorrection)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bReplicateForCorrection)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldUseClientValue(const FServerSettings& Settings) const
  {
    gmc_ck(GetSimulationType() != ESimType::None)

    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bUseClientValue)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bUseClientValue)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldValidate(const FServerSettings& Settings) const
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove)

    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bValidate)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bValidate)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldValidate(const FClientSettings& Settings) const
  {
    gmc_ck(GetSimulationType() == ESimType::APMove)

    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bValidate)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bValidate)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldForceNetUpdate(const FServerSettings& Settings) const
  {
    gmc_ck(GetSimulationType() == ESimType::SPMove)

    if (GetSimulationState() == ESimState::Input)
    {
      if (Settings.InputState.bForceUpdateOnChange)
      {
        return true;
      }
      return false;
    }

    if (GetSimulationState() == ESimState::Output)
    {
      if (Settings.OutputState.bForceUpdateOnChange)
      {
        return true;
      }
      return false;
    }

    gmc_ckne()
    return false;
  }

  bool ShouldProcess(int32 Filters, EDataFilterMode FilterMode, const TSyncTypeData<UType>& Data) const
  {
    gmc_ck(FilterMode == EDataFilterMode::Inclusive || FilterMode == EDataFilterMode::Exclusive)

    const bool bExclusiveFiltering = FilterMode == EDataFilterMode::Exclusive;

    if (!bExclusiveFiltering && Filters == int32(EDataFilter::None))
    {
      return true;
    }

    // ESimState::None is only valid when all data is processed.
    gmc_ck(GetSimulationState() == ESimState::Input || GetSimulationState() == ESimState::Output)

    const bool bIsInputState = GetSimulationState() == ESimState::Input;

    {
      const auto& ServerSettings = bIsInputState ? Data.Settings.ServerSettings.InputState : Data.Settings.ServerSettings.OutputState;

      if (Filters & int32(EDataFilter::SV_ReplicateForSimulation) && ServerSettings.bReplicateForSimulation)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::SV_ReplicateForPrediction) && ServerSettings.bReplicateForPrediction)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::SV_ReplicateForCorrection) && ServerSettings.bReplicateForCorrection)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::SV_Validate) && ServerSettings.bValidate)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::SV_UseClientValue) && ServerSettings.bUseClientValue)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::SV_ForceUpdateOnChange) && ServerSettings.bForceUpdateOnChange)
      {
        return bExclusiveFiltering;
      }
    }

    {
      const auto& ClientSettings = bIsInputState ? Data.Settings.ClientSettings.InputState : Data.Settings.ClientSettings.OutputState;

      if (Filters & int32(EDataFilter::CL_SendToServer) && ClientSettings.bSendToServer)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::CL_Validate) && ClientSettings.bValidate)
      {
        return bExclusiveFiltering;
      }
    }

    {
      const auto& LocalSettings = Data.Settings.LocalSettings;

      if (Filters & int32(EDataFilter::Wildcard1) && LocalSettings.bWildcard1)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::Wildcard2) && LocalSettings.bWildcard2)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::Wildcard3) && LocalSettings.bWildcard3)
      {
        return bExclusiveFiltering;
      }

      if (Filters & int32(EDataFilter::Wildcard4) && LocalSettings.bWildcard4)
      {
        return bExclusiveFiltering;
      }
    }

    return !bExclusiveFiltering;
  }

private:

  ESimState SimulationState{};
  ESimType SimulationType{};

  bool ShouldCombineInternal(
    const FClientSettings& Settings,
    const UType& Value,
    const UType& PreviousValue,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove)
    gmc_ck(GetSimulationState() == ESimState::Input)

    switch (static_cast<EGMC_CombineMode>(Settings.InputState.CombineMode))
    {
      case EGMC_CombineMode::AlwaysCombine:
      {
        return true;
      }
      case EGMC_CombineMode::CombineIfUnchanged:
      {
        return IsEqual(Value, PreviousValue, Component);
      }
      case EGMC_CombineMode::Default:
      {
        return ShouldCombine(Value, PreviousValue, Component);
      }
      case EGMC_CombineMode::NeverCombine:
      {
        return false;
      }
      default:
      {
        gmc_ckne()
      }
    }

    gmc_ckne()
    return false;
  }

  UType& GetLastSerializedValue(
    TSyncTypeData<UType>& Data,
    const AActor* const TargetConnection,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    auto Connection = const_cast<AActor*>(TargetConnection);

    if (!Data.LastSerializedValue.Contains(Connection))
    {
      auto& NewValue = Data.LastSerializedValue.Add(Connection);
      InitDefault(NewValue, Component);
      return NewValue;
    }

    return Data.LastSerializedValue[Connection];
  }

  virtual void NetSerializeClientToServer(
    FArchive& Ar,
    UPackageMap* Map,
    const AActor* const TargetConnection,
    const FGMC_NetInfo& NetInfo,
    const FGMC_MetaData& MetaData,
    bool bForceFullSerialization
  )
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove)

    if (Ar.IsSaving())
    {
      gmc_ck(TargetConnection)
      for (int32 Index = 0; Index < GetNumData(); ++Index)
      {
        auto& Data = GetData(Index);
        if (!ShouldSendToServer(GetClientSettings(Data)))
        {
          continue;
        }

        Ar.SerializeBits(&Data.bSuspended, 1);
        if (Data.bSuspended)
        {
          Data.bWasSuspended = true;
          continue;
        }

        auto& bWasSuspended = Data.bWasSuspended;
        auto& Value = Data.Value;
        auto& LastSerializedValue = GetLastSerializedValue(Data, TargetConnection, NetInfo.OwningComponent.Get());
        bool bNewValue = bForceFullSerialization || bWasSuspended || !IsEqual(Value, LastSerializedValue, NetInfo.OwningComponent.Get());
        Ar.SerializeBits(&bNewValue, 1);
        if (bNewValue)
        {
          Quantize(Value, NetInfo.OwningComponent.Get());
          Serialize(Value, Ar, Map, NetInfo, MetaData);
          LastSerializedValue = Value;
        }

        bWasSuspended = false;
      }
    }
    else
    {
      gmc_ck(Ar.IsLoading())
      // It is possible that TargetConnection is nullptr during loading.
      for (int32 Index = 0; Index < GetNumData(); ++Index)
      {
        auto& Data = GetData(Index);

        // Only relevant for the saving machine.
        gmc_ck(!Data.bWasSuspended)

        if (!ShouldSendToServer(GetClientSettings(Data)))
        {
          continue;
        }

        Ar.SerializeBits(&Data.bSuspended, 1);
        if (Data.bSuspended)
        {
          continue;
        }

        auto& Value = Data.Value;
        auto& LastSerializedValue = GetLastSerializedValue(Data, TargetConnection, NetInfo.OwningComponent.Get());
        bool bNewValue = false;
        Ar.SerializeBits(&bNewValue, 1);
        if (bNewValue)
        {
          Serialize(Value, Ar, Map, NetInfo, MetaData);
          Quantize(Value, NetInfo.OwningComponent.Get());
          LastSerializedValue = Value;
        }
        else
        {
          Value = LastSerializedValue;
        }
      }
    }
  }

  virtual void NetSerializeServerToClient(
    FArchive& Ar,
    UPackageMap* Map,
    const AActor* const TargetConnection,
    const FGMC_NetInfo& NetInfo,
    const FGMC_MetaData& MetaData,
    bool bForceFullSerialization
  )
  {
    gmc_ck(GetSimulationType() == ESimType::APMove || GetSimulationType() == ESimType::SPMove)

    if (Ar.IsSaving())
    {
      gmc_ck(TargetConnection)
      for (int32 Index = 0; Index < GetNumData(); ++Index)
      {
        auto& Data = GetData(Index);
        if (!ShouldReplicateToClient(GetServerSettings(Data), MetaData.bPredictedClientMove, MetaData.bValidClientMove))
        {
          continue;
        }

        Ar.SerializeBits(&Data.bSuspended, 1);
        if (Data.bSuspended)
        {
          Data.bWasSuspended = true;
          continue;
        }

        auto& bWasSuspended = Data.bWasSuspended;
        auto& Value = Data.Value;
        auto& LastSerializedValue = GetLastSerializedValue(Data, TargetConnection, NetInfo.OwningComponent.Get());
        bool bNewValue = bForceFullSerialization || bWasSuspended || !IsEqual(Value, LastSerializedValue, NetInfo.OwningComponent.Get());
        Ar.SerializeBits(&bNewValue, 1);
        if (bNewValue)
        {
          Quantize(Value, NetInfo.OwningComponent.Get());
          Serialize(Value, Ar, Map, NetInfo, MetaData);
          LastSerializedValue = Value;
        }

        bWasSuspended = false;
      }
    }
    else
    {
      gmc_ck(Ar.IsLoading())
      // It is possible that TargetConnection is nullptr during loading.
      for (int32 Index = 0; Index < GetNumData(); ++Index)
      {
        auto& Data = GetData(Index);

        // Only relevant for the saving machine.
        gmc_ck(!Data.bWasSuspended)

        if (!ShouldReplicateToClient(GetServerSettings(Data), MetaData.bPredictedClientMove, MetaData.bValidClientMove))
        {
          continue;
        }

        Ar.SerializeBits(&Data.bSuspended, 1);
        if (Data.bSuspended)
        {
          continue;
        }

        auto& Value = Data.Value;
        auto& LastSerializedValue = GetLastSerializedValue(Data, TargetConnection, NetInfo.OwningComponent.Get());
        bool bNewValue = false;
        Ar.SerializeBits(&bNewValue, 1);
        if (bNewValue)
        {
          Serialize(Value, Ar, Map, NetInfo, MetaData);
          Quantize(Value, NetInfo.OwningComponent.Get());
          LastSerializedValue = Value;
        }
        else
        {
          Value = LastSerializedValue;
        }
      }
    }
  }
};

template<typename T>
struct TSyncTypeSingle : public TSyncTypeBase<T>
{
  using UType = TUnderlyingSyncType<T>;

protected:

  TSyncTypeData<UType>& GetData(int32 DataIndex) override final { return SingleData; }

  const TSyncTypeData<UType>& GetData(int32 DataIndex) const override final { return SingleData; }

  int32 GetNumData() const override final { return this->NumSingle; }

  virtual UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const = 0;

  virtual void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const = 0;

  virtual UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const = 0;

  virtual UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const = 0;

public:

  void Initialize(
    FSyncSettings Settings,
    ESimState SimState,
    ESimType SimType,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    this->InitializeBase(SimState, SimType);

    auto& Data = this->GetData(this->IndexSingle);
    Data = {};
    Data.Settings = Settings;
    this->InitDefault(Data.Value, Component);
  }

  void Process(
    FDataOpDirective Directive,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    int32 Operations = Directive.GetDataOps();
    if (Operations == 0) return;

    const bool bSave = Operations & int32(EDataOp::Save);
    const bool bQuantize = Operations & int32(EDataOp::Quantize);
    const bool bApply = Operations & int32(EDataOp::Apply);

    auto& Data = this->GetData(this->IndexSingle);

    if (bSave && this->ShouldProcess(Directive.GetSaveFilterMask(), (EDataFilterMode)Directive.GetSaveFilterMode(), Data))
    {
      Data.Value = this->GetValue(bUseRelative, Component);
    }

    if (bQuantize && this->ShouldProcess(Directive.GetQuantizeFilterMask(), (EDataFilterMode)Directive.GetQuantizeFilterMode(), Data))
    {
      this->Quantize(Data.Value, Component);
    }

    if (bApply && this->ShouldProcess(Directive.GetApplyFilterMask(), (EDataFilterMode)Directive.GetApplyFilterMode(), Data))
    {
      this->ApplyValue(Data.Value, bUseRelative, Component);
    }
  }

  bool IsValidCheckOtherClientValue(
    const TSyncTypeBase<T>& Other,
    int32 Filters,
    EDataFilterMode FilterMode,
    EGMC_SyncType& OutFirstDeviatingSyncType,
    int32& OutFirstDeviatingSyncTypeIndex,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(this->GetSimulationType() == ESimType::APMove)

    const auto& Data = this->GetData(this->IndexSingle);
    const auto& DataOther = Other.GetData(this->IndexSingle);
    gmc_ck(Data.Settings.Compare(DataOther.Settings))

    if (!this->ShouldValidate(this->GetClientSettings(Data)) || !this->ShouldProcess(Filters, FilterMode, Data))
    {
      return true;
    }

    if (!this->IsValid(Data.Value, DataOther.Value, Component))
    {
      if (OutFirstDeviatingSyncType == EGMC_SyncType::MAX)
      {
        OutFirstDeviatingSyncType = this->GetTypeEnum(Component);
        gmc_ck(OutFirstDeviatingSyncTypeIndex == -1)
      }

      GMC_LOG(
        LogGMCReplication,
        ((UActorComponent*)Component)->GetOwner(),
        Verbose,
        TEXT("%s was not valid: client value = %s, server value = %s"),
        *this->GetLogName(Component),
        *this->GetLogValue(DataOther.Value, Component),
        *this->GetLogValue(Data.Value, Component)
      )

      return false;
    }

    return true;
  }

  bool IsValidCheckCurrentServerValue(
    int32 Filters,
    EDataFilterMode FilterMode,
    bool bUseRelative,
    EGMC_SyncType& OutFirstDeviatingSyncType,
    int32& OutFirstDeviatingSyncTypeIndex,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(this->GetSimulationType() == ESimType::LocalMove)
    gmc_ck(this->GetSimulationState() == ESimState::Input || this->GetSimulationState() == ESimState::Output)

    const auto& Data = this->GetData(this->IndexSingle);

    if (!this->ShouldValidate(this->GetServerSettings(Data)) || !this->ShouldProcess(Filters, FilterMode, Data))
    {
      return true;
    }

    const auto& CurrentValue = this->GetValue(bUseRelative, Component);
    if (!this->IsValid(Data.Value, CurrentValue, Component))
    {
      if (OutFirstDeviatingSyncType == EGMC_SyncType::MAX)
      {
        OutFirstDeviatingSyncType = this->GetTypeEnum(Component);
        gmc_ck(OutFirstDeviatingSyncTypeIndex == -1)
      }

      GMC_LOG(
        LogGMCReplication,
        ((UActorComponent*)Component)->GetOwner(),
        Verbose,
        TEXT("%s was not valid:  client value = %s  server value = %s"),
        *this->GetLogName(Component),
        *this->GetLogValue(Data.Value, Component),
        *this->GetLogValue(CurrentValue, Component)
      )

      return false;
    }

    return true;
  }

  void ToRelativeValue(
    const USceneComponent* const TargetBase,
    int32 Filters,
    EDataFilterMode FilterMode,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    gmc_ck(this->GetSimulationState() != ESimState::None)

    auto& Data = this->GetData(this->IndexSingle);

    if (!this->ShouldProcess(Filters, FilterMode, Data))
    {
      return;
    }

    Data.Value = this->ToRelative(Data.Value, TargetBase, Component);
  }

  void ToAbsoluteValue(
    const USceneComponent* const SourceBase,
    int32 Filters,
    EDataFilterMode FilterMode,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    gmc_ck(this->GetSimulationState() != ESimState::None)

    auto& Data = this->GetData(this->IndexSingle);

    if (!this->ShouldProcess(Filters, FilterMode, Data))
    {
      return;
    }

    Data.Value = this->ToAbsolute(Data.Value, SourceBase, Component);
  }

  void Log(
    ::UGMC_ReplicationCmp* const Component,
    const FString& PrefixStr = TEXT("")
  ) const
  {
    GMC_LOG(
      LogGMCReplication,
      ((UActorComponent*)Component)->GetOwner(),
      Log,
      TEXT("%s")
      TEXT("%s:  ")
      TEXT("%s"),
      *PrefixStr,
      *this->GetLogName(Component),
      *this->GetLogValue(this->GetData(this->IndexSingle).Value, Component)
    )
  }

private:

  TSyncTypeData<UType> SingleData;
};

template<typename T>
struct TSyncTypeMulti : public TSyncTypeBase<T>
{
  using UType = TUnderlyingSyncType<T>;

protected:

  TSyncTypeData<UType>& GetData(int32 DataIndex) override final { return MultiData[DataIndex]; }

  const TSyncTypeData<UType>& GetData(int32 DataIndex) const override final { return MultiData[DataIndex]; }

  int32 GetNumData() const override final { return MultiData.Num(); }

public:

  void Initialize(
    const TMemberAlias<T>& Alias,
    ESimState SimState,
    ESimType SimType,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    this->InitializeBase(SimState, SimType);

    MultiData.Reset();
    for (int32 Index = 0; Index < Alias.GetNumData(); ++Index)
    {
      TSyncTypeData<UType> NewData;
      NewData.Settings = Alias.GetData(Index).Settings;
      this->InitDefault(NewData.Value, Component);
      MultiData.Emplace(MoveTemp(NewData));
    }
  }

  void Process(
    const TMemberAlias<T>& Alias,
    FDataOpDirective Directive,
    ::UGMC_ReplicationCmp* const Component
  )
  {
    int32 Operations = Directive.GetDataOps();
    if (Operations == 0) return;

    const bool bSave = Operations & int32(EDataOp::Save);
    const bool bQuantize = Operations & int32(EDataOp::Quantize);
    const bool bApply = Operations & int32(EDataOp::Apply);

    gmc_ck(this->GetNumData() == Alias.GetNumData())
    for (int32 Index = 0; Index < this->GetNumData(); ++Index)
    {
      auto& Data = this->GetData(Index);
      const auto& AliasData = Alias.GetData(Index);
      gmc_ck(Data.Settings.Compare(AliasData.Settings))

      if (bSave && this->ShouldProcess(Directive.GetSaveFilterMask(), (EDataFilterMode)Directive.GetSaveFilterMode(), Data))
      {
        Data.Value = *AliasData.ValueRef;
      }

      if (bQuantize && this->ShouldProcess(Directive.GetQuantizeFilterMask(), (EDataFilterMode)Directive.GetQuantizeFilterMode(), Data))
      {
        this->Quantize(Data.Value, Component);
      }

      if (bApply && this->ShouldProcess(Directive.GetApplyFilterMask(), (EDataFilterMode)Directive.GetApplyFilterMode(), Data))
      {
        gmc_ck(AliasData.ValueRef)
        *AliasData.ValueRef = Data.Value;
      }
    }
  }

  bool IsValidCheckOtherClientValue(
    const TSyncTypeBase<T>& Other,
    int32 Filters,
    EDataFilterMode FilterMode,
    EGMC_SyncType& OutFirstDeviatingSyncType,
    int32& OutFirstDeviatingSyncTypeIndex,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(this->GetSimulationType() == ESimType::APMove)

    gmc_ck(this->GetNumData() == Other.GetNumData())
    for (int32 Index = 0; Index < this->GetNumData(); ++Index)
    {
      const auto& Data = this->GetData(Index);
      const auto& DataOther = Other.GetData(Index);
      gmc_ck(Data.Settings.Compare(DataOther.Settings))

      if (!this->ShouldValidate(this->GetClientSettings(Data)) || !this->ShouldProcess(Filters, FilterMode, Data))
      {
        continue;
      }

      if (!this->IsValid(Data.Value, DataOther.Value, Component))
      {
        if (OutFirstDeviatingSyncType == EGMC_SyncType::MAX)
        {
          gmc_ck(OutFirstDeviatingSyncTypeIndex == -1)
          OutFirstDeviatingSyncType = this->GetTypeEnum(Component);
          OutFirstDeviatingSyncTypeIndex = Index;
        }

        GMC_LOG(
          LogGMCReplication,
          ((UActorComponent*)Component)->GetOwner(),
          Verbose,
          TEXT("%s[%d] was not valid:  client value = %s  server value = %s"),
          *this->GetLogName(Component),
          Index,
          *this->GetLogValue(DataOther.Value, Component),
          *this->GetLogValue(Data.Value, Component)
        )

        return false;
      }
    }

    return true;
  }

  bool IsValidCheckCurrentServerValue(
    const TMemberAlias<T>& Alias,
    int32 Filters,
    EDataFilterMode FilterMode,
    EGMC_SyncType& OutFirstDeviatingSyncType,
    int32& OutFirstDeviatingSyncTypeIndex,
    ::UGMC_ReplicationCmp* const Component
  ) const
  {
    gmc_ck(this->GetSimulationType() == ESimType::LocalMove)
    gmc_ck(this->GetSimulationState() == ESimState::Input || this->GetSimulationState() == ESimState::Output)

    gmc_ck(this->GetNumData() == Alias.GetNumData())
    for (int32 Index = 0; Index < this->GetNumData(); ++Index)
    {
      const auto& Data = this->GetData(Index);
      const auto& AliasData = Alias.GetData(Index);
      gmc_ck(Data.Settings.Compare(AliasData.Settings))

      if (!this->ShouldValidate(this->GetServerSettings(Data)) || !this->ShouldProcess(Filters, FilterMode, Data))
      {
        return true;
      }

      const auto& CurrentValue = *AliasData.ValueRef;
      if (!this->IsValid(Data.Value, CurrentValue, Component))
      {
        if (OutFirstDeviatingSyncType == EGMC_SyncType::MAX)
        {
          gmc_ck(OutFirstDeviatingSyncTypeIndex == -1)
          OutFirstDeviatingSyncType = this->GetTypeEnum(Component);
          OutFirstDeviatingSyncTypeIndex = Index;
        }

        GMC_LOG(
          LogGMCReplication,
          ((UActorComponent*)Component)->GetOwner(),
          Verbose,
          TEXT("%s[%d] was not valid: client value = %s, server value = %s"),
          *this->GetLogName(Component),
          Index,
          *this->GetLogValue(Data.Value, Component),
          *this->GetLogValue(CurrentValue, Component)
        )

        return false;
      }
    }

    return true;
  }

  void Log(
    ::UGMC_ReplicationCmp* const Component,
    const FString& PrefixStr = TEXT("")
  ) const
  {
    FString LogMsg = this->GetLogName(Component) + TEXT(":");

    int32 Index = 0;
    for (; Index < this->GetNumData(); ++Index)
    {
      LogMsg += TEXT("  [") + FString::Printf(TEXT("%d"), Index) + TEXT("] ") + this->GetLogValue(this->GetData(Index).Value, Component);
    }

    if (Index > 0)
    {
      GMC_LOG(
        LogGMCReplication,
        ((UActorComponent*)Component)->GetOwner(),
        Log,
        TEXT("%s")
        TEXT("%s"),
        *PrefixStr,
        *LogMsg
      )
    }
  }

private:

  TArray<TSyncTypeData<UType>> MultiData;
};

template<typename T>
struct TSyncType : public TSyncTypeMulti<T> {};

//==============================================================================================================================================================
// Integrated Sync Types

template<>
struct GMCCORE_API TSyncType<FActorBase> : public TSyncTypeSingle<FActorBase>
{
  UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const override;

  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FLinearVelocity> : public TSyncTypeSingle<FLinearVelocity>
{
  UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const override;

  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FAngularVelocity> : public TSyncTypeSingle<FAngularVelocity>
{
  UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const override;

  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FActorLocation> : public TSyncTypeSingle<FActorLocation>
{
  UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const override;

  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FActorRotation> : public TSyncTypeSingle<FActorRotation>
{
  UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const override;

  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FActorScale> : public TSyncTypeSingle<FActorScale>
{
  UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const override;

  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FControlRotation> : public TSyncTypeSingle<FControlRotation>
{
  UType GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  void ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const override;

  UType ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const override;

  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

//==============================================================================================================================================================
// Generic Sync Types

template<>
struct GMCCORE_API TSyncType<FBit> : public TSyncTypeMulti<FBit>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;

private:

  // Conditional serialization and suspension don't make sense for bools since they contain just 1 bit of information anyway.

  void NetSerializeClientToServer(
    FArchive& Ar,
    UPackageMap* Map,
    const AActor* const TargetConnection,
    const FGMC_NetInfo& NetInfo,
    const FGMC_MetaData& MetaData,
    bool bForceFullSerialization
  ) override
  {
    gmc_ck(GetSimulationType() == ESimType::LocalMove)

    // It is possible that TargetConnection is nullptr during loading.
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      auto& Data = GetData(Index);
      if (!ShouldSendToServer(GetClientSettings(Data)))
      {
        continue;
      }

      Data.bSuspended = Data.bWasSuspended = false;

      auto& Value = Data.Value;
      auto& LastSerializedValue = GetLastSerializedValue(Data, TargetConnection, NetInfo.OwningComponent.Get());

      if (Ar.IsSaving())
      {
        Quantize(Value, NetInfo.OwningComponent.Get());
        Serialize(Value, Ar, Map, NetInfo, MetaData);
      }
      else
      {
        gmc_ck(Ar.IsLoading())
        Serialize(Value, Ar, Map, NetInfo, MetaData);
        Quantize(Value, NetInfo.OwningComponent.Get());
      }

      LastSerializedValue = Value;
    }
  }

  void NetSerializeServerToClient(
    FArchive& Ar,
    UPackageMap* Map,
    const AActor* const TargetConnection,
    const FGMC_NetInfo& NetInfo,
    const FGMC_MetaData& MetaData,
    bool bForceFullSerialization
  ) override
  {
    gmc_ck(GetSimulationType() == ESimType::APMove || GetSimulationType() == ESimType::SPMove)

    // It is possible that TargetConnection is nullptr during loading.
    for (int32 Index = 0; Index < GetNumData(); ++Index)
    {
      auto& Data = GetData(Index);
      if (!ShouldReplicateToClient(GetServerSettings(Data), MetaData.bPredictedClientMove, MetaData.bValidClientMove))
      {
        continue;
      }

      Data.bSuspended = Data.bWasSuspended = false;

      auto& Value = Data.Value;
      auto& LastSerializedValue = GetLastSerializedValue(Data, TargetConnection, NetInfo.OwningComponent.Get());

      if (Ar.IsSaving())
      {
        Quantize(Value, NetInfo.OwningComponent.Get());
        Serialize(Value, Ar, Map, NetInfo, MetaData);
      }
      else
      {
        gmc_ck(Ar.IsLoading())
        Serialize(Value, Ar, Map, NetInfo, MetaData);
        Quantize(Value, NetInfo.OwningComponent.Get());
      }

      LastSerializedValue = Value;
    }
  }
};

template<>
struct GMCCORE_API TSyncType<FUnsignedInt4> : public TSyncTypeMulti<FUnsignedInt4>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FUnsignedInt8> : public TSyncTypeMulti<FUnsignedInt8>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FInt32> : public TSyncTypeMulti<FInt32>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FFloat> : public TSyncTypeMulti<FFloat>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FFloat2Dec> : public TSyncTypeMulti<FFloat2Dec>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FDouble> : public TSyncTypeMulti<FDouble>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FDouble2Dec> : public TSyncTypeMulti<FDouble2Dec>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FDoubleAsFloat> : public TSyncTypeMulti<FDoubleAsFloat>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FVector2Double2Dec> : public TSyncTypeMulti<FVector2Double2Dec>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FVector3Double2Dec> : public TSyncTypeMulti<FVector3Double2Dec>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FRotator3Double2Dec> : public TSyncTypeMulti<FRotator3Double2Dec>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FActorReference> : public TSyncTypeMulti<FActorReference>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FActorComponentReference> : public TSyncTypeMulti<FActorComponentReference>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FAnimMontageReference> : public TSyncTypeMulti<FAnimMontageReference>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FName> : public TSyncTypeMulti<FName>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FGameplayTag> : public TSyncTypeMulti<FGameplayTag>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FGameplayTagContainer> : public TSyncTypeMulti<FGameplayTagContainer>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

template<>
struct GMCCORE_API TSyncType<FInstancedStruct> : public TSyncTypeMulti<FInstancedStruct>
{
  void InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component) override;

  void Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  bool ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const override;

  void Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component) override;

  bool IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  bool IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const override;

  UType Interpolate(
    uint8 Function,
    const UType& StartValue,
    const UType& TargetValue,
    const UType& PreviousValue,
    const UType& NextValue,
    const USceneComponent* const StartActorBase,
    const USceneComponent* const TargetActorBase,
    const USceneComponent* const PreviousActorBase,
    const USceneComponent* const NextActorBase,
    double Ratio,
    bool bUseRelative,
    ::UGMC_ReplicationCmp* const Component
  ) const override;

  void Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const override;

  FString GetLogName(::UGMC_ReplicationCmp* const Component) const override;

  FString GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const override;

  EGMC_SyncType GetTypeEnum(::UGMC_ReplicationCmp* const Component) const override;
};

#ifdef GMC_ENABLE_USER_SYNC_TYPES
#include GMC_USER_INCLUDE_PATH_TYPE_INTF
#endif

}
