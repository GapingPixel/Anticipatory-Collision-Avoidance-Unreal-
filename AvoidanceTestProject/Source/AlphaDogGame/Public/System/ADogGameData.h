// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"

#include "ADogGameData.generated.h"

//class UGameplayEffect;
class UObject;

/**
 * ADog defines default GameplayEffects for damage, healing, and one for adding/removing dynamic gameplay tags
 * For now, this will be empty but could be a good spot for global effects
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "AlphaDog Game Data", ShortTooltip = "Data asset containing global game data."))
class ALPHADOGGAME_API UADogGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UADogGameData();

	// Returns the loaded game data.
	static const UADogGameData& Get();

public:

	/*
	// Gameplay effect used to apply damage.  Uses SetByCaller for the damage magnitude.
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Damage Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> DamageGameplayEffect_SetByCaller;

	// Gameplay effect used to apply healing.  Uses SetByCaller for the healing magnitude.
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Heal Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> HealGameplayEffect_SetByCaller;

	// Gameplay effect used to add and remove dynamic tags.
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> DynamicTagGameplayEffect;
	*/
};
