// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UObject/Interface.h"
#include "ADogAbilitySystemInterface.generated.h"

class UADogAbilitySystemComponent;

// Interface for actors that expose access to an ability system component
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UADogAbilitySystemInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ALPHADOGGAME_API IADogAbilitySystemInterface
{
	GENERATED_IINTERFACE_BODY()
	
	/** Returns the ability system component to use for this actor. It may live on another actor, such as a Pawn using the PlayerState's component */
	virtual UADogAbilitySystemComponent* GetAbilitySystemComponent() const = 0;
	
};
