// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "ADogPawnData.generated.h"

class APawn;
class UADogAbilitySet;
class UADogAbilityTagRelationshipMapping;
class UADogCameraMode;
class UADogInputConfig;
class UGMCAbilityMapData;
class UObject;

/**
 * UADogPawnData
 *
 *	Non-mutable data asset that contains properties used to define a pawn.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Alpha Dog Pawn Data", ShortTooltip = "Data asset used to define a Pawn."))
class ALPHADOGGAME_API UADogPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UADogPawnData(const FObjectInitializer& ObjectInitializer);

public:

	// Class to instantiate for this pawn (should usually derive from AADogCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaDog|Pawn")
	TSubclassOf<APawn> PawnClass;

	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaDog|Abilities")
	TArray<TObjectPtr<UADogAbilitySet>> AbilitySets;
	
	// What mapping of ability tags to use for actions taking by this pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaDog|Abilities")
	TObjectPtr<UADogAbilityTagRelationshipMapping> TagRelationshipMapping; 
	
	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	// Native actions should be bound here since we won't automatically bind native actions from GameFeature Actions!
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaDog|Input")
	TObjectPtr<UADogInputConfig> InputConfig;
	
	// Default camera mode used by player controlled pawns.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AlphaDog|Camera")
	TSubclassOf<UADogCameraMode> DefaultCameraMode; 
};
