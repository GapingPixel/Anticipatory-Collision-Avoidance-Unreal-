// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModularGameState.h"

#include "ADogGameState.generated.h"

struct FADogVerbMessage;

class APlayerState;
//class UAbilitySystemComponent;
//class UADogAbilitySystemComponent;
class UADogExperienceManagerComponent;
class UObject;
struct FFrame;

/**
 * AADogGameState
 *
 *	The base game state class used by this project.
 *
 *	TODO Implement alternate phase system
 *	Since the GMC_AbilitySystem won't work on the GameState without an update or large refactor, all the
 *	AbilitySystem related code has been commented out. An alternative for the phasing system will have to be
 *	implemented at another time.
 */
UCLASS(Config = Game)
class ALPHADOGGAME_API AADogGameState : public AModularGameStateBase//, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	AADogGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	/*//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	// Gets the ability system component used for game wide things
	UFUNCTION(BlueprintCallable, Category = "AlphaDog|GameState")
	UADogAbilitySystemComponent* GetADogAbilitySystemComponent() const { return AbilitySystemComponent; }*/

	// Send a message that all clients will (probably) get
	// (use only for client notifications like eliminations, server join messages, etc... that can handle being lost)
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "AlphaDog|GameState")
	void MulticastMessageToClients(const FADogVerbMessage Message);

	// Send a message that all clients will be guaranteed to get
	// (use only for client notifications that cannot handle being lost)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "AlphaDog|GameState")
	void MulticastReliableMessageToClients(const FADogVerbMessage Message);

	// Gets the server's FPS, replicated to clients
	float GetServerFPS() const;

	// Indicate the local player state is recording a replay
	void SetRecorderPlayerState(APlayerState* NewPlayerState);

	// Gets the player state that recorded the replay, if valid
	APlayerState* GetRecorderPlayerState() const;

	// Delegate called when the replay player state changes
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRecorderPlayerStateChanged, APlayerState*);
	FOnRecorderPlayerStateChanged OnRecorderPlayerStateChangedEvent;

private:
	// Handles loading and managing the current gameplay experience
	UPROPERTY()
	TObjectPtr<UADogExperienceManagerComponent> ExperienceManagerComponent;

	/*// The ability system component subobject for game-wide things (primarily gameplay cues)
	UPROPERTY(VisibleAnywhere, Category = "AlphaDog|GameState")
	TObjectPtr<UADogAbilitySystemComponent> AbilitySystemComponent;*/

protected:
	UPROPERTY(Replicated)
	float ServerFPS;

	// The player state that recorded a replay, it is used to select the right pawn to follow
	// This is only set in replay streams and is not replicated normally
	UPROPERTY(Transient, ReplicatedUsing = OnRep_RecorderPlayerState)
	TObjectPtr<APlayerState> RecorderPlayerState;

	UFUNCTION()
	void OnRep_RecorderPlayerState();

};
