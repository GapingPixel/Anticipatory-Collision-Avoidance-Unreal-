// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/ADogLogging.h"

DEFINE_LOG_CATEGORY(LogADog);
DEFINE_LOG_CATEGORY(LogADogExperience);
DEFINE_LOG_CATEGORY(LogADogAbilitySystem);
DEFINE_LOG_CATEGORY(LogADogTeams);
DEFINE_LOG_CATEGORY(LogADogGameSettingRegistry);

FString GetClientServerContextString(UObject* ContextObject)
{
	ENetRole Role = ROLE_None;

	if (AActor* Actor = Cast<AActor>(ContextObject))
	{
		Role = Actor->GetLocalRole();
	}
	else if (UActorComponent* Component = Cast<UActorComponent>(ContextObject))
	{
		Role = Component->GetOwnerRole();
	}

	if (Role != ROLE_None)
	{
		return (Role == ROLE_Authority) ? TEXT("Server") : TEXT("Client");
	}
	else
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			extern ENGINE_API FString GPlayInEditorContextString;
			return GPlayInEditorContextString;
		}
#endif
	}

	return TEXT("[]");
}
