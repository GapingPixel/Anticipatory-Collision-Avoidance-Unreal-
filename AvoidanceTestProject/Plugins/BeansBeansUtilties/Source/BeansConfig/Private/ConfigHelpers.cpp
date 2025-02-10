// Fill out your copyright notice in the Description page of Project Settings.


#include "ConfigHelpers.h"

FString UConfigHelpers::GetGameSetting(FString Section, FString Key)
{
	FString Value;
	GConfig->GetString(
		*Section,
		*Key,
		Value,
		GGameIni
	);
	return Value;
}
