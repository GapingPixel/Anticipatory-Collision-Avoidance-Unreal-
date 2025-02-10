// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PushPrioritySystem/PushPriorityComponent.h"

#include "Character/ADogCharacter.h"
#include "Character/ADogMovementComponent.h"
#include "Character/PushPrioritySystem/PushPrioritySubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Navigation/CrowdFollowingComponent.h"


// Sets default values for this component's properties
UPushPriorityComponent::UPushPriorityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

}

void UPushPriorityComponent::HandlePush(AActor* OtherActor)
{
	UPushPriorityComponent* otherComponent = OtherActor->GetComponentByClass<UPushPriorityComponent>();
	if (otherComponent)
	{
		UPushPriorityComponent* loser = PushPrioritySubsystem->ResolvePush(this, otherComponent);
		UPushPriorityComponent* winner = otherComponent == loser ? this : otherComponent;

		PerformPush(winner, loser);
	}
}


// Called when the game starts
void UPushPriorityComponent::BeginPlay()
{
	Super::BeginPlay();

	const UGameInstance* gameInstance = GetWorld()->GetGameInstance();
	check(IsValid(gameInstance));
	PushPrioritySubsystem = gameInstance->GetSubsystem<UPushPrioritySubsystem>();
	check(IsValid(PushPrioritySubsystem));

	RegisterWithSubsystem();
}

void UPushPriorityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterWithSubsystem();
	PushPrioritySubsystem = nullptr;
}

void UPushPriorityComponent::RegisterWithSubsystem()
{
	// TODO validate that owner has movement component
	
	if (IsValid(PushPrioritySubsystem))
	{
		PushPrioritySubsystem->RegisterComponent(this);
	}
}

void UPushPriorityComponent::UnregisterWithSubsystem()
{
	if (IsValid(PushPrioritySubsystem))
	{
		PushPrioritySubsystem->UnregisterComponent(this);
	}
}

void UPushPriorityComponent::PerformPush(UPushPriorityComponent* Winner, UPushPriorityComponent* Loser)
{
	const FRotator lookAt = UKismetMathLibrary::FindLookAtRotation(Winner->GetOwner()->GetActorLocation(), Loser->GetOwner()->GetActorLocation());
	const FVector impulse = UKismetMathLibrary::GetForwardVector(lookAt) * PushMagnitude;

	if (bDrawDebug)
	{
		DrawDebugDirectionalArrow(GetWorld(), Winner->GetOwner()->GetActorLocation(), Loser->GetOwner()->GetActorLocation(), 3.f, FColor::Orange, false, 1.f);
	}
		
	Cast<AADogCharacter>(Loser->GetOwner())->GetADogMovementComponent()->AddImpulse(impulse, true);
}


// Called every frame
void UPushPriorityComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

