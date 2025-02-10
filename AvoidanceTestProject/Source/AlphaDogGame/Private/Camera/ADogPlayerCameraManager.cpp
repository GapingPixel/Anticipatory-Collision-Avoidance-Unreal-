// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/ADogPlayerCameraManager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/ADogCameraComponent.h"
#include "Camera/ADogUICameraManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogPlayerCameraManager)

class FDebugDisplayInfo;

static FName UICameraComponentName(TEXT("UICamera"));

AADogPlayerCameraManager::AADogPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = ADOG_CAMERA_DEFAULT_FOV;
	ViewPitchMin = ADOG_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = ADOG_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UADogUICameraManagerComponent>(UICameraComponentName);
}

UADogUICameraManagerComponent* AADogPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void AADogPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// If the UI Camera is looking at something, let it have priority.
	if (UICamera->NeedsToUpdateViewTarget())
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
		UICamera->UpdateViewTarget(OutVT, DeltaTime);
		return;
	}

	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void AADogPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("ADogPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UADogCameraComponent* CameraComponent = UADogCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}

