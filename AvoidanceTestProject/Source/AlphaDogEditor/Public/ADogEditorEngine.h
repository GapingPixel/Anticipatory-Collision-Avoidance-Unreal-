// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Editor/UnrealEdEngine.h"
#include "ADogEditorEngine.generated.h"

class IEngineLoop;
class UObject;

/**
 * 
 */
UCLASS()
class ALPHADOGEDITOR_API UADogEditorEngine : public UUnrealEdEngine
{
	GENERATED_BODY()

public:

	UADogEditorEngine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void Init(IEngineLoop* InEngineLoop) override;
	virtual void Start() override;
	virtual void Tick(float DeltaSeconds, bool bIdleMode) override;
	
	virtual FGameInstancePIEResult PreCreatePIEInstances(const bool bAnyBlueprintErrors, const bool bStartInSpectatorMode, const float PIEStartTime, const bool bSupportsOnlinePIE, int32& InNumOnlinePIEInstances) override;

private:
	void FirstTickSetup();
	
	bool bFirstTickSetup = false;
};
