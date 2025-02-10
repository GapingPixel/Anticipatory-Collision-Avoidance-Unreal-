// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/AITask.h"
#include "ADogAITask_UpdateMoveTarget.generated.h"

class AADogCharacter;
class AADogMoveTarget;

USTRUCT()
struct FUpdateMoveTargetRequest
{
	GENERATED_BODY()
	
	float TurnRate = 400.f;
	float TurnRateTimerLimit = 0.5f;
	float TurnRateThrottleDistance = 200;
	float TurnThrottleTrigger = -0.75f;
	float MoveTargetDistanceTime = 2.f;
	float MoveTargetDistanceMax = 400.f;

	bool bDrawDebug;
	
	UPROPERTY()
	TObjectPtr<AActor> TargetActor;
	
};

/**
 * Responsible for manipulating the position/rotation of the ADogMoveTarget
 */
UCLASS(MinimalAPI)
class UADogAITask_UpdateMoveTarget : public UAITask
{
	GENERATED_BODY()

public:

	/** prepare move task for activation */
	bool SetUp(AAIController* InOwnerController, APawn* InControlledPawn, const FUpdateMoveTargetRequest& InUpdateMoveTargetRequest);

	virtual void TickTask(float DeltaTime) override;

	void ThrottleTurnRate(float DeltaTime);
	
	bool bInitialized = false;
	
protected:

	UPROPERTY()
	TObjectPtr<APawn> ControlledPawn;
	UPROPERTY()
	TObjectPtr<AADogMoveTarget> MoveTarget;
	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	float DefaultTurnRate = 4.0f;

private:

	
	FVector ControlledPawnLocation;
	float TurnRate = 0.0f;
	FRotator ControlledPawnRotation;
	FRotator TargetActorRotation;
	FVector TargetActorLocation;
	float DistanceToMoveTarget = 0.0f;
	float DistanceToTargetActor = 0.0f;
	float MoveTargetDistanceSpeed = 0.0f;
	float MoveTargetDistanceMax = 250.f;

	bool bTurnRateThrottled = false;
	float TurnRateThrottleDistance = 0.0f;
	float TurnRateTimer = 0.0f;
	float TurnRateTimeLimit = 0.0f;
	float TurnThrottleTrigger = 0.0f;
	
	bool bDrawDebug = false;
	
};
