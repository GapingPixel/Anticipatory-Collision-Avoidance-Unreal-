// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"

#ifdef GMC_ENABLE_USER_SYNC_TYPES
#include GMC_USER_INCLUDE_PATH_TYPES
#endif

#define INTEGRATED_SYNC_TYPES\
  ActorBase,\
  LinearVelocity,\
  AngularVelocity,\
  ActorLocation,\
  ActorRotation,\
  ActorScale,\
  ControlRotation

#define INTEGRATED_SYNC_TYPES_DEFAULTS\
  ActorBase,\
  LinearVelocity,\
  AngularVelocity,\
  ActorLocation,\
  ActorRotation,\
  ActorScale,\
  ControlRotation

#define INTEGRATED_SYNC_TYPES_TAGS\
  // None yet.

#define INTEGRATED_SYNC_TYPES_NO_PHYSICS\
  ActorBase,\
  ActorScale,\
  ControlRotation

#define GMC_GENERIC_SYNC_TYPES\
  Bit,\
  UnsignedInt4,\
  UnsignedInt8,\
  Int32,\
  Float,\
  Float2Dec,\
  Double,\
  Double2Dec,\
  DoubleAsFloat,\
  Vector2Double2Dec,\
  Vector3Double2Dec,\
  Rotator3Double2Dec,\
  ActorReference,\
  ActorComponentReference,\
  AnimMontageReference,\
  Name,\
  GameplayTag,\
  GameplayTagContainer,\
  InstancedStruct

#ifdef GMC_ENABLE_USER_SYNC_TYPES
#define GENERIC_SYNC_TYPES_EXP GMC_GENERIC_SYNC_TYPES, TYPE_NAMES
#define GENERIC_SYNC_TYPES GENERIC_SYNC_TYPES_EXP
#else
#define GENERIC_SYNC_TYPES GMC_GENERIC_SYNC_TYPES
#endif

#define ALL_SYNC_TYPES_EXP INTEGRATED_SYNC_TYPES, GENERIC_SYNC_TYPES
#define ALL_SYNC_TYPES ALL_SYNC_TYPES_EXP

namespace GMCReplication
{

#define DEFINE_SYNC_TYPE(Name, Type) struct F##Name { Type Name{}; Type& Get() { return Name; } Type const& Get() const { return Name; } };

// Integrated sync types.
DEFINE_SYNC_TYPE(ActorBase, ::USceneComponent*)
DEFINE_SYNC_TYPE(LinearVelocity, ::FVector)
DEFINE_SYNC_TYPE(AngularVelocity, ::FVector)
DEFINE_SYNC_TYPE(ActorLocation, ::FVector)
DEFINE_SYNC_TYPE(ActorRotation, ::FRotator)
DEFINE_SYNC_TYPE(ActorScale, ::FVector)
DEFINE_SYNC_TYPE(ControlRotation, ::FRotator)

// Generic sync types.
DEFINE_SYNC_TYPE(Bit, bool) // Bool serialized as 1 bit.
DEFINE_SYNC_TYPE(UnsignedInt4, ::uint8) // Unsigned 8 bit integer serialized as 4 bit.
DEFINE_SYNC_TYPE(UnsignedInt8, ::uint8)
DEFINE_SYNC_TYPE(Int32, ::int32)
DEFINE_SYNC_TYPE(Float, float)
DEFINE_SYNC_TYPE(Float2Dec, float) // Float serialized with 2 decimal places of precision.
DEFINE_SYNC_TYPE(Double, double)
DEFINE_SYNC_TYPE(Double2Dec, double) // Double serialized with 2 decimal places of precision.
DEFINE_SYNC_TYPE(DoubleAsFloat, double) // Truncated double serialized as 32 bit float.
DEFINE_SYNC_TYPE(Vector2Double2Dec, ::FVector2D) // 2D double vector serialized with 2 decimal places of precision.
DEFINE_SYNC_TYPE(Vector3Double2Dec, ::FVector) // 3D double vector serialized with 2 decimal places of precision.
DEFINE_SYNC_TYPE(Rotator3Double2Dec, ::FRotator) // 3D double rotator serialized with 2 decimal places of precision.
DEFINE_SYNC_TYPE(ActorReference, ::AActor*)
DEFINE_SYNC_TYPE(ActorComponentReference, ::UActorComponent*)
DEFINE_SYNC_TYPE(AnimMontageReference, ::UAnimMontage*)
DEFINE_SYNC_TYPE(Name, ::FName)
DEFINE_SYNC_TYPE(GameplayTag, ::FGameplayTag)
DEFINE_SYNC_TYPE(GameplayTagContainer, ::FGameplayTagContainer)
DEFINE_SYNC_TYPE(InstancedStruct, ::FInstancedStruct)

#ifdef GMC_ENABLE_USER_SYNC_TYPES
TYPE_DEFS
#endif

#undef DEFINE_SYNC_TYPE

}

UENUM(BlueprintType)
enum class EGMC_SyncType : uint8
{
  ActorBase,
  LinearVelocity,
  AngularVelocity,
  ActorLocation,
  ActorRotation,
  ActorScale,
  ControlRotation,
  Bool,
  HalfByte,
  Byte,
  Int,
  SinglePrecisionFloat,
  CompressedSinglePrecisionFloat,
  DoublePrecisionFloat,
  CompressedDoublePrecisionFloat,
  TruncatedDoublePrecisionFloat,
  CompressedVector2D,
  CompressedVector,
  CompressedRotator,
  ActorReference,
  ActorComponentReference,
  AnimMontageReference,
  Name,
  GameplayTag,
  GameplayTagContainer,
  InstancedStruct,

#ifdef GMC_ENABLE_USER_SYNC_TYPES
#include GMC_USER_INCLUDE_PATH_TYPE_LIST
#endif

  MAX UMETA(Hidden)
};
