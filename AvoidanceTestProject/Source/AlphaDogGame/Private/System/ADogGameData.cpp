// Fill out your copyright notice in the Description page of Project Settings.


#include "System/ADogGameData.h"
#include "System/ADogAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogGameData)

UADogGameData::UADogGameData()
{
}

const UADogGameData& UADogGameData::UADogGameData::Get()
{
	return UADogAssetManager::Get().GetGameData();
}