// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConfigHelpers.generated.h"

// Primarily used to send config data through GameplayMessageSystem
USTRUCT(Blueprintable)
struct BEANSCONFIG_API FBeansConfigValue
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	FString Key;

	UPROPERTY(BlueprintReadWrite)
	FString Value;
};

/**
 * 
 */
UCLASS()
class BEANSCONFIG_API UConfigHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/*
	 * Access value of Key in Section in DefaultGame.ini
	 * Ex: FString ProjectVersion = GetGameSetting("/Script/EngineSettings.GeneralProjectSettings", "ProjectVersion")
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "GetGameSetting"), Category = "Beans|Config")
	static FString GetGameSetting(FString Section, FString Key);
};
