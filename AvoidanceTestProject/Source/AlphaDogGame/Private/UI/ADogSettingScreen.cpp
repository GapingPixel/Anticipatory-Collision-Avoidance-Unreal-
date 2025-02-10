// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ADogSettingScreen.h"

#include "Input/CommonUIInputTypes.h"
#include "Player/ADogLocalPlayer.h"
#include "Settings/ADogGameSettingRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogSettingScreen)

class UGameSettingRegistry;

void UADogSettingScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BackHandle = RegisterUIActionBinding(FBindUIActionArgs(BackInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleBackAction)));
	ApplyHandle = RegisterUIActionBinding(FBindUIActionArgs(ApplyInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleApplyAction)));
	CancelChangesHandle = RegisterUIActionBinding(FBindUIActionArgs(CancelChangesInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleCancelChangesAction)));
}

UGameSettingRegistry* UADogSettingScreen::CreateRegistry()
{
	UADogGameSettingRegistry* NewRegistry = NewObject<UADogGameSettingRegistry>();

	if (UADogLocalPlayer* LocalPlayer = CastChecked<UADogLocalPlayer>(GetOwningLocalPlayer()))
	{
		NewRegistry->Initialize(LocalPlayer);
	}

	return NewRegistry;
}

void UADogSettingScreen::HandleBackAction()
{
	if (AttemptToPopNavigation())
	{
		return;
	}

	ApplyChanges();

	DeactivateWidget();
}

void UADogSettingScreen::HandleApplyAction()
{
	ApplyChanges();
}

void UADogSettingScreen::HandleCancelChangesAction()
{
	CancelChanges();
}

void UADogSettingScreen::OnSettingsDirtyStateChanged_Implementation(bool bSettingsDirty)
{
	if (bSettingsDirty)
	{
		if (!GetActionBindings().Contains(ApplyHandle))
		{
			AddActionBinding(ApplyHandle);
		}
		if (!GetActionBindings().Contains(CancelChangesHandle))
		{
			AddActionBinding(CancelChangesHandle);
		}
	}
	else
	{
		RemoveActionBinding(ApplyHandle);
		RemoveActionBinding(CancelChangesHandle);
	}
}
