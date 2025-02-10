// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "OUUTestObject.generated.h"

/** Used in place of plain UObject for tests, because UObject itself is abstract */
UCLASS(hidedropdown)
class BEANSTESTUTILITIES_API UOUUTestObject : public UObject
{
	GENERATED_BODY()
};

/** Used in place of plain UUserWidget for tests, because UUserWidget itself is abstract */
UCLASS(hidedropdown)
class BEANSTESTUTILITIES_API UOUUTestWidget : public UUserWidget
{
	GENERATED_BODY()
};
