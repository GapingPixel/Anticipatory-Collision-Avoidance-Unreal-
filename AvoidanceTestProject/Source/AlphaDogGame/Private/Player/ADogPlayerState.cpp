// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ADogPlayerState.h"

#include "Character/ADogPawnData.h"
#include "Character/ADogPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/ADogExperienceManagerComponent.h"
//@TODO: Would like to isolate this a bit better to get the pawn data in here without this having to know about other stuff
#include "GameModes/ADogGameMode.h"
#include "Debug/ADogLogging.h"
#include "GameplayTagStack.h"
#include "LoadingProcessTask.h"
#include "GameModes/ADogGameState.h"
#include "Player/ADogPlayerController.h"
#include "Messages/ADogVerbMessage.h"
#include "Net/UnrealNetwork.h"

class AController;
class APlayerState;
class FLifetimeProperty;


AADogPlayerState::AADogPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MyPlayerConnectionType(EADogPlayerConnectionType::Player)
{

	MyTeamID = FGenericTeamId::NoTeam;
}

void AADogPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AADogPlayerState::Reset()
{
	Super::Reset();
}

void AADogPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	if (UADogPawnExtensionComponent* PawnExtComp = UADogPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		PawnExtComp->CheckDefaultInitialization();
	}
}

void AADogPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	//@TODO: Copy stats
}

void AADogPlayerState::OnDeactivated()
{
	bool bDestroyDeactivatedPlayerState = false;

	switch (GetPlayerConnectionType())
	{
		case EADogPlayerConnectionType::Player:
		case EADogPlayerConnectionType::InactivePlayer:
			//@TODO: Ask the experience if we should destroy disconnecting players immediately or leave them around
			// (e.g., for long running servers where they might build up if lots of players cycle through)
			bDestroyDeactivatedPlayerState = true;
			break;
		default:
			bDestroyDeactivatedPlayerState = true;
			break;
	}
	
	SetPlayerConnectionType(EADogPlayerConnectionType::InactivePlayer);

	if (bDestroyDeactivatedPlayerState)
	{
		Destroy();
	}
}

void AADogPlayerState::OnReactivated()
{
	if (GetPlayerConnectionType() == EADogPlayerConnectionType::InactivePlayer)
	{
		SetPlayerConnectionType(EADogPlayerConnectionType::Player);
	}
}

void AADogPlayerState::OnExperienceLoaded(const UADogExperienceDefinition* /*CurrentExperience*/)
{
	if (AADogGameMode* ADogGameMode = GetWorld()->GetAuthGameMode<AADogGameMode>())
	{
		BEANS_ULOG(LogADog, VeryVerbose, TEXT("Gamemode exists so we have authority, attempt to set the PawnData: %s"), *GetNameSafe(this));
		if (const UADogPawnData* NewPawnData = ADogGameMode->GetPawnDataForController(GetOwningController()))
		{
			SetPawnData(NewPawnData);
		}
		else
		{
			BEANS_ULOG(LogADog, Error, TEXT("AADogPlayerState::OnExperienceLoaded(): Unable to find PawnData to initialize player state [%s]!"), *GetNameSafe(this));
		}
	}
}

void AADogPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyPlayerConnectionType, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);

	DOREPLIFETIME(ThisClass, StatTags);	
}

AADogPlayerController* AADogPlayerState::GetADogPlayerController() const
{
	return Cast<AADogPlayerController>(GetOwner());
}

void AADogPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		AGameStateBase* GameState = GetWorld()->GetGameState();
		check(GameState);
		UADogExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UADogExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnADogExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

void AADogPlayerState::SetPawnData(const UADogPawnData* InPawnData)
{
	check(InPawnData);
	const ENetRole role = GetLocalRole();
	const ENetMode netMode = GetNetMode();
	const bool isLocal = GetOwningController()->IsLocalController();
	
	if (role != ROLE_Authority)
	{
		return;
	}
	
	if (PawnData)
	{
		BEANS_ULOG(LogADog, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	// Manually call OnRep_PawnData on listen server
	if (netMode != NM_DedicatedServer && isLocal)
	{
		OnRep_PawnData();
	}
	
	ForceNetUpdate();
}

void AADogPlayerState::OnRep_PawnData()
{
}

void AADogPlayerState::SetPlayerConnectionType(EADogPlayerConnectionType NewType)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyPlayerConnectionType, this);
	MyPlayerConnectionType = NewType;
}

void AADogPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogADogTeams, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}
}

FGenericTeamId AADogPlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnADogTeamIndexChangedDelegate* AADogPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AADogPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void AADogPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AADogPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AADogPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AADogPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void AADogPlayerState::ClientBroadcastMessage_Implementation(const FADogVerbMessage Message)
{
	// This check is needed to prevent running the action when in standalone mode
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}
