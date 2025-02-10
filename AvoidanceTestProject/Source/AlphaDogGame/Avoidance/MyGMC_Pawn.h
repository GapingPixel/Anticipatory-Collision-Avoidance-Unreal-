// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/GMCPawn.h"
#include "MyGMC_Pawn.generated.h"

class UFloatingPawnMovement;
class UAvoidanceComponent;
class UGMC_OrganicMovementCmp;
/**
 * 
 */
UCLASS()
class ALPHADOGGAME_API AMyGMC_Pawn : public AGMC_Pawn
{
	GENERATED_BODY()

public:
	AMyGMC_Pawn();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components", DisplayName="Capsule", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components", DisplayName="Skeletal Mesh", meta=(AllowPrivateAccess=true))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Components")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAvoidanceComponent> AvoidanceComponent;
	
};
