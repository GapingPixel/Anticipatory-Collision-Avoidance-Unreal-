// Fill out your copyright notice in the Description page of Project Settings.

#include "AvoidancePlannerSubsystem.h"

#include "AvoidanceComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "MyGMC_Pawn.h"
#include "DrawDebugHelpers.h"
#include "Character/ADogCharacter.h"

void UAvoidancePlannerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FTimerHandle UnusedHandle;
    GetWorld()->GetTimerManager().SetTimer(
        UnusedHandle, this, &ThisClass::InitializeInternal, 4.f, false);
   
}

void UAvoidancePlannerSubsystem::Deinitialize()
{
    Super::Deinitialize();
    //CleanUps
}

void UAvoidancePlannerSubsystem::Tick(float DeltaTime)
{
    ComputeForces(DeltaTime);
}

void UAvoidancePlannerSubsystem::InitializeInternal()
{
    GatherAgents();
    InitializeAgents();
}

void UAvoidancePlannerSubsystem::GatherAgents()
{
    Agents.Empty();
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor->FindComponentByClass<UAvoidanceComponent>())
        {
            Agents.Add(Actor);
        }
    }
    Positions.SetNum(Agents.Num());
    Radii.SetNum(Agents.Num());
    Velocities.SetNum(Agents.Num());
    GoalVelocities.SetNum(Agents.Num());
}

void UAvoidancePlannerSubsystem::InitializeAgents()
{
    for (int i = 0; i < Agents.Num(); ++i)
    {
        const AActor* Agent = Agents[i];
        Positions[i] = Agent->GetActorLocation();
        Radii[i] = Agent->FindComponentByClass<UAvoidanceComponent>()->Radious; 
        Velocities[i] = Agent->GetVelocity();
        GoalVelocities[i] = Agent->FindComponentByClass<UAvoidanceComponent>()->AvoidanceVelocity;
    }
}

void UAvoidancePlannerSubsystem::ComputeForces(float DeltaTime)
{
    TArray<FVector> Forces;
    Forces.SetNum(Agents.Num());
    for (int i = 0; i < Agents.Num(); ++i)
    {
        Positions[i] = Agents[i]->GetActorLocation();
        Forces[i] = 2 * (GoalVelocities[i] - Velocities[i]);
    }
    for (int i = 0; i < Agents.Num(); ++i)
    {
        for (int j = 0; j < Agents.Num(); ++j)
        {
            if (i != j && FVector::Dist(Positions[i], Positions[j]) <= SensingRadius)
            {
                const float t = ComputeTimeToCollision(i, j);

                if (FVector FAvoid = Positions[i] + Velocities[i] * t - Positions[j] - Velocities[j] * t; FAvoid.SizeSquared() > 0.0f)
                {
                    FAvoid.Normalize();
                    float Mag = 0.0f;
                    if (t >= 0.0f && t <= TimeHorizon)
                    {
                        Mag = (TimeHorizon - t) / (t + 0.001f);
                    }

                    if (Mag > MaxForce)
                    {
                        Mag = MaxForce;
                    }
                    FAvoid *= Mag;
                    Forces[i] += FAvoid;
                }
            }
        }
        for (int j = 0; j < Agents.Num(); ++j)
        {
            //if (i != j && FVector::Dist(Positions[i], Positions[j]) < 200.0f) // Separation distance
            if (i != j && FVector::Dist(Positions[i], Positions[j]) < 50.0f) // Separation distance
            {
                FVector SeparationDirection = Positions[i] - Positions[j];
                SeparationDirection.Normalize();
                Forces[i] += SeparationDirection * 200.0f; // Separation force magnitude
            }
        }
    }
    // Apply forces
    for (int i = 0; i < Agents.Num(); ++i)
    {
        if ( !Agents[i]->FindComponentByClass<UAvoidanceComponent>()->bHasReachGoal)
        {
            Velocities[i] += Forces[i] * DeltaTime;
            Agents[i]->FindComponentByClass<UAvoidanceComponent>()->UpdateAvoidanceVelocity(Forces[i]); 
            Positions[i] += Velocities[i] * DeltaTime;
            Cast<APawn>(Agents[i])->AddMovementInput(Velocities[i].GetSafeNormal(), Velocities[i].Size());
            Positions[i] = Agents[i]->GetActorLocation();
            if (Agents[i]->FindComponentByClass<UAvoidanceComponent>()->bDebug)
            {
                DrawDebugSphere(GetWorld(), Positions[i], Radii[i], 12, FColor::Red, false, -1.0f, 0, 0.1f);
                DrawDebugLine(GetWorld(), Positions[i], Positions[i] + Velocities[i], FColor::Green, false, -1.0f, 0, 1.0f);
            }
        }
    }
}

float UAvoidancePlannerSubsystem::ComputeTimeToCollision(const int i, const int j)
{
    const float r = Radii[i] + Radii[j];
    const FVector w = Positions[j] - Positions[i];
    const float c = FVector::DotProduct(w, w) - r * r;
    if (c < 0.0f) // Agents are colliding
    {
        return 0.0f;
    }
    const FVector v = Velocities[i] - Velocities[j];
    const float a = FVector::DotProduct(v, v);
    const float b = FVector::DotProduct(w, v);
    const float Discr = b * b - a * c;

    if (Discr <= 0.0f) { return FLT_MAX; }

    const float Tau = (b - FMath::Sqrt(Discr)) / a;

    if (Tau < 0.0f)
    {
        return FLT_MAX;
    }
    return Tau;
}

void UAvoidancePlannerSubsystem::SetGoalForAllActors(const FVector& Goal) const
{
    for (const AActor* Actor : Agents)
    {
        Actor->FindComponentByClass<UAvoidanceComponent>()->GoalLocation = Goal;
    }
}



