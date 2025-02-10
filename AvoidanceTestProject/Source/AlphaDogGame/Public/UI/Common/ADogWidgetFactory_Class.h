// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ADogWidgetFactory.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"

#include "ADogWidgetFactory_Class.generated.h"

class UObject;
class UUserWidget;

UCLASS()
class ALPHADOGGAME_API UADogWidgetFactory_Class : public UADogWidgetFactory
{
	GENERATED_BODY()

public:
	UADogWidgetFactory_Class() { }

	virtual TSubclassOf<UUserWidget> FindWidgetClassForData_Implementation(const UObject* Data) const override;
	
protected:
	UPROPERTY(EditAnywhere, Category = ListEntries, meta = (AllowAbstract))
	TMap<TSoftClassPtr<UObject>, TSubclassOf<UUserWidget>> EntryWidgetForClass;
};
