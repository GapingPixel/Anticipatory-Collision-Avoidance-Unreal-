// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ADogCheatManager.h"

#include "GameFramework/Pawn.h"
#include "Player/ADogPlayerController.h"
#include "Player/ADogDebugCameraController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Console.h"
#include "GameFramework/HUD.h"
#include "System/ADogAssetManager.h"
#include "System/ADogGameData.h"
#include "ADogGameplayTags.h"
#include "AbilitySystem/ADogAbilitySystemComponent.h"
//#include "AbilitySystemGlobals.h"
//#include "Character/ADogHealthComponent.h"
#include "AbilitySystem/ADogAbilitySystemInterface.h"
#include "Character/ADogPawnExtensionComponent.h"
#include "System/ADogSystemStatics.h"
#include "Development/ADogDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogCheatManager)

DEFINE_LOG_CATEGORY(LogADogCheat);

namespace ADogCheat
{
	static const FName NAME_Fixed = FName(TEXT("Fixed"));
	
	static bool bEnableDebugCameraCycling = false;
	static FAutoConsoleVariableRef CVarEnableDebugCameraCycling(
		TEXT("AlphaDogCheat.EnableDebugCameraCycling"),
		bEnableDebugCameraCycling,
		TEXT("If true then you can cycle the debug camera while running the game."),
		ECVF_Cheat);

	static bool bStartInGodMode = false;
	static FAutoConsoleVariableRef CVarStartInGodMode(
		TEXT("AlphaDogCheat.StartInGodMode"),
		bStartInGodMode,
		TEXT("If true then the God cheat will be applied on begin play"),
		ECVF_Cheat);
};


UADogCheatManager::UADogCheatManager()
{
	DebugCameraControllerClass = AADogDebugCameraController::StaticClass();
}

void UADogCheatManager::InitCheatManager()
{
	Super::InitCheatManager();

#if WITH_EDITOR
	if (GIsEditor)
	{
		APlayerController* PC = GetOuterAPlayerController();
		for (const FADogCheatToRun& CheatRow : GetDefault<UADogDeveloperSettings>()->CheatsToRun)
		{
			if (CheatRow.Phase == ECheatExecutionTime::OnCheatManagerCreated)
			{
				PC->ConsoleCommand(CheatRow.Cheat, /*bWriteToLog=*/ true);
			}
		}
	}
#endif

	if (ADogCheat::bStartInGodMode)
	{
		God();	
	}
}

void UADogCheatManager::CheatOutputText(const FString& TextToOutput)
{
#if USING_CHEAT_MANAGER
	// Output to the console.
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->ViewportConsole)
	{
		GEngine->GameViewport->ViewportConsole->OutputText(TextToOutput);
	}

	// Output to log.
	UE_LOG(LogADogCheat, Display, TEXT("%s"), *TextToOutput);
#endif // USING_CHEAT_MANAGER
}

void UADogCheatManager::Cheat(const FString& Msg)
{
	if (AADogPlayerController* ADogPC = Cast<AADogPlayerController>(GetOuterAPlayerController()))
	{
		ADogPC->ServerCheat(Msg.Left(128));
	}
}

void UADogCheatManager::CheatAll(const FString& Msg)
{
	if (AADogPlayerController* ADogPC = Cast<AADogPlayerController>(GetOuterAPlayerController()))
	{
		ADogPC->ServerCheatAll(Msg.Left(128));
	}
}

void UADogCheatManager::PlayNextGame()
{
	UADogSystemStatics::PlayNextGame(this);
}

void UADogCheatManager::EnableDebugCamera()
{
	Super::EnableDebugCamera();
}

void UADogCheatManager::DisableDebugCamera()
{
	FVector DebugCameraLocation;
	FRotator DebugCameraRotation;

	ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	APlayerController* OriginalPC = nullptr;

	if (DebugCC)
	{
		OriginalPC = DebugCC->OriginalControllerRef;
		DebugCC->GetPlayerViewPoint(DebugCameraLocation, DebugCameraRotation);
	}

	Super::DisableDebugCamera();

	if (OriginalPC && OriginalPC->PlayerCameraManager && (OriginalPC->PlayerCameraManager->CameraStyle == ADogCheat::NAME_Fixed))
	{
		OriginalPC->SetInitialLocationAndRotation(DebugCameraLocation, DebugCameraRotation);

		OriginalPC->PlayerCameraManager->ViewTarget.POV.Location = DebugCameraLocation;
		OriginalPC->PlayerCameraManager->ViewTarget.POV.Rotation = DebugCameraRotation;
		OriginalPC->PlayerCameraManager->PendingViewTarget.POV.Location = DebugCameraLocation;
		OriginalPC->PlayerCameraManager->PendingViewTarget.POV.Rotation = DebugCameraRotation;
	}
}

bool UADogCheatManager::InDebugCamera() const
{
	return (Cast<ADebugCameraController>(GetOuter()) ? true : false);
}

void UADogCheatManager::EnableFixedCamera()
{
	const ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	APlayerController* PC = (DebugCC ? ToRawPtr(DebugCC->OriginalControllerRef) : GetOuterAPlayerController());

	if (PC && PC->PlayerCameraManager)
	{
		PC->SetCameraMode(ADogCheat::NAME_Fixed);
	}
}

void UADogCheatManager::DisableFixedCamera()
{
	const ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	APlayerController* PC = (DebugCC ? ToRawPtr(DebugCC->OriginalControllerRef) : GetOuterAPlayerController());

	if (PC && PC->PlayerCameraManager)
	{
		PC->SetCameraMode(NAME_Default);
	}
}

bool UADogCheatManager::InFixedCamera() const
{
	const ADebugCameraController* DebugCC = Cast<ADebugCameraController>(GetOuter());
	const APlayerController* PC = (DebugCC ? ToRawPtr(DebugCC->OriginalControllerRef) : GetOuterAPlayerController());

	if (PC && PC->PlayerCameraManager)
	{
		return (PC->PlayerCameraManager->CameraStyle == ADogCheat::NAME_Fixed);
	}

	return false;
}

void UADogCheatManager::ToggleFixedCamera()
{
	if (InFixedCamera())
	{
		DisableFixedCamera();
	}
	else
	{
		EnableFixedCamera();
	}
}

void UADogCheatManager::CycleDebugCameras()
{
	if (!ADogCheat::bEnableDebugCameraCycling)
	{
		return;
	}
	
	if (InDebugCamera())
	{
		EnableFixedCamera();
		DisableDebugCamera();
	}
	else if (InFixedCamera())
	{
		DisableFixedCamera();
		DisableDebugCamera();
	}
	else
	{
		EnableDebugCamera();
		DisableFixedCamera();
	}
}

void UADogCheatManager::CycleAbilitySystemDebug()
{
	APlayerController* PC = Cast<APlayerController>(GetOuterAPlayerController());

	if (PC && PC->MyHUD)
	{
		if (!PC->MyHUD->bShowDebugInfo || !PC->MyHUD->DebugDisplay.Contains(TEXT("AbilitySystem")))
		{
			PC->MyHUD->ShowDebug(TEXT("AbilitySystem"));
		}

		PC->ConsoleCommand(TEXT("AbilitySystem.Debug.NextCategory"));
	}
}

void UADogCheatManager::CancelActivatedAbilities()
{
	if (UADogAbilitySystemComponent* ADogASC = GetPlayerAbilitySystemComponent())
	{
		const bool bReplicateCancelAbility = true;
		//ADogASC->CancelInputActivatedAbilities(bReplicateCancelAbility);
	}
}

void UADogCheatManager::AddTagToSelf(FString TagName)
{
	FGameplayTag Tag = ADogGameplayTags::FindTagByString(TagName, true);
	if (Tag.IsValid())
	{
		if (UADogAbilitySystemComponent* ADogASC = GetPlayerAbilitySystemComponent())
		{
			//ADogASC->AddDynamicTagGameplayEffect(Tag);
		}
	}
	else
	{
		UE_LOG(LogADogCheat, Display, TEXT("AddTagToSelf: Could not find any tag matching [%s]."), *TagName);
	}
}

void UADogCheatManager::RemoveTagFromSelf(FString TagName)
{
	FGameplayTag Tag = ADogGameplayTags::FindTagByString(TagName, true);
	if (Tag.IsValid())
	{
		if (UADogAbilitySystemComponent* ADogASC = GetPlayerAbilitySystemComponent())
		{
			//ADogASC->RemoveDynamicTagGameplayEffect(Tag);
		}
	}
	else
	{
		UE_LOG(LogADogCheat, Display, TEXT("RemoveTagFromSelf: Could not find any tag matching [%s]."), *TagName);
	}
}

void UADogCheatManager::DepleteHydrationFromSelf(float HydrationAmount)
{
	if (UADogAbilitySystemComponent* ADogASC = GetPlayerAbilitySystemComponent())
	{
		//ApplySetByCallerDamage(ADogASC, DamageAmount);
	}
}

void UADogCheatManager::DepleteHydrationFromTarget(float HydrationAmount)
{
	if (AADogPlayerController* ADogPC = Cast<AADogPlayerController>(GetOuterAPlayerController()))
	{
		if (ADogPC->GetNetMode() == NM_Client)
		{
			// Automatically send cheat to server for convenience.
			ADogPC->ServerCheat(FString::Printf(TEXT("HydrationTarget %.2f"), HydrationAmount));
			return;
		}

		FHitResult TargetHitResult;
		AActor* TargetActor = GetTarget(ADogPC, TargetHitResult);

		if (TargetActor->Implements<UADogAbilitySystemInterface>())
		{
			UADogAbilitySystemComponent* ASC = Cast<IADogAbilitySystemInterface>(TargetActor)->GetAbilitySystemComponent();
			// deplete hydration on ASC
		}
	}
}

void UADogCheatManager::AddHydrationToSelf(float Amount)
{
	if (UADogAbilitySystemComponent* ADogASC = GetPlayerAbilitySystemComponent())
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Attribute.Hydration");
		ADogASC->GetAttributeByTag(Tag)->ApplyModifier(FGMCAttributeModifier{FGameplayTag::EmptyTag, Amount, EModifierType::Add}, false);
	}
}

void UADogCheatManager::AddHydrationToTarget(float HealAmount)
{
	if (AADogPlayerController* ADogPC = Cast<AADogPlayerController>(GetOuterAPlayerController()))
	{
		FHitResult TargetHitResult;
		AActor* TargetActor = GetTarget(ADogPC, TargetHitResult);

		if (TargetActor->Implements<UADogAbilitySystemInterface>())
		{
			UADogAbilitySystemComponent* ASC = Cast<IADogAbilitySystemInterface>(TargetActor)->GetAbilitySystemComponent();
			// add hydration on ASC
		}
	}
}

UADogAbilitySystemComponent* UADogCheatManager::GetPlayerAbilitySystemComponent() const
{
	if (AADogPlayerController* ADogPC = Cast<AADogPlayerController>(GetOuterAPlayerController()))
	{
		return ADogPC->GetADogAbilitySystemComponent();
	}
	return nullptr;
}



void UADogCheatManager::God()
{
	if (AADogPlayerController* ADogPC = Cast<AADogPlayerController>(GetOuterAPlayerController()))
	{
		if (ADogPC->GetNetMode() == NM_Client)
		{
			// Automatically send cheat to server for convenience.
			ADogPC->ServerCheat(FString::Printf(TEXT("God")));
			return;
		}

		if (UADogAbilitySystemComponent* ADogASC = ADogPC->GetADogAbilitySystemComponent())
		{
			const FGameplayTag Tag = ADogGameplayTags::Cheat_GodMode;

			/*
			const bool bHasTag = ADogASC->HasMatchingGameplayTag(Tag);

			if (bHasTag)
			{
				ADogASC->RemoveDynamicTagGameplayEffect(Tag);
			}
			else
			{
				ADogASC->AddDynamicTagGameplayEffect(Tag);
			}
			*/
		}
	}
}

void UADogCheatManager::UnlimitedHydration(int32 Enabled)
{
	if (UADogAbilitySystemComponent* ADogASC = GetPlayerAbilitySystemComponent())
	{
		const FGameplayTag Tag = ADogGameplayTags::Cheat_UnlimitedHydration;

		const bool bHasTag = ADogASC->HasActiveTag(Tag);

		if ((Enabled == -1) || ((Enabled > 0) && !bHasTag) || ((Enabled == 0) && bHasTag))
		{
			if (bHasTag)
			{
				ADogASC->RemoveActiveTag(Tag);
			}
			else
			{
				ADogASC->AddActiveTag(Tag);
				AddHydrationToSelf(100);
			}
		}
	}
}

