// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "PauseMenu.h"
#include "SteamGameInstance.h"

bool UPauseMenu::Initialize()
{
  if (Super::Initialize())
  {
    if (ExitButton) ExitButton->OnClicked.AddDynamic(this, &UPauseMenu::OnExitButtonClicked);
    return true;
  }
  return false;
}

void UPauseMenu::OnExitButtonClicked()
{
  if (USteamGameInstance* SteamGameInstance = Cast<USteamGameInstance>(GetGameInstance()))
  {
    IOnlineSessionPtr SessionInterface = SteamGameInstance->SessionInterface;
    if (SessionInterface.IsValid())
    {
      FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SteamGameInstance->SessionLabel);
      if (ExistingSession)
      {
        SessionInterface->DestroySession(ExistingSession->SessionName);
      }
    }
    SteamGameInstance->ReturnToMainMenu();
  }
}
