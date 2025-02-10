// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/ADogCameraAssistInterface.h"
#include "CommonPlayerController.h"
#include "Teams/ADogTeamAgentInterface.h"

#include "ADogPlayerController.generated.h"

struct FGenericTeamId;

class ULoadingProcessTask;
class AADogHUD;
class AADogPlayerState;
class APawn;
class APlayerState;
class FPrimitiveComponentId;
class IInputInterface;
class UADogAbilitySystemComponent;
class UADogSettingsShared;
class UObject;
class UPlayer;
struct FFrame;

/**
 * AADogPlayerController
 *
 *	The base player controller class used by this project.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class ALPHADOGGAME_API AADogPlayerController : public ACommonPlayerController, public IADogCameraAssistInterface, public IADogTeamAgentInterface
{
	GENERATED_BODY()

	public:

	AADogPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "AlphaDog|PlayerController")
	AADogPlayerState* GetADogPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "AlphaDog|PlayerController")
	UADogAbilitySystemComponent* GetADogAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = "AlphaDog|PlayerController")
	AADogHUD* GetADogHUD() const;

	// Run a cheat command on the server.
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheat(const FString& Msg);

	// Run a cheat command on the server for all players.
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheatAll(const FString& Msg);

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetPlayer(UPlayer* InPlayer) override;
	virtual void AddCheats(bool bForce) override;
	virtual void UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId) override;
	virtual void UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents) override;
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface

	//~IADogCameraAssistInterface interface
	virtual void OnCameraPenetratingTarget() override;
	//~End of IADogCameraAssistInterface interface
	
	//~IADogTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnADogTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IADogTeamAgentInterface interface

	// TODO is that actually needed? I think this was added to keep a loading screen up until the character was ready
	void RegisterLoadingProcessTask();
	// Primarily used to coordinate initial spawn with the loading screen task
	void SetInitialSpawn();

private:
	UPROPERTY()
	FOnADogTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	UPROPERTY()
	TObjectPtr<ULoadingProcessTask> InitialSpawnLoadingTask;

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

protected:
	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();

protected:

	//~APlayerController interface

	//~End of APlayerController interface

	void OnSettingsChanged(UADogSettingsShared* Settings);

	// Should we hide the 
	bool bHideViewTargetPawnNextFrame = false;
};

