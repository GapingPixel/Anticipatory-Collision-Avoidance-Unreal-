// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameSession.h"

#include "ADogGameSession.generated.h"

class UObject;

UCLASS(Config = Game)
class ALPHADOGGAME_API AADogGameSession : public AGameSession
{
	GENERATED_BODY()

public:

	AADogGameSession(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	/** Override to disable the default behavior */
	virtual bool ProcessAutoLogin() override;

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
};
