// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCAssert.h"
#include "FloatPrecision.generated.h"

enum EGMC_FloatPrecision : int32
{
  WholeNumber = 1,
  OneDecimal = 10,
  TwoDecimals = 100,
  ThreeDecimals = 1000,
  FourDecimals = 10000,
  FiveDecimals = 100000,
  SixDecimals = 1000000,
  FullPrecision = 0,
};

UENUM(BlueprintType)
enum class EGMC_FloatPrecisionBlueprint : uint8
{
  WholeNumber,
  OneDecimal,
  TwoDecimals,
  ThreeDecimals,
  FourDecimals,
  FiveDecimals,
  SixDecimals,
  FullPrecision,
};

inline EGMC_FloatPrecision ToNativeEnum(EGMC_FloatPrecisionBlueprint Value)
{
  switch (Value)
  {
    case EGMC_FloatPrecisionBlueprint::WholeNumber: return EGMC_FloatPrecision::WholeNumber;
    case EGMC_FloatPrecisionBlueprint::OneDecimal: return EGMC_FloatPrecision::OneDecimal;
    case EGMC_FloatPrecisionBlueprint::TwoDecimals: return EGMC_FloatPrecision::TwoDecimals;
    case EGMC_FloatPrecisionBlueprint::ThreeDecimals: return EGMC_FloatPrecision::ThreeDecimals;
    case EGMC_FloatPrecisionBlueprint::FourDecimals: return EGMC_FloatPrecision::FourDecimals;
    case EGMC_FloatPrecisionBlueprint::FiveDecimals: return EGMC_FloatPrecision::FiveDecimals;
    case EGMC_FloatPrecisionBlueprint::SixDecimals: return EGMC_FloatPrecision::SixDecimals;
    case EGMC_FloatPrecisionBlueprint::FullPrecision: return EGMC_FloatPrecision::FullPrecision;
    default: gmc_ckne();
  }
  return EGMC_FloatPrecision{};
}
