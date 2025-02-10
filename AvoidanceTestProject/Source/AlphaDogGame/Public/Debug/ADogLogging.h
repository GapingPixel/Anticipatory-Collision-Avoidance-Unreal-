// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Logging/LogMacros.h"
#include "BeansLoggingUtils.h"

class UObject;

ALPHADOGGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogADog, VeryVerbose, All);
ALPHADOGGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogADogExperience, Log, All);
ALPHADOGGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogADogAbilitySystem, Log, All);
ALPHADOGGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogADogTeams, Log, All);
ALPHADOGGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogADogGameSettingRegistry, Log, All);

ALPHADOGGAME_API FString GetClientServerContextString(UObject* ContextObject = nullptr);

