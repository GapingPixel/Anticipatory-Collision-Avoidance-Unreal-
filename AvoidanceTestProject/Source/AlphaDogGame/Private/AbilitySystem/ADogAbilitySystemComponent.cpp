// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ADogAbilitySystemComponent.h"

#include "Ability/GMCAbility.h"
#include "AbilitySystem/ADogAbilitySet.h"
#include "AbilitySystem/ADogAbilityTagRelationshipMapping.h"
#include "Character/ADogCharacter.h"
#include "Character/ADogPawnData.h"
#include "Debug/ADogLogging.h"


// Sets default values for this component's properties
UADogAbilitySystemComponent::UADogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

void UADogAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UADogAbilitySystemComponent::SetTagRelationshipMapping()
{
	if (const AADogCharacter* pawn = Cast<AADogCharacter>(GetOwner()))
	{
		if (const AADogPlayerState* playerState = Cast<AADogPlayerState>(pawn->GetPlayerState()))
		{
			BEANS_ULOG(LogADogAbilitySystem, VeryVerbose, TEXT("Attempting to set TagRelationshipMapping from PawnData from PlayerState %s for character %s and player %s"), *GetNameSafe(playerState), *GetNameSafe(pawn), *pawn->GetPlayerState()->GetPlayerName());

			const UADogPawnData* pawnData = playerState->GetPawnData<UADogPawnData>();
			check(pawnData)
			if (pawnData && pawnData->TagRelationshipMapping)
			{
				TagRelationshipMapping = pawnData->TagRelationshipMapping;
				BEANS_ULOG(LogADogAbilitySystem, VeryVerbose, TEXT("TagRelationshipMapping set from PawnData from PlayerState %s for character %s and player %s"), *GetNameSafe(playerState), *GetNameSafe(pawn), *pawn->GetPlayerState()->GetPlayerName());
			}
		}
	}
}

bool UADogAbilitySystemComponent::CanQueueAbility(FGameplayTag AbilityTag)
{
	TArray<TSubclassOf<UGMCAbility>> abilities = GetGrantedAbilitiesByTag(AbilityTag);

	for (TSubclassOf<UGMCAbility> abilityClass : abilities)
	{
		UGMCAbility* abilityCDO = Cast<UGMCAbility>(abilityClass->GetDefaultObject());
		if (!IsValid(abilityCDO))
		{
			BEANS_ULOG(LogADogAbilitySystem, Warning, TEXT("Ability CDO is invalid: %s"), *GetNameSafe(abilityCDO));
			continue;
		}

		if (!CheckActivationTags(abilityCDO))
		{
			BEANS_ULOG(LogADogAbilitySystem, VeryVerbose, TEXT("Ability failed ActivationTag check: %s"), *GetNameSafe(abilityCDO));
			return false;
		}
	}
	
	return true;
}

void UADogAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags,
                                                                         FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const
{
	if (TagRelationshipMapping)
	{
		TagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
	}
}

bool UADogAbilitySystemComponent::CheckActivationTags(const UGMCAbility* Ability) const
{
	// If we don't have a TagRelationshipMapping fallback to Ability's Reqs
	if (!TagRelationshipMapping)
	{
		return Super::CheckActivationTags(Ability);
	}

	bool bBlocked = false;
	bool bMissing = false;

	/*
	// TODO support global activation tags
	UADogAbilitySystemGlobals& AbilitySystemGlobals = UADogAbilitySystemGlobals::Get();
	const FGameplayTag& BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
	const FGameplayTag& MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;
	*/

	// Check if any of this ability's tags are currently blocked
	for (const FGameplayTag Tag : Ability->ActivationBlockedTags)
	{
		if (HasActiveTag(Tag))
		{
			UE_LOG(LogADogAbilitySystem, Verbose, TEXT("Ability can't activate, blocked by tag: %s"), *Tag.ToString());
			bBlocked = true;
		}
	}
	
	static FGameplayTagContainer AllRequiredTags;
	static FGameplayTagContainer AllBlockedTags;

	// TODO Ability doesn't support multiple AbilityTag so we need to create a container
	const FGameplayTagContainer AbilityTags = FGameplayTagContainer(Ability->AbilityTag);
	
	AllRequiredTags = Ability->ActivationRequiredTags;
	AllBlockedTags = Ability->ActivationBlockedTags;

	// Expand our ability tags to add additional required/blocked tags
	GetAdditionalActivationTagRequirements(AbilityTags, AllRequiredTags, AllBlockedTags);
	
	// Check to see the required/blocked tags for this ability
	if (AllBlockedTags.Num() || AllRequiredTags.Num())
	{
		static FGameplayTagContainer AbilitySystemComponentTags;
		
		AbilitySystemComponentTags.Reset();
		AbilitySystemComponentTags.AppendTags(GetActiveTags());

		if (AbilitySystemComponentTags.HasAny(AllBlockedTags))
		{
			// TODO implement OptionalRelevantTags https://github.com/EpicGames/UnrealEngine/blob/release/Samples/Games/Lyra/Source/LyraGame/AbilitySystem/Abilities/LyraAbilityCost.h#L30
			/*if (OptionalRelevantTags && AbilitySystemComponentTags.HasTag(LyraGameplayTags::Status_Death))
			{
				// If player is dead and was rejected due to blocking tags, give that feedback
				OptionalRelevantTags->AddTag(LyraGameplayTags::Ability_ActivateFail_IsDead);
			}*/

			bBlocked = true;
		}

		if (!AbilitySystemComponentTags.HasAll(AllRequiredTags))
		{
			bMissing = true;
		}
	}

	// TODO GMC_AbilitySystemComponent doesn't support Source/Target Tags
	/*if (SourceTags != nullptr)
	{
		if (SourceBlockedTags.Num() || SourceRequiredTags.Num())
		{
			if (SourceTags->HasAny(SourceBlockedTags))
			{
				bBlocked = true;
			}

			if (!SourceTags->HasAll(SourceRequiredTags))
			{
				bMissing = true;
			}
		}
	}

	if (TargetTags != nullptr)
	{
		if (TargetBlockedTags.Num() || TargetRequiredTags.Num())
		{
			if (TargetTags->HasAny(TargetBlockedTags))
			{
				bBlocked = true;
			}

			if (!TargetTags->HasAll(TargetRequiredTags))
			{
				bMissing = true;
			}
		}
	}*/

	if (bBlocked)
	{
		// TODO implement OptionalRelevantTags
		/*if (OptionalRelevantTags && BlockedTag.IsValid())
		{
			OptionalRelevantTags->AddTag(BlockedTag);
		}*/
		return false;
	}
	if (bMissing)
	{
		// TODO implement OptionalRelevantTags
		/*if (OptionalRelevantTags && MissingTag.IsValid())
		{
			OptionalRelevantTags->AddTag(MissingTag);
		}*/
		return false;
	}

	return true;
	
}

