// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "ModularPawn.h"
#include "ADogPawnExtensionComponent.h"
#include "AbilitySystem/ADogAbilitySystemInterface.h"
#include "Navigation/CrowdAgentInterface.h"

class UADogAbilitySystemComponent;
class UADogMovementComponent;
class UADogCameraComponent;
class UAvoidanceComponent;

struct FGameplayTag;
struct FGameplayTagContainer;

#include "ADogCharacter.generated.h"

UCLASS(Meta = (ShortTooltip = "The base character pawn class used by this project."))
class ALPHADOGGAME_API AADogCharacter : public AModularPawn, public IGameplayTagAssetInterface,
	public IADogAbilitySystemInterface, public ICrowdAgentInterface
{
	GENERATED_BODY()
	
public:
	// Sets default values for this pawn's properties
	AADogCharacter();

	static const FName NAME_ADogAbilityReady;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//~IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	//~End of IGameplayTagAssetInterface

	//~APawn interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime) override;
	//~End of APawn interface

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void OnRep_PlayerState() override;
	//~End of AActor interface

	//~IADogAbilitySystemInterface
	virtual UADogAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; };
	//~End of IADogAbilitySystemInterface

	UADogMovementComponent* GetADogMovementComponent() { return MovementComponent; };
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAvoidanceComponent> AvoidanceComponent;

	//~ICrowdAgentInterface
	virtual FVector GetCrowdAgentLocation() const override;
	virtual FVector GetCrowdAgentVelocity() const override;
	virtual void GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const override;
	virtual float GetCrowdAgentMaxSpeed() const override;
	//~End of ICrowdAgentInterface

	// Registers player characters with crowd manager for ai avoidance. ADogPlayerBotController registers AI
	virtual void RegisterCrowdAgent();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components", DisplayName="Capsule", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components", DisplayName="Skeletal Mesh", meta=(AllowPrivateAccess=true))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Components")
	TObjectPtr<UADogAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Components")
	TObjectPtr<UADogMovementComponent> MovementComponent;

private:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaDog|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UADogPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlphaDog|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UADogCameraComponent> CameraComponent;

	/** set when agent is registered in crowd simulation (either controlled or an obstacle) */
	uint8 bRegisteredWithCrowdSimulation : 1;
};
