 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GMCAbilityComponent.h"
#include "GMCOrganicMovementComponent.h"
#include "Character/ADogPawnData.h"
#include "ADogAbilitySystemComponent.generated.h"

class UADogAbilityTagRelationshipMapping;

/*
 * Adds a bool to a GameplayTag for toggling
 */
USTRUCT(BlueprintType)
struct FADogTagBool
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	bool Value = false;
};


UCLASS()
class ALPHADOGGAME_API UADogAbilitySystemComponent : public UGMC_AbilitySystemComponent
{
	GENERATED_BODY()

public:
	
	UADogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Returns the camera component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "AlphaDog|AbilitySystem")
	static UADogAbilitySystemComponent* FindAbilitySystemComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UADogAbilitySystemComponent>() : nullptr); }
	
	/** Looks at ability tags and gathers additional required and blocking tags */
	void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTag, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const;

	//~UGMC_AbilitySystemComponent interface
	virtual bool CheckActivationTags(const UGMCAbility* Ability) const override;
	//~End of UGMC_AbilitySystemComponent interface

	/** Sets the current tag relationship mapping, if null it will clear it out */
	void SetTagRelationshipMapping();

	// Used to determine if input should even queue the ability. This is useful to prevent WaitForKeyRelease tasks
	// from gobbling up inputs for other abilities. This will return false if ANY abilities that match the AbilityTag
	// should be blocked.
	bool CanQueueAbility(FGameplayTag AbilityTag);
	
protected:
	virtual void BeginPlay() override;

protected:
	
	// If set, this table is used to look up tag relationships for activate and cancel
	UPROPERTY()
	TObjectPtr<UADogAbilityTagRelationshipMapping> TagRelationshipMapping;
	
};
