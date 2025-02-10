// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ADogPlayerController.h"

#include "CommonInputTypeEnum.h"
#include "Components/PrimitiveComponent.h"
#include "Debug/ADogLogging.h"
#include "Player/ADogCheatManager.h"
#include "Player/ADogPlayerState.h"
#include "Camera/ADogPlayerCameraManager.h"
#include "UI/ADogHUD.h"
#include "EngineUtils.h"
#include "ADogGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "CommonInputSubsystem.h"
#include "Player/ADogLocalPlayer.h"
#include "GameModes/ADogGameState.h"
#include "Settings/ADogSettingsLocal.h"
#include "Settings/ADogSettingsShared.h"
#include "ReplaySubsystem.h"
#include "Development/ADogDeveloperSettings.h"
#include "GameMapsSettings.h"
#include "LoadingProcessTask.h"
#include "Character/ADogCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogPlayerController)

namespace ADog
{
	namespace Input
	{
		static int32 ShouldAlwaysPlayForceFeedback = 0;
		static FAutoConsoleVariableRef CVarShouldAlwaysPlayForceFeedback(TEXT("ADogPC.ShouldAlwaysPlayForceFeedback"),
			ShouldAlwaysPlayForceFeedback,
			TEXT("Should force feedback effects be played, even if the last input device was not a gamepad?"));
	}
}

AADogPlayerController::AADogPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = AADogPlayerCameraManager::StaticClass();

#if USING_CHEAT_MANAGER
	CheatClass = UADogCheatManager::StaticClass();
#endif // #if USING_CHEAT_MANAGER
}

void AADogPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	RegisterLoadingProcessTask();
}

void AADogPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(false);
}

void AADogPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AADogPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
}

void AADogPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void AADogPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
}

AADogPlayerState* AADogPlayerController::GetADogPlayerState() const
{
	return CastChecked<AADogPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UADogAbilitySystemComponent* AADogPlayerController::GetADogAbilitySystemComponent() const
{
	if (AADogCharacter* character = Cast<AADogCharacter>(GetPawn()))
	{
		return character->GetAbilitySystemComponent();
	}
	return nullptr;
}

AADogHUD* AADogPlayerController::GetADogHUD() const
{
	return CastChecked<AADogHUD>(GetHUD(), ECastCheckedType::NullAllowed);
}

void AADogPlayerController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void AADogPlayerController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events
}

void AADogPlayerController::BroadcastOnPlayerStateChanged()
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

void AADogPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AADogPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AADogPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AADogPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);

	if (const UADogLocalPlayer* ADogLocalPlayer = Cast<UADogLocalPlayer>(InPlayer))
	{
		UADogSettingsShared* UserSettings = ADogLocalPlayer->GetSharedSettings();
		UserSettings->OnSettingChanged.AddUObject(this, &ThisClass::OnSettingsChanged);

		OnSettingsChanged(UserSettings);
	}
}

void AADogPlayerController::OnSettingsChanged(UADogSettingsShared* InSettings)
{
	bForceFeedbackEnabled = InSettings->GetForceFeedbackEnabled();
}

void AADogPlayerController::AddCheats(bool bForce)
{
#if USING_CHEAT_MANAGER
	Super::AddCheats(true);
#else //#if USING_CHEAT_MANAGER
	Super::AddCheats(bForce);
#endif // #else //#if USING_CHEAT_MANAGER
}

void AADogPlayerController::ServerCheat_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogADog, Warning, TEXT("ServerCheat: %s"), *Msg);
		ClientMessage(ConsoleCommand(Msg));
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AADogPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void AADogPlayerController::ServerCheatAll_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogADog, Warning, TEXT("ServerCheatAll: %s"), *Msg);
		for (TActorIterator<AADogPlayerController> It(GetWorld()); It; ++It)
		{
			AADogPlayerController* ADogPC = (*It);
			if (ADogPC)
			{
				ADogPC->ClientMessage(ADogPC->ConsoleCommand(Msg));
			}
		}
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AADogPlayerController::ServerCheatAll_Validate(const FString& Msg)
{
	return true;
}

void AADogPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void AADogPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AADogPlayerController::OnCameraPenetratingTarget()
{
	bHideViewTargetPawnNextFrame = true;
}

void AADogPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

#if WITH_SERVER_CODE && WITH_EDITOR
	if (GIsEditor && (InPawn != nullptr) && (GetPawn() == InPawn))
	{
		for (const FADogCheatToRun& CheatRow : GetDefault<UADogDeveloperSettings>()->CheatsToRun)
		{
			if (CheatRow.Phase == ECheatExecutionTime::OnPlayerPawnPossession)
			{
				ConsoleCommand(CheatRow.Cheat, /*bWriteToLog=*/ true);
			}
		}
	}
#endif
	
}

void AADogPlayerController::UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId)
{
	if (bForceFeedbackEnabled)
	{
		if (const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(GetLocalPlayer()))
		{
			const ECommonInputType CurrentInputType = CommonInputSubsystem->GetCurrentInputType();
			if (ADog::Input::ShouldAlwaysPlayForceFeedback || CurrentInputType == ECommonInputType::Gamepad || CurrentInputType == ECommonInputType::Touch)
			{
				InputInterface->SetForceFeedbackChannelValues(ControllerId, ForceFeedbackValues);
				return;
			}
		}
	}
	
	InputInterface->SetForceFeedbackChannelValues(ControllerId, FForceFeedbackValues());
}

void AADogPlayerController::UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents)
{
	Super::UpdateHiddenComponents(ViewLocation, OutHiddenComponents);

	// This hides possessed pawn's components, search for bHideViewTargetPawnNextFrame is changed. Ex: Lyra hides it when camera intersects mesh
	if (bHideViewTargetPawnNextFrame)
	{
		AActor* const ViewTargetPawn = PlayerCameraManager ? Cast<AActor>(PlayerCameraManager->GetViewTarget()) : nullptr;
		if (ViewTargetPawn)
		{
			// internal helper func to hide all the components
			auto AddToHiddenComponents = [&OutHiddenComponents](const TInlineComponentArray<UPrimitiveComponent*>& InComponents)
			{
				// add every component and all attached children
				for (UPrimitiveComponent* Comp : InComponents)
				{
					if (Comp->IsRegistered())
					{
						OutHiddenComponents.Add(Comp->GetPrimitiveSceneId());

						for (USceneComponent* AttachedChild : Comp->GetAttachChildren())
						{
							static FName NAME_NoParentAutoHide(TEXT("NoParentAutoHide"));
							UPrimitiveComponent* AttachChildPC = Cast<UPrimitiveComponent>(AttachedChild);
							if (AttachChildPC && AttachChildPC->IsRegistered() && !AttachChildPC->ComponentTags.Contains(NAME_NoParentAutoHide))
							{
								OutHiddenComponents.Add(AttachChildPC->GetPrimitiveSceneId());
							}
						}
					}
				}
			};

			//TODO Solve with an interface.  Gather hidden components or something.
			//TODO Hiding isn't awesome, sometimes you want the effect of a fade out over a proximity, needs to bubble up to designers.

			// hide pawn's components
			TInlineComponentArray<UPrimitiveComponent*> PawnComponents;
			ViewTargetPawn->GetComponents(PawnComponents);
			AddToHiddenComponents(PawnComponents);

			//// hide weapon too
			//if (ViewTargetPawn->CurrentWeapon)
			//{
			//	TInlineComponentArray<UPrimitiveComponent*> WeaponComponents;
			//	ViewTargetPawn->CurrentWeapon->GetComponents(WeaponComponents);
			//	AddToHiddenComponents(WeaponComponents);
			//}
		}

		// we consumed it, reset for next frame
		bHideViewTargetPawnNextFrame = false;
	}
}

void AADogPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogADogTeams, Error, TEXT("You can't set the team ID on a player controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AADogPlayerController::GetGenericTeamId() const
{
	if (const IADogTeamAgentInterface* PSWithTeamInterface = Cast<IADogTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnADogTeamIndexChangedDelegate* AADogPlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AADogPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AADogPlayerController::RegisterLoadingProcessTask()
{
	const bool isLocal = IsLocalController();
	const ENetRole role = GetLocalRole();
	const ENetMode netMode = GetNetMode();
	
	if (netMode != NM_DedicatedServer && isLocal)
	{
		InitialSpawnLoadingTask = ULoadingProcessTask::CreateLoadingScreenProcessTask(this, TEXT("Waiting for initial spawn"));
	}
}

void AADogPlayerController::SetInitialSpawn()
{
	if (InitialSpawnLoadingTask != nullptr)
	{
		InitialSpawnLoadingTask->Unregister();
		InitialSpawnLoadingTask = nullptr;
	}
}
