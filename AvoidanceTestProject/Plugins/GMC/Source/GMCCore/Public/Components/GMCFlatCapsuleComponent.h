// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCCapsuleComponent.h"
#include "GMCFlatCapsuleComponent.generated.h"

/// A horizontally oriented capsule component.
UCLASS(ClassGroup = "Collision", Blueprintable, meta = (DisplayName = "Flat Capsule Collision", BlueprintSpawnableComponent))
class GMCCORE_API UGMC_FlatCapsuleCmp : public UGMC_CapsuleCmp
{
  GENERATED_BODY()

public:

  UGMC_FlatCapsuleCmp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  /// The orientation of the capsule. Note that other values may not work properly with UGMC_MovementUtilityCmp.
  inline static const FRotator FLAT_ROTATION = {90., 0., 0.};
};
