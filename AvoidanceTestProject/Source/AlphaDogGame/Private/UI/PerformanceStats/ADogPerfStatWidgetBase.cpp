// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PerformanceStats/ADogPerfStatWidgetBase.h"

#include "Engine/GameInstance.h"
#include "Performance/ADogPerformanceStatSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogPerfStatWidgetBase)

//////////////////////////////////////////////////////////////////////
// UADogPerfStatWidgetBase

UADogPerfStatWidgetBase::UADogPerfStatWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

double UADogPerfStatWidgetBase::FetchStatValue()
{
	if (CachedStatSubsystem == nullptr)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				CachedStatSubsystem = GameInstance->GetSubsystem<UADogPerformanceStatSubsystem>();
			}
		}
	}

	if (CachedStatSubsystem)
	{
		return CachedStatSubsystem->GetCachedStat(StatToDisplay);
	}
	else
	{
		return 0.0;
	}
}

