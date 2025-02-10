// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GMCOrganicMovementComponent.h"
#include "AbilitySystem/ADogAbilitySystemComponent.h"
#include "ADogMovementComponent.generated.h"

// Describes energy based movement
UENUM(BlueprintType)
enum class EMovementEnergy : uint8
{
	Tired UMETA(ToolTip = "Low energy, tired movement"),
	Normal UMETA(ToolTip = "The standard movement."),
	Zooming UMETA(ToolTip = "High energy, fast movement")
  };

UCLASS()
class ALPHADOGGAME_API UADogMovementComponent : public UGMC_OrganicMovementCmp
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UADogMovementComponent(const FObjectInitializer& ObjectInitializer);

	// GMC overrides
	virtual void BindReplicationData_Implementation() override;
	virtual void GenAncillaryTick_Implementation(float DeltaTime, bool bLocalMove, bool bCombinedClientMove) override;
	virtual void GenPredictionTick_Implementation(float DeltaTime) override;
	virtual void GenSimulationTick_Implementation(float DeltaTime) override;
	virtual void PreLocalMoveExecution_Implementation(const FGMC_Move& LocalMove) override;
	virtual void MovementUpdate_Implementation(float DeltaSeconds) override;
	virtual bool OnCumulativeMoveInitialized_Implementation(FGMC_PawnState& InputState, EGMC_InterpolationStates SimStates, float DeltaTime, double Timestamp) override;
	virtual void ApplyRotation(bool bIsDirectBotMove, const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) override;
	virtual void CalculateVelocityCustom_Implementation(const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) override;
	// End GMC overrides
	
	virtual void MatchStateTags();

	UPROPERTY(BlueprintReadOnly, Category = "AlphaDog Movement Component")
	int32 BI_ProcessedInputVector{-1};

	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Components")
	/// Convenience reference to the GMAS ability system component.
	UADogAbilitySystemComponent* AbilitySystemComponent;
	
	void SetMovementFromAttrs();

};
