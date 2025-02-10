// Fill out your copyright notice in the Description page of Project Settings.


#include "Audio/ADogAudioMixEffectsSubsystem.h"

#include "AudioMixerBlueprintLibrary.h"
#include "AudioModulationStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "LoadingScreenManager.h"
#include "Audio/ADogAudioSettings.h"
#include "Settings/ADogSettingsLocal.h"
#include "Sound/SoundEffectSubmix.h"
#include "SoundControlBus.h"
#include "SoundControlBusMix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogAudioMixEffectsSubsystem)

class FSubsystemCollectionBase;

void UADogAudioMixEffectsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UADogAudioMixEffectsSubsystem::Deinitialize()
{
	if (ULoadingScreenManager* LoadingScreenManager = UGameInstance::GetSubsystem<ULoadingScreenManager>(GetWorld()->GetGameInstance()))
	{
		LoadingScreenManager->OnLoadingScreenVisibilityChangedDelegate().RemoveAll(this);
		ApplyOrRemoveLoadingScreenMix(false);
	}

	Super::Deinitialize();
}

bool UADogAudioMixEffectsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	bool bShouldCreateSubsystem = Super::ShouldCreateSubsystem(Outer);

	if (Outer)
	{
		if (UWorld* World = Outer->GetWorld())
		{
			bShouldCreateSubsystem = DoesSupportWorldType(World->WorldType) && bShouldCreateSubsystem;
		}
	}

	return bShouldCreateSubsystem;
}

void UADogAudioMixEffectsSubsystem::PostInitialize()
{
	if (const UADogAudioSettings* ADogAudioSettings = GetDefault<UADogAudioSettings>())
	{
		if (UObject* ObjPath = ADogAudioSettings->DefaultControlBusMix.TryLoad())
		{
			if (USoundControlBusMix* SoundControlBusMix = Cast<USoundControlBusMix>(ObjPath))
			{
				DefaultBaseMix = SoundControlBusMix;
			}
			else
			{
				ensureMsgf(SoundControlBusMix, TEXT("Default Control Bus Mix reference missing from ADog Audio Settings."));
			}
		}

		if (UObject* ObjPath = ADogAudioSettings->LoadingScreenControlBusMix.TryLoad())
		{
			if (USoundControlBusMix* SoundControlBusMix = Cast<USoundControlBusMix>(ObjPath))
			{
				LoadingScreenMix = SoundControlBusMix;
			}
			else
			{
				ensureMsgf(SoundControlBusMix, TEXT("Loading Screen Control Bus Mix reference missing from ADog Audio Settings."));
			}
		}

		if (UObject* ObjPath = ADogAudioSettings->UserSettingsControlBusMix.TryLoad())
		{
			if (USoundControlBusMix* SoundControlBusMix = Cast<USoundControlBusMix>(ObjPath))
			{
				UserMix = SoundControlBusMix;
			}
			else
			{
				ensureMsgf(SoundControlBusMix, TEXT("User Control Bus Mix reference missing from ADog Audio Settings."));
			}
		}

		if (UObject* ObjPath = ADogAudioSettings->OverallVolumeControlBus.TryLoad())
		{
			if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
			{
				OverallControlBus = SoundControlBus;
			}
			else
			{
				ensureMsgf(SoundControlBus, TEXT("Overall Control Bus reference missing from ADog Audio Settings."));
			}
		}

		if (UObject* ObjPath = ADogAudioSettings->MusicVolumeControlBus.TryLoad())
		{
			if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
			{
				MusicControlBus = SoundControlBus;
			}
			else
			{
				ensureMsgf(SoundControlBus, TEXT("Music Control Bus reference missing from ADog Audio Settings."));
			}
		}

		if (UObject* ObjPath = ADogAudioSettings->SoundFXVolumeControlBus.TryLoad())
		{
			if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
			{
				SoundFXControlBus = SoundControlBus;
			}
			else
			{
				ensureMsgf(SoundControlBus, TEXT("SoundFX Control Bus reference missing from ADog Audio Settings."));
			}
		}

		if (UObject* ObjPath = ADogAudioSettings->DialogueVolumeControlBus.TryLoad())
		{
			if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
			{
				DialogueControlBus = SoundControlBus;
			}
			else
			{
				ensureMsgf(SoundControlBus, TEXT("Dialogue Control Bus reference missing from ADog Audio Settings."));
			}
		}

		if (UObject* ObjPath = ADogAudioSettings->VoiceChatVolumeControlBus.TryLoad())
		{
			if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
			{
				VoiceChatControlBus = SoundControlBus;
			}
			else
			{
				ensureMsgf(SoundControlBus, TEXT("VoiceChat Control Bus reference missing from ADog Audio Settings."));
			}
		}

		// Load HDR Submix Effect Chain
		for (const FADogSubmixEffectChainMap& SoftSubmixEffectChain : ADogAudioSettings->HDRAudioSubmixEffectChain)
		{
			FADogAudioSubmixEffectsChain NewEffectChain;

			if (UObject* SubmixObjPath = SoftSubmixEffectChain.Submix.LoadSynchronous())
			{
				if (USoundSubmix* Submix = Cast<USoundSubmix>(SubmixObjPath))
				{
					NewEffectChain.Submix = Submix;
					TArray<USoundEffectSubmixPreset*> NewPresetChain;

					for (const TSoftObjectPtr<USoundEffectSubmixPreset>& SoftEffect : SoftSubmixEffectChain.SubmixEffectChain)
					{
						if (UObject* EffectObjPath = SoftEffect.LoadSynchronous())
						{
							if (USoundEffectSubmixPreset* SubmixPreset = Cast<USoundEffectSubmixPreset>(EffectObjPath))
							{
								NewPresetChain.Add(SubmixPreset);
							}
						}
					}

					NewEffectChain.SubmixEffectChain.Append(NewPresetChain);
				}
			}

			HDRSubmixEffectChain.Add(NewEffectChain);
		}

		// Load LDR Submix Effect Chain
		for (const FADogSubmixEffectChainMap& SoftSubmixEffectChain : ADogAudioSettings->LDRAudioSubmixEffectChain)
		{
			FADogAudioSubmixEffectsChain NewEffectChain;

			if (UObject* SubmixObjPath = SoftSubmixEffectChain.Submix.LoadSynchronous())
			{
				if (USoundSubmix* Submix = Cast<USoundSubmix>(SubmixObjPath))
				{
					NewEffectChain.Submix = Submix;
					TArray<USoundEffectSubmixPreset*> NewPresetChain;

					for (const TSoftObjectPtr<USoundEffectSubmixPreset>& SoftEffect : SoftSubmixEffectChain.SubmixEffectChain)
					{
						if (UObject* EffectObjPath = SoftEffect.LoadSynchronous())
						{
							if (USoundEffectSubmixPreset* SubmixPreset = Cast<USoundEffectSubmixPreset>(EffectObjPath))
							{
								NewPresetChain.Add(SubmixPreset);
							}
						}
					}

					NewEffectChain.SubmixEffectChain.Append(NewPresetChain);
				}
			}

			LDRSubmixEffectChain.Add(NewEffectChain);
		}
	}

	// Register with the loading screen manager
	if (ULoadingScreenManager* LoadingScreenManager = UGameInstance::GetSubsystem<ULoadingScreenManager>(GetWorld()->GetGameInstance()))
	{
		LoadingScreenManager->OnLoadingScreenVisibilityChangedDelegate().AddUObject(this, &ThisClass::OnLoadingScreenStatusChanged);
		ApplyOrRemoveLoadingScreenMix(LoadingScreenManager->GetLoadingScreenDisplayStatus());
	}
}

void UADogAudioMixEffectsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	if (const UWorld* World = InWorld.GetWorld())
	{
		// Activate the default base mix
		if (DefaultBaseMix)
		{
			UAudioModulationStatics::ActivateBusMix(World, DefaultBaseMix);
		}

		// Retrieve the user settings
		if (const UADogSettingsLocal* ADogSettingsLocal = GetDefault<UADogSettingsLocal>())
		{
			// Activate the User Mix
			if (UserMix)
			{
				UAudioModulationStatics::ActivateBusMix(World, UserMix);

				if (OverallControlBus && MusicControlBus && SoundFXControlBus && DialogueControlBus && VoiceChatControlBus)
				{
					const FSoundControlBusMixStage OverallControlBusMixStage = UAudioModulationStatics::CreateBusMixStage(World, OverallControlBus, ADogSettingsLocal->GetOverallVolume());
					const FSoundControlBusMixStage MusicControlBusMixStage = UAudioModulationStatics::CreateBusMixStage(World, MusicControlBus, ADogSettingsLocal->GetMusicVolume());
					const FSoundControlBusMixStage SoundFXControlBusMixStage = UAudioModulationStatics::CreateBusMixStage(World, SoundFXControlBus, ADogSettingsLocal->GetSoundFXVolume());
					const FSoundControlBusMixStage DialogueControlBusMixStage = UAudioModulationStatics::CreateBusMixStage(World, DialogueControlBus, ADogSettingsLocal->GetDialogueVolume());
					const FSoundControlBusMixStage VoiceChatControlBusMixStage = UAudioModulationStatics::CreateBusMixStage(World, VoiceChatControlBus, ADogSettingsLocal->GetVoiceChatVolume());

					TArray<FSoundControlBusMixStage> ControlBusMixStageArray;
					ControlBusMixStageArray.Add(OverallControlBusMixStage);
					ControlBusMixStageArray.Add(MusicControlBusMixStage);
					ControlBusMixStageArray.Add(SoundFXControlBusMixStage);
					ControlBusMixStageArray.Add(DialogueControlBusMixStage);
					ControlBusMixStageArray.Add(VoiceChatControlBusMixStage);

					UAudioModulationStatics::UpdateMix(World, UserMix, ControlBusMixStageArray);
				}
			}

			ApplyDynamicRangeEffectsChains(ADogSettingsLocal->IsHDRAudioModeEnabled());
		}
	}
}

void UADogAudioMixEffectsSubsystem::ApplyDynamicRangeEffectsChains(bool bHDRAudio)
{
	TArray<FADogAudioSubmixEffectsChain> AudioSubmixEffectsChainToApply;
	TArray<FADogAudioSubmixEffectsChain> AudioSubmixEffectsChainToClear;

	// If HDR Audio is selected, then we clear out any existing LDR Submix Effect Chain Overrides
	// otherwise the reverse is the case.
	if (bHDRAudio)
	{
		AudioSubmixEffectsChainToApply.Append(HDRSubmixEffectChain);
		AudioSubmixEffectsChainToClear.Append(LDRSubmixEffectChain);
	}
	else
	{
		AudioSubmixEffectsChainToApply.Append(LDRSubmixEffectChain);
		AudioSubmixEffectsChainToClear.Append(HDRSubmixEffectChain);
	}

	// We want to collect just the submixes we need to actually clear, otherwise they'll be overridden by the new settings
	TArray<USoundSubmix*> SubmixesLeftToClear;

	// We want to get the submixes that are not being overridden by the new effect chains, so we can clear those out separately
	for (const FADogAudioSubmixEffectsChain& EffectChainToClear : AudioSubmixEffectsChainToClear)
	{
		bool bAddToList = true;

		for (const FADogAudioSubmixEffectsChain& SubmixEffectChain : AudioSubmixEffectsChainToApply)
		{
			if (SubmixEffectChain.Submix == EffectChainToClear.Submix)
			{
				bAddToList = false;

				break;
			}
		}

		if (bAddToList)
		{
			SubmixesLeftToClear.Add(EffectChainToClear.Submix);
		}
	}


	// Override submixes
	for (const FADogAudioSubmixEffectsChain& SubmixEffectChain : AudioSubmixEffectsChainToApply)
	{
		if (SubmixEffectChain.Submix)
		{
			UAudioMixerBlueprintLibrary::SetSubmixEffectChainOverride(GetWorld(), SubmixEffectChain.Submix, SubmixEffectChain.SubmixEffectChain, 0.1f);

		}
	}

	// Clear remaining submixes
	for (USoundSubmix* Submix : SubmixesLeftToClear)
	{
		UAudioMixerBlueprintLibrary::ClearSubmixEffectChainOverride(GetWorld(), Submix, 0.1f);
	}
}

void UADogAudioMixEffectsSubsystem::OnLoadingScreenStatusChanged(bool bShowingLoadingScreen)
{
	ApplyOrRemoveLoadingScreenMix(bShowingLoadingScreen);
}

void UADogAudioMixEffectsSubsystem::ApplyOrRemoveLoadingScreenMix(bool bWantsLoadingScreenMix)
{
	UWorld* World = GetWorld();

	if (bAppliedLoadingScreenMix != bWantsLoadingScreenMix && LoadingScreenMix && World)
	{
		if (bWantsLoadingScreenMix)
		{
			// Apply the mix
			UAudioModulationStatics::ActivateBusMix(World, LoadingScreenMix);
		}
		else
		{
			// Remove the mix
			UAudioModulationStatics::DeactivateBusMix(World, LoadingScreenMix);
		}
		bAppliedLoadingScreenMix = bWantsLoadingScreenMix;
	}
}

bool UADogAudioMixEffectsSubsystem::DoesSupportWorldType(const EWorldType::Type World) const
{
	// We only need this subsystem on Game worlds (PIE included)
	return (World == EWorldType::Game || World == EWorldType::PIE);
}

