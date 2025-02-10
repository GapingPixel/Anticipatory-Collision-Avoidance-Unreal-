// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "SyncCore.h"
#include "GMCPawn.h"
#include "Numerics.h"
#include "Compression.h"
#include "FloatPrecision.h"

namespace GMCReplication
{

TSyncType<FActorBase>::UType TSyncType<FActorBase>::GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  return Component->GetActorBase();
}

void TSyncType<FActorBase>::ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  Component->SetActorBase(Value);
}

TSyncType<FActorBase>::UType TSyncType<FActorBase>::ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const
{
  return Value;
}

TSyncType<FActorBase>::UType TSyncType<FActorBase>::ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const
{
  return Value;
}

void TSyncType<FActorBase>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == nullptr)
}

void TSyncType<FActorBase>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FActorBase>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FActorBase>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FActorBase>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FActorBase>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FActorBase>::UType TSyncType<FActorBase>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FActorBase>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FActorBase>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("ActorBase");
}

FString TSyncType<FActorBase>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetObjectNameWithOuter(Value);
}

EGMC_SyncType TSyncType<FActorBase>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::ActorBase;
}

TSyncType<FLinearVelocity>::UType TSyncType<FLinearVelocity>::GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    return Component->GetBasedLinearVelocity();
  }

  return Component->GetLinearVelocity_GMC();
}

void TSyncType<FLinearVelocity>::ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    Component->SetBasedLinearVelocity(Value);
    return;
  }

  Component->SetLinearVelocity_GMC(Value);
}

TSyncType<FLinearVelocity>::UType TSyncType<FLinearVelocity>::ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(TargetBase))
  {
    return Value;
  }

  return Component->GetBasedLinearVelocityFor(Value, TargetBase->GetComponentTransform());
}

TSyncType<FLinearVelocity>::UType TSyncType<FLinearVelocity>::ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(SourceBase))
  {
    return Value;
  }

  return Component->GetWorldLinearVelocityFor(Value, SourceBase->GetComponentTransform());
}

void TSyncType<FLinearVelocity>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FVector::ZeroVector;
}

void TSyncType<FLinearVelocity>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round3D(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.LinearVelocity));

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FLinearVelocity>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FLinearVelocity>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FLinearVelocity>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  EGMC_FloatPrecision Compression = ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.LinearVelocity);
  return GMCCompression::IsNetEqual(Value, OtherValue, Compression);
}

bool TSyncType<FLinearVelocity>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  const double Tolerance = Component->ReplicationSettings.DefaultErrorTolerances.LinearVelocity;
  return Value.Equals(OtherValue, Tolerance);
}

TSyncType<FLinearVelocity>::UType TSyncType<FLinearVelocity>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && StartActorBase ? Component->GetWorldLinearVelocityFor(StartValue, StartActorBase->GetComponentTransform()) : StartValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && TargetActorBase ? Component->GetWorldLinearVelocityFor(TargetValue, TargetActorBase->GetComponentTransform()) : TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && PreviousActorBase ? Component->GetWorldLinearVelocityFor(PreviousValue, PreviousActorBase->GetComponentTransform()) : PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && NextActorBase ? Component->GetWorldLinearVelocityFor(NextValue, NextActorBase->GetComponentTransform()) : NextValue},
      Ratio
    ).GetVector()
  );
}

void TSyncType<FLinearVelocity>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  FGMC_Move::SerializeLinearVelocity(Value, Ar, NetInfo, MetaData);
}

FString TSyncType<FLinearVelocity>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("LinearVelocity");
}

FString TSyncType<FLinearVelocity>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetVectorAsString(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.LinearVelocity));
}

EGMC_SyncType TSyncType<FLinearVelocity>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::LinearVelocity;
}

TSyncType<FAngularVelocity>::UType TSyncType<FAngularVelocity>::GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    return Component->GetBasedAngularVelocity();
  }

  return Component->GetAngularVelocity_GMC();
}

void TSyncType<FAngularVelocity>::ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    Component->SetBasedAngularVelocity(Value);
    return;
  }

  Component->SetAngularVelocity_GMC(Value);
}

TSyncType<FAngularVelocity>::UType TSyncType<FAngularVelocity>::ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(TargetBase))
  {
    return Value;
  }

  return Component->GetBasedAngularVelocityFor(Value, TargetBase->GetComponentTransform());
}

TSyncType<FAngularVelocity>::UType TSyncType<FAngularVelocity>::ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(SourceBase))
  {
    return Value;
  }

  return Component->GetWorldAngularVelocityFor(Value, SourceBase->GetComponentTransform());
}

void TSyncType<FAngularVelocity>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FVector::ZeroVector;
}

void TSyncType<FAngularVelocity>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round3D(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.AngularVelocity));

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FAngularVelocity>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FAngularVelocity>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FAngularVelocity>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  EGMC_FloatPrecision Compression = ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.AngularVelocity);
  return GMCCompression::IsNetEqual(Value, OtherValue, Compression);
}

bool TSyncType<FAngularVelocity>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  const double Tolerance = Component->ReplicationSettings.DefaultErrorTolerances.AngularVelocity;
  return Value.Equals(OtherValue, Tolerance);
}

TSyncType<FAngularVelocity>::UType TSyncType<FAngularVelocity>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && StartActorBase ? Component->GetWorldAngularVelocityFor(StartValue, StartActorBase->GetComponentTransform()) : StartValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && TargetActorBase ? Component->GetWorldAngularVelocityFor(TargetValue, TargetActorBase->GetComponentTransform()) : TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && PreviousActorBase ? Component->GetWorldAngularVelocityFor(PreviousValue, PreviousActorBase->GetComponentTransform()) : PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && NextActorBase ? Component->GetWorldAngularVelocityFor(NextValue, NextActorBase->GetComponentTransform()) : NextValue},
      Ratio
    ).GetVector()
  );
}

void TSyncType<FAngularVelocity>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  FGMC_Move::SerializeAngularVelocity(Value, Ar, NetInfo, MetaData);
}

FString TSyncType<FAngularVelocity>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("AngularVelocity");
}

FString TSyncType<FAngularVelocity>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetVectorAsString(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.AngularVelocity));
}

EGMC_SyncType TSyncType<FAngularVelocity>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::AngularVelocity;
}

TSyncType<FActorLocation>::UType TSyncType<FActorLocation>::GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    return Component->GetBasedActorLocation();
  }

  return Component->GetActorLocation_GMC();
}

void TSyncType<FActorLocation>::ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    Component->SetBasedActorLocation(Value);
    return;
  }

  Component->SetActorLocation_GMC(Value, true);
}

TSyncType<FActorLocation>::UType TSyncType<FActorLocation>::ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(TargetBase))
  {
    return Value;
  }

  return Component->GetBasedActorLocationFor(Value, TargetBase->GetComponentTransform());
}

TSyncType<FActorLocation>::UType TSyncType<FActorLocation>::ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(SourceBase))
  {
    return Value;
  }

  return Component->GetWorldActorLocationFor(Value, SourceBase->GetComponentTransform());
}

void TSyncType<FActorLocation>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FVector::ZeroVector;
}

void TSyncType<FActorLocation>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round3D(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorLocation));

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FActorLocation>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FActorLocation>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FActorLocation>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  EGMC_FloatPrecision Compression = ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorLocation);
  return GMCCompression::IsNetEqual(Value, OtherValue, Compression);
}

bool TSyncType<FActorLocation>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  const double Tolerance = Component->ReplicationSettings.DefaultErrorTolerances.ActorLocation;
  return Value.Equals(OtherValue, Tolerance);
}

TSyncType<FActorLocation>::UType TSyncType<FActorLocation>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && StartActorBase ? Component->GetWorldActorLocationFor(StartValue, StartActorBase->GetComponentTransform()) : StartValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && TargetActorBase ? Component->GetWorldActorLocationFor(TargetValue, TargetActorBase->GetComponentTransform()) : TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && PreviousActorBase ? Component->GetWorldActorLocationFor(PreviousValue, PreviousActorBase->GetComponentTransform()) : PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && NextActorBase ? Component->GetWorldActorLocationFor(NextValue, NextActorBase->GetComponentTransform()) : NextValue},
      Ratio
    ).GetVector()
  );
}

void TSyncType<FActorLocation>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  FGMC_Move::SerializeActorLocation(Value, Ar, NetInfo, MetaData);
}

FString TSyncType<FActorLocation>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("ActorLocation");
}

FString TSyncType<FActorLocation>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetVectorAsString(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorLocation));
}

EGMC_SyncType TSyncType<FActorLocation>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::ActorLocation;
}

TSyncType<FActorRotation>::UType TSyncType<FActorRotation>::GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    return Component->GetBasedActorRotation();
  }

  return Component->GetActorRotation_GMC();
}

void TSyncType<FActorRotation>::ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    Component->SetBasedActorRotation(Value);
    return;
  }

  Component->SetActorRotation_GMC(Value, true);
}

TSyncType<FActorRotation>::UType TSyncType<FActorRotation>::ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(TargetBase))
  {
    return Value;
  }

  return Component->GetBasedActorRotationFor(Value, TargetBase->GetComponentTransform());
}

TSyncType<FActorRotation>::UType TSyncType<FActorRotation>::ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(SourceBase))
  {
    return Value;
  }

  return Component->GetWorldActorRotationFor(Value, SourceBase->GetComponentTransform());
}

void TSyncType<FActorRotation>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FRotator::ZeroRotator;
}

void TSyncType<FActorRotation>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  Value.Normalize();
  GMCCompression::Round3D(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorRotation));

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FActorRotation>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FActorRotation>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FActorRotation>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  EGMC_FloatPrecision Compression = ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorRotation);
  return GMCCompression::IsNetEqual(Value, OtherValue, Compression);
}

bool TSyncType<FActorRotation>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  const double Tolerance = Component->ReplicationSettings.DefaultErrorTolerances.ActorRotation;
  return Value.Equals(OtherValue, Tolerance);
}

TSyncType<FActorRotation>::UType TSyncType<FActorRotation>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && StartActorBase ? Component->GetWorldActorRotationFor(StartValue, StartActorBase->GetComponentTransform()) : StartValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && TargetActorBase ? Component->GetWorldActorRotationFor(TargetValue, TargetActorBase->GetComponentTransform()) : TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && PreviousActorBase ? Component->GetWorldActorRotationFor(PreviousValue, PreviousActorBase->GetComponentTransform()) : PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && NextActorBase ? Component->GetWorldActorRotationFor(NextValue, NextActorBase->GetComponentTransform()) : NextValue},
      Ratio
    ).GetRotator()
  );
}

void TSyncType<FActorRotation>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  FGMC_Move::SerializeActorRotation(Value, Ar, NetInfo, MetaData);
}

FString TSyncType<FActorRotation>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("ActorRotation");
}

FString TSyncType<FActorRotation>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetRotatorAsString(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorRotation));
}

EGMC_SyncType TSyncType<FActorRotation>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::ActorRotation;
}

TSyncType<FActorScale>::UType TSyncType<FActorScale>::GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    return Component->GetBasedActorScale();
  }

  return Component->GetActorScale_GMC();
}

void TSyncType<FActorScale>::ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    Component->SetBasedActorScale(Value);
    return;
  }

  Component->SetActorScale_GMC(Value);
}

TSyncType<FActorScale>::UType TSyncType<FActorScale>::ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(TargetBase))
  {
    return Value;
  }

  return Component->GetBasedActorScaleFor(Value, TargetBase->GetComponentTransform());
}

TSyncType<FActorScale>::UType TSyncType<FActorScale>::ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(SourceBase))
  {
    return Value;
  }

  return Component->GetWorldActorScaleFor(Value, SourceBase->GetComponentTransform());
}

void TSyncType<FActorScale>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FVector::OneVector;
}

void TSyncType<FActorScale>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round3D(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorScale));

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FActorScale>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FActorScale>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FActorScale>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  EGMC_FloatPrecision Compression = ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorScale);
  return GMCCompression::IsNetEqual(Value, OtherValue, Compression);
}

bool TSyncType<FActorScale>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  const double Tolerance = Component->ReplicationSettings.DefaultErrorTolerances.ActorScale;
  return Value.Equals(OtherValue, Tolerance);
}

TSyncType<FActorScale>::UType TSyncType<FActorScale>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && StartActorBase ? Component->GetWorldActorScaleFor(StartValue, StartActorBase->GetComponentTransform()) : StartValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && TargetActorBase ? Component->GetWorldActorScaleFor(TargetValue, TargetActorBase->GetComponentTransform()) : TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && PreviousActorBase ? Component->GetWorldActorScaleFor(PreviousValue, PreviousActorBase->GetComponentTransform()) : PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, bUseRelative && NextActorBase ? Component->GetWorldActorScaleFor(NextValue, NextActorBase->GetComponentTransform()) : NextValue},
      Ratio
    ).GetVector()
  );
}

void TSyncType<FActorScale>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  FGMC_Move::SerializeActorScale(Value, Ar, NetInfo, MetaData);
}

FString TSyncType<FActorScale>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("ActorScale");
}

FString TSyncType<FActorScale>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetVectorAsString(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ActorScale));
}

EGMC_SyncType TSyncType<FActorScale>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::ActorScale;
}

TSyncType<FControlRotation>::UType TSyncType<FControlRotation>::GetValue(bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    return Component->GetBasedControlRotation();
  }

  return Component->GetControllerRotation_GMC();
}

void TSyncType<FControlRotation>::ApplyValue(const UType& Value, bool bUseRelative, ::UGMC_ReplicationCmp* const Component) const
{
  if (bUseRelative)
  {
    Component->SetBasedControlRotation(Value);
    return;
  }

  Component->SetControllerRotation_GMC(Value);
}

TSyncType<FControlRotation>::UType TSyncType<FControlRotation>::ToRelative(const UType& Value, const USceneComponent* const TargetBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(TargetBase))
  {
    return Value;
  }

  return Component->GetBasedControlRotationFor(Value, TargetBase->GetComponentTransform());
}

TSyncType<FControlRotation>::UType TSyncType<FControlRotation>::ToAbsolute(const UType& Value, const USceneComponent* const SourceBase, ::UGMC_ReplicationCmp* const Component) const
{
  if (!::IsValid(SourceBase))
  {
    return Value;
  }

  return Component->GetWorldControlRotationFor(Value, SourceBase->GetComponentTransform());
}

void TSyncType<FControlRotation>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FRotator::ZeroRotator;
}

void TSyncType<FControlRotation>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  Value.Normalize();
  GMCCompression::Round3D(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ControlRotation));

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FControlRotation>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FControlRotation>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FControlRotation>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  EGMC_FloatPrecision Compression = ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ControlRotation);
  return GMCCompression::IsNetEqual(Value, OtherValue, Compression);
}

bool TSyncType<FControlRotation>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  const double Tolerance = Component->ReplicationSettings.DefaultErrorTolerances.ControlRotation;
  return Value.Equals(OtherValue, Tolerance);
}

TSyncType<FControlRotation>::UType TSyncType<FControlRotation>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && StartActorBase ? Component->GetWorldControlRotationFor(StartValue, StartActorBase->GetComponentTransform()) : StartValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && TargetActorBase ? Component->GetWorldControlRotationFor(TargetValue, TargetActorBase->GetComponentTransform()) : TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && PreviousActorBase ? Component->GetWorldControlRotationFor(PreviousValue, PreviousActorBase->GetComponentTransform()) : PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, bUseRelative && NextActorBase ? Component->GetWorldControlRotationFor(NextValue, NextActorBase->GetComponentTransform()) : NextValue},
      Ratio
    ).GetRotator()
  );
}

void TSyncType<FControlRotation>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  FGMC_Move::SerializeControlRotation(Value, Ar, NetInfo, MetaData);
}

FString TSyncType<FControlRotation>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("ControlRotation");
}

FString TSyncType<FControlRotation>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetRotatorAsString(Value, ToNativeEnum(Component->ReplicationSettings.DefaultCompressionSettings.ControlRotation));
}

EGMC_SyncType TSyncType<FControlRotation>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::ControlRotation;
}

void TSyncType<FBit>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == false)
}

void TSyncType<FBit>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FBit>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FBit>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FBit>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FBit>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FBit>::UType TSyncType<FBit>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FBit>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar.SerializeBits(&Value, 1);
}

FString TSyncType<FBit>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("Bool");
}

FString TSyncType<FBit>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%s"), Value ? TEXT("true") : TEXT("false"));
}

EGMC_SyncType TSyncType<FBit>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::Bool;
}

void TSyncType<FUnsignedInt4>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0)
}

void TSyncType<FUnsignedInt4>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FUnsignedInt4>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FUnsignedInt4>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FUnsignedInt4>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FUnsignedInt4>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FUnsignedInt4>::UType TSyncType<FUnsignedInt4>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Int, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Int, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Int, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Int, NextValue},
      Ratio
    ).GetInt()
  );
}

void TSyncType<FUnsignedInt4>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar.SerializeBits(&Value, 4);
}

FString TSyncType<FUnsignedInt4>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("HalfByte");
}

FString TSyncType<FUnsignedInt4>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%d"), Value);
}

EGMC_SyncType TSyncType<FUnsignedInt4>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::HalfByte;
}

void TSyncType<FUnsignedInt8>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0)
}

void TSyncType<FUnsignedInt8>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FUnsignedInt8>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FUnsignedInt8>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FUnsignedInt8>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FUnsignedInt8>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FUnsignedInt8>::UType TSyncType<FUnsignedInt8>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Int, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Int, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Int, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Int, NextValue},
      Ratio
    ).GetInt()
  );
}

void TSyncType<FUnsignedInt8>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FUnsignedInt8>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("Byte");
}

FString TSyncType<FUnsignedInt8>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%d"), Value);
}

EGMC_SyncType TSyncType<FUnsignedInt8>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::Byte;
}

void TSyncType<FInt32>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0)
}

void TSyncType<FInt32>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FInt32>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FInt32>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FInt32>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FInt32>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FInt32>::UType TSyncType<FInt32>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Int, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Int, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Int, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Int, NextValue},
      Ratio
    ).GetInt()
  );
}

void TSyncType<FInt32>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FInt32>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("Int");
}

FString TSyncType<FInt32>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%d"), Value);
}

EGMC_SyncType TSyncType<FInt32>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::Int;
}

void TSyncType<FFloat>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0.f)
}

void TSyncType<FFloat>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FFloat>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FFloat>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FFloat>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FFloat>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FFloat>::UType TSyncType<FFloat>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Float, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Float, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Float, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Float, NextValue},
      Ratio
    ).GetFloat()
  );
}

void TSyncType<FFloat>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FFloat>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("SinglePrecisionFloat");
}

FString TSyncType<FFloat>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%.6f"), Value);
}

EGMC_SyncType TSyncType<FFloat>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::SinglePrecisionFloat;
}

void TSyncType<FFloat2Dec>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0.f)
}

void TSyncType<FFloat2Dec>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round(Value, EGMC_FloatPrecision::TwoDecimals);

#if !NO_LOGGING
  if (PreQuantizeValue != 0.f && Value == 0.f)
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FFloat2Dec>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FFloat2Dec>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FFloat2Dec>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return GMCCompression::IsNetEqual(Value, OtherValue, EGMC_FloatPrecision::TwoDecimals);
}

bool TSyncType<FFloat2Dec>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FFloat2Dec>::UType TSyncType<FFloat2Dec>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Float, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Float, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Float, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Float, NextValue},
      Ratio
    ).GetFloat()
  );
}

void TSyncType<FFloat2Dec>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeFloat(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeFloat(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
}

FString TSyncType<FFloat2Dec>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("CompressedSinglePrecisionFloat");
}

FString TSyncType<FFloat2Dec>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%.2f"), Value);
}

EGMC_SyncType TSyncType<FFloat2Dec>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::CompressedSinglePrecisionFloat;
}

void TSyncType<FDouble>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0.)
}

void TSyncType<FDouble>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FDouble>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FDouble>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FDouble>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FDouble>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FDouble>::UType TSyncType<FDouble>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Float, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Float, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Float, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Float, NextValue},
      Ratio
    ).GetFloat()
  );
}

void TSyncType<FDouble>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FDouble>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("DoublePrecisionFloat");
}

FString TSyncType<FDouble>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%.14f"), Value);
}

EGMC_SyncType TSyncType<FDouble>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::DoublePrecisionFloat;
}

void TSyncType<FDouble2Dec>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0.)
}

void TSyncType<FDouble2Dec>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round(Value, EGMC_FloatPrecision::TwoDecimals);

#if !NO_LOGGING
  if (PreQuantizeValue != 0. && Value == 0.)
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FDouble2Dec>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FDouble2Dec>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FDouble2Dec>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return GMCCompression::IsNetEqual(Value, OtherValue, EGMC_FloatPrecision::TwoDecimals);
}

bool TSyncType<FDouble2Dec>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FDouble2Dec>::UType TSyncType<FDouble2Dec>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Float, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Float, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Float, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Float, NextValue},
      Ratio
    ).GetFloat()
  );
}

void TSyncType<FDouble2Dec>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeFloat(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeFloat(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
}

FString TSyncType<FDouble2Dec>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("CompressedDoublePrecisionFloat");
}

FString TSyncType<FDouble2Dec>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%.2f"), Value);
}

EGMC_SyncType TSyncType<FDouble2Dec>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::CompressedDoublePrecisionFloat;
}

void TSyncType<FDoubleAsFloat>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == 0.)
}

void TSyncType<FDoubleAsFloat>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  Value = static_cast<float>(Value);

#if !NO_LOGGING
  if (PreQuantizeValue != 0. && Value == 0.)
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FDoubleAsFloat>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FDoubleAsFloat>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FDoubleAsFloat>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return static_cast<float>(Value) == static_cast<float>(OtherValue);
}

bool TSyncType<FDoubleAsFloat>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FDoubleAsFloat>::UType TSyncType<FDoubleAsFloat>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Float, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Float, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Float, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Float, NextValue},
      Ratio
    ).GetFloat()
  );
}

void TSyncType<FDoubleAsFloat>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  float FloatValue = static_cast<float>(Value);
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeFloat(FloatValue, EGMC_FloatPrecision::FullPrecision, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeFloat(FloatValue, EGMC_FloatPrecision::FullPrecision, Ar);
    Value = FloatValue;
  }
}

FString TSyncType<FDoubleAsFloat>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("TruncatedDoublePrecisionFloat");
}

FString TSyncType<FDoubleAsFloat>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%.6f"), Value);
}

EGMC_SyncType TSyncType<FDoubleAsFloat>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::TruncatedDoublePrecisionFloat;
}

void TSyncType<FVector2Double2Dec>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FVector2D::ZeroVector;
}

void TSyncType<FVector2Double2Dec>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round2D(Value, EGMC_FloatPrecision::TwoDecimals);

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FVector2Double2Dec>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FVector2Double2Dec>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FVector2Double2Dec>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return GMCCompression::IsNetEqual(Value, OtherValue, EGMC_FloatPrecision::TwoDecimals);
}

bool TSyncType<FVector2Double2Dec>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FVector2Double2Dec>::UType TSyncType<FVector2Double2Dec>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Vector2D, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Vector2D, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Vector2D, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Vector2D, NextValue},
      Ratio
    ).GetVector2D()
  );
}

void TSyncType<FVector2Double2Dec>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeVector(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeVector(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
}

FString TSyncType<FVector2Double2Dec>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("CompressedVector2D");
}

FString TSyncType<FVector2Double2Dec>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetVector2DAsString(Value, EGMC_FloatPrecision::TwoDecimals);
}

EGMC_SyncType TSyncType<FVector2Double2Dec>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::CompressedVector2D;
}

void TSyncType<FVector3Double2Dec>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FVector::ZeroVector;
}

void TSyncType<FVector3Double2Dec>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  GMCCompression::Round3D(Value, EGMC_FloatPrecision::TwoDecimals);

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FVector3Double2Dec>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FVector3Double2Dec>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FVector3Double2Dec>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return GMCCompression::IsNetEqual(Value, OtherValue, EGMC_FloatPrecision::TwoDecimals);
}

bool TSyncType<FVector3Double2Dec>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FVector3Double2Dec>::UType TSyncType<FVector3Double2Dec>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Vector, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Vector, NextValue},
      Ratio
    ).GetVector()
  );
}

void TSyncType<FVector3Double2Dec>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeVector(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeVector(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
}

FString TSyncType<FVector3Double2Dec>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("CompressedVector");
}

FString TSyncType<FVector3Double2Dec>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetVectorAsString(Value, EGMC_FloatPrecision::TwoDecimals);
}

EGMC_SyncType TSyncType<FVector3Double2Dec>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::CompressedVector;
}

void TSyncType<FRotator3Double2Dec>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  Value = FRotator::ZeroRotator;
}

void TSyncType<FRotator3Double2Dec>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
#if !NO_LOGGING
  const auto PreQuantizeValue = Value;
#endif

  Value.Normalize();
  GMCCompression::Round3D(Value, EGMC_FloatPrecision::TwoDecimals);

#if !NO_LOGGING
  if (!PreQuantizeValue.IsZero() && Value.IsZero())
  {
    GMC_LOG(LogGMCReplication, Component->GetPawnOwner(), VeryVerbose, TEXT("%s was quantized to zero."), *this->GetLogName(Component))
  }
#endif
}

bool TSyncType<FRotator3Double2Dec>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FRotator3Double2Dec>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FRotator3Double2Dec>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return GMCCompression::IsNetEqual(Value, OtherValue, EGMC_FloatPrecision::TwoDecimals);
}

bool TSyncType<FRotator3Double2Dec>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FRotator3Double2Dec>::UType TSyncType<FRotator3Double2Dec>::Interpolate(
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
) const
{
  return static_cast<UType>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{EGMC_NumericType::Rotator, StartValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, TargetValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, PreviousValue},
      FGMC_NumericValue{EGMC_NumericType::Rotator, NextValue},
      Ratio
    ).GetRotator()
  );
}

void TSyncType<FRotator3Double2Dec>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  if (Ar.IsSaving())
  {
    GMCCompression::SerializeRotator(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
  else
  {
    gmc_ck(Ar.IsLoading())
    GMCCompression::DeserializeRotator(Value, EGMC_FloatPrecision::TwoDecimals, Ar);
  }
}

FString TSyncType<FRotator3Double2Dec>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("CompressedRotator");
}

FString TSyncType<FRotator3Double2Dec>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return GetRotatorAsString(Value, EGMC_FloatPrecision::TwoDecimals);
}

EGMC_SyncType TSyncType<FRotator3Double2Dec>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::CompressedRotator;
}

void TSyncType<FActorReference>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == nullptr)
}

void TSyncType<FActorReference>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FActorReference>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FActorReference>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FActorReference>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FActorReference>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FActorReference>::UType TSyncType<FActorReference>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FActorReference>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FActorReference>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("ActorReference");
}

FString TSyncType<FActorReference>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%s"), ::IsValid(Value) ? *Value->GetName() : TEXT("Null"));
}

EGMC_SyncType TSyncType<FActorReference>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::ActorReference;
}

void TSyncType<FActorComponentReference>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == nullptr)
}

void TSyncType<FActorComponentReference>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FActorComponentReference>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FActorComponentReference>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FActorComponentReference>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FActorComponentReference>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FActorComponentReference>::UType TSyncType<FActorComponentReference>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FActorComponentReference>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FActorComponentReference>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("ActorComponentReference");
}

FString TSyncType<FActorComponentReference>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%s"), ::IsValid(Value) ? *Value->GetName() : TEXT("Null"));
}

EGMC_SyncType TSyncType<FActorComponentReference>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::ActorComponentReference;
}

void TSyncType<FAnimMontageReference>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value == nullptr)
}

void TSyncType<FAnimMontageReference>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FAnimMontageReference>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FAnimMontageReference>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FAnimMontageReference>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FAnimMontageReference>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FAnimMontageReference>::UType TSyncType<FAnimMontageReference>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FAnimMontageReference>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FAnimMontageReference>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("AnimMontageReference");
}

FString TSyncType<FAnimMontageReference>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%s"), ::IsValid(Value) ? *Value->GetName() : TEXT("Null"));
}

EGMC_SyncType TSyncType<FAnimMontageReference>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::AnimMontageReference;
}

void TSyncType<FName>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value.IsNone())
}

void TSyncType<FName>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FName>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FName>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FName>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FName>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FName>::UType TSyncType<FName>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FName>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Ar << Value;
}

FString TSyncType<FName>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("Name");
}

FString TSyncType<FName>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%s"), *Value.ToString());
}

EGMC_SyncType TSyncType<FName>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::Name;
}

void TSyncType<FGameplayTag>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value.GetTagName().IsNone())
}

void TSyncType<FGameplayTag>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FGameplayTag>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FGameplayTag>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FGameplayTag>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FGameplayTag>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FGameplayTag>::UType TSyncType<FGameplayTag>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FGameplayTag>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Value.NetSerialize(Ar, Map, UNUSED(bool));
}

FString TSyncType<FGameplayTag>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("GameplayTag");
}

FString TSyncType<FGameplayTag>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%s"), *Value.ToString());
}

EGMC_SyncType TSyncType<FGameplayTag>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::GameplayTag;
}

void TSyncType<FGameplayTagContainer>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(Value.IsEmpty())
}

void TSyncType<FGameplayTagContainer>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FGameplayTagContainer>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FGameplayTagContainer>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FGameplayTagContainer>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FGameplayTagContainer>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FGameplayTagContainer>::UType TSyncType<FGameplayTagContainer>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FGameplayTagContainer>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
  Value.NetSerialize(Ar, Map, UNUSED(bool));
}

FString TSyncType<FGameplayTagContainer>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("GameplayTagContainer");
}

FString TSyncType<FGameplayTagContainer>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("%s"), *Value.ToStringSimple());
}

EGMC_SyncType TSyncType<FGameplayTagContainer>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::GameplayTagContainer;
}

void TSyncType<FInstancedStruct>::InitDefault(UType& Value, ::UGMC_ReplicationCmp* const Component)
{
  gmc_ck(!Value.GetScriptStruct())
  gmc_ck(!Value.GetMemory())
}

void TSyncType<FInstancedStruct>::Quantize(UType& Value, ::UGMC_ReplicationCmp* const Component) const
{}

bool TSyncType<FInstancedStruct>::ShouldCombine(const UType& Value, const UType& PreviousValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, PreviousValue, Component);
}

void TSyncType<FInstancedStruct>::Combine(UType& Value, const UType& ValueToCombine, ::UGMC_ReplicationCmp* const Component)
{}

bool TSyncType<FInstancedStruct>::IsEqual(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return Value == OtherValue;
}

bool TSyncType<FInstancedStruct>::IsValid(const UType& Value, const UType& OtherValue, ::UGMC_ReplicationCmp* const Component) const
{
  return IsEqual(Value, OtherValue, Component);
}

TSyncType<FInstancedStruct>::UType TSyncType<FInstancedStruct>::Interpolate(
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
) const
{
  return *static_cast<UType*>(
    Component->InterpolateValue(
      Function,
      FGMC_NumericValue{(void*)&StartValue},
      FGMC_NumericValue{(void*)&TargetValue},
      FGMC_NumericValue{(void*)&PreviousValue},
      FGMC_NumericValue{(void*)&NextValue},
      Ratio
    ).GetBare()
  );
}

void TSyncType<FInstancedStruct>::Serialize(UType& Value, FArchive& Ar, UPackageMap* Map, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData) const
{
#if UE_VERSION_OLDER_THAN(5, 2, 0)

  gmc_ckne()
  GMC_LOG(LogGMCReplication, NetInfo.OwningComponent->GetPawnOwner(), Error, TEXT("Instanced struct net serialization not supported for this engine version."))

#else

  Value.NetSerialize(Ar, Map, UNUSED(bool));

#endif
}

FString TSyncType<FInstancedStruct>::GetLogName(::UGMC_ReplicationCmp* const Component) const
{
  return TEXT("InstancedStruct");
}

FString TSyncType<FInstancedStruct>::GetLogValue(const UType& Value, ::UGMC_ReplicationCmp* const Component) const
{
  return FString::Printf(TEXT("<logging not supported>"));
}

EGMC_SyncType TSyncType<FInstancedStruct>::GetTypeEnum(::UGMC_ReplicationCmp* const Component) const
{
  return EGMC_SyncType::InstancedStruct;
}

#ifdef GMC_ENABLE_USER_SYNC_TYPES
#include GMC_USER_INCLUDE_PATH_TYPE_IMPL
#endif

}
