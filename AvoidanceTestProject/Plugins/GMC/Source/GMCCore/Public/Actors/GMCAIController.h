// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GMCAIController.generated.h"

/// AI controller class intended to be used with UGMC_MovementReplicationComponent.
UCLASS(BlueprintType, Blueprintable)
class GMCCORE_API AGMC_AIController : public AAIController
{
  GENERATED_BODY()

public:

  AGMC_AIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void BeginPlay() override;
  void Tick(float DeltaTime) override;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", AdvancedDisplay)
  /// If true, the integrated "UpdateControlRotation" function of the default AI controller will be executed each tick.
  bool bUpdateControlRotation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", AdvancedDisplay)
  /// The value for the parameter of the same name that should be passed to the "UpdateControlRotation" function (if it executes).
  bool bUpdatePawn{false};

protected:

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// Cached reference to the GMC aggregator (if present).
  TObjectPtr<class AGMC_Aggregator> GMCAggregator{nullptr};
};
