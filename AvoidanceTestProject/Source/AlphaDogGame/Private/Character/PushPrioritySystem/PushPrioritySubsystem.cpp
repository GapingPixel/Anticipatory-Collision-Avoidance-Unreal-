// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PushPrioritySystem/PushPrioritySubsystem.h"

#include "Character/PushPrioritySystem/PushPriorityComponent.h"

void UPushPrioritySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPushPrioritySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPushPrioritySubsystem::RegisterComponent(UPushPriorityComponent* ComponentToRegister)
{
	RegisteredComponents.AddUnique(ComponentToRegister);
	ComponentToRegister->BasePushPriority = PriorityCounter;
	PriorityCounter++;
}

void UPushPrioritySubsystem::UnregisterComponent(UPushPriorityComponent* ComponentToUnregister)
{
	if (RegisteredComponents.Contains(ComponentToUnregister))
	{
		RegisteredComponents.Remove(ComponentToUnregister);
	}
}

UPushPriorityComponent* UPushPrioritySubsystem::ResolvePush(UPushPriorityComponent* FirstComponent,
	UPushPriorityComponent* SecondComponent)
{
	UPushPriorityComponent* loser = FirstComponent->BasePushPriority > SecondComponent->BasePushPriority ? SecondComponent : FirstComponent;
	return loser;
}

void UPushPrioritySubsystem::ResolveIgnoreForDetour(UPushPriorityComponent* InComponent)
{
	
}
