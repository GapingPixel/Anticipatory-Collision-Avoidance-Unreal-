// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ADogMovementComponent.h"

#include "AbilitySystem/ADogAbilitySystemGlobals.h"
#include "Character/ADogCharacter.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values for this component's properties
UADogMovementComponent::UADogMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


// Called when the game starts
void UADogMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UADogMovementComponent::BindReplicationData_Implementation()
{
	Super::BindReplicationData_Implementation();
	
	AbilitySystemComponent = UADogAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<AADogCharacter>(GetGMCPawnOwner()));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GMCMovementComponent = this;
		AbilitySystemComponent->BindReplicationData();
	}

	BI_ProcessedInputVector = BindCompressedVector(
	ProcessedInputVector,
	EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
	EGMC_CombineMode::CombineIfUnchanged,
	EGMC_SimulationMode::Periodic_Output,
	EGMC_InterpolationFunction::Linear
  );
}

void UADogMovementComponent::GenAncillaryTick_Implementation(float DeltaTime, bool bLocalMove,
	bool bCombinedClientMove)
{
	Super::GenAncillaryTick_Implementation(DeltaTime, bLocalMove, bCombinedClientMove);

	if (AbilitySystemComponent) AbilitySystemComponent->GenAncillaryTick(DeltaTime, bCombinedClientMove);
}

void UADogMovementComponent::GenPredictionTick_Implementation(float DeltaTime)
{
	Super::GenPredictionTick_Implementation(DeltaTime);

	if (AbilitySystemComponent) AbilitySystemComponent->GenPredictionTick(DeltaTime);
}

void UADogMovementComponent::GenSimulationTick_Implementation(float DeltaTime)
{
	Super::GenSimulationTick_Implementation(DeltaTime);

	if (AbilitySystemComponent) AbilitySystemComponent->GenSimulationTick(DeltaTime);
}

void UADogMovementComponent::PreLocalMoveExecution_Implementation(const FGMC_Move& LocalMove)
{
	Super::PreLocalMoveExecution_Implementation(LocalMove);

	if (AbilitySystemComponent) AbilitySystemComponent->PreLocalMoveExecution();
}

void UADogMovementComponent::MovementUpdate_Implementation(float DeltaSeconds)
{
	Super::MovementUpdate_Implementation(DeltaSeconds);

	SetMovementFromAttrs();
	MatchStateTags();
	
}

bool UADogMovementComponent::OnCumulativeMoveInitialized_Implementation(FGMC_PawnState& InputState,
	EGMC_InterpolationStates SimStates, float DeltaTime, double Timestamp)
{
	bool ReturnValue = Super::OnCumulativeMoveInitialized_Implementation(InputState, SimStates, DeltaTime, Timestamp);

	const int32 LastIdx = MoveHistory.Num() - 1;
	gmc_ck(LastIdx >= 0)

	const auto& SourceState = SimStates == EGMC_InterpolationStates::Input ? MoveHistory[LastIdx].InputState : MoveHistory[LastIdx].OutputState;

	return ReturnValue;
}

void UADogMovementComponent::ApplyRotation(bool bIsDirectBotMove,
	const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
	if (HasRootMotion() && !RootMotionMetaData.bApplyRotationWithRootMotion)
	{
		return;
	}

	// Calculate a throttled rotation rate for AI when necessary
	if (!IsPlayerControlledPawn())
	{
		// Get rotation delta
		constexpr float maxThrottleYaw = 2.f;
		const float yawDelta = FMath::Abs(GetLinearVelocity_GMC().ToOrientationRotator().Yaw - GetActorRotation_GMC().Yaw);
		if (yawDelta < maxThrottleYaw)
		{
			RotationRate *= UKismetMathLibrary::MapRangeClamped(yawDelta, 0.0, 15.0, 0.15f, 0.5f);
			//GEngine->AddOnScreenDebugMessage(33002, 5, FColor::Black, FString::Printf(TEXT("Throttled rotationRate: %f"), RotationRate));
		}
	}

	

	// Apply rotation
	if (bUseSafeRotations)
	{
		RotateYawTowardsDirectionSafe(bIsDirectBotMove ? Velocity : GetLinearVelocity_GMC(), RotationRate, DeltaSeconds);
	}
	else
	{
		RotateYawTowardsDirection(bIsDirectBotMove ? Velocity : GetLinearVelocity_GMC(), RotationRate, DeltaSeconds);
	}
}

void UADogMovementComponent::CalculateVelocityCustom_Implementation(
	const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
	Super::CalculateVelocityCustom_Implementation(RootMotionMetaData, DeltaSeconds);

	// Implements a turn radius by adding force when rotating
	/*const FVector inputVector = FVector(GetProcessedInputVector().X, GetProcessedInputVector().Y, 0.f);
	const FVector velocityVector = FVector(GetVelocity().X, GetVelocity().Y,0.f);
	if (IsMovingOnGround()
		&& !velocityVector.IsZero()
		&& !inputVector.IsZero())
	{
		const float vectorDot = UKismetMathLibrary::Dot_VectorVector(UKismetMathLibrary::Vector_Normal2D(inputVector), PawnOwner->GetActorForwardVector());
		GEngine->AddOnScreenDebugMessage(33034, -1, FColor::Black, FString::Printf(TEXT("InputVectorDot: %f"), vectorDot));
		if (!FMath::IsNearlyEqual(vectorDot, 1.f))
		{
			if (PawnOwner->IsPlayerControlled())
			{
				GEngine->AddOnScreenDebugMessage(33034, -1, FColor::Black, FString::Printf(TEXT("InputVectorDot: %f"), vectorDot));
				GEngine->AddOnScreenDebugMessage(33035, -1, FColor::Blue, FString::Printf(TEXT("Adding force")));
			}
			// Add small offset to forward vector to prevent rotation from getting stuck
			const FVector force = UKismetMathLibrary::MapRangeClamped(vectorDot, -1.0, 1.0, 1.f, 0.f) * (10000000 * (PawnOwner->GetActorForwardVector() +FVector(.001,.001,0)));
			AddForce(force);
		}
	}*/
	Super::CalculateVelocityCustom_Implementation(RootMotionMetaData, DeltaSeconds);

	// Implements a turn radius by adding force when rotating
	const FVector inputVector = FVector(GetProcessedInputVector().X, GetProcessedInputVector().Y, 0.f);
	const FVector velocityVector = FVector(GetVelocity().X, GetVelocity().Y, 0.f);

	if (IsMovingOnGround() && !velocityVector.IsZero() && !inputVector.IsZero())
	{
		const float vectorDot = UKismetMathLibrary::Dot_VectorVector(UKismetMathLibrary::Vector_Normal2D(inputVector), PawnOwner->GetActorForwardVector());
		GEngine->AddOnScreenDebugMessage(33034, -1, FColor::Black, FString::Printf(TEXT("InputVectorDot: %f"), vectorDot));

		if (!FMath::IsNearlyEqual(vectorDot, 1.f))
		{
			if (PawnOwner->IsPlayerControlled())
			{
				GEngine->AddOnScreenDebugMessage(33034, -1, FColor::Black, FString::Printf(TEXT("InputVectorDot: %f"), vectorDot));
				GEngine->AddOnScreenDebugMessage(33035, -1, FColor::Blue, FString::Printf(TEXT("Adding force")));
			}

			// Calculate the desired forward force
			const FVector DesiredForce = UKismetMathLibrary::MapRangeClamped(vectorDot, -1.0, 1.0, 1.f, 0.f) * 
				(10000000.f * (PawnOwner->GetActorForwardVector() + FVector(.001f, .001f, 0.f)));

			// Perform a trace to check for obstacles ahead before applying the force
			const FVector StartLocation = PawnOwner->GetActorLocation();
			const FVector EndLocation = StartLocation + PawnOwner->GetActorForwardVector() * 200.0f; // Adjust trace distance as necessary

			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(PawnOwner);

			bool bHit = GetWorld()->LineTraceSingleByChannel(
				HitResult,
				StartLocation,
				EndLocation,
				ECC_Visibility,
				QueryParams
			);

			// Only apply the force if there's no obstacle directly ahead
			if (!bHit || !HitResult.bBlockingHit)
			{
				AddForce(DesiredForce);
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(33036, -1, FColor::Red, TEXT("Obstacle detected, force not applied"));
			}
		}
	}
}

void UADogMovementComponent::SetMovementFromAttrs()
{
	check(AbilitySystemComponent)
	
	MaxDesiredSpeed = AbilitySystemComponent->GetAttributeByTag(FGameplayTag::RequestGameplayTag("Attribute.Movement.MoveSpeed"))->Value;
	RotationRate = AbilitySystemComponent->GetAttributeByTag(FGameplayTag::RequestGameplayTag("Attribute.Movement.RotationRate"))->Value;
	InputAccelerationGrounded = AbilitySystemComponent->GetAttributeByTag(FGameplayTag::RequestGameplayTag("Attribute.Movement.Acceleration"))->Value;
}

void UADogMovementComponent::MatchStateTags()
{
	check(AbilitySystemComponent)
	
	EGMC_MovementMode moveMode = GetMovementMode();

	// Set InAir tag
	FGameplayTag inAirTag = FGameplayTag::RequestGameplayTag("State.InAir");
	const bool airborneMode = moveMode == EGMC_MovementMode::Airborne ? true : false;   
	AbilitySystemComponent->MatchTagToBool(inAirTag, airborneMode);
	
}
