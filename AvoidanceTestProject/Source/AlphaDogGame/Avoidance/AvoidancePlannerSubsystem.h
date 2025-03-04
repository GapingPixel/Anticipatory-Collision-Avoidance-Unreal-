// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AvoidancePlannerSubsystem.generated.h"

class UAvoidanceComponent;
/**
 * 
 */
UCLASS()
class ALPHADOGGAME_API UAvoidancePlannerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
protected:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAvoidancePlannerSubsystem, STATGROUP_Tickables); }

	// Methods for handling agents and forces
	void GatherAgents();
	void InitializeAgents();
	void ComputeForces(float DeltaTime);
	
private:
	UPROPERTY()
	TArray<TObjectPtr<APawn>> Agents;
	TArray<UAvoidanceComponent*> AvoidanceComponents;
	TArray<float> Radii;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> GoalVelocities;
	//Simulation Parameters
	float SensingRadius = 100.0f;
	float TimeHorizon = 20.0f;
	float MaxForce = 20.0f;

	float ComputeTimeToCollision(const int32 i, const int32 j);

	// Precompute squared thresholds.
	const float SensingRadiusSq = SensingRadius * SensingRadius;
	const float SeparationDistance = 50.0f;
	const float SeparationDistanceSq = SeparationDistance * SeparationDistance;
	const float SeparationForceMag = 200.0f;

};