// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "Performance/ADogPerformanceStatTypes.h"

#include "ADogPerfStatContainerBase.generated.h"

class UObject;
struct FFrame;

/**
 * UADogPerfStatsContainerBase
 *
 * Panel that contains a set of UADogPerfStatWidgetBase widgets and manages
 * their visibility based on user settings.
 */
 UCLASS(Abstract)
class UADogPerfStatContainerBase : public UCommonUserWidget
{
public:
	UADogPerfStatContainerBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	GENERATED_BODY()

	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~End of UUserWidget interface

	UFUNCTION(BlueprintCallable)
	void UpdateVisibilityOfChildren();

protected:
	// Are we showing text or graph stats?
	UPROPERTY(EditAnywhere, Category=Display)
	EADogStatDisplayMode StatDisplayModeFilter = EADogStatDisplayMode::TextAndGraph;
};
