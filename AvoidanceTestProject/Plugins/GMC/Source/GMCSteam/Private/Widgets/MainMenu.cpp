// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "MainMenu.h"
#include "SteamGameInstance.h"
#include "ServerListEntry.h"

bool UMainMenu::Initialize()
{
  if (Super::Initialize())
  {
    if (Main_HostButton) Main_HostButton->OnClicked.AddDynamic(this, &UMainMenu::OnMain_HostButtonClicked);
    if (Main_JoinButton) Main_JoinButton->OnClicked.AddDynamic(this, &UMainMenu::OnMain_JoinButtonClicked);
    if (Main_ExitButton) Main_ExitButton->OnClicked.AddDynamic(this, &UMainMenu::OnMain_ExitButtonClicked);
    if (Join_CancelButton) Join_CancelButton->OnClicked.AddDynamic(this, &UMainMenu::OnJoin_CancelButtonClicked);
    if (Join_JoinButton) Join_JoinButton->OnClicked.AddDynamic(this, &UMainMenu::OnJoin_JoinButtonClicked);
    if (Join_RefreshButton) Join_RefreshButton->OnClicked.AddDynamic(this, &UMainMenu::OnJoin_RefreshButtonClicked);
    if (Host_CancelButton) Host_CancelButton->OnClicked.AddDynamic(this, &UMainMenu::OnHost_CancelButtonClicked);
    if (Host_HostButton) Host_HostButton->OnClicked.AddDynamic(this, &UMainMenu::OnHost_HostButtonClicked);
    if (Host_SinglePlayerButton) Host_SinglePlayerButton->OnClicked.AddDynamic(this, &UMainMenu::OnHost_SinglePlayerButtonClicked);
    if (Host_MultiplayerButton) Host_MultiplayerButton->OnClicked.AddDynamic(this, &UMainMenu::OnHost_MultiplayerButtonClicked);
    if (Host_UserDesiredSessionNameTextBox) Host_UserDesiredSessionNameTextBox->OnTextCommitted.AddDynamic(this, &UMainMenu::OnHost_UserDesiredSessionNameTextCommitted);

    // Bind delegates.
    if (USteamGameInstance* SteamGameInstance = Cast<USteamGameInstance>(GetGameInstance()))
    {
      SteamGameInstance->OnCreateSessionCompleted.BindUObject(this, &UMainMenu::ResetHostButton);
      SteamGameInstance->OnJoinSessionCompleted.BindUObject(this, &UMainMenu::ResetJoinButton);
      SteamGameInstance->OnServerListUpdated.BindUObject(this, &UMainMenu::RefreshServerListUI);
    }
    return true;
  }
  return false;
}

void UMainMenu::OnMain_HostButtonClicked()
{
  if (Main_MenuSwitcher)
  {
    Main_MenuSwitcher->SetActiveWidget(Host); // Switch to hosting submenu.
  }
}

void UMainMenu::OnMain_JoinButtonClicked()
{
  if (Main_MenuSwitcher)
  {
    Main_MenuSwitcher->SetActiveWidget(Join); // Switch to joining submenu.
  }
}

void UMainMenu::OnHost_UserDesiredSessionNameTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
  if (CommitMethod == ETextCommit::OnEnter)
  {
    UserDesiredSessionName = Text.ToString();
    OnHost_HostButtonClicked(); // When confirming with enter we instantly start the hosting process (as if the host button was clicked).
  }
  else
  {
    UserDesiredSessionName = Text.ToString();
  }
}

void UMainMenu::OnMain_ExitButtonClicked()
{
  if (const UGameInstance* GameInstance = GetGameInstance())
  {
    const auto& LocalPlayerController = GameInstance->GetFirstLocalPlayerController();
    UKismetSystemLibrary::QuitGame(GetWorld(), LocalPlayerController, EQuitPreference::Quit, false);
  }
}

void UMainMenu::OnHost_HostButtonClicked()
{
  if (USteamGameInstance* GameInstance = Cast<USteamGameInstance>(GetGameInstance()))
  {
    if (UserDesiredSessionName != TEXT(""))
    {
      GameInstance->SessionSettings.NumPublicConnections = GameInstance->MaxPublicConnections;
      if (GameInstance->HostGameSession(UserDesiredSessionName))
      {
        // Indicate that we are in the process of creating a session with a different button style.
        if (Host_HostText)
        {
          Host_HostText->SetText(FText::FromString(TEXT("Creating session...")));
        }
        SetActiveButtonStyle(Host_HostButton);
      }
    }
    else
    {
      UE_LOG(LogGMCOnlineSubsystem, Warning, TEXT("Session name required."))
    }
  }
}

void UMainMenu::OnHost_CancelButtonClicked()
{
  if (Main_MenuSwitcher)
  {
    Main_MenuSwitcher->SetActiveWidget(Main);
  }
}

void UMainMenu::OnHost_SinglePlayerButtonClicked()
{
  if (Host_SinglePlayerButton) Host_SinglePlayerButton->SetBackgroundColor(FColor::Red);
  if (Host_MultiplayerButton) Host_MultiplayerButton->SetBackgroundColor(FColor::White);
}

void UMainMenu::OnHost_MultiplayerButtonClicked()
{
  if (Host_MultiplayerButton) Host_MultiplayerButton->SetBackgroundColor(FColor::Red);
  if (Host_SinglePlayerButton) Host_SinglePlayerButton->SetBackgroundColor(FColor::White);
}

void UMainMenu::ResetHostButton()
{
  // Set the host button style back to normal. This is only relevant if hosting failed.
  if (Host_HostText)
  {
    Host_HostText->SetText(FText::FromString(TEXT("Host")));
  }
  SetDefaultButtonStyle(Host_HostButton);
}

void UMainMenu::OnJoin_JoinButtonClicked()
{
  if (SelectedServerListIndex.IsSet())
  {
    if (USteamGameInstance* SteamGameInstance = Cast<USteamGameInstance>(GetGameInstance()))
    {
      if (SteamGameInstance->JoinGameSession(SelectedServerListIndex.GetValue()))
      {
        // Indicate that we are in the process of joining a session with a different button style.
        if (Join_JoinText)
        {
          Join_JoinText->SetText(FText::FromString(TEXT("Joining...")));
        }
        SetActiveButtonStyle(Join_JoinButton);
      }
    }
  }
}

void UMainMenu::OnJoin_CancelButtonClicked()
{
  if (Main_JoinButton)
  {
    Main_MenuSwitcher->SetActiveWidget(Main);
  }
}

void UMainMenu::OnJoin_RefreshButtonClicked()
{
  if (USteamGameInstance* SteamGameInstance = Cast<USteamGameInstance>(GetGameInstance()))
  {
    if (SteamGameInstance->RefreshServerList())
    {
      // Indicate that we are currently searching for sessions with a different button style.
      if (Join_RefreshText)
      {
        Join_RefreshText->SetText(FText::FromString(TEXT("Searching...")));
      }
      SetActiveButtonStyle(Join_RefreshButton);
    }
  }
}

void UMainMenu::RefreshServerListUI()
{
  // Set the refresh button style back to normal.
  if (Join_RefreshText)
  {
    Join_RefreshText->SetText(FText::FromString(TEXT("Refresh")));
  }
  SetDefaultButtonStyle(Join_RefreshButton);

  const USteamGameInstance* SteamGameInstance = Cast<USteamGameInstance>(GetGameInstance());
  SelectedServerListIndex.Reset();
  if (SteamGameInstance && Join_ServerListScrollBox)
  {
    Join_ServerListScrollBox->ClearChildren(); // Clear the server list before repopulating.
    for (int32 Index = 0; Index < SteamGameInstance->ServerList.Num(); ++Index)
    {
      UServerListEntry* ServerListEntry = CreateWidget<UServerListEntry>(this, SteamGameInstance->ServerListEntry);
      if (ServerListEntry)
      {
        UTextBlock* ServerName = ServerListEntry->ServerName;
        UTextBlock* HostName = ServerListEntry->HostName;
        UTextBlock* NumCurrentPlayers = ServerListEntry->NumCurrentPlayers;
        UTextBlock* NumMaxPlayers = ServerListEntry->NumMaxPlayers;
        UTextBlock* Ping = ServerListEntry->Ping;
        if (ServerName && HostName && NumCurrentPlayers && NumMaxPlayers)
        {
          ServerName->SetText(FText::FromString(SteamGameInstance->ServerList[Index].ServerName));
          HostName->SetText(FText::FromString(SteamGameInstance->ServerList[Index].HostName));
          NumMaxPlayers->SetText(FText::FromString(FString::FromInt(SteamGameInstance->ServerList[Index].NumMaxPlayers)));
          NumCurrentPlayers->SetText(FText::FromString(FString::FromInt(SteamGameInstance->ServerList[Index].NumCurrentPlayers)));
          Ping->SetText(FText::FromString(FString::FromInt(SteamGameInstance->ServerList[Index].Ping)));
          ServerListEntry->SetEntryIndex(this, Index);
          Join_ServerListScrollBox->AddChild(ServerListEntry);
        }
      }
    }
  }
}

void UMainMenu::ResetJoinButton()
{
  // Set the join button style back to normal. This is only relevant if joining failed.
  if (Join_JoinText)
  {
    Join_JoinText->SetText(FText::FromString(TEXT("Join")));
  }
  SetDefaultButtonStyle(Join_JoinButton);
}

void UMainMenu::SetSelectedServerListIndex(uint32 Index)
{
  SelectedServerListIndex = Index;
}

void UMainMenu::SetDefaultButtonStyle(UButton* Button)
{
  if (!Button) return;
  FButtonStyle ButtonStyle{};
  FSlateBrush SlateBrush{};
  SlateBrush.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
  ButtonStyle.Normal = SlateBrush;
  ButtonStyle.Hovered = SlateBrush;
  ButtonStyle.Pressed = SlateBrush;
  Button->SetStyle(ButtonStyle);
}

void UMainMenu::SetActiveButtonStyle(UButton* Button)
{
  if (!Button) return;
  FButtonStyle ButtonStyle{};
  FSlateBrush SlateBrush{};
  SlateBrush.TintColor = FLinearColor(1.0f, 0.5f, 0.2f, 1.0f);
  ButtonStyle.Normal = SlateBrush;
  ButtonStyle.Hovered = SlateBrush;
  ButtonStyle.Pressed = SlateBrush;
  Button->SetStyle(ButtonStyle);
}
