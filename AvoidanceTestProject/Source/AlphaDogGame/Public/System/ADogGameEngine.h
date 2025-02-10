// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameEngine.h"

#include "ADogGameEngine.generated.h"

class IEngineLoop;
class UObject;

UCLASS()
class ALPHADOGGAME_API UADogGameEngine : public UGameEngine
{
	GENERATED_BODY()
	
public:

	UADogGameEngine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void Init(IEngineLoop* InEngineLoop) override;
};
