// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "IndicatorLibrary.generated.h"

class AController;
class UBeansIndicatorManagerComponent;
class UObject;
struct FFrame;

UCLASS()
class BEANSINDICATORS_API UIndicatorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UIndicatorLibrary();
	
	/**  */
	UFUNCTION(BlueprintCallable, Category = Indicator)
	static UBeansIndicatorManagerComponent* GetIndicatorManagerComponent(AController* Controller);
};
