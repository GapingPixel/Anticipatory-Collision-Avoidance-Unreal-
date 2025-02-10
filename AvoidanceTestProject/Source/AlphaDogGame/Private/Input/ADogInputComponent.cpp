// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ADogInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "Player/ADogLocalPlayer.h"
#include "Settings/ADogSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogInputComponent)

class UADogInputConfig;

UADogInputComponent::UADogInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UADogInputComponent::AddInputMappings(const UADogInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to add something from your input config if required
}

void UADogInputComponent::RemoveInputMappings(const UADogInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to remove input mappings that you may have added above
}

void UADogInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
