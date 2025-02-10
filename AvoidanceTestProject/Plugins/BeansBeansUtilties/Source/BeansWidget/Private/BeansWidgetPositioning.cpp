// Fill out your copyright notice in the Description page of Project Settings.


#include "BeansWidgetPositioning.h"
#include "CoreMinimal.h"
#include "Engine/UserInterfaceSettings.h"
#include "Kismet/KismetMathLibrary.h"

FVector2D UBeansWidgetPositioning::GetBoundedWidgetPosition(APlayerController* PlayerController, const FVector2D& WidgetResolution, float Tolerance)
{
	FVector2D mousePos;
	PlayerController->GetMousePosition(mousePos.X, mousePos.Y);
	int32 resX = 0, resY = 0;
	PlayerController->GetViewportSize(resX, resY);
	const FVector2D viewportSize = { float(resX), float(resY)};
	const float resolutionScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(viewportSize.X, viewportSize.Y));

	return CalculateBounds(mousePos, viewportSize, WidgetResolution, resolutionScale, Tolerance);
}

FVector2D UBeansWidgetPositioning::CalculateBounds(const FVector2D& MousePosition, const FVector2D& ViewportSize,
	const FVector2D& WidgetResolution, const float ResolutionScale, float Tolerance)
{
	const FVector2D scaledViewport = UKismetMathLibrary::Divide_Vector2DVector2D(ViewportSize, {ResolutionScale, ResolutionScale});
	const FVector2D scaledMousePosition = UKismetMathLibrary::Divide_Vector2DVector2D(MousePosition, {ResolutionScale, ResolutionScale});
	FVector2D widgetPos = scaledMousePosition;

	const float clampedTolerance = FMath::Clamp(Tolerance, 0.0f, 100.0f);
	const float offset = (clampedTolerance * 0.01f) * 0.5f;
	
	// test top and left bounds of viewport
	const FVector2D topLeft = scaledMousePosition - WidgetResolution * offset;
	if (topLeft.X < 0.0f) // if we get a negative number we know widget will be partially off viewport
		{
		widgetPos.X = FMath::Abs(topLeft.X) + widgetPos.X;
		}
	if (topLeft.Y < 0.0f) // if we get a negative number we know widget will be partially off viewport
		{
		widgetPos.Y = FMath::Abs(topLeft.Y) + widgetPos.Y;
		}
	
	// test bottom and right bounds of viewport
	const FVector2D botRight = (scaledMousePosition + (WidgetResolution * offset)) - scaledViewport;
	if (botRight.X > 0.0f) // if we get a positive number we know widget will be partially off viewport
		{
		widgetPos.X = widgetPos.X - botRight.X;
		}
	if (botRight.Y > 0.0f) // if we get a positive number we know widget will be partially off viewport
		{
		widgetPos.Y = widgetPos.Y - botRight.Y;
		}

	return widgetPos;
}
