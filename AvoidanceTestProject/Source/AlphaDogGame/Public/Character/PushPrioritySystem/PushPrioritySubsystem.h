// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PushPriorityComponent.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PushPrioritySubsystem.generated.h"

class UPushPriorityComponent;
/**
 * 
 */
UCLASS(meta=(DisplayName = "PushPrioritySubsystem"))
class ALPHADOGGAME_API UPushPrioritySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// Begin Subsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End Subsystem

	virtual void RegisterComponent(UPushPriorityComponent* ComponentToRegister);
	virtual void UnregisterComponent(UPushPriorityComponent* ComponentToUnregister);

	// Returns the PushPriorityComponent with the lower priority - the one that should be pushed
	virtual UPushPriorityComponent* ResolvePush(UPushPriorityComponent* FirstComponent, UPushPriorityComponent* SecondComponent);

	// Adds appropriate actors to ignore list
	virtual void ResolveIgnoreForDetour(UPushPriorityComponent* InComponent);

private:

	UPROPERTY()
	TArray<UPushPriorityComponent*> RegisteredComponents;

	

	// Used when assigning priority automatically
	int PriorityCounter = 0;
};
