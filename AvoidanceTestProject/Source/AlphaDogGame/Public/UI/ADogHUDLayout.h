// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ADogActivatableWidget.h"

#include "ADogHUDLayout.generated.h"

class UCommonActivatableWidget;
class UObject;


/**
 * UADogHUDLayout
 *
 *	Widget used to lay out the player's HUD (typically specified by an Add Widgets action in the experience)
 */
UCLASS(Abstract, BlueprintType, Blueprintable, Meta = (DisplayName = "AlphaDog HUD Layout", Category = "AlphaDog|HUD"))
class UADogHUDLayout : public UADogActivatableWidget
{
	GENERATED_BODY()

public:

	UADogHUDLayout(const FObjectInitializer& ObjectInitializer);

	void NativeOnInitialized() override;

protected:
	void HandleEscapeAction();

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;
};
