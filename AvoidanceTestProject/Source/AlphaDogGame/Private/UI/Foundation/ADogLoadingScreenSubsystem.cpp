// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Foundation/ADogLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogLoadingScreenSubsystem)

class UUserWidget;

//////////////////////////////////////////////////////////////////////
// UADogLoadingScreenSubsystem

UADogLoadingScreenSubsystem::UADogLoadingScreenSubsystem()
{
}

void UADogLoadingScreenSubsystem::SetLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
	if (LoadingScreenWidgetClass != NewWidgetClass)
	{
		LoadingScreenWidgetClass = NewWidgetClass;

		OnLoadingScreenWidgetChanged.Broadcast(LoadingScreenWidgetClass);
	}
}

TSubclassOf<UUserWidget> UADogLoadingScreenSubsystem::GetLoadingScreenContentWidget() const
{
	return LoadingScreenWidgetClass;
}

