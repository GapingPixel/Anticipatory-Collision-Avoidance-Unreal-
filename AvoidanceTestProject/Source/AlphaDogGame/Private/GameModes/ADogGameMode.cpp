// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ADogGameMode.h"

#include "AssetRegistry/AssetData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Debug/ADogLogging.h"
#include "Misc/CommandLine.h"
#include "System/ADogAssetManager.h"
#include "GameModes/ADogGameState.h"
#include "System/ADogGameSession.h"
#include "Player/ADogPlayerController.h"
#include "Player/ADogPlayerBotController.h"
#include "Player/ADogPlayerState.h"
#include "Character/ADogCharacter.h"
#include "UI/ADogHUD.h"
#include "Character/ADogPawnExtensionComponent.h"
#include "Character/ADogPawnData.h"
#include "GameModes/ADogWorldSettings.h"
#include "GameModes/ADogExperienceDefinition.h"
#include "GameModes/ADogExperienceManagerComponent.h"
#include "GameModes/ADogUserFacingExperienceDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Development/ADogDeveloperSettings.h"
#include "Player/ADogPlayerSpawningManagerComponent.h"
#include "CommonUserSubsystem.h"
#include "CommonSessionSubsystem.h"
#include "TimerManager.h"
#include "GameMapsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogGameMode)

AADogGameMode::AADogGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AADogGameState::StaticClass();
	GameSessionClass = AADogGameSession::StaticClass();
	PlayerControllerClass = AADogPlayerController::StaticClass();
	PlayerStateClass = AADogPlayerState::StaticClass();
	DefaultPawnClass = AADogCharacter::StaticClass();
	HUDClass = AADogHUD::StaticClass();
}

const UADogPawnData* AADogGameMode::GetPawnDataForController(const AController* InController) const
{
	// See if pawn data is already set on the player state
	if (InController != nullptr)
	{
		if (const AADogPlayerState* ADogPS = InController->GetPlayerState<AADogPlayerState>())
		{
			if (const UADogPawnData* PawnData = ADogPS->GetPawnData<UADogPawnData>())
			{
				return PawnData;
			}
		}
	}

	// If not, fall back to the the default for the current experience
	check(GameState);
	UADogExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UADogExperienceManagerComponent>();
	check(ExperienceComponent);

	if (ExperienceComponent->IsExperienceLoaded())
	{
		const UADogExperienceDefinition* Experience = ExperienceComponent->GetCurrentExperienceChecked();
		if (Experience->DefaultPawnData != nullptr)
		{
			return Experience->DefaultPawnData;
		}

		// Experience is loaded and there's still no pawn data, fall back to the default for now
		return UADogAssetManager::Get().GetDefaultPawnData();
	}

	// Experience not loaded yet, so there is no pawn data to be had
	return nullptr;
}

void AADogGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Wait for the next frame to give time to initialize startup settings
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

void AADogGameMode::HandleMatchAssignmentIfNotExpectingOne()
{
	FPrimaryAssetId ExperienceId;
	FString ExperienceIdSource;

	// Precedence order (highest wins)
	//  - Matchmaking assignment (if present)
	//  - URL Options override
	//  - Developer Settings (PIE only)
	//  - Command Line override
	//  - World Settings
	//  - Dedicated server
	//  - Default experience

	UWorld* World = GetWorld();

	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UADogExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}

	if (!ExperienceId.IsValid() && World->IsPlayInEditor())
	{
		ExperienceId = GetDefault<UADogDeveloperSettings>()->ExperienceOverride;
		ExperienceIdSource = TEXT("DeveloperSettings");
	}

	// see if the command line wants to set the experience
	if (!ExperienceId.IsValid())
	{
		FString ExperienceFromCommandLine;
		if (FParse::Value(FCommandLine::Get(), TEXT("Experience="), ExperienceFromCommandLine))
		{
			ExperienceId = FPrimaryAssetId::ParseTypeAndName(ExperienceFromCommandLine);
			if (!ExperienceId.PrimaryAssetType.IsValid())
			{
				ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UADogExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromCommandLine));
			}
			ExperienceIdSource = TEXT("CommandLine");
		}
	}

	// see if the world settings has a default experience
	if (!ExperienceId.IsValid())
	{
		if (AADogWorldSettings* TypedWorldSettings = Cast<AADogWorldSettings>(GetWorldSettings()))
		{
			ExperienceId = TypedWorldSettings->GetDefaultGameplayExperience();
			ExperienceIdSource = TEXT("WorldSettings");
		}
	}

	UADogAssetManager& AssetManager = UADogAssetManager::Get();
	FAssetData Dummy;
	if (ExperienceId.IsValid() && !AssetManager.GetPrimaryAssetData(ExperienceId, /*out*/ Dummy))
	{
		UE_LOG(LogADogExperience, Error, TEXT("EXPERIENCE: Wanted to use %s but couldn't find it, falling back to the default)"), *ExperienceId.ToString());
		ExperienceId = FPrimaryAssetId();
	}

	// Final fallback to the default experience
	if (!ExperienceId.IsValid())
	{
		if (TryDedicatedServerLogin())
		{
			// This will start to host as a dedicated server
			return;
		}

		//@TODO: Pull this from a config setting or something
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("ADogExperienceDefinition"), FName("B_ADogDefaultExperience"));
		ExperienceIdSource = TEXT("Default");
	}

	OnMatchAssignmentGiven(ExperienceId, ExperienceIdSource);
}

bool AADogGameMode::TryDedicatedServerLogin()
{
	// Some basic code to register as an active dedicated server, this would be heavily modified by the game
	FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance && World && World->GetNetMode() == NM_DedicatedServer && World->URL.Map == DefaultMap)
	{
		// Only register if this is the default map on a dedicated server
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();

		// Dedicated servers may need to do an online login
		UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &AADogGameMode::OnUserInitializedForDedicatedServer);

		// There are no local users on dedicated server, but index 0 means the default platform user which is handled by the online login code
		if (!UserSubsystem->TryToLoginForOnlinePlay(0))
		{
			OnUserInitializedForDedicatedServer(nullptr, false, FText(), ECommonUserPrivilege::CanPlayOnline, ECommonUserOnlineContext::Default);
		}

		return true;
	}

	return false;
}

void AADogGameMode::HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode)
{
	FPrimaryAssetType UserExperienceType = UADogUserFacingExperienceDefinition::StaticClass()->GetFName();
	
	// Figure out what UserFacingExperience to load
	FPrimaryAssetId UserExperienceId;
	FString UserExperienceFromCommandLine;
	if (FParse::Value(FCommandLine::Get(), TEXT("UserExperience="), UserExperienceFromCommandLine) ||
		FParse::Value(FCommandLine::Get(), TEXT("Playlist="), UserExperienceFromCommandLine))
	{
		UserExperienceId = FPrimaryAssetId::ParseTypeAndName(UserExperienceFromCommandLine);
		if (!UserExperienceId.PrimaryAssetType.IsValid())
		{
			UserExperienceId = FPrimaryAssetId(FPrimaryAssetType(UserExperienceType), FName(*UserExperienceFromCommandLine));
		}
	}

	// Search for the matching experience, it's fine to force load them because we're in dedicated server startup
	UADogAssetManager& AssetManager = UADogAssetManager::Get();
	TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAssetsWithType(UserExperienceType);
	if (ensure(Handle.IsValid()))
	{
		Handle->WaitUntilComplete();
	}

	TArray<UObject*> UserExperiences;
	AssetManager.GetPrimaryAssetObjectList(UserExperienceType, UserExperiences);
	UADogUserFacingExperienceDefinition* FoundExperience = nullptr;
	UADogUserFacingExperienceDefinition* DefaultExperience = nullptr;

	for (UObject* Object : UserExperiences)
	{
		UADogUserFacingExperienceDefinition* UserExperience = Cast<UADogUserFacingExperienceDefinition>(Object);
		if (ensure(UserExperience))
		{
			if (UserExperience->GetPrimaryAssetId() == UserExperienceId)
			{
				FoundExperience = UserExperience;
				break;
			}
			
			if (UserExperience->bIsDefaultExperience && DefaultExperience == nullptr)
			{
				DefaultExperience = UserExperience;
			}
		}
	}

	if (FoundExperience == nullptr)
	{
		FoundExperience = DefaultExperience;
	}
	
	UGameInstance* GameInstance = GetGameInstance();
	if (ensure(FoundExperience && GameInstance))
	{
		// Actually host the game
		UCommonSession_HostSessionRequest* HostRequest = FoundExperience->CreateHostingRequest(this);
		if (ensure(HostRequest))
		{
			HostRequest->OnlineMode = OnlineMode;

			// TODO override other parameters?

			UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>();
			SessionSubsystem->HostSession(nullptr, HostRequest);
			
			// This will handle the map travel
		}
	}

}

void AADogGameMode::OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		// Unbind
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
		UserSubsystem->OnUserInitializeComplete.RemoveDynamic(this, &AADogGameMode::OnUserInitializedForDedicatedServer);

		// Dedicated servers do not require user login, but some online subsystems may expect it
		if (bSuccess && ensure(UserInfo))
		{
			UE_LOG(LogADogExperience, Log, TEXT("Dedicated server user login succeeded for id %s, starting online server"), *UserInfo->GetNetId().ToString());
		}
		else
		{
			UE_LOG(LogADogExperience, Log, TEXT("Dedicated server user login unsuccessful, starting online server as login is not required"));
		}
		
		HostDedicatedServerMatch(ECommonSessionOnlineMode::Online);
	}
}

void AADogGameMode::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
	if (ExperienceId.IsValid())
	{
		UE_LOG(LogADogExperience, Log, TEXT("Identified experience %s (Source: %s)"), *ExperienceId.ToString(), *ExperienceIdSource);

		UADogExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UADogExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->SetCurrentExperience(ExperienceId);
	}
	else
	{
		UE_LOG(LogADogExperience, Error, TEXT("Failed to identify experience, loading screen will stay up forever"));
	}
}

void AADogGameMode::OnExperienceLoaded(const UADogExperienceDefinition* CurrentExperience)
{
	BEANS_ULOG(LogADog, VeryVerbose, TEXT("Experience loaded, attempting to restart players for experience: %s"), *GetNameSafe(CurrentExperience));
	
	// Spawn any players that are already attached
	//@TODO: Here we're handling only *player* controllers, but in GetDefaultPawnClassForController_Implementation we skipped all controllers
	// GetDefaultPawnClassForController_Implementation might only be getting called for players anyways
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				BEANS_ULOG(LogADog, VeryVerbose, TEXT("Player can restart so we're going to restart it for controller: %s"), *PC->PlayerState->GetPlayerName());
				RestartPlayer(PC);
			}
			BEANS_ULOG(LogADog, VeryVerbose, TEXT("Not restarting player yet for controller: %s"), *PC->PlayerState->GetPlayerName());
		}
	}
}

bool AADogGameMode::IsExperienceLoaded() const
{
	check(GameState);
	UADogExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UADogExperienceManagerComponent>();
	check(ExperienceComponent);

	return ExperienceComponent->IsExperienceLoaded();
}

UClass* AADogGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const UADogPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* AADogGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// Never save the default player pawns into a map.
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (AADogCharacter* SpawnedPawn = GetWorld()->SpawnActor<AADogCharacter>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (UADogPawnExtensionComponent* pawnExtComp = UADogPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const UADogPawnData* pawnData = GetPawnDataForController(NewPlayer))
				{
					pawnExtComp->SetPawnData(pawnData);
				}
			}
			
			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
		else
		{
			UE_LOG(LogADog, Error, TEXT("Game mode was unable to spawn Pawn of class [%s] at [%s]."), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
		}
	}
	else
	{
		UE_LOG(LogADog, Error, TEXT("Game mode was unable to spawn Pawn due to NULL pawn class."));
	}

	return nullptr;
}

bool AADogGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// We never want to use the start spot, always use the spawn management component.
	return false;
}

void AADogGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Delay starting new players until the experience has been loaded
	// (players who log in prior to that will be started by OnExperienceLoaded)
	if (IsExperienceLoaded())
	{
		BEANS_ULOG(LogADog, VeryVerbose, TEXT("Experience loaded, attempting to start new player for Controller: %s"), *NewPlayer->PlayerState->GetPlayerName());
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
		return;
	}
	BEANS_ULOG(LogADog, Warning, TEXT("Experience isn't loaded yet, not starting new player for Controller: %s"), *NewPlayer->PlayerState->GetPlayerName());
}

AActor* AADogGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	BEANS_ULOG(LogADog, VeryVerbose, TEXT("Choosing player start"));
	if (UADogPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UADogPlayerSpawningManagerComponent>())
	{
		BEANS_ULOG(LogADog, VeryVerbose, TEXT("Choosing player start"));
		return PlayerSpawningComponent->ChoosePlayerStart(Player);
	}
	
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AADogGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (UADogPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UADogPlayerSpawningManagerComponent>())
	{
		PlayerSpawningComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	}

	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool AADogGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	BEANS_ULOG(LogADog, VeryVerbose, TEXT("Checking if Player can restart for Controller: %s"), *Player->PlayerState->GetPlayerName());
	return ControllerCanRestart(Player);
}

bool AADogGameMode::ControllerCanRestart(AController* Controller)
{
	BEANS_ULOG(LogADog, VeryVerbose, TEXT("Checking if Controller can restart for Controller: %s"), *Controller->PlayerState->GetPlayerName());
	if (APlayerController* pc = Cast<APlayerController>(Controller))
	{
		BEANS_ULOG(LogADog, VeryVerbose, TEXT("Check parent class if we can restart for Controller: %s"), *Controller->PlayerState->GetPlayerName());
		if (!Super::PlayerCanRestart_Implementation(pc))
		{
			BEANS_ULOG(LogADog, VeryVerbose, TEXT("Parent class determines we can't restart for Controller: %s"), *Controller->PlayerState->GetPlayerName());
			return false;
		}
	}
	else
	{
		// Bot version of Super::PlayerCanRestart_Implementation
		if ((Controller == nullptr) || Controller->IsPendingKillPending())
		{
			BEANS_ULOG(LogADog, Warning, TEXT("Controller is null or IsPendingKillPending: %s"), *GetNameSafe(Controller));
			return false;
		}
	}
	
	UADogPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UADogPlayerSpawningManagerComponent>();
	if (!PlayerSpawningComponent)
	{
		BEANS_ULOG(LogADog, VeryVerbose, TEXT("No PlayerSpawningComponent present, determining that we can restart for: %s"), *Controller->PlayerState->GetPlayerName());
		return true;
	}

	BEANS_ULOG(LogADog, VeryVerbose, TEXT("GameMode is ok with restarting Player, escalate to the PlayerSpawningComponent for: %s"), *Controller->PlayerState->GetPlayerName());
	return PlayerSpawningComponent->ControllerCanRestart(Controller);
}

void AADogGameMode::InitGameState()
{
	Super::InitGameState();

	// Listen for the experience load to complete	
	UADogExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UADogExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnADogExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void AADogGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);

	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void AADogGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
	else if (AADogPlayerBotController* BotController = Cast<AADogPlayerBotController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(BotController, &AADogPlayerBotController::ServerRestartController);
	}
}

bool AADogGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	// Do nothing, we'll wait until PostLogin when we try to spawn the player for real.
	// Doing anything right now is no good, systems like team assignment haven't even occurred yet.
	return true;
}

void AADogGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	// If we tried to spawn a pawn and it failed, lets try again *note* check if there's actually a pawn class
	// before we try this forever.
	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer))
		{
			// If it's a player don't loop forever, maybe something changed and they can no longer restart if so stop trying.
			if (PlayerCanRestart(NewPC))
			{
				RequestPlayerRestartNextFrame(NewPlayer, false);			
			}
			else
			{
				UE_LOG(LogADog, Verbose, TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."), *GetPathNameSafe(NewPlayer));
			}
		}
		else
		{
			RequestPlayerRestartNextFrame(NewPlayer, false);
		}
	}
	else
	{
		UE_LOG(LogADog, Verbose, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."), *GetPathNameSafe(NewPlayer));
	}
}

