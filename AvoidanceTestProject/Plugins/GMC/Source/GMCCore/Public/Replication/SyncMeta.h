// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCAssert.h"

namespace GMCReplication
{

enum class ESimState
{
  Input,
  Output,
  None,
};

enum class ESimType
{
  LocalMove, // Player or AI move.
  APMove, // Data replicated to autonomous proxy.
  SPMove, // Data replicated to simulated proxy.
  None, // Temporary data.
};

enum class EDataOp : int32
{
  // Within one function call operations always happen in the order Save -> Quantize -> Apply.
  None       = 0x0,
  Save       = 0x1,
  Quantize   = 0x2,
  Apply      = 0x4,
  AllDataOps = -1,
};

enum class EDataFilter : int32
{
  None                      = 0x0000,
  SV_ReplicateForSimulation = 0x0001,
  SV_ReplicateForPrediction = 0x0002,
  SV_ReplicateForCorrection = 0x0004,
  SV_Validate               = 0x0008,
  SV_UseClientValue         = 0x0010,
  SV_ForceUpdateOnChange    = 0x0020,
  CL_SendToServer           = 0x0040,
  CL_Validate               = 0x0080,
  Wildcard1                 = 0x0100,
  Wildcard2                 = 0x0200,
  Wildcard3                 = 0x0400,
  Wildcard4                 = 0x0800,
  AllDataFilters            = -1,
};

enum class EDataFilterMode : int32
{
  Inclusive, // Process everything by default, filters specify data that should not be processed.
  Exclusive, // Process nothing by default, filters specify data that should be processed.
};

struct FDataOpDirective
{
  // Convenience constructor for using the same filter settings with all passed data ops.
  FDataOpDirective(int32 InDataOps, int32 InFilters = int32(EDataFilter::None), int32 InFilterMode = int32(EDataFilterMode::Inclusive))
  {
    SetDataOps(InDataOps);
    SetSaveFilters(InFilters, InFilterMode);
    SetQuantizeFilters(InFilters, InFilterMode);
    SetApplyFilters(InFilters, InFilterMode);
  }

  void SetDataOps(int32 InDataOps)
  {
    SaveSettings.Op = EDataOp(InDataOps & int32(EDataOp::Save));
    QuantizeSettings.Op = EDataOp(InDataOps & int32(EDataOp::Quantize));
    ApplySettings.Op = EDataOp(InDataOps & int32(EDataOp::Apply));
  }

  int32 GetDataOps() const
  {
    gmc_ck(int32(SaveSettings.Op) > int32(EDataOp::AllDataOps))
    gmc_ck(int32(QuantizeSettings.Op) > int32(EDataOp::AllDataOps))
    gmc_ck(int32(ApplySettings.Op) > int32(EDataOp::AllDataOps))
    return (int32)SaveSettings.Op | (int32)QuantizeSettings.Op | (int32)ApplySettings.Op;
  }

  void SetSaveFilters(int32 InFilerMask, int32 InFilterMode = int32(EDataFilterMode::Inclusive))
  {
    SaveSettings.FilterMask = InFilerMask;
    SaveSettings.FilterMode = (EDataFilterMode)InFilterMode;
  }

  int32 GetSaveFilterMask() const
  {
    return SaveSettings.FilterMask;
  }

  int32 GetSaveFilterMode() const
  {
    return (int32)SaveSettings.FilterMode;
  }

  void SetQuantizeFilters(int32 InFilerMask, int32 InFilterMode = int32(EDataFilterMode::Inclusive))
  {
    QuantizeSettings.FilterMask = InFilerMask;
    QuantizeSettings.FilterMode = (EDataFilterMode)InFilterMode;
  }

  int32 GetQuantizeFilterMask() const
  {
    return QuantizeSettings.FilterMask;
  }

  int32 GetQuantizeFilterMode() const
  {
    return (int32)QuantizeSettings.FilterMode;
  }

  void SetApplyFilters(int32 InFilerMask, int32 InFilterMode = int32(EDataFilterMode::Inclusive))
  {
    ApplySettings.FilterMask = InFilerMask;
    ApplySettings.FilterMode = (EDataFilterMode)InFilterMode;
  }

  int32 GetApplyFilterMask() const
  {
    return ApplySettings.FilterMask;
  }

  int32 GetApplyFilterMode() const
  {
    return (int32)ApplySettings.FilterMode;
  }

private:

  struct FOpSettings
  {
    EDataOp Op{EDataOp::None};
    int32 FilterMask{0};
    EDataFilterMode FilterMode{EDataFilterMode::Inclusive};
  };

  FOpSettings SaveSettings{};
  FOpSettings QuantizeSettings{};
  FOpSettings ApplySettings{};
};

}
