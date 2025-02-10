// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/ADogUICameraManagerComponent.h"

#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Camera/ADogPlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogUICameraManagerComponent)

class AActor;
class FDebugDisplayInfo;

UADogUICameraManagerComponent* UADogUICameraManagerComponent::GetComponent(APlayerController* PC)
{
	if (PC != nullptr)
	{
		if (AADogPlayerCameraManager* PCCamera = Cast<AADogPlayerCameraManager>(PC->PlayerCameraManager))
		{
			return PCCamera->GetUICameraComponent();
		}
	}

	return nullptr;
}

UADogUICameraManagerComponent::UADogUICameraManagerComponent()
{
	bWantsInitializeComponent = true;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Register "showdebug" hook.
		if (!IsRunningDedicatedServer())
		{
			AHUD::OnShowDebugInfo.AddUObject(this, &ThisClass::OnShowDebugInfo);
		}
	}
}

void UADogUICameraManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UADogUICameraManagerComponent::SetViewTarget(AActor* InViewTarget, FViewTargetTransitionParams TransitionParams)
{
	TGuardValue<bool> UpdatingViewTargetGuard(bUpdatingViewTarget, true);

	ViewTarget = InViewTarget;
	CastChecked<AADogPlayerCameraManager>(GetOwner())->SetViewTarget(ViewTarget, TransitionParams);
}

bool UADogUICameraManagerComponent::NeedsToUpdateViewTarget() const
{
	return false;
}

void UADogUICameraManagerComponent::UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime)
{
}

void UADogUICameraManagerComponent::OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
}
