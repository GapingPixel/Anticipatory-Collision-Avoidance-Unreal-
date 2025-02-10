// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PushPriorityComponent.generated.h"


class UPushPrioritySubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALPHADOGGAME_API UPushPriorityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPushPriorityComponent();

	UFUNCTION(BlueprintCallable, Category= "PushPriorityComponent")
	virtual void HandlePush(AActor* OtherActor);

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RegisterWithSubsystem();
	void UnregisterWithSubsystem();

	virtual void PerformPush(UPushPriorityComponent* Winner, UPushPriorityComponent* Loser);

	

public:

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	bool bDrawDebug = true;
	
	int BasePushPriority = -1;

private:

	UPROPERTY()
	TObjectPtr<UPushPrioritySubsystem> PushPrioritySubsystem;

	const int PushMagnitude = 800;
};
