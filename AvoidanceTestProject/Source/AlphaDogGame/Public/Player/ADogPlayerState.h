// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include "AbilitySystemInterface.h"
#include "GameplayTagStack.h"
#include "ModularPlayerState.h"
#include "LoadingProcessInterface.h"
#include "Teams/ADogTeamAgentInterface.h"

#include "ADogPlayerState.generated.h"

class ULoadingProcessTask;
class UADogAbilitySet;
struct FADogVerbMessage;

class AController;
class AADogPlayerController;
class APlayerState;
class FName;
class UAbilitySystemComponent;
class UADogAbilitySystemComponent;
class UADogExperienceDefinition;
class UADogPawnData;
class UObject;
struct FFrame;
struct FGameplayTag;

/** Defines the types of client connected */
UENUM()
enum class EADogPlayerConnectionType : uint8
{
	// An active player
	Player = 0,

	// Spectator connected to a running game
	LiveSpectator,

	// Spectating a demo recording offline
	ReplaySpectator,

	// A deactivated player (disconnected)
	InactivePlayer
};

/**
 * AADogPlayerState
 *
 *	Base player state class used by this project.
 */
UCLASS(Config = Game)
class ALPHADOGGAME_API AADogPlayerState : public AModularPlayerState, public IADogTeamAgentInterface
{
	GENERATED_BODY()

	public:
	AADogPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "AlphaDog|PlayerState")
	AADogPlayerController* GetADogPlayerController() const;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	void SetPawnData(const UADogPawnData* InPawnData);
	
	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~End of APlayerState interface

	//~IADogTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnADogTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IADogTeamAgentInterface interface
	
	void SetPlayerConnectionType(EADogPlayerConnectionType NewType);
	EADogPlayerConnectionType GetPlayerConnectionType() const { return MyPlayerConnectionType; }

	/** Returns the Team ID of the team the player belongs to. */
	UFUNCTION(BlueprintCallable)
	int32 GetTeamId() const
	{
		return GenericTeamIdToInteger(MyTeamID);
	}

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool HasStatTag(FGameplayTag Tag) const;

	// Send a message to just this player
	// (use only for client notifications like accolades, quest toasts, etc... that can handle being occasionally lost)
	UFUNCTION(Client, Unreliable, BlueprintCallable, Category = "AlphaDog|PlayerState")
	void ClientBroadcastMessage(const FADogVerbMessage Message);

private:
	void OnExperienceLoaded(const UADogExperienceDefinition* CurrentExperience);

protected:

	// The PawnData is required before a Character can be spawned in order to initialize the movement and ability
	// components correctly. Client will call ClientInitialized() to tell server its done.
	UFUNCTION()
	void OnRep_PawnData();
	
protected:

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UADogPawnData> PawnData;

private:

	UPROPERTY(Replicated)
	EADogPlayerConnectionType MyPlayerConnectionType;

	UPROPERTY()
	FOnADogTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY(ReplicatedUsing=OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	UPROPERTY()
	TObjectPtr<ULoadingProcessTask> InitialSpawnLoadingTask;

private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);
	
};
