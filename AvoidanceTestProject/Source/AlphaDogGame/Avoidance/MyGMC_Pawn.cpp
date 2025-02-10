// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGMC_Pawn.h"

#include "AvoidanceComponent.h"
#include "GMCFlatCapsuleComponent.h"
#include "GMCOrganicMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

static FName NAME_ADogCharacterCollisionProfile_Capsule(TEXT("ADogPawnCapsule"));
static FName NAME_ADogCharacterCollisionProfile_Mesh(TEXT("ADogPawnMesh"));

AMyGMC_Pawn::AMyGMC_Pawn()
{
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
	
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement Component"));
	
	/*MovementComponent->MaxStepUpHeight = 1;
	MovementComponent->MaxStepUpHeight = 1;*/

	AvoidanceComponent = CreateDefaultSubobject<UAvoidanceComponent>(TEXT("Avoidance Component"));
	//MovementComponent->
	/*bool bR = FMath::RandBool();
	Velocity = FVector(bR ? 1 : -1,0,0 ); 
	GoalVelocity  = Velocity;*/
}






