// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/AITasks/ADogAITask_UpdateMoveTarget.h"

#include "Bot/ADogMoveTarget.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/ADogPlayerBotController.h"

bool UADogAITask_UpdateMoveTarget::SetUp(AAIController* InOwnerController, APawn* InControlledPawn,
	const FUpdateMoveTargetRequest& InUpdateMoveTargetRequest)
{
	bTickingTask = 1;
	
	MoveTarget = Cast<AADogPlayerBotController>(InOwnerController)->GetMoveTarget();
	ControlledPawn = InControlledPawn;
	ControlledPawnRotation = ControlledPawn->GetActorRotation();
	TargetActor = InUpdateMoveTargetRequest.TargetActor;
	DefaultTurnRate = InUpdateMoveTargetRequest.TurnRate;
	TurnRate = DefaultTurnRate;
	TurnThrottleTrigger = InUpdateMoveTargetRequest.TurnThrottleTrigger;
	TurnRateTimeLimit = InUpdateMoveTargetRequest.TurnRateTimerLimit;
	TurnRateThrottleDistance = InUpdateMoveTargetRequest.TurnRateThrottleDistance;
	MoveTargetDistanceSpeed = InUpdateMoveTargetRequest.MoveTargetDistanceTime;
	MoveTargetDistanceMax = InUpdateMoveTargetRequest.MoveTargetDistanceMax;
	DistanceToMoveTarget = MoveTargetDistanceMax;
	bInitialized = false;
	bDrawDebug = InUpdateMoveTargetRequest.bDrawDebug;
	
	if (!IsValid(MoveTarget) || !IsValid(ControlledPawn) || !IsValid(TargetActor))
	{
		check(MoveTarget)
		return false;
	}
	
	if (MoveTarget->SetupOnOwnerPawn(MoveTargetDistanceMax))
	{
		bInitialized = true;
		return true;
	};

	return false;
}

void UADogAITask_UpdateMoveTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (bInitialized)
	{
		ControlledPawnLocation = ControlledPawn->GetActorLocation();

		if (!IsValid(TargetActor))
		{
			bInitialized = false;
		}
		
		TargetActorLocation = TargetActor->GetActorLocation();

		TargetActorRotation = UKismetMathLibrary::FindLookAtRotation(ControlledPawnLocation, TargetActorLocation);
		TargetActorRotation.Roll = 0.0f;
		DistanceToTargetActor = FVector::Distance(ControlledPawnLocation, TargetActorLocation);

		ControlledPawnRotation = UKismetMathLibrary::RInterpTo(ControlledPawnRotation, TargetActorRotation, DeltaTime, TurnRate);
		const float clampedDistRange = UKismetMathLibrary::MapRangeClamped(DistanceToTargetActor, 50.f, 500.f, 50.f, MoveTargetDistanceMax);
		float oldDistance = DistanceToMoveTarget;
		DistanceToMoveTarget = UKismetMathLibrary::FInterpTo(DistanceToMoveTarget, clampedDistRange, DeltaTime, MoveTargetDistanceSpeed);

		const FVector newMoveTargetLoc = ControlledPawnLocation + (UKismetMathLibrary::GetForwardVector(ControlledPawnRotation) * DistanceToMoveTarget);
		MoveTarget->SetActorLocation(newMoveTargetLoc);

		//check(DistanceToMoveTarget > 55.f);
		if (bDrawDebug)
		{
			GEngine->AddOnScreenDebugMessage(33001, 5, FColor::Black, FString::Printf(TEXT("TurnRate: %f"), TurnRate));
			GEngine->AddOnScreenDebugMessage(33002, 5, FColor::Purple, FString::Printf(TEXT("DistanceToMoveTarget: %f"), DistanceToMoveTarget));
			GEngine->AddOnScreenDebugMessage(33003, 5, FColor::Blue, FString::Printf(TEXT("DistanceToTargetActor: %f"), DistanceToTargetActor));

			DrawDebugSphere(GetWorld(), MoveTarget->GetActorLocation(), 10.f, 5, FColor::Orange);
		}
		
		ThrottleTurnRate(DeltaTime);
	}
}

void UADogAITask_UpdateMoveTarget::ThrottleTurnRate(float DeltaTime)
{
	// Trigger the TurnRate Throttle
	if (DistanceToTargetActor < TurnRateThrottleDistance && !bTurnRateThrottled &&
		FVector::DotProduct(TargetActor->GetActorForwardVector(), ControlledPawn->GetActorForwardVector()) < TurnThrottleTrigger)
	{
		TurnRate = 1.0f;
		TurnRateTimer = 0.0f;
		bTurnRateThrottled = true;
	}
	
	if (bTurnRateThrottled)
	{
		if (TurnRateTimer >= TurnRateTimeLimit || FMath::IsNearlyEqual(TurnRate, DefaultTurnRate, 0.001f))
		{
			bTurnRateThrottled = false;
			TurnRate = DefaultTurnRate;

			return;
		}

		if (DistanceToTargetActor <= TurnRateThrottleDistance)
		{
			TurnRateTimer += DeltaTime;
		}
		else
		{
			// Reset TurnRateTimer because we're outside a range we'd care about it
			TurnRateTimer = 0.0f;
		}

		TurnRate = UKismetMathLibrary::FInterpTo_Constant(TurnRate, DefaultTurnRate, DeltaTime, 1.f);
		return;
	}
}


