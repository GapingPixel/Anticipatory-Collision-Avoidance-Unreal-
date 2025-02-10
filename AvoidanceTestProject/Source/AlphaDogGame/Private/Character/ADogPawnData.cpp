// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ADogPawnData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogPawnData)

UADogPawnData::UADogPawnData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PawnClass = nullptr;
	InputConfig = nullptr;
	DefaultCameraMode = nullptr;
}