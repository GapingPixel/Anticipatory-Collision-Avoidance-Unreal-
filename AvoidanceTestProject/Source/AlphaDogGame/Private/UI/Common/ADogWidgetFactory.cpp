// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Common/ADogWidgetFactory.h"
#include "Templates/SubclassOf.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogWidgetFactory)

class UUserWidget;

TSubclassOf<UUserWidget> UADogWidgetFactory::FindWidgetClassForData_Implementation(const UObject* Data) const
{
	return TSubclassOf<UUserWidget>();
}
