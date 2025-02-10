// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenu.generated.h"

/// Provides the bindings for a widget Blueprint which can expose the functionality of USteamGameInstance as UI.
UCLASS()
class GMCSTEAM_API UMainMenu : public UUserWidget
{
  GENERATED_BODY()

public:

  bool Initialize() override;

  /// Menu top-level.

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UWidget> Main{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UButton> Main_HostButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Main_JoinButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UTextBlock> Main_HostText{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Main_ExitButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UWidgetSwitcher> Main_MenuSwitcher{nullptr};

  UFUNCTION()
  void OnMain_HostButtonClicked();

  UFUNCTION()
  void OnMain_JoinButtonClicked();

  UFUNCTION()
  void OnMain_ExitButtonClicked();

  /// Host submenu.

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UWidget> Host{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Host_HostButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UTextBlock> Host_HostText{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Host_CancelButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Host_SinglePlayerButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Host_MultiplayerButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UEditableTextBox> Host_UserDesiredSessionNameTextBox{nullptr};

  UFUNCTION()
  void OnHost_HostButtonClicked();

  UFUNCTION()
  void OnHost_CancelButtonClicked();

  UFUNCTION()
  void OnHost_SinglePlayerButtonClicked();

  UFUNCTION()
  void OnHost_MultiplayerButtonClicked();

  UFUNCTION()
  void OnHost_UserDesiredSessionNameTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
  FString UserDesiredSessionName;

  void ResetHostButton();

  /// Join submenu.

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UWidget> Join{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Join_JoinButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UTextBlock> Join_JoinText{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Join_CancelButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<class UScrollBox> Join_ServerListScrollBox{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UButton> Join_RefreshButton{nullptr};

  UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidgetOptional))
  TObjectPtr<UTextBlock> Join_RefreshText{nullptr};

  UFUNCTION()
  void OnJoin_JoinButtonClicked();

  UFUNCTION()
  void OnJoin_CancelButtonClicked();

  UFUNCTION()
  void OnJoin_RefreshButtonClicked();

  void RefreshServerListUI();

  void ResetJoinButton();

  void SetSelectedServerListIndex(uint32 Index);

  TOptional<uint32> SelectedServerListIndex;

private:

  void SetDefaultButtonStyle(UButton* Button);

  void SetActiveButtonStyle(UButton* Button);
};
