// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ADogPlayerBotController.h"

#include "Bot/ADogMoveTarget.h"
#include "Character/PushPrioritySystem/PushPriorityComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/ADogGameMode.h"
#include "Debug/ADogLogging.h"
#include "GameModes/ADogExperienceManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogPlayerBotController)

class UObject;

AADogPlayerBotController::AADogPlayerBotController(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	bWantsPlayerState = true;
	bStopAILogicOnUnposses = false;
	bUpdatePawn = true;
}

void AADogPlayerBotController::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the experience load to complete
	AGameStateBase* GameState = GetWorld()->GetGameState();
	UADogExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UADogExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(FOnADogExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void AADogPlayerBotController::OnExperienceLoaded(const UADogExperienceDefinition* Experience)
{
	bExperienceLoaded = true;
	TryRunPostReady();
	/*

	UCrowdFollowingComponent* crowdComponent = Cast<UCrowdFollowingComponent>(GetComponentByClass<UPathFollowingComponent>());
	if (crowdComponent)
	{
		//crowdComponent->SetCrowdAnticipateTurns(true, false);
		crowdComponent->SetCrowdCollisionQueryRange(300);
		//crowdComponent->SetCrowdSeparation(true, false);
		crowdComponent->UpdateCrowdAgentParams();
	}
	*/
}

void AADogPlayerBotController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void AADogPlayerBotController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events
}

void AADogPlayerBotController::TryRunPostReady()
{
	APawn* pawn = GetPawn();
	if (bExperienceLoaded && IsValid(pawn))
	{
		//IgnoreDetourAgents();
		RunBehaviorTree(BTAsset);
	}	
}

void AADogPlayerBotController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	// Unbind from the old player state, if any
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IADogTeamAgentInterface* PlayerStateTeamInterface = Cast<IADogTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// Bind to the new player state, if any
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IADogTeamAgentInterface* PlayerStateTeamInterface = Cast<IADogTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// Broadcast the team change (if it really has)
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	LastSeenPlayerState = PlayerState;
}

void AADogPlayerBotController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AADogPlayerBotController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AADogPlayerBotController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AADogPlayerBotController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogADogTeams, Error, TEXT("You can't set the team ID on a player bot controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AADogPlayerBotController::GetGenericTeamId() const
{
	if (IADogTeamAgentInterface* PSWithTeamInterface = Cast<IADogTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnADogTeamIndexChangedDelegate* AADogPlayerBotController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}


void AADogPlayerBotController::ServerRestartController()
{
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	ensure((GetPawn() == nullptr) && IsInState(NAME_Inactive));

	if (IsInState(NAME_Inactive) || (IsInState(NAME_Spectating)))
	{
 		AADogGameMode* const GameMode = GetWorld()->GetAuthGameMode<AADogGameMode>();

		if ((GameMode == nullptr) || !GameMode->ControllerCanRestart(this))
		{
			return;
		}

		// If we're still attached to a Pawn, leave it
		if (GetPawn() != nullptr)
		{
			UnPossess();
		}

		// Re-enable input, similar to code in ClientRestart
		ResetIgnoreInputFlags();

		GameMode->RestartPlayer(this);
	}
}

ETeamAttitude::Type AADogPlayerBotController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const APawn* OtherPawn = Cast<APawn>(&Other)) {

		if (const IADogTeamAgentInterface* TeamAgent = Cast<IADogTeamAgentInterface>(OtherPawn->GetController()))
		{
			FGenericTeamId OtherTeamID = TeamAgent->GetGenericTeamId();

			//Checking Other pawn ID to define Attitude
			if (OtherTeamID.GetId() != GetGenericTeamId().GetId())
			{
				return ETeamAttitude::Hostile;
			}
			else
			{
				return ETeamAttitude::Friendly;
			}
		}
	}

	return ETeamAttitude::Neutral;
}

void AADogPlayerBotController::UpdateTeamAttitude(UAIPerceptionComponent* AIPerception)
{
	if (AIPerception)
	{
		AIPerception->RequestStimuliListenerUpdate();
	}
}

void AADogPlayerBotController::IgnoreDetourAgents()
{
	UCrowdFollowingComponent* crowdComponent = Cast<UCrowdFollowingComponent>(GetComponentByClass<UPathFollowingComponent>());
	if (crowdComponent)
	{		
		int priority = 0;
		UPushPriorityComponent* pushComponent = GetPawn()->FindComponentByClass<UPushPriorityComponent>();
		if (pushComponent)
		{
			// Setup our avoidance group
			FNavAvoidanceMask avoidanceGroup = FNavAvoidanceMask();
			avoidanceGroup.SetGroup(pushComponent->BasePushPriority);
			crowdComponent->SetAvoidanceGroup(avoidanceGroup.Packed);
			crowdComponent->UpdateCrowdAgentParams();
			
			// BasePushPriority doesn't have valid priority
			if (pushComponent->BasePushPriority == -1)
			{
				return;
			}
			priority = pushComponent->BasePushPriority;
		}
		
		FNavAvoidanceMask ignoreGroups = FNavAvoidanceMask();
		for (int i = 0; i < priority; i++)
		{
			ignoreGroups.SetGroup(i);
		}
		crowdComponent->SetGroupsToIgnore(ignoreGroups.Packed);
		crowdComponent->UpdateCrowdAgentParams();
	}
}

void AADogPlayerBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		const FTransform transform = AADogMoveTarget::GetStartingTransform(InPawn, 400.f);
		MoveTarget = GetWorld()->SpawnActorDeferred<AADogMoveTarget>(
						AADogMoveTarget::StaticClass(), transform,
						this, InPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		UGameplayStatics::FinishSpawningActor(MoveTarget, transform);

		TryRunPostReady();
	}
#endif
}

void AADogPlayerBotController::OnUnPossess()
{
	Super::OnUnPossess();
}

