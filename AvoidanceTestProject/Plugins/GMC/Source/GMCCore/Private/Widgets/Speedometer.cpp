// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "Speedometer.h"

bool UGMC_Speedometer::Initialize()
{
  if (Super::Initialize())
  {
    SetNumberFormattingOptions();
    return true;
  }
  return false;
}

void UGMC_Speedometer::SetNumberFormattingOptions()
{
  NumberFormat.SetAlwaysSign(false);
  NumberFormat.SetUseGrouping(false);
  NumberFormat.SetRoundingMode(ERoundingMode::HalfFromZero);
  NumberFormat.SetMinimumIntegralDigits(1);
  NumberFormat.SetMaximumIntegralDigits(10);
  NumberFormat.SetMinimumFractionalDigits(0);
  NumberFormat.SetMaximumFractionalDigits(0);
}

void UGMC_Speedometer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);

  Value_Speedometer = 0.f;
  if (const auto& MovementComponent = GetLocalPawnMovementComponent())
  {
    const FVector& Velocity = MovementComponent->Velocity;
    Value_Speedometer = bIncludeVelocityZ ? Velocity.Size() : Velocity.Size2D();
  }

  if (Text_Speedometer) Text_Speedometer->SetText(FText::AsNumber(Value_Speedometer, &NumberFormat));
}

UPawnMovementComponent* UGMC_Speedometer::GetLocalPawnMovementComponent() const
{
  if (const auto& World = GetWorld())
  {
    for (auto Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
      const auto& PlayerController = Iterator->Get();
      if (PlayerController && PlayerController->IsLocalController())
      {
        if (const auto& LocalPawn = PlayerController->GetPawn())
        {
          return LocalPawn->FindComponentByClass<UPawnMovementComponent>();
        }
        break;
      }
    }
  }
  return nullptr;
}
