// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

#include "ADogCameraComponent.generated.h"

class UCanvas;
class UADogCameraMode;
class UADogCameraModeStack;
class UObject;
struct FFrame;
struct FGameplayTag;
struct FMinimalViewInfo;
template <class TClass> class TSubclassOf;

DECLARE_DELEGATE_RetVal(TSubclassOf<UADogCameraMode>, FADogCameraModeDelegate);


/**
 * UADogCameraComponent
 *
 *	The base camera component class used by this project.
 */
UCLASS()
class UADogCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:

	UADogCameraComponent(const FObjectInitializer& ObjectInitializer);

	// Returns the camera component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "AlphaDog|Camera")
	static UADogCameraComponent* FindCameraComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UADogCameraComponent>() : nullptr); }

	// Camera mode that will be used before the CameraMode from PawnData is added to the stack. This helps avoid
	// flickering at world location 0,0,0 or hard transitions from existing CameraComponent
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CameraModeStack")
	TSubclassOf<UADogCameraMode> DefaultCameraMode;
	
	// Returns the target actor that the camera is looking at.
	virtual AActor* GetTargetActor() const { return GetOwner(); }

	// Delegate used to query for the best camera mode.
	FADogCameraModeDelegate DetermineCameraModeDelegate;

	// Add an offset to the field of view.  The offset is only for one frame, it gets cleared once it is applied.
	void AddFieldOfViewOffset(float FovOffset) { FieldOfViewOffset += FovOffset; }

	virtual void DrawDebug(UCanvas* Canvas) const;

	// Gets the tag associated with the top layer and the blend weight of it
	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;

protected:

	virtual void OnRegister() override;
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	virtual void UpdateCameraModes();

protected:

	// Stack used to blend the camera modes.
	UPROPERTY()
	TObjectPtr<UADogCameraModeStack> CameraModeStack;

	// Offset applied to the field of view.  The offset is only for one frame, it gets cleared once it is applied.
	float FieldOfViewOffset;

};
