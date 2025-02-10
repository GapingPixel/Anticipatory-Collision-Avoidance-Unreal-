// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCAIController.h"
#include "GMCAggregator.h"
#include "GMCLog.h"

AGMC_AIController::AGMC_AIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{}

void AGMC_AIController::BeginPlay()
{
  Super::BeginPlay();

  gmc_ck(PrimaryActorTick.IsTickFunctionRegistered())

  GMCAggregator = AGMC_Aggregator::GetGMCAggregator(this);

  if (IsValid(GMCAggregator))
  {
    GMCAggregator->RegisterController(this);
  }
}

void AGMC_AIController::Tick(float DeltaTime)
{
  AActor::Tick(DeltaTime);

  if (bUpdateControlRotation)
  {
    UpdateControlRotation(DeltaTime, bUpdatePawn);
  }
}
