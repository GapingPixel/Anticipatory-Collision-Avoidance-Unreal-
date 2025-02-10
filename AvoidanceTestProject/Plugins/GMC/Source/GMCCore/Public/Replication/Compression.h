// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "FloatPrecision.h"

namespace GMCCompression
{

template<typename T, typename TEnableIf<TIsSigned<T>::Value && sizeof(T) <= 4u, int32>::Type X = -1>
uint32 GetBitsNeeded(T Value)
{
  typedef typename TUnsignedIntType<sizeof(T)>::Type SmallUnsignedType;
  const uint32 MassagedValue = uint32(SmallUnsignedType(T(Value ^ Value >> (sizeof(T) * 8u - 1u))));
  return 33u - static_cast<uint32>(FPlatformMath::CountLeadingZeros(MassagedValue));
}

template<typename T, typename TEnableIf<!TIsSigned<T>::Value && sizeof(T) <= 4u, uint32>::Type X = 1u>
uint32 GetBitsNeeded(T Value)
{
  return 32u - static_cast<uint32>(FPlatformMath::CountLeadingZeros(uint32(Value)));
}

template<typename T, typename TEnableIf<TIsSigned<T>::Value && sizeof(T) == 8u, int64>::Type X = -1ll>
uint32 GetBitsNeeded(T Value)
{
  const uint64 MassagedValue = uint64(Value ^ Value >> 63u);
  return 65u - static_cast<uint32>(FPlatformMath::CountLeadingZeros64(MassagedValue));
}

template<typename T, typename TEnableIf<!TIsSigned<T>::Value && sizeof(T) == 8u, uint64>::Type X = 1ull>
uint32 GetBitsNeeded(T Value)
{
  return 64u - static_cast<uint32>(FPlatformMath::CountLeadingZeros64(Value));
}

inline int32 RoundFloatToInt(float Value)
{
  return int32(Value + FPlatformMath::Sign(Value) * 0.5f);
}

inline int64 RoundFloatToInt(double Value)
{
  return int64(Value + FPlatformMath::Sign(Value) * 0.5);
}

template<typename T>
bool IsNetEqual(T Value, T OtherValue, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == OtherValue;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return FPlatformMath::Abs(Value - OtherValue) < Tolerance;
}

template<typename T>
bool IsNetEqual(const UE::Math::TVector2<T>& Value, const UE::Math::TVector2<T>& OtherValue, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == OtherValue;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FPlatformMath::Abs(Value.X - OtherValue.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y - OtherValue.Y) < Tolerance;
}

template<typename T>
bool IsNetEqual(const UE::Math::TVector<T>& Value, const UE::Math::TVector<T>& OtherValue, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == OtherValue;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FPlatformMath::Abs(Value.X - OtherValue.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y - OtherValue.Y) < Tolerance &&
    FPlatformMath::Abs(Value.Z - OtherValue.Z) < Tolerance;
}

template<typename T>
bool IsNetEqual(const UE::Math::TRotator<T>& Value, const UE::Math::TRotator<T>& OtherValue, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == OtherValue;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FMath::Abs(UE::Math::TRotator<T>::NormalizeAxis(Value.Pitch - OtherValue.Pitch)) < Tolerance &&
    FMath::Abs(UE::Math::TRotator<T>::NormalizeAxis(Value.Yaw - OtherValue.Yaw)) < Tolerance &&
    FMath::Abs(UE::Math::TRotator<T>::NormalizeAxis(Value.Roll - OtherValue.Roll)) < Tolerance;
}

template<typename T>
bool IsNetEqual(const UE::Math::TVector4<T>& Value, const UE::Math::TVector4<T>& OtherValue, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == OtherValue;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FPlatformMath::Abs(Value.X - OtherValue.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y - OtherValue.Y) < Tolerance &&
    FPlatformMath::Abs(Value.Z - OtherValue.Z) < Tolerance &&
    FPlatformMath::Abs(Value.W - OtherValue.W) < Tolerance;
}

template<typename T>
bool IsNetEqual(const UE::Math::TQuat<T>& Value, const UE::Math::TQuat<T>& OtherValue, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == OtherValue;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return (
    FPlatformMath::Abs(Value.X - OtherValue.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y - OtherValue.Y) < Tolerance &&
    FPlatformMath::Abs(Value.Z - OtherValue.Z) < Tolerance &&
    FPlatformMath::Abs(Value.W - OtherValue.W) < Tolerance
  ) || (
    FPlatformMath::Abs(Value.X + OtherValue.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y + OtherValue.Y) < Tolerance &&
    FPlatformMath::Abs(Value.Z + OtherValue.Z) < Tolerance &&
    FPlatformMath::Abs(Value.W + OtherValue.W) < Tolerance
  );
}

template<typename T>
bool IsNetNearlyZero(T Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == (T)0;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return FPlatformMath::Abs(Value) < Tolerance;
}

template<typename T>
bool IsNetNearlyZero(const UE::Math::TVector2<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == UE::Math::TVector2<T>::ZeroVector;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FPlatformMath::Abs(Value.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y) < Tolerance;
}

template<typename T>
bool IsNetNearlyZero(const UE::Math::TVector<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == UE::Math::TVector<T>::ZeroVector;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FPlatformMath::Abs(Value.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y) < Tolerance &&
    FPlatformMath::Abs(Value.Z) < Tolerance;
}

template<typename T>
bool IsNetNearlyZero(const UE::Math::TRotator<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == UE::Math::TRotator<T>::ZeroRotator;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FMath::Abs(UE::Math::TRotator<T>::NormalizeAxis(Value.Pitch)) < Tolerance &&
    FMath::Abs(UE::Math::TRotator<T>::NormalizeAxis(Value.Yaw)) < Tolerance &&
    FMath::Abs(UE::Math::TRotator<T>::NormalizeAxis(Value.Roll)) < Tolerance;
}

template<typename T>
bool IsNetNearlyZero(const UE::Math::TVector4<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == UE::Math::TVector4<T>::ZeroVector;
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FPlatformMath::Abs(Value.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y) < Tolerance &&
    FPlatformMath::Abs(Value.Z) < Tolerance &&
    FPlatformMath::Abs(Value.W) < Tolerance;
}

template<typename T>
bool IsNetNearlyZero(const UE::Math::TQuat<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision)
  {
    return Value == UE::Math::TQuat<T>((T)0);
  }

  double Tolerance = 0.99 * (1. / (int32)Precision);
  return
    FPlatformMath::Abs(Value.X) < Tolerance &&
    FPlatformMath::Abs(Value.Y) < Tolerance &&
    FPlatformMath::Abs(Value.Z) < Tolerance &&
    FPlatformMath::Abs(Value.W) < Tolerance;
}

template<typename T>
void NetClamp(T& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  if (IsNetNearlyZero(Value, Precision)) Value = (T)0;
}

template<typename T>
void NetClamp(UE::Math::TVector2<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  if (IsNetNearlyZero(Value.X, Precision)) Value.X = (T)0;
  if (IsNetNearlyZero(Value.Y, Precision)) Value.Y = (T)0;
}

template<typename T>
void NetClamp(UE::Math::TVector<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  if (IsNetNearlyZero(Value.X, Precision)) Value.X = (T)0;
  if (IsNetNearlyZero(Value.Y, Precision)) Value.Y = (T)0;
  if (IsNetNearlyZero(Value.Z, Precision)) Value.Z = (T)0;
}

template<typename T>
void NetClamp(UE::Math::TRotator<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  if (IsNetNearlyZero(Value.Roll, Precision)) Value.Roll = (T)0;
  if (IsNetNearlyZero(Value.Pitch, Precision)) Value.Pitch = (T)0;
  if (IsNetNearlyZero(Value.Yaw, Precision)) Value.Yaw = (T)0;
}

template<typename T>
void NetClamp(UE::Math::TVector4<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  if (IsNetNearlyZero(Value.X, Precision)) Value.X = (T)0;
  if (IsNetNearlyZero(Value.Y, Precision)) Value.Y = (T)0;
  if (IsNetNearlyZero(Value.Z, Precision)) Value.Z = (T)0;
  if (IsNetNearlyZero(Value.W, Precision)) Value.W = (T)0;
}

template<typename T>
void NetClamp(UE::Math::TQuat<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  if (IsNetNearlyZero(Value.X, Precision)) Value.X = (T)0;
  if (IsNetNearlyZero(Value.Y, Precision)) Value.Y = (T)0;
  if (IsNetNearlyZero(Value.Z, Precision)) Value.Z = (T)0;
  if (IsNetNearlyZero(Value.W, Precision)) Value.W = (T)0;
}

template<typename T>
void Round(T& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  Value = FMath::RoundToFloat(Value * (int32)Precision) / (int32)Precision;
}

template<typename T>
void Round2D(UE::Math::TVector2<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  Value.X = FMath::RoundToFloat(Value.X * (int32)Precision) / (int32)Precision;
  Value.Y = FMath::RoundToFloat(Value.Y * (int32)Precision) / (int32)Precision;
}

template<typename T>
void Round3D(UE::Math::TVector<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  Value.X = FMath::RoundToFloat(Value.X * (int32)Precision) / (int32)Precision;
  Value.Y = FMath::RoundToFloat(Value.Y * (int32)Precision) / (int32)Precision;
  Value.Z = FMath::RoundToFloat(Value.Z * (int32)Precision) / (int32)Precision;
}

template<typename T>
void Round3D(UE::Math::TRotator<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  Value.Roll = FMath::RoundToFloat(Value.Roll * (int32)Precision) / (int32)Precision;
  Value.Pitch = FMath::RoundToFloat(Value.Pitch * (int32)Precision) / (int32)Precision;
  Value.Yaw = FMath::RoundToFloat(Value.Yaw * (int32)Precision) / (int32)Precision;
}

template<typename T>
void Round4D(UE::Math::TVector4<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  Value.X = FMath::RoundToFloat(Value.X * (int32)Precision) / (int32)Precision;
  Value.Y = FMath::RoundToFloat(Value.Y * (int32)Precision) / (int32)Precision;
  Value.Z = FMath::RoundToFloat(Value.Z * (int32)Precision) / (int32)Precision;
  Value.W = FMath::RoundToFloat(Value.W * (int32)Precision) / (int32)Precision;
}

template<typename T>
void Round4D(UE::Math::TQuat<T>& Value, EGMC_FloatPrecision Precision)
{
  if (Precision == FullPrecision) return;
  Value.X = FMath::RoundToFloat(Value.X * (int32)Precision) / (int32)Precision;
  Value.Y = FMath::RoundToFloat(Value.Y * (int32)Precision) / (int32)Precision;
  Value.Z = FMath::RoundToFloat(Value.Z * (int32)Precision) / (int32)Precision;
  Value.W = FMath::RoundToFloat(Value.W * (int32)Precision) / (int32)Precision;
}

template<typename T>
T GetAbsMin(const UE::Math::TVector2<T>& Value)
{
  return FMath::Min(FMath::Abs(Value.X), FMath::Abs(Value.Y));
}

template<typename T>
T GetAbsMin(const UE::Math::TVector<T>& Value)
{
  return FMath::Min(FMath::Min(FMath::Abs(Value.X), FMath::Abs(Value.Y)), FMath::Abs(Value.Z));
}

template<typename T>
T GetAbsMin(const UE::Math::TRotator<T>& Value)
{
  return FMath::Min(FMath::Min(FMath::Abs(Value.Roll), FMath::Abs(Value.Pitch)), FMath::Abs(Value.Yaw));
}

template<typename T>
T GetAbsMin(const UE::Math::TVector4<T>& Value)
{
  return FMath::Min(FMath::Min(FMath::Min(FMath::Abs(Value.X), FMath::Abs(Value.Y)), FMath::Abs(Value.Z)), FMath::Abs(Value.W));
}

template<typename T>
T GetAbsMin(const UE::Math::TQuat<T>& Value)
{
  return FMath::Min(FMath::Min(FMath::Min(FMath::Abs(Value.X), FMath::Abs(Value.Y)), FMath::Abs(Value.Z)), FMath::Abs(Value.W));
}

template<typename T>
T GetAbsMax(const UE::Math::TVector2<T>& Value)
{
  return FMath::Max(FMath::Abs(Value.X), FMath::Abs(Value.Y));
}

template<typename T>
T GetAbsMax(const UE::Math::TVector<T>& Value)
{
  return FMath::Max(FMath::Max(FMath::Abs(Value.X), FMath::Abs(Value.Y)), FMath::Abs(Value.Z));
}

template<typename T>
T GetAbsMax(const UE::Math::TRotator<T>& Value)
{
  return FMath::Max(FMath::Max(FMath::Abs(Value.Roll), FMath::Abs(Value.Pitch)), FMath::Abs(Value.Yaw));
}

template<typename T>
T GetAbsMax(const UE::Math::TVector4<T>& Value)
{
  return FMath::Max(FMath::Max(FMath::Max(FMath::Abs(Value.X), FMath::Abs(Value.Y)), FMath::Abs(Value.Z)), FMath::Abs(Value.W));
}

template<typename T>
T GetAbsMax(const UE::Math::TQuat<T>& Value)
{
  return FMath::Max(FMath::Max(FMath::Max(FMath::Abs(Value.X), FMath::Abs(Value.Y)), FMath::Abs(Value.Z)), FMath::Abs(Value.W));
}

template<typename T>
T ClampAngle(T Angle)
{
  Angle = FPlatformMath::Fmod(Angle, (T)360);
  if (Angle < 0) Angle += 360;
  return Angle;
}

template<bool Condition, typename True, typename False>
struct TConditional { using Type = True; };

template<typename True, typename False>
struct TConditional<false, True, False> { using Type = False; };

template<typename T>
void SerializeNormalizedFloat(T Value, EGMC_FloatPrecision Scale, FArchive& Ar)
{
  gmc_ck(Ar.IsSaving())

  constexpr SIZE_T TypeSize = sizeof(T);
  constexpr SIZE_T FloatTypeSize = 4u;
  constexpr SIZE_T DoubleTypeSize = 8u;

  static_assert(
    TIsFloatingPoint<T>::Value && (TypeSize == FloatTypeSize || TypeSize == DoubleTypeSize),
    "The passed value must be either a 4 byte float or 8 byte double."
  );

  if (Scale == FullPrecision)
  {
    Ar << Value;
    return;
  }

  int64 CompressedValue = RoundFloatToInt(Value * Scale);
  const uint32 BitCount = GetBitsNeeded(int64(2 * Scale + 1));
  Ar.SerializeBits(&CompressedValue, BitCount);
}

template<typename T>
void DeserializeNormalizedFloat(T& OutValue, EGMC_FloatPrecision CompressionScale, FArchive& Ar)
{
  gmc_ck(Ar.IsLoading())

  constexpr SIZE_T TypeSize = sizeof(T);
  constexpr SIZE_T FloatTypeSize = 4u;
  constexpr SIZE_T DoubleTypeSize = 8u;

  static_assert(
    TIsFloatingPoint<T>::Value && (TypeSize == FloatTypeSize || TypeSize == DoubleTypeSize),
    "The passed value must be either a 4 byte float or 8 byte double."
  );

  if (CompressionScale == FullPrecision)
  {
    Ar << OutValue;
    return;
  }

  int64 DecompressedValue = 0;
  const uint32 BitCount = GetBitsNeeded(int64(2 * CompressionScale + 1));
  Ar.SerializeBits(&DecompressedValue, BitCount);

  // Sign-extend the deserialized value to get the correct 64 bit integer.
  const uint64 SignBit = 1ull << (BitCount - 1u);
  DecompressedValue = (DecompressedValue ^ SignBit) - SignBit;

  OutValue = (T)DecompressedValue / CompressionScale;
}

template<typename T>
void SerializeFloat(T Value, EGMC_FloatPrecision Scale, FArchive& Ar)
{
  gmc_ck(Ar.IsSaving())

  constexpr SIZE_T TypeSize = sizeof(T);
  constexpr SIZE_T FloatTypeSize = 4u;
  constexpr SIZE_T DoubleTypeSize = 8u;

  static_assert(
    TIsFloatingPoint<T>::Value && (TypeSize == FloatTypeSize || TypeSize == DoubleTypeSize),
    "The passed value must be either a 4 byte float or 8 byte double."
  );

  if (Scale == FullPrecision)
  {
    Ar << Value;
    return;
  }

  // Beyond 2^MaxExponentForScaling scaling cannot improve the precision as the next floating point value is at least 1.0 more.
  using IntType = typename TSignedIntType<TypeSize>::Type;
  constexpr uint32 MaxExponentForScaling = TypeSize == FloatTypeSize ? 23u : 52u;
  constexpr T MaxValueToScale = T(IntType(1) << MaxExponentForScaling);

  // Rounding of large values can introduce additional precision errors and the extra cost to serialize with full precision is small.
  constexpr uint32 MaxExponentAfterScaling = TypeSize == FloatTypeSize ? 30u : 62u;
  constexpr T MaxScaledValue = T(IntType(1) << MaxExponentAfterScaling);

  // Scale the value by the factor specified if it will improve precision.
  const bool bScaled = Value < MaxValueToScale;
  T ScaledValue = Value * (bScaled ? Scale : 1);

  // Determine the bit count of the compressed value.
  int64 CompressedValue = RoundFloatToInt(ScaledValue);
  const uint32 BitCount = GetBitsNeeded(CompressedValue);

  constexpr uint32 ShiftAmount = TypeSize == FloatTypeSize ? 5u : 6u;

  if (BitCount < TypeSize * 8u && FMath::Abs(ScaledValue) < MaxScaledValue && !FMath::IsNaN(Value))
  {
    uint32 ScaleInfoAndBitCount = bScaled << ShiftAmount | BitCount;
    Ar.SerializeInt(ScaleInfoAndBitCount, 1u << (ShiftAmount + 1u));
    Ar.SerializeBits(&CompressedValue, BitCount);

    return;
  }

  // A bit count of 0 indicates full precision.
  uint32 ScaleInfoAndBitCount = 0u;
  Ar.SerializeInt(ScaleInfoAndBitCount, 1u << (ShiftAmount + 1u));
  Ar.SerializeBits(&Value, TypeSize * 8u);
}

template<typename T>
void DeserializeFloat(T& OutValue, EGMC_FloatPrecision CompressionScale, FArchive& Ar)
{
  gmc_ck(Ar.IsLoading())

  constexpr SIZE_T TypeSize = sizeof(T);
  constexpr SIZE_T FloatTypeSize = 4u;
  constexpr SIZE_T DoubleTypeSize = 8u;

  static_assert(
    TIsFloatingPoint<T>::Value && (TypeSize == FloatTypeSize || TypeSize == DoubleTypeSize),
    "The passed value must be either a 4 byte float or 8 byte double."
  );

  if (CompressionScale == FullPrecision)
  {
    Ar << OutValue;
    return;
  }

  constexpr uint32 ShiftAmount = TypeSize == FloatTypeSize ? 5u : 6u;

  uint32 ScaleInfoAndBitCount = 0;
  Ar.SerializeInt(ScaleInfoAndBitCount, 1u << (ShiftAmount + 1u));

  if (ScaleInfoAndBitCount != 0u)
  {
    int64 DecompressedValue = 0;
    const uint32 BitCount = ScaleInfoAndBitCount & (1u << ShiftAmount) - 1u;
    Ar.SerializeBits(&DecompressedValue, BitCount);

    // Sign-extend the deserialized value to get the correct 64 bit integer.
    const uint64 SignBit = 1ull << (BitCount - 1u);
    DecompressedValue = (DecompressedValue ^ SignBit) - SignBit;

    OutValue = T(DecompressedValue);

    // Apply scaling if needed.
    const bool bScaled = ScaleInfoAndBitCount >> ShiftAmount;
    if (bScaled)
    {
      OutValue = OutValue / T(CompressionScale);
    }

    return;
  }

  // We received the full precision value.
  Ar.SerializeBits(&OutValue, TypeSize * 8u);
}

template<typename T>
void SerializeVector(const T& Vector, EGMC_FloatPrecision Scale, FArchive& Ar)
{
  gmc_ck(Ar.IsSaving())

  constexpr bool bIsFloatType =
    std::is_same<T, FVector2f>::value ||
    std::is_same<T, FVector3f>::value ||
    std::is_same<T, FRotator3f>::value ||
    std::is_same<T, FVector4f>::value ||
    std::is_same<T, FQuat4f>::value;
  constexpr bool bIsDoubleType =
    std::is_same<T, FVector2d>::value ||
    std::is_same<T, FVector3d>::value ||
    std::is_same<T, FRotator3d>::value ||
    std::is_same<T, FVector4d>::value ||
    std::is_same<T, FQuat4d>::value;

  static_assert(bIsFloatType ^ bIsDoubleType, "Unknown vector type.");

  using ScalarType = typename TConditional<bIsFloatType, float, double>::Type;
  constexpr SIZE_T ScalarTypeSize = sizeof(ScalarType);
  constexpr SIZE_T FloatTypeSize = 4u;
  constexpr SIZE_T DoubleTypeSize = 8u;

  static_assert(ScalarTypeSize == FloatTypeSize || ScalarTypeSize == DoubleTypeSize, "Unknown floating point type.");

  constexpr bool bIs2D = std::is_same<T, UE::Math::TVector2<ScalarType>>::value;
  constexpr bool bIs3D = std::is_same<T, UE::Math::TVector<ScalarType>>::value || std::is_same<T, UE::Math::TRotator<ScalarType>>::value;
  constexpr bool bIs4D = std::is_same<T, UE::Math::TVector4<ScalarType>>::value || std::is_same<T, UE::Math::TQuat<ScalarType>>::value;

  SIZE_T NumComponents{};
  if      constexpr (bIs2D) { NumComponents = 2u;                             }
  else if constexpr (bIs3D) { NumComponents = 3u;                             }
  else if constexpr (bIs4D) { NumComponents = 4u;                             }
  else                      { static_assert(bIs2D ^ bIs3D ^ bIs4D, "Unknown vector format."); }

  if (Scale == FullPrecision)
  {
    Ar.SerializeBits((void*)&Vector, ScalarTypeSize * 8u * NumComponents);
    return;
  }

  // Beyond 2^MaxExponentForScaling scaling cannot improve the precision as the next floating point value is at least 1.0 more.
  using IntType = typename TSignedIntType<ScalarTypeSize>::Type;
  constexpr uint32 MaxExponentForScaling = ScalarTypeSize == 4u ? 23u : 52u;
  constexpr ScalarType MaxValueToScale = ScalarType(IntType(1) << MaxExponentForScaling);

  // Rounding of large values can introduce additional precision errors and the extra cost to serialize with full precision is small.
  constexpr uint32 MaxExponentAfterScaling = ScalarTypeSize == 4u ? 30u : 62u;
  constexpr ScalarType MaxScaledValue = ScalarType(IntType(1) << MaxExponentAfterScaling);

  // Use aliases for the vector components so we can treat them all in the same way.
  const auto& VectorX = [&]() constexpr -> const auto& {
    if constexpr (std::is_same<T, UE::Math::TRotator<ScalarType>>::value) { return Vector.Roll; } // 3D rotator
    else { return Vector.X; } // 2D/3D/4D vector, 4D quat
  }();
  const auto& VectorY = [&]() constexpr -> const auto& {
    if constexpr (std::is_same<T, UE::Math::TRotator<ScalarType>>::value) { return Vector.Pitch; } // 3D rotator
    else { return Vector.Y; } // 2D/3D/4D vector, 4D quat
  }();
  ScalarType UnusedZ{NAN};
  const auto& VectorZ = [&]() constexpr -> const auto& {
    if constexpr (std::is_same<T, UE::Math::TRotator<ScalarType>>::value) { return Vector.Yaw; } // 3D rotator
    else if constexpr (std::is_same<T, UE::Math::TVector2<ScalarType>>::value) { return UnusedZ; } // 2D vector
    else { return Vector.Z; } // 3D/4D vector, 4D quat
  }();
  ScalarType UnusedW{NAN};
  const auto& VectorW = [&]() constexpr -> const auto& {
    if constexpr (std::is_same<T, UE::Math::TVector4<ScalarType>>::value || std::is_same<T, UE::Math::TQuat<ScalarType>>::value) { return Vector.W; } // 4D vector, 4D quat
    else { return UnusedW; } // 2D/3D vector, 3D rotator
  }();

  ScalarType ScaledVectorX{VectorX};
  ScalarType ScaledVectorY{VectorY};
  ScalarType ScaledVectorZ{VectorZ};
  ScalarType ScaledVectorW{VectorW};

  // Scale the components by the factor specified if it will improve precision.
  const bool bScaled = GetAbsMin(Vector) < MaxValueToScale;
  if (bScaled)
  {
    ScaledVectorX *= (int32)Scale;
    ScaledVectorY *= (int32)Scale;
    if constexpr (bIs3D || bIs4D) { ScaledVectorZ *= (int32)Scale; } else {}
    if constexpr (bIs4D         ) { ScaledVectorW *= (int32)Scale; } else {}
  }

  // Determine the bit count max of the compressed vector.
  IntType CompressedX{};
  IntType CompressedY{};
  IntType CompressedZ{};
  IntType CompressedW{};
  CompressedX = RoundFloatToInt(ScaledVectorX);
  CompressedY = RoundFloatToInt(ScaledVectorY);
  if constexpr (bIs3D || bIs4D) { CompressedZ = RoundFloatToInt(ScaledVectorZ); } else {}
  if constexpr (bIs4D         ) { CompressedW = RoundFloatToInt(ScaledVectorW); } else {}
  uint32 ComponentBitCount = FMath::Max(GetBitsNeeded(CompressedX), GetBitsNeeded(CompressedY));
  if constexpr (bIs3D || bIs4D) { ComponentBitCount = FMath::Max(ComponentBitCount, GetBitsNeeded(CompressedZ)); } else {}
  if constexpr (bIs4D         ) { ComponentBitCount = FMath::Max(ComponentBitCount, GetBitsNeeded(CompressedW)); } else {}

  constexpr uint32 ShiftAmount = ScalarTypeSize == FloatTypeSize ? 5u : 6u;

  T ScaledVector{};
  if      constexpr (bIs2D) { ScaledVector = {ScaledVectorX, ScaledVectorY};                               }
  else if constexpr (bIs3D) { ScaledVector = {ScaledVectorX, ScaledVectorY, ScaledVectorZ};                }
  else if constexpr (bIs4D) { ScaledVector = {ScaledVectorX, ScaledVectorY, ScaledVectorZ, ScaledVectorW}; }
  else {}

  if (ComponentBitCount < ScalarTypeSize * 8u && GetAbsMax(ScaledVector) < MaxScaledValue && !Vector.ContainsNaN())
  {
    uint32 ScaleInfoAndBitCount = bScaled << ShiftAmount | ComponentBitCount;
    Ar.SerializeInt(ScaleInfoAndBitCount, 1u << (ShiftAmount + 1u));
    Ar.SerializeBits(&CompressedX, ComponentBitCount);
    Ar.SerializeBits(&CompressedY, ComponentBitCount);
    if constexpr (bIs3D || bIs4D) { Ar.SerializeBits(&CompressedZ, ComponentBitCount); } else {}
    if constexpr (bIs4D         ) { Ar.SerializeBits(&CompressedW, ComponentBitCount); } else {}

    return;
  }

  // A bit count of 0 indicates full precision.
  uint32 ScaleInfoAndBitCount = 0u;
  Ar.SerializeInt(ScaleInfoAndBitCount, 1u << (ShiftAmount + 1u));
  Ar.SerializeBits((void*)&Vector, ScalarTypeSize * 8u * NumComponents);
}

template<typename T>
void DeserializeVector(T& OutVector, EGMC_FloatPrecision CompressionScale, FArchive& Ar)
{
  gmc_ck(Ar.IsLoading())

  constexpr bool bIsFloatType =
    std::is_same<T, FVector2f>::value ||
    std::is_same<T, FVector3f>::value ||
    std::is_same<T, FRotator3f>::value ||
    std::is_same<T, FVector4f>::value ||
    std::is_same<T, FQuat4f>::value;
  constexpr bool bIsDoubleType =
    std::is_same<T, FVector2d>::value ||
    std::is_same<T, FVector3d>::value ||
    std::is_same<T, FRotator3d>::value ||
    std::is_same<T, FVector4d>::value ||
    std::is_same<T, FQuat4d>::value;

  static_assert(bIsFloatType ^ bIsDoubleType, "Unknown vector type.");

  using ScalarType = typename TConditional<bIsFloatType, float, double>::Type;
  constexpr SIZE_T ScalarTypeSize = sizeof(ScalarType);
  constexpr SIZE_T FloatTypeSize = 4u;
  constexpr SIZE_T DoubleTypeSize = 8u;

  static_assert(ScalarTypeSize == FloatTypeSize || ScalarTypeSize == DoubleTypeSize, "Unknown floating point type.");

  constexpr bool bIs2D = std::is_same<T, UE::Math::TVector2<ScalarType>>::value;
  constexpr bool bIs3D = std::is_same<T, UE::Math::TVector<ScalarType>>::value || std::is_same<T, UE::Math::TRotator<ScalarType>>::value;
  constexpr bool bIs4D = std::is_same<T, UE::Math::TVector4<ScalarType>>::value || std::is_same<T, UE::Math::TQuat<ScalarType>>::value;

  SIZE_T NumComponents{};
  if      constexpr (bIs2D) { NumComponents = 2u;                             }
  else if constexpr (bIs3D) { NumComponents = 3u;                             }
  else if constexpr (bIs4D) { NumComponents = 4u;                             }
  else                      { static_assert(bIs2D ^ bIs3D ^ bIs4D, "Unknown vector format."); }

  if (CompressionScale == FullPrecision)
  {
    Ar.SerializeBits(&OutVector, ScalarTypeSize * 8u * NumComponents);
    return;
  }

  // Use aliases for the vector components so we can treat them all in the same way.
  auto& OutVectorX = [&]() constexpr -> auto& {
    if constexpr (std::is_same<T, UE::Math::TRotator<ScalarType>>::value) { return OutVector.Roll; } // 3D rotator
    else { return OutVector.X; } // 2D/3D/4D vector, 4D quat
  }();
  auto& OutVectorY = [&]() constexpr -> auto& {
    if constexpr (std::is_same<T, UE::Math::TRotator<ScalarType>>::value) { return OutVector.Pitch; } // 3D rotator
    else { return OutVector.Y; } // 2D/3D/4D vector, 4D quat
  }();
  ScalarType UnusedZ{NAN};
  auto& OutVectorZ = [&]() constexpr -> auto& {
    if constexpr (std::is_same<T, UE::Math::TRotator<ScalarType>>::value) { return OutVector.Yaw; } // 3D rotator
    else if constexpr (std::is_same<T, UE::Math::TVector2<ScalarType>>::value) { return UnusedZ; } // 2D vector
    else { return OutVector.Z; } // 3D/4D vector, 4D quat
  }();
  ScalarType UnusedW{NAN};
  auto& OutVectorW = [&]() constexpr -> auto& {
    if constexpr (std::is_same<T, UE::Math::TVector4<ScalarType>>::value || std::is_same<T, UE::Math::TQuat<ScalarType>>::value) { return OutVector.W; } // 4D vector, 4D quat
    else { return UnusedW; } // 2D/3D vector, 3D rotator
  }();

  constexpr uint32 ShiftAmount = ScalarTypeSize == FloatTypeSize ? 5u : 6u;

  uint32 ScaleInfoAndBitCount = 0u;
  Ar.SerializeInt(ScaleInfoAndBitCount, 1u << (ShiftAmount + 1u));

  if (ScaleInfoAndBitCount != 0u)
  {
    int64 DecompressedX = 0;
    int64 DecompressedY = 0;
    int64 DecompressedZ = 0;
    int64 DecompressedW = 0;
    const uint32 ComponentBitCount = ScaleInfoAndBitCount & (1u << ShiftAmount) - 1u;
    Ar.SerializeBits(&DecompressedX, ComponentBitCount);
    Ar.SerializeBits(&DecompressedY, ComponentBitCount);
    if constexpr (bIs3D || bIs4D) { Ar.SerializeBits(&DecompressedZ, ComponentBitCount); } else {}
    if constexpr (bIs4D         ) { Ar.SerializeBits(&DecompressedW, ComponentBitCount); } else {}

    // Sign-extend the deserialized vector components to get the correct 64 bit integers.
    const uint64 SignBit = 1ull << (ComponentBitCount - 1u);
    DecompressedX = (DecompressedX ^ SignBit) - SignBit;
    DecompressedY = (DecompressedY ^ SignBit) - SignBit;
    if constexpr (bIs3D || bIs4D) { DecompressedZ = (DecompressedZ ^ SignBit) - SignBit; } else {}
    if constexpr (bIs4D         ) { DecompressedW = (DecompressedW ^ SignBit) - SignBit; } else {}

    OutVectorX = ScalarType(DecompressedX);
    OutVectorY = ScalarType(DecompressedY);
    if constexpr (bIs3D || bIs4D) { OutVectorZ = ScalarType(DecompressedZ); } else {}
    if constexpr (bIs4D         ) { OutVectorW = ScalarType(DecompressedW); } else {}

    // Apply scaling if needed.
    const bool bScaled = ScaleInfoAndBitCount >> ShiftAmount;
    if (bScaled)
    {
      // Workaround for missing TRotator::operator/ overload.
      OutVector = OutVector * (1 / ScalarType(CompressionScale));
    }

    return;
  }

  // Received the full precision vector.
  Ar.SerializeBits(&OutVector, ScalarTypeSize * 8u * NumComponents);
}

template<typename T>
void SerializeAngle(T Angle, EGMC_FloatPrecision Scale, FArchive& Ar)
{
  gmc_ck(Ar.IsSaving())

  Angle = ClampAngle(Angle);

  if (Scale == FullPrecision)
  {
    Ar << Angle;
    return;
  }

  uint32 CompressedAngle = RoundFloatToInt(Angle * (int32)Scale);
  const uint32 BitCount = GetBitsNeeded(uint32(360 * (int32)Scale));
  const uint32 Mask = (1u << BitCount) - 1;
  CompressedAngle = CompressedAngle & Mask;
  Ar.SerializeBits(&CompressedAngle, BitCount);
}

template<typename T>
void DeserializeAngle(T& OutAngle, EGMC_FloatPrecision CompressionScale, FArchive& Ar)
{
  gmc_ck(Ar.IsLoading())

  if (CompressionScale == FullPrecision)
  {
    Ar << OutAngle;
    return;
  }

  uint32 CompressedAngle = 0u;
  const uint32 BitCount = GetBitsNeeded(uint32(360 * (int32)CompressionScale));
  Ar.SerializeBits(&CompressedAngle, BitCount);
  OutAngle = T(CompressedAngle) / T((int32)CompressionScale);
}

template<typename T>
void SerializeRotator(const T& Rotator, EGMC_FloatPrecision Scale, FArchive& Ar)
{
  SerializeAngle(Rotator.Roll, Scale, Ar);
  SerializeAngle(Rotator.Pitch, Scale, Ar);
  SerializeAngle(Rotator.Yaw, Scale, Ar);
}

template<typename T>
void DeserializeRotator(T& OutRotator, EGMC_FloatPrecision CompressionScale, FArchive& Ar)
{
  DeserializeAngle(OutRotator.Roll, CompressionScale, Ar);
  DeserializeAngle(OutRotator.Pitch, CompressionScale, Ar);
  DeserializeAngle(OutRotator.Yaw, CompressionScale, Ar);
}

}
