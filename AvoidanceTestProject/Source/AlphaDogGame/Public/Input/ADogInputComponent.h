// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnhancedInputComponent.h"
#include "ADogInputConfig.h"
#include "Character/ADogHeroComponent.h"

#include "ADogInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UObject;


/**
 * UADogInputComponent
 *
 *	Component used to manage input mappings and bindings using an input config data asset.
 */
UCLASS(Config = Input)
class UADogInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:

	UADogInputComponent(const FObjectInitializer& ObjectInitializer);

	void AddInputMappings(const UADogInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	void RemoveInputMappings(const UADogInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

	template<class UserClass, typename FuncType>
	void BindNativeAction(const UADogInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UADogInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	void RemoveBinds(TArray<uint32>& BindHandles);
};


template<class UserClass, typename FuncType>
void UADogInputComponent::BindNativeAction(const UADogInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	check(InputConfig);
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UADogInputComponent::BindAbilityActions(const UADogInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);

	for (const FADogInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction<UADogHeroComponent, FGameplayTag, const UInputAction*>(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag, Action.InputAction).GetHandle());
			}
			/*if (ReleasedFunc)
			{
				BindHandles.Add(BindAction<UADogHeroComponent, FGameplayTag, const UInputAction*>(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag, Action.InputAction).GetHandle());
			}*/
		}
	}
}
