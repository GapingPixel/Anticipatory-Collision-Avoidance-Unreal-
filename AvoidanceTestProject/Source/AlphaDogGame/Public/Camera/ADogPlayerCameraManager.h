// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/PlayerCameraManager.h"

#include "ADogPlayerCameraManager.generated.h"

class FDebugDisplayInfo;
class UCanvas;
class UObject;


#define ADOG_CAMERA_DEFAULT_FOV			(80.0f)
#define ADOG_CAMERA_DEFAULT_PITCH_MIN	(-89.0f)
#define ADOG_CAMERA_DEFAULT_PITCH_MAX	(89.0f)

class UADogUICameraManagerComponent;

/**
 * AADogPlayerCameraManager
 *
 *	The base player camera manager class used by this project.
 */
UCLASS(notplaceable, MinimalAPI)
class AADogPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	AADogPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	UADogUICameraManagerComponent* GetUICameraComponent() const;

protected:

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

private:
	/** The UI Camera Component, controls the camera when UI is doing something important that gameplay doesn't get priority over. */
	UPROPERTY(Transient)
	TObjectPtr<UADogUICameraManagerComponent> UICamera;

	// Primarily needed to keep track of initial spawn so we can hide loading screen when camera isn't in 0,0,0
	UPROPERTY()
	bool bInitialSpawn;
};
