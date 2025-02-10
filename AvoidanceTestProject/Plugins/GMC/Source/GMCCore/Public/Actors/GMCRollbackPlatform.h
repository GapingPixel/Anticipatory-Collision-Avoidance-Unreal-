// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCRollbackActor.h"
#include "GMCRollbackPlatform.generated.h"

/// Rollback actor that will continuously rotate and/or move back and forth in a straight line between two points at a fixed speed.
UCLASS(ClassGroup = "Movement", BlueprintType, Blueprintable)
class GMCCORE_API AGMC_RollbackPlatform : public AGMC_RollbackActor
{
  GENERATED_BODY()

public:

  AGMC_RollbackPlatform(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void UpdateState_Implementation(double Time, float DeltaTime, const struct FGMC_Move& Move, EGMC_NetContext Context) override;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0", UIMin = "0.1"))
  /// For how long the platform should move with the configured linear velocity before reverting its direction (in seconds).
  double LinearMoveDuration{3.};
};
