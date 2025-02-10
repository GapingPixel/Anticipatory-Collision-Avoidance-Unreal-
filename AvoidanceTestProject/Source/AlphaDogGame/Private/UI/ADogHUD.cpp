// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ADogHUD.h"

#include "GMCAbilityComponent.h"
#include "Async/TaskGraphInterfaces.h"
#include "Components/GameFrameworkComponentManager.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogHUD)

class AActor;
class UWorld;

//////////////////////////////////////////////////////////////////////
// AADogHUD

AADogHUD::AADogHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AADogHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AADogHUD::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);

	Super::BeginPlay();
}

void AADogHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}

void AADogHUD::GetDebugActorList(TArray<AActor*>& InOutList)
{
	UWorld* World = GetWorld();

	Super::GetDebugActorList(InOutList);

	// Add all actors with an ability system component.
	for (TObjectIterator<UGMC_AbilitySystemComponent> It; It; ++It)
	{
		if (UGMC_AbilitySystemComponent* ASC = *It)
		{
			if (!ASC->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
			{
				AActor* OwnerActor = ASC->GetOwner();

				if (OwnerActor /*&& UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor)*/)
				{
					AddActorToDebugList(OwnerActor, InOutList, World);
				}
			}
		}
	}
}
