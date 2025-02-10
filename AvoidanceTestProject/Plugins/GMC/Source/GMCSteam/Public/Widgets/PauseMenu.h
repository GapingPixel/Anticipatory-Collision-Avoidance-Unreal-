// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenu.generated.h"

/// Provides the bindings for a widget Blueprint which displays a very simple pause menu for sessions created with USteamGameInstance.
UCLASS()
class GMCSTEAM_API UPauseMenu : public UUserWidget
{
  GENERATED_BODY()

protected:

  bool Initialize() override;

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UButton> ExitButton{nullptr};

  UFUNCTION()
  void OnExitButtonClicked();
};
