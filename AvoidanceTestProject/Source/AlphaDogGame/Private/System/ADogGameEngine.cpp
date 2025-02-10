// Fill out your copyright notice in the Description page of Project Settings.


#include "System/ADogGameEngine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogGameEngine)

class IEngineLoop;


UADogGameEngine::UADogGameEngine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UADogGameEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);
}