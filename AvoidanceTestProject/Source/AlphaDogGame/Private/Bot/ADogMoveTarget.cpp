// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/ADogMoveTarget.h"

#include "AIController.h"


// Sets default values
AADogMoveTarget::AADogMoveTarget()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bNetLoadOnClient = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot"));
	
	SetActorEnableCollision(false);

	
}

bool AADogMoveTarget::SetupOnOwnerPawn(const float StartingMoveTargetDistance)
{
	const AAIController* ownerController = Cast<AAIController>(GetOwner());
	if (!ownerController && !ownerController->GetPawn())
	{
		return false;
	}
	
	return SetActorTransform(GetStartingTransform(ownerController->GetPawn(), StartingMoveTargetDistance));
}

FTransform AADogMoveTarget::GetStartingTransform(const APawn* OwningPawn, const float StartingMoveTargetDistance)
{
	if (!IsValid(OwningPawn))
	{
		return FTransform();
	}
	FTransform moveTargetTransform = OwningPawn->GetActorTransform();
	moveTargetTransform.SetLocation((moveTargetTransform.GetRotation().GetForwardVector() * StartingMoveTargetDistance) + moveTargetTransform.GetLocation());
	return moveTargetTransform;
}

// Called when the game starts or when spawned
void AADogMoveTarget::BeginPlay()
{
	Super::BeginPlay();
}
