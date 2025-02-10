// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ADogAbilitySet.h"
#include "Debug/ADogLogging.h"
#include "AbilitySystem/ADogAbilitySystemComponent.h"

UADogAbilitySet::UADogAbilitySet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UADogAbilitySet::GiveToAbilitySystem(UADogAbilitySystemComponent* ADogASC) const
{
	check(ADogASC);

	// Grant the gameplay abilities.
	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilityMaps.Num(); ++AbilityIndex)
	{
		const FADogAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilityMaps[AbilityIndex];

		if (!IsValid(AbilityToGrant.AbilityMap))
		{
			BEANS_ULOG(LogADogAbilitySystem, Error, TEXT("GrantedGameplayAbilityMaps[%d] on ability set [%s] is not valid."), AbilityIndex, *GetNameSafe(this));
			continue;
		}

		ADogASC->AddAbilityMapData(AbilityToGrant.AbilityMap);
	}

	// Grant the gameplay effects.
	TArray<TSubclassOf<UGMCAbilityEffect>> startingEffects;
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FADogAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogADogAbilitySystem, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid"), EffectIndex, *GetNameSafe(this));
			continue;
		}

		startingEffects.AddUnique(EffectToGrant.GameplayEffect);
	}
	if (startingEffects.Num() > 0)
	{
		ADogASC->AddStartingEffects(startingEffects);
	}
	
}

void UADogAbilitySet::RemoveFromAbilitySystem(UADogAbilitySystemComponent* ADogASC) const
{
	check(ADogASC);

	// Remove the gameplay abilities.
	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilityMaps.Num(); ++AbilityIndex)
	{
		const FADogAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilityMaps[AbilityIndex];

		if (!IsValid(AbilityToGrant.AbilityMap))
		{
			BEANS_ULOG(LogADogAbilitySystem, Error, TEXT("GrantedGameplayAbilityMaps[%d] on ability set [%s] is not valid."), AbilityIndex, *GetNameSafe(this));
			continue;
		}

		ADogASC->RemoveAbilityMapData(AbilityToGrant.AbilityMap);
	}

	// Grant the gameplay effects.
	TArray<TSubclassOf<UGMCAbilityEffect>> startingEffects;
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FADogAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogADogAbilitySystem, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid"), EffectIndex, *GetNameSafe(this));
			continue;
		}

		startingEffects.Remove(EffectToGrant.GameplayEffect);
	}
	if (startingEffects.Num() > 0)
	{
		ADogASC->RemoveStartingEffects(startingEffects);
	}
}


