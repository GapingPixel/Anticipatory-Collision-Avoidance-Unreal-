// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AvoidancePlannerSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ALPHADOGGAME_API UAvoidancePlannerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	
	virtual void Tick(float DeltaTime);
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UAvoidancePlannerSubsystem, STATGROUP_Tickables); }

	// Methods for handling agents and forces
	void GatherAgents();
	void InitializeAgents();
	void ComputeForces(float DeltaTime);

	void SetGoalForAllActors(const FVector& Goal) const;

protected:

	virtual void InitializeInternal();
	
private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Agents;
	TArray<FVector> Positions;
	TArray<float> Radii;
	TArray<FVector> Velocities;
	TArray<FVector> GoalVelocities;

	float SensingRadius = 100.0f;
	float TimeHorizon = 20.0f;
	float MaxForce = 20.0f;

	float ComputeTimeToCollision(const int i, const int j);

	
	
};