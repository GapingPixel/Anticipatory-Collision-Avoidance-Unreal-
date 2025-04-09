// Fill out your copyright notice in the Description page of Project Settings.

#include "AvoidancePlannerSubsystem.h"
#include "AvoidanceComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

void UAvoidancePlannerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FTimerHandle Timer;
    GetWorld()->GetTimerManager().SetTimer(Timer,
    FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        GatherAgents();
        InitializeAgents();
    }), 4.f, false);
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

void UAvoidancePlannerSubsystem::GatherAgents()
{
    Agents.Empty();
    AvoidanceComponents.Empty();

    // Iterate over all actors and cache those with an AvoidanceComponent.
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* Actor = *It;
        if (UAvoidanceComponent* AvoidComp = Actor->FindComponentByClass<UAvoidanceComponent>())
        {
            Agents.Add(Actor);
            AvoidanceComponents.Add(AvoidComp);
        }
    }
    const uint32 NumAgents = Agents.Num();
    Positions.SetNum(NumAgents);
    Radii.SetNum(NumAgents);
    Velocities.SetNum(NumAgents);
    GoalVelocities.SetNum(NumAgents);
}

void UAvoidancePlannerSubsystem::InitializeAgents()
{
    const uint32 NumAgents = Agents.Num();
    for (uint32 i = 0; i <  NumAgents; ++i)
    {
        const AActor* Agent = Agents[i];
        Positions[i] = Agent->GetActorLocation();
        Radii[i] = AvoidanceComponents[i]->Radious;
        Velocities[i] = Agent->GetVelocity();
        GoalVelocities[i] = AvoidanceComponents[i]->AvoidanceVelocity;
    }
}

void UAvoidancePlannerSubsystem::ComputeForces(float DeltaTime)
{
    const uint32 NumAgents = Agents.Num();
    TArray<FVector> Forces;
    Forces.SetNum(NumAgents);
    // Refresh positions and initialize forces based on goal velocities.
    for (uint32 i = 0; i < NumAgents; ++i)
    {
        Positions[i] = Agents[i]->GetActorLocation();
        Forces[i] = 2.0f * (GoalVelocities[i] - Velocities[i]);
    }
    
#pragma region Multi-Threaded version
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(ComputeForces_Parallel);
        ParallelFor(NumAgents, [&](const uint32 i)
        {
            const FVector Pos_I = Positions[i];
            const FVector Vel_I = Velocities[i];
            FVector LocalForce = FVector::ZeroVector;
                constexpr uint32 BatchSize = 4; //Process agents in batches.  Adjust based on SIMD width (4 for SSE, 8 for AVX)
            for (uint32 j = 0; j < NumAgents; j += BatchSize)
            {
                const uint32 BatchEnd = FMath::Min(j + BatchSize, NumAgents);
                float DistSqBatch[BatchSize]; // SIMD-like arrays for batch processing
                FVector PosBatch[BatchSize];
                FVector VelBatch[BatchSize];
                FVector FAvoidBatch[BatchSize];
                float MagBatch[BatchSize];
                for (uint32 k = j; k < BatchEnd; ++k) // Load batch data
                {
                    const uint32 Idx = k - j;
                    if (k == i) { // Skip self-interaction
                        DistSqBatch[Idx] = FLT_MAX; // Invalid distance
                        continue;
                    }
                    PosBatch[Idx] = Positions[k];
                    VelBatch[Idx] = Velocities[k];
                    DistSqBatch[Idx] = FVector::DistSquared(Pos_I, PosBatch[Idx]);
                }
                for (uint32 k = j; k < BatchEnd; ++k)// Process collision avoidance in batch
                {
                    const uint32 Idx = k - j;
                    if (DistSqBatch[Idx] <= SensingRadiusSq)
                    {
                        const float t = ComputeTimeToCollision(i, k); // Scalar for now
                        FVector PredictedPos_I = Pos_I + Vel_I * t;
                        FVector PredictedPos_J = PosBatch[Idx] + VelBatch[Idx] * t;
                        FAvoidBatch[Idx] = PredictedPos_I - PredictedPos_J;
                        const float SizeSq = FAvoidBatch[Idx].SizeSquared();
                        if (SizeSq > 0.0f) {
                            FAvoidBatch[Idx] /= FMath::Sqrt(SizeSq); // Normalize
                            MagBatch[Idx] = (t >= 0.0f && t <= TimeHorizon) ? (TimeHorizon - t) / (t + 0.001f) : 0.0f;
                            MagBatch[Idx] = FMath::Min(MagBatch[Idx], MaxForce);
                            FAvoidBatch[Idx] *= MagBatch[Idx];
                        }
                        else {
                            FAvoidBatch[Idx] = FVector::ZeroVector;
                        }
                    }
                    else {
                        FAvoidBatch[Idx] = FVector::ZeroVector;
                    }
                    if (DistSqBatch[Idx] < SeparationDistanceSq) { // Separation force
                        FVector SeparationDirection = Pos_I - PosBatch[Idx];
                        SeparationDirection.Normalize();
                        LocalForce += SeparationDirection * SeparationForceMag;
                    }
                }
                for (uint32 k = j; k < BatchEnd; ++k) {  // Accumulate avoidance forces from batch
                    LocalForce += FAvoidBatch[k - j];
                }
            }Forces[i] += LocalForce;// Accumulate the computed force
        }, EParallelForFlags::BackgroundPriority);
    }
#pragma endregion

#pragma region Single-thread version:
/*{ 
    TRACE_CPUPROFILER_EVENT_SCOPE(ComputeForces_SingleThread);
        for (uint32 i = 0; i < NumAgents; ++i)
        {
            const FVector Pos_I = Positions[i];
            const FVector Vel_I = Velocities[i];
            FVector LocalForce = FVector::ZeroVector;

            for (uint32 j = 0; j < NumAgents; ++j)
            {
                if (i == j)
                    continue;

                const FVector Pos_J = Positions[j];
                const FVector Vel_J = Velocities[j];
                const float DistSq = FVector::DistSquared(Pos_I, Pos_J);

                // Collision avoidance if within sensing radius.
                if (DistSq <= SensingRadiusSq)
                {
                    const float t = ComputeTimeToCollision(i, j);
                    FVector PredictedPos_I = Pos_I + Vel_I * t;
                    FVector PredictedPos_J = Pos_J + Vel_J * t;
                    FVector FAvoid = PredictedPos_I - PredictedPos_J;
                    if (FAvoid.SizeSquared() > 0.0f)
                    {
                        FAvoid.Normalize();
                        float Mag = (t >= 0.0f && t <= TimeHorizon) ? (TimeHorizon - t) / (t + 0.001f) : 0.0f;
                        Mag = FMath::Min(Mag, MaxForce);
                        FAvoid *= Mag;
                        LocalForce += FAvoid;
                    }
                }
                // Separation force when agents are very close.
                if (DistSq < SeparationDistanceSq)
                {
                    FVector SeparationDirection = Pos_I - Pos_J;
                    SeparationDirection.Normalize();
                    LocalForce += SeparationDirection * SeparationForceMag;
                }
            }
            // Accumulate the computed force.
            Forces[i] += LocalForce;
        }
}*/
#pragma endregion

    // Update simulation state on the game thread.
    for (uint32 i = 0; i < NumAgents; ++i)
    {
        if (!AvoidanceComponents[i]->bHasReachGoal)
        {
            Velocities[i] += Forces[i] * DeltaTime;
            AvoidanceComponents[i]->AvoidanceVelocity = Forces[i];
            Positions[i] += Velocities[i] * DeltaTime;

            Agents[i]->AddMovementInput(Velocities[i].GetSafeNormal(), Velocities[i].Size());
            // Re-sync the position in case the actor’s location was modified.
            Positions[i] = Agents[i]->GetActorLocation();

            if (AvoidanceComponents[i]->bDebug)
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
    return (Tau < 0.0f) ? FLT_MAX : Tau;
}





