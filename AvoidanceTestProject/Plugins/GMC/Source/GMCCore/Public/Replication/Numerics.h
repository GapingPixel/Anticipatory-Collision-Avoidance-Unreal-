// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCAssert.h"
#include "Numerics.generated.h"

UENUM()
enum class EGMC_NumericType : uint8
{
  None,
  Int,
  Float,
  Vector2D,
  Vector,
  Rotator,
  Quat,
};

USTRUCT(BlueprintType)
struct FGMC_NumericValue
{
  GENERATED_BODY()

  static inline constexpr int32 NUM_COMPONENTS = 4;

  FGMC_NumericValue() = default;

  FGMC_NumericValue(EGMC_NumericType NumericType, double Index0, double Index1 = 0., double Index2 = 0., double Index3 = 0.)
  {
    gmc_ck(NumericType != EGMC_NumericType::None)
    Type = NumericType;
    Value0 = Int = Float = Vector2D.X = Vector.X = Rotator.Roll = Quat.X = Index0;
    Value1 = Vector2D.Y = Vector.Y = Rotator.Pitch = Quat.Y = Index1;
    Value2 = Vector.Z = Rotator.Yaw = Quat.Z = Index2;
    Value3 = Quat.W = Index3;
  }

  FGMC_NumericValue(EGMC_NumericType NumericType, FVector2D InVector2D)
  {
    gmc_ck(NumericType != EGMC_NumericType::None)
    Type = NumericType;
    Value0 = Int = Float = Vector2D.X = Vector.X = Rotator.Roll = InVector2D.X;
    Value1 = Vector2D.Y = Vector.Y = Rotator.Pitch = Quat.Y = InVector2D.Y;
    Value2 = Vector.Z = Rotator.Yaw = Quat.Z = 0.;
    Value3 = Quat.W = 0.;
  }

  FGMC_NumericValue(EGMC_NumericType NumericType, FVector InVector)
  {
    gmc_ck(NumericType != EGMC_NumericType::None)
    Type = NumericType;
    Value0 = Int = Float = Vector2D.X = Vector.X = Rotator.Roll = Quat.X = InVector.X;
    Value1 = Vector2D.Y = Vector.Y = Rotator.Pitch = Quat.Y = InVector.Y;
    Value2 = Vector.Z = Rotator.Yaw = Quat.Z = InVector.Z;
    Value3 = Quat.W = 0.;
  }

  FGMC_NumericValue(EGMC_NumericType NumericType, FRotator InRotator)
  {
    gmc_ck(NumericType != EGMC_NumericType::None)
    Type = NumericType;
    Value0 = Int = Float = Vector2D.X = Vector.X = Rotator.Roll = Quat.X = InRotator.Roll;
    Value1 = Vector2D.Y = Vector.Y = Rotator.Pitch = Quat.Y = InRotator.Pitch;
    Value2 = Vector.Z = Rotator.Yaw = Quat.Z = InRotator.Yaw;
    Value3 = Quat.W = 0.;
  }

  FGMC_NumericValue(EGMC_NumericType NumericType, FQuat InQuat)
  {
    gmc_ck(NumericType != EGMC_NumericType::None)
    Type = NumericType;
    Value0 = Int = Float = Vector2D.X = Vector.X = Rotator.Roll = Quat.X = InQuat.X;
    Value1 = Vector2D.Y = Vector.Y = Rotator.Pitch = Quat.Y = InQuat.Y;
    Value2 = Vector.Z = Rotator.Yaw = Quat.Z = InQuat.Z;
    Value3 = Quat.W = InQuat.W;
  }

  FGMC_NumericValue(void* NonNumeric)
  {
    Type = EGMC_NumericType::None;
    BareValue = NonNumeric;
  }

  EGMC_NumericType GetType() const
  {
    return Type;
  }

  double GetValue(int32 Index) const
  {
    switch (Index)
    {
      case 0: return Value0;
      case 1: return Value1;
      case 2: return Value2;
      case 3: return Value3;
      default: gmc_ckne();
    }
    gmc_ckne();
    return 0.;
  }

  int64 GetInt() const
  {
    return Int;
  }

  double GetFloat() const
  {
    return Float;
  }

  FVector2D GetVector2D() const
  {
    return Vector2D;
  }

  FVector GetVector() const
  {
    return Vector;
  }

  FRotator GetRotator() const
  {
    return Rotator;
  }

  FQuat GetQuat() const
  {
    return Quat;
  }

  void* GetBare() const
  {
    return BareValue;
  }

  // Constructors to avoid ambiguous overload resolutions and narrowing conversion errors.

  FGMC_NumericValue(EGMC_NumericType NumericType, int8 Index0, int8 Index1 = 0, int8 Index2 = 0, int8 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

  FGMC_NumericValue(EGMC_NumericType NumericType, int16 Index0, int16 Index1 = 0, int16 Index2 = 0, int16 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

  FGMC_NumericValue(EGMC_NumericType NumericType, int32 Index0, int32 Index1 = 0, int32 Index2 = 0, int32 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

  FGMC_NumericValue(EGMC_NumericType NumericType, int64 Index0, int64 Index1 = 0, int64 Index2 = 0, int64 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

  FGMC_NumericValue(EGMC_NumericType NumericType, uint8 Index0, uint8 Index1 = 0, uint8 Index2 = 0, uint8 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

  FGMC_NumericValue(EGMC_NumericType NumericType, uint16 Index0, uint16 Index1 = 0, uint16 Index2 = 0, uint16 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

  FGMC_NumericValue(EGMC_NumericType NumericType, uint32 Index0, uint32 Index1 = 0, uint32 Index2 = 0, uint32 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

  FGMC_NumericValue(EGMC_NumericType NumericType, uint64 Index0, uint64 Index1 = 0, uint64 Index2 = 0, uint64 Index3 = 0)
    : FGMC_NumericValue(NumericType, (double)Index0, (double)Index1, (double)Index2, (double)Index3) {}

private:

  void* BareValue{};

  FQuat Quat{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  EGMC_NumericType Type{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  double Value0{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  double Value1{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  double Value2{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  double Value3{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  int64 Int{0};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  double Float{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  FVector2D Vector2D{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  FVector Vector{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component", meta = (AllowPrivateAccess = "true"))
  FRotator Rotator{0.};
};
