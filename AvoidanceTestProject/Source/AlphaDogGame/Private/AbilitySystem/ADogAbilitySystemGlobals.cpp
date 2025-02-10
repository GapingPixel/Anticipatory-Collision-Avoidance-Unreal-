// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ADogAbilitySystemGlobals.h"

#include "AbilitySystem/ADogAbilitySystemComponent.h"
#include "AbilitySystem/ADogAbilitySystemInterface.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogAbilitySystemGlobals)

UADogAbilitySystemComponent* UADogAbilitySystemGlobals::GetAbilitySystemComponentFromActor(const AActor* Actor,
                                                                                            bool LookForComponent)
{
	if (Actor == nullptr)
	{
		return nullptr;
	}

	const IADogAbilitySystemInterface* ASI = Cast<IADogAbilitySystemInterface>(Actor);
	if (ASI)
	{
		return ASI->GetAbilitySystemComponent();
	}

	if (LookForComponent)
	{
		// Fall back to a component search to better support BP-only actors
		return Actor->FindComponentByClass<UADogAbilitySystemComponent>();
	}

	return nullptr;
}
