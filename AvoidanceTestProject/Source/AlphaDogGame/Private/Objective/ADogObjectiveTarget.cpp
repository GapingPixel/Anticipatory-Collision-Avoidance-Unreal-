// Fill out your copyright notice in the Description page of Project Settings.


#include "Objective/ADogObjectiveTarget.h"

#include "Components/CapsuleComponent.h"

static FName NAME_ADogObjectiveTarget_Capsule(TEXT("Interactable_BlockDynamic"));

// Sets default values
AADogObjectiveTarget::AADogObjectiveTarget()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCapsuleRadius(30.f);
	CapsuleComponent->SetCapsuleHalfHeight(30.f);
	CapsuleComponent->bEditableWhenInherited = true;
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetCollisionProfileName(NAME_ADogObjectiveTarget_Capsule);
	
}

// Called when the game starts or when spawned
void AADogObjectiveTarget::BeginPlay()
{
	Super::BeginPlay();
	
}

