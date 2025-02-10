// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ADogEditorStatics.generated.h"

/**
 * 
 */
UCLASS(MinimalAPI)
class UADogEditorStatics : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	// This function tells the GameplayCue editor what classes to expose when creating new notifies.
	static void GetGameplayCueDefaultClasses(TArray<UClass*>& Classes);

	// This function tells the GameplayCue editor what classes to search for GameplayCue events.
	static void GetGameplayCueInterfaceClasses(TArray<UClass*>& Classes);

	// This function tells the GameplayCue editor where to create the GameplayCue notifies based on their tag.
	static FString GetGameplayCuePath(FString GameplayCueTag);

	static bool HasPlayWorld();
	
	static bool HasNoPlayWorld();

	static bool HasPlayWorldAndRunning();

	static void OpenCommonMap_Clicked(const FString MapPath);

	static bool CanShowCommonMaps();

	static TSharedRef<SWidget> GetCommonMapsDropdown();

	static void CheckGameContent_Clicked();

	static void RegisterGameEditorMenus();
};
