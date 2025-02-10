// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ADogAbilitySystemGlobals.generated.h"

class UADogAbilitySystemComponent;

/**
 * Holds global data for the ability system. Can be configured per project via config file
 * Mostly copied from GameplayAbilities UAbilitySystemGlobals
 */
UCLASS(config = Game)
class ALPHADOGGAME_API UADogAbilitySystemGlobals : public UObject
{
	GENERATED_BODY()

public:
	
	/** Searches the passed in actor for an ability system component, will use IAbilitySystemInterface or fall back to a component search */
	static UADogAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent=true);
};
