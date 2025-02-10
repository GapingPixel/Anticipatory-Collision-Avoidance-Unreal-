// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AlphaDogEditorStatics.generated.h"

/**
 * 
 */
UCLASS(MinimalAPI)
class UAlphaDogEditorStatics : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	static bool HasPlayWorld();
	
	static bool HasNoPlayWorld();

	static bool HasPlayWorldAndRunning();

	static void OpenCommonMap_Clicked(const FString MapPath);

	static bool CanShowCommonMaps();

	static TSharedRef<SWidget> GetCommonMapsDropdown();

	static void CheckGameContent_Clicked();

	static void RegisterGameEditorMenus();
};
