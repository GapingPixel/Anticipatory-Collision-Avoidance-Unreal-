// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Speedometer.generated.h"

/// Provides the bindings for a widget Blueprint which displays the current speed of the pawn.
UCLASS()
class GMCCORE_API UGMC_Speedometer : public UUserWidget
{
  GENERATED_BODY()

public:

  bool Initialize() override;

  void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

  virtual void SetNumberFormattingOptions();

  FNumberFormattingOptions NumberFormat;

  UFUNCTION(BlueprintCallable, Category = "UI")
  virtual class UPawnMovementComponent* GetLocalPawnMovementComponent() const;

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UTextBlock> Text_Speedometer{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI")
  float Value_Speedometer{0.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
  bool bIncludeVelocityZ{false};
};
