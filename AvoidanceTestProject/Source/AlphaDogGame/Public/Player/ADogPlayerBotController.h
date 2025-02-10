// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModularAIController.h"
#include "Teams/ADogTeamAgentInterface.h"

#include "ADogPlayerBotController.generated.h"

class AADogMoveTarget;

namespace ETeamAttitude { enum Type : int; }
struct FGenericTeamId;

class APlayerState;
class UAIPerceptionComponent;
class UADogExperienceDefinition;
class UObject;
struct FFrame;

/**
 * AADogPlayerBotController
 *
 *	The controller class used by player bots in this project.
 */
UCLASS(Blueprintable)
class ALPHADOGGAME_API AADogPlayerBotController : public AModularAIController, public IADogTeamAgentInterface
{
	GENERATED_BODY()

public:
	AADogPlayerBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	void OnExperienceLoaded(const UADogExperienceDefinition* Experience);

	//~IADogTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnADogTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~End of IADogTeamAgentInterface interface

	// Attempts to restart this controller (e.g., to respawn it)
	void ServerRestartController();

	//Update Team Attitude for the AI
	UFUNCTION(BlueprintCallable, Category = "AlphaDog AI Player Controller")
	void UpdateTeamAttitude(UAIPerceptionComponent* AIPerception);

	UFUNCTION(BlueprintCallable)
	void IgnoreDetourAgents();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	AADogMoveTarget* GetMoveTarget() { return MoveTarget; }

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

protected:
	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

	virtual void TryRunPostReady();

private:
	void BroadcastOnPlayerStateChanged();

protected:	
	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	
	//~End of AController interface

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	TObjectPtr<UBehaviorTree> BTAsset;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AADogMoveTarget> MoveTarget;

private:
	UPROPERTY()
	FOnADogTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
	
	bool bExperienceLoaded;

};
