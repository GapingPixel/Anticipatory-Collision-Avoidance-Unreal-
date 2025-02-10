// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Foundation/ADogControllerDisconnectedScreen.h"

#include "Components/HorizontalBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogControllerDisconnectedScreen)

void UADogControllerDisconnectedScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	HBox_SwitchUser->SetVisibility(ESlateVisibility::Collapsed);

//#if PLATFORM_HAS_STRICT_CONTROLLER_PAIRING
//	if (FPlatformApplicationMisc::GetControllerIdForUnpairedControllers() != INDEX_NONE)
//	{
//		HBox_SwitchUser->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
//	}
//#endif
}


