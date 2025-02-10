// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BeansWidgetPositioning.generated.h"

/**
 * 
 */
UCLASS()
class BEANSWIDGET_API UBeansWidgetPositioning : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/*
	*	Retrieves a position for a widget which is within visible bounds of viewport
	*	Essentially used to make sure a widget isn't displayed outside viewport bounds
	*	Tolerance represents the percentage of the widget that is forced to be displayed in viewport bounds.
	*	A value of 10 for example would force 90% of the widget to be displayed in viewport.
	*/ 
	UFUNCTION(BlueprintCallable, Category = "Beans|Widget")
	static FVector2D GetBoundedWidgetPosition(APlayerController* PlayerController, const FVector2D& WidgetResolution, float Tolerance);
	
	static FVector2D CalculateBounds(const FVector2D& MousePosition, const FVector2D& ViewportSize, const FVector2D& WidgetResolution,
		const float ResolutionScale, float Tolerance = 100.0);
};
