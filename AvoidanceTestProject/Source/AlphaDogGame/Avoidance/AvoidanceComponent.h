// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AvoidanceComponent.generated.h"

class UNavigationInvokerComponent;
class AGMC_Pawn;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPHADOGGAME_API UAvoidanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAvoidanceComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Components")
	float Radious = 30;
	
	void UpdateAvoidanceVelocity(const FVector& NewVelocity);
	

	FVector AvoidanceVelocity; // External velocity from the avoidance planner
	FVector DesiredVelocity; // Desired velocity based on the navigation system
	FVector CombinedVelocity; // Final combined velocity
	FVector NextLocation;
	TArray<FVector> PathPoints;
	void ApplySteering(float DeltaTime);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Movement")
	bool bHasGoal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	bool bHasReachGoal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	FVector GoalLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	float StopRadius = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MovementSpeed = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	bool bDebug = true;
	
private:
	UPROPERTY()
	class UNavigationSystemV1* NavSystem = nullptr;
	UPROPERTY()
	UNavigationInvokerComponent* NavInvokerComponent;
	FTimerHandle NavMeshUpdateTimer;
	void FindNewPath();
	bool ShouldRecalculatePath();
	void UpdatePathPoints();
	void UpdateNavMesh();
	static AActor* FindFirstActorWithTag(const UWorld* World, FName Tag);

	UPROPERTY()
	TObjectPtr<AGMC_Pawn>ActorIns;

	
};
