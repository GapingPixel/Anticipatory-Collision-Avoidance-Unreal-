// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADogObjectiveTarget.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;

UCLASS(Abstract)
class ALPHADOGGAME_API AADogObjectiveTarget : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AADogObjectiveTarget();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components", DisplayName="Capsule", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
};
