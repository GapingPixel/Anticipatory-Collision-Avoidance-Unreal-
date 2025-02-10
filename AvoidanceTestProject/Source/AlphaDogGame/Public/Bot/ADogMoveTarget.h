// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ADogMoveTarget.generated.h"

/*
 * Used as the actor which an AI Dog should move towards
 * This actor shouldn't do anything and is merely used to provide a reference to a world transform for movement purposes.
 * This is usually spawned by the AIController
 */

UCLASS()
class ALPHADOGGAME_API AADogMoveTarget : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AADogMoveTarget();

	virtual bool SetupOnOwnerPawn(const float StartingMoveTargetDistance);
	static FTransform GetStartingTransform(const APawn* OwningPawn, const float StartingMoveTargetDistance);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
};
