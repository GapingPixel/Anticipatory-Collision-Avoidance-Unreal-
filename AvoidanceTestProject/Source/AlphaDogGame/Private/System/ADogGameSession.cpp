// Fill out your copyright notice in the Description page of Project Settings.


#include "System/ADogGameSession.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogGameSession)


AADogGameSession::AADogGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool AADogGameSession::ProcessAutoLogin()
{
	// This is actually handled in ADogGameMode::TryDedicatedServerLogin
	return true;
}

void AADogGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void AADogGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}
