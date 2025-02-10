// Fill out your copyright notice in the Description page of Project Settings.
#include "AvoidanceComponent.h"
#include "GMCPawn.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "NavigationInvokerComponent.h"

UAvoidanceComponent::UAvoidanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    NavInvokerComponent = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
   
}

void UAvoidanceComponent::BeginPlay()
{
    Super::BeginPlay();
    ActorIns = Cast<AGMC_Pawn>(GetOwner());
    NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavInvokerComponent)
    {
        NavInvokerComponent->Activate(true);
    }
    // Periodically request navmesh updates
    GetWorld()->GetTimerManager().SetTimer(NavMeshUpdateTimer, this, &UAvoidanceComponent::UpdateNavMesh, 1.0f, true);

    if (const AActor* GoalActor = FindFirstActorWithTag(GetWorld(), FName("GoalPoint")))
    {
        GoalLocation = GoalActor->GetActorLocation();
    }
    FindNewPath();
}

void UAvoidanceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (NavSystem && bHasGoal)
    {
        const float DistanceToGoal = FVector::Dist(ActorIns->GetActorLocation(), GoalLocation);

        if (DistanceToGoal <= StopRadius)
        {
            GoalLocation = FVector::ZeroVector;
            CombinedVelocity = FVector::ZeroVector;
            bHasReachGoal = true;
        }
        else
        {
            ApplySteering(DeltaTime);
            // Adjust path if necessary or follow the path points
            if (FVector::Dist(ActorIns->GetActorLocation(), NextLocation) < 100.0f)
            {
                UpdatePathPoints();
            }
            if (ShouldRecalculatePath())
            {
                FindNewPath();
            }
        }
    }
}

void UAvoidanceComponent::UpdateAvoidanceVelocity(const FVector& NewVelocity)
{
    AvoidanceVelocity = NewVelocity;
}

void UAvoidanceComponent::ApplySteering(float DeltaTime)
{
    // Calculate the desired velocity towards the next path point
    DesiredVelocity = (NextLocation - ActorIns->GetActorLocation()).GetSafeNormal() * MovementSpeed;
    CombinedVelocity = DesiredVelocity + AvoidanceVelocity;
    CombinedVelocity = CombinedVelocity.GetClampedToMaxSize(MovementSpeed); 
    // Apply the combined velocity as movement input
    ActorIns->AddMovementInput(CombinedVelocity.GetSafeNormal(), CombinedVelocity.Size() * DeltaTime);
    AvoidanceVelocity = FVector::ZeroVector;
}

void UAvoidanceComponent::FindNewPath()
{
    if (NavSystem && !GoalLocation.IsNearlyZero())
    {
        const UNavigationPath* NavPath = NavSystem->FindPathToLocationSynchronously(GetWorld(), ActorIns->GetActorLocation(), GoalLocation);
        if (NavPath && NavPath->PathPoints.Num() > 1)
        {
            PathPoints = NavPath->PathPoints;
            NextLocation = PathPoints[1];  // Move to the second point in the path
        }
        else
        {
            NextLocation = GoalLocation;
            PathPoints.Empty();  // Clear path points as only the goal remains
        }
    }
}

void UAvoidanceComponent::UpdatePathPoints()
{
    if (PathPoints.Num() > 1)
    {
        // Remove the first path point and move to the next
        PathPoints.RemoveAt(0);
        NextLocation = PathPoints[0];
    }
    else
    {
        NextLocation = GoalLocation;
    }

    if (ShouldRecalculatePath())
    {
        FindNewPath();
    }
}

bool UAvoidanceComponent::ShouldRecalculatePath()
{
    // Check if the actor has deviated too far from the current path
    constexpr float DeviationThreshold = 200.0f;
    if (FVector::Dist(ActorIns->GetActorLocation(), NextLocation) > DeviationThreshold)
    {
        return true;
    }
    const FVector TraceStart = ActorIns->GetActorLocation();
    const FVector TraceEnd = TraceStart + CombinedVelocity.GetSafeNormal() * 500.0f; 
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ActorIns);
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
    {
        //If there's an obstacle, recalculate the path
        return true;
    }
    return false;  
}

void UAvoidanceComponent::UpdateNavMesh()
{
    if (NavSystem)
    {
        NavSystem->Build();
        FindNewPath(); 
    }
}

AActor* UAvoidanceComponent::FindFirstActorWithTag(const UWorld* World, const FName Tag)
{
    TArray<AActor*> ActorsWithTag;
    UGameplayStatics::GetAllActorsWithTag(World, Tag, ActorsWithTag);
    if (ActorsWithTag.Num() > 0)
    {
        return ActorsWithTag[0];  // Return the first actor with the tag
    }
    return nullptr;
}