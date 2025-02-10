// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/ADogGameSettingRegistry.h"

#include "GameSettingCollection.h"
#include "Settings/ADogSettingsLocal.h"
#include "Settings/ADogSettingsShared.h"
#include "Player/ADogLocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogGameSettingRegistry)

#define LOCTEXT_NAMESPACE "ADog"

//--------------------------------------
// UADogGameSettingRegistry
//--------------------------------------

UADogGameSettingRegistry::UADogGameSettingRegistry()
{
}

UADogGameSettingRegistry* UADogGameSettingRegistry::Get(UADogLocalPlayer* InLocalPlayer)
{
	UADogGameSettingRegistry* Registry = FindObject<UADogGameSettingRegistry>(InLocalPlayer, TEXT("ADogGameSettingRegistry"), true);
	if (Registry == nullptr)
	{
		Registry = NewObject<UADogGameSettingRegistry>(InLocalPlayer, TEXT("ADogGameSettingRegistry"));
		Registry->Initialize(InLocalPlayer);
	}

	return Registry;
}

bool UADogGameSettingRegistry::IsFinishedInitializing() const
{
	if (Super::IsFinishedInitializing())
	{
		if (UADogLocalPlayer* LocalPlayer = Cast<UADogLocalPlayer>(OwningLocalPlayer))
		{
			if (LocalPlayer->GetSharedSettings() == nullptr)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void UADogGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	UADogLocalPlayer* ADogLocalPlayer = Cast<UADogLocalPlayer>(InLocalPlayer);

	VideoSettings = InitializeVideoSettings(ADogLocalPlayer);
	InitializeVideoSettings_FrameRates(VideoSettings, ADogLocalPlayer);
	RegisterSetting(VideoSettings);

	AudioSettings = InitializeAudioSettings(ADogLocalPlayer);
	RegisterSetting(AudioSettings);

	GameplaySettings = InitializeGameplaySettings(ADogLocalPlayer);
	RegisterSetting(GameplaySettings);

	MouseAndKeyboardSettings = InitializeMouseAndKeyboardSettings(ADogLocalPlayer);
	RegisterSetting(MouseAndKeyboardSettings);

	GamepadSettings = InitializeGamepadSettings(ADogLocalPlayer);
	RegisterSetting(GamepadSettings);
}

void UADogGameSettingRegistry::SaveChanges()
{
	Super::SaveChanges();
	
	if (UADogLocalPlayer* LocalPlayer = Cast<UADogLocalPlayer>(OwningLocalPlayer))
	{
		// Game user settings need to be applied to handle things like resolution, this saves indirectly
		LocalPlayer->GetLocalSettings()->ApplySettings(false);
		
		LocalPlayer->GetSharedSettings()->ApplySettings();
		LocalPlayer->GetSharedSettings()->SaveSettings();
	}
}

#undef LOCTEXT_NAMESPACE

