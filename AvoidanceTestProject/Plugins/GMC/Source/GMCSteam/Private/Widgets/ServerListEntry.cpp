// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "ServerListEntry.h"
#include "MainMenu.h"

void UServerListEntry::SetEntryIndex(UUserWidget* InParent, uint32 InIndex)
{
  Parent = InParent;
  Index = InIndex;
  if (ServerButton) ServerButton->OnClicked.AddDynamic(this, &UServerListEntry::OnServerButtonClicked);
}

void UServerListEntry::OnServerButtonClicked()
{
  if (UMainMenu* MainMenu = Cast<UMainMenu>(Parent))
  {
    MainMenu->SetSelectedServerListIndex(Index);
    if (UScrollBox* ServerListScrollBox = MainMenu->Join_ServerListScrollBox)
    {
      // Reset all buttons in the scroll box.
      FButtonStyle ButtonStyle{};
      FSlateBrush SlateBrush{};
      SlateBrush.TintColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.0f);
      ButtonStyle.Normal = SlateBrush;
      SlateBrush.TintColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.4f);
      ButtonStyle.Hovered = SlateBrush;
      SlateBrush.TintColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.6f);
      ButtonStyle.Pressed = SlateBrush;
      TArray<UWidget*> AllEntries = ServerListScrollBox->GetAllChildren();
      for (UWidget* Entry : AllEntries)
      {
        if (UServerListEntry* ServerListEntry = Cast<UServerListEntry>(Entry))
        {
          if (UButton* EntryServerButton = ServerListEntry->ServerButton)
          {
            EntryServerButton->SetStyle(ButtonStyle);
          }
        }
      }
      if (ServerButton)
      {
        // Highlight the selected button.
        SlateBrush.TintColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.2f);
        ButtonStyle.Normal = SlateBrush;
        ServerButton->SetStyle(ButtonStyle);
      }
    }
  }
}
