// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "BeansContextEffectsInterface.generated.h"

class UAnimSequenceBase;
class UObject;
class USceneComponent;
struct FFrame;

/**
 *
 */
UENUM()
enum EEffectsContextMatchType: int
{
	ExactMatch,
	BestMatch
};

/**
 *
 */
 UINTERFACE(Blueprintable)
 class BEANSCONTEXTEFFECTS_API UBeansContextEffectsInterface : public UInterface
 {
	 GENERATED_BODY()

 };
 
 class BEANSCONTEXTEFFECTS_API IBeansContextEffectsInterface : public IInterface
 {
	 GENERATED_BODY()

 public:

	/** */
 	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AnimMotionEffect(const FName Bone
		, const FGameplayTag MotionEffect
		, USceneComponent* StaticMeshComponent
		, const FVector LocationOffset
		, const FRotator RotationOffset
		, const UAnimSequenceBase* AnimationSequence
		, const bool bHitSuccess
		, const FHitResult HitResult
		, FGameplayTagContainer Contexts
		, FVector VFXScale = FVector(1)
		, float AudioVolume = 1
		, float AudioPitch = 1);
 };

