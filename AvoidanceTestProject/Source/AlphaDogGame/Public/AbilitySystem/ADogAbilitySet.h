// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ADogAbilitySet.generated.h"

class UGMCAbilityMapData;
class UADogAbilitySystemComponent;
class UGMCAbilityEffect;
class UGMCAttributesData;


/**
 * FADogAbilitySet_GameplayAbility
 *
 *	Data used by the ability set to grant gameplay abilities.
 */
USTRUCT(BlueprintType)
struct FADogAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:

	// AbilityMap to grant.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UGMCAbilityMapData> AbilityMap = nullptr;

	// Tags to specify starting abilities
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Starting Abilities"))
	FGameplayTagContainer StartingAbilities;
};


/**
 * FADogAbilitySet_GameplayEffect
 *
 *	Data used by the ability set to grant gameplay effects.
 */
USTRUCT(BlueprintType)
struct FADogAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:

	// Gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGMCAbilityEffect> GameplayEffect = nullptr;
	
};

/**
 * UADogAbilitySet
 *
 *	Non-mutable data asset used to grant gameplay abilities and gameplay effects. Attributes are required pre-net
 *	serialization on the pawn so they can't be loaded from GameplayFeatures.
 */
UCLASS()
class ALPHADOGGAME_API UADogAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UADogAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Grants the AbilitySet to a specified ability system component.
	void GiveToAbilitySystem(UADogAbilitySystemComponent* ADogASC) const;

	// Grants the AbilitySet to a specified ability system component.
	void RemoveFromAbilitySystem(UADogAbilitySystemComponent* ADogASC) const;
	
	// Gameplay abilities to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FADogAbilitySet_GameplayAbility> GrantedGameplayAbilityMaps;

	// Gameplay effects to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FADogAbilitySet_GameplayEffect> GrantedGameplayEffects;
	
};
