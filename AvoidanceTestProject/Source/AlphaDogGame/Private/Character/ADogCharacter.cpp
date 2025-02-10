// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ADogCharacter.h"

#include "BeansLoggingUtils.h"
#include "GMCFlatCapsuleComponent.h"
#include "AlphaDogGame/Avoidance/AvoidanceComponent.h"
#include "Camera/ADogCameraComponent.h"
#include "Character/ADogMovementComponent.h"
#include "Debug/ADogLogging.h"
#include "GameModes/ADogExperienceManagerComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Navigation/CrowdManager.h"

#include "Player/ADogPlayerController.h"

static FName NAME_ADogCharacterCollisionProfile_Capsule(TEXT("ADogPawnCapsule"));
static FName NAME_ADogCharacterCollisionProfile_Mesh(TEXT("ADogPawnMesh"));

const FName AADogCharacter::NAME_ADogAbilityReady("ADogAbilitiesReady");

// Sets default values
AADogCharacter::AADogCharacter()
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	NetCullDistanceSquared = 900000000.0f;

	CapsuleComponent = CreateDefaultSubobject<UGMC_FlatCapsuleCmp>(TEXT("Capsule"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCapsuleRadius(30.f);
	CapsuleComponent->SetCapsuleHalfHeight(90.f);
	CapsuleComponent->bEditableWhenInherited = true;
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetCollisionProfileName(NAME_ADogCharacterCollisionProfile_Capsule);

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	MeshComponent->bEditableWhenInherited = true;
	MeshComponent->SetupAttachment(CapsuleComponent);
	MeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	MeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	MeshComponent->SetCollisionProfileName(NAME_ADogCharacterCollisionProfile_Mesh);

	AbilitySystemComponent = CreateDefaultSubobject<UADogAbilitySystemComponent>(TEXT("ASC"));
	MovementComponent = CreateDefaultSubobject<UADogMovementComponent>(TEXT("Movement Component"));
	
	AvoidanceComponent = CreateDefaultSubobject<UAvoidanceComponent>(TEXT("Avoidance Component"));

	PawnExtComponent = CreateDefaultSubobject<UADogPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	
	CameraComponent = CreateDefaultSubobject<UADogCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
	CameraComponent->SetupAttachment(RootComponent);
}

void AADogCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
}

bool AADogCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return IGameplayTagAssetInterface::HasMatchingGameplayTag(TagToCheck);
}

bool AADogCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return IGameplayTagAssetInterface::HasAllMatchingGameplayTags(TagContainer);
}

bool AADogCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return IGameplayTagAssetInterface::HasAnyMatchingGameplayTags(TagContainer);
}

void AADogCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PawnExtComponent->HandlePlayerStateReplicated();

	RegisterCrowdAgent();
}

void AADogCharacter::FaceRotation(FRotator NewControlRotation, float DeltaTime)
{

	//DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation()+NewControlRotation.Vector()*100, 3, FColor::Red);

	/*const FVector inputVector = FVector(MovementComponent->GetProcessedInputVector().X, MovementComponent->GetProcessedInputVector().Y, 0.f);
	const FVector velocityVector = FVector(MovementComponent->GetVelocity().X, MovementComponent->GetVelocity().Y,0.f);

	if (MovementComponent->IsMovingOnGround()
		&& !velocityVector.IsZero()
		&& !inputVector.IsZero())
	{
		const float vectorDot = UKismetMathLibrary::Dot_VectorVector(UKismetMathLibrary::Vector_Normal2D(inputVector), GetActorForwardVector());
		if (!FMath::IsNearlyEqual(vectorDot, 1.f))
		{
			const FVector force = UKismetMathLibrary::MapRangeClamped(vectorDot, -1.0, 1.0, 1.0, 0.0) * (5000000 * GetActorForwardVector());
			MovementComponent->AddForce(force);
		}
	}*/
	
	//DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation()+GetActorForwardVector()*100, 3, FColor::Purple);
}

void AADogCharacter::PreInitializeComponents()
{
	
	Super::PreInitializeComponents();
}

void AADogCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AADogCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AADogCharacter::Reset()
{
	Super::Reset();
}

void AADogCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AADogCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	PawnExtComponent->SetupPlayerInputComponent();
}

void AADogCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

FVector AADogCharacter::GetCrowdAgentLocation() const
{
	return GetActorLocation();
}

FVector AADogCharacter::GetCrowdAgentVelocity() const
{
	return MovementComponent->GetLinearVelocity_GMC();
}

void AADogCharacter::GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const
{
	CylinderRadius = CapsuleComponent->GetScaledCapsuleRadius();
	CylinderHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
}

float AADogCharacter::GetCrowdAgentMaxSpeed() const
{
	return AbilitySystemComponent->GetAttributeValueByTag(FGameplayTag::RequestGameplayTag("Attribute.Movement.MoveSpeed"));
}

void AADogCharacter::RegisterCrowdAgent()
{
	UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
	if (CrowdManager && IsPlayerControlled())
	{
		ICrowdAgentInterface* IAgent = Cast<ICrowdAgentInterface>(this);
		CrowdManager->RegisterAgent(IAgent);
		bRegisteredWithCrowdSimulation = true;
	}
	else
	{
		bRegisteredWithCrowdSimulation = false;
	}
}

// Called when the game starts or when spawned
void AADogCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AADogCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AADogCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PawnExtComponent->SetupPlayerInputComponent();
}

