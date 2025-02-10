// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/ADogGameInstance.h"

#include "CommonSessionSubsystem.h"
#include "CommonUserSubsystem.h"
#include "Components/GameFrameworkComponentManager.h"
#include "HAL/IConsoleManager.h"
#include "ADogGameplayTags.h"
#include "Player/ADogPlayerController.h"
#include "Player/ADogLocalPlayer.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogGameInstance)

UADogGameInstance::UADogGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UADogGameInstance::Init()
{
	Super::Init();

	// Register our custom init states
	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);

	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(ADogGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(ADogGameplayTags::InitState_DataAvailable, false, ADogGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(ADogGameplayTags::InitState_DataInitialized, false, ADogGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(ADogGameplayTags::InitState_GameplayReady, false, ADogGameplayTags::InitState_DataInitialized);
	}

	if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->OnPreClientTravelEvent.AddUObject(this, &UADogGameInstance::OnPreClientTravelToSession);
	}
}

void UADogGameInstance::Shutdown()
{
	if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->OnPreClientTravelEvent.RemoveAll(this);
	}

	Super::Shutdown();
}

AADogPlayerController* UADogGameInstance::GetPrimaryPlayerController() const
{
	return Cast<AADogPlayerController>(Super::GetPrimaryPlayerController(false));
}

bool UADogGameInstance::CanJoinRequestedSession() const
{
	// Temporary first pass:  Always return true
	// This will be fleshed out to check the player's state
	if (!Super::CanJoinRequestedSession())
	{
		return false;
	}
	return true;
}

void UADogGameInstance::HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	Super::HandlerUserInitialized(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);

	// If login succeeded, tell the local player to load their settings
	if (bSuccess && ensure(UserInfo))
	{
		UADogLocalPlayer* LocalPlayer = Cast<UADogLocalPlayer>(GetLocalPlayerByIndex(UserInfo->LocalPlayerIndex));

		// There will not be a local player attached to the dedicated server user
		if (LocalPlayer)
		{
			LocalPlayer->LoadSharedSettingsFromDisk();
		}
	}
}

void UADogGameInstance::OnPreClientTravelToSession(FString& URL)
{
}