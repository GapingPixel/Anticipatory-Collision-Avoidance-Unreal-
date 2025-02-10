// Copyright Epic Games, Inc. All Rights Reserved.

#include "BeansIndicatorManagerComponent.h"

#include "IndicatorDescriptor.h"
#include "IndicatorLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BeansIndicatorManagerComponent)

UBeansIndicatorManagerComponent::UBeansIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
}

/*static*/ UBeansIndicatorManagerComponent* UBeansIndicatorManagerComponent::GetComponent(AController* Controller)
{
	if (Controller)
	{
		return Controller->FindComponentByClass<UBeansIndicatorManagerComponent>();
	}

	return nullptr;
}

void UBeansIndicatorManagerComponent::AddIndicator(UIndicatorDescriptor* IndicatorDescriptor)
{
	IndicatorDescriptor->SetIndicatorManagerComponent(this);
	OnIndicatorAdded.Broadcast(IndicatorDescriptor);
	Indicators.Add(IndicatorDescriptor);
}

void UBeansIndicatorManagerComponent::RemoveIndicator(UIndicatorDescriptor* IndicatorDescriptor)
{
	if (IndicatorDescriptor)
	{
		ensure(IndicatorDescriptor->GetIndicatorManagerComponent() == this);
	
		OnIndicatorRemoved.Broadcast(IndicatorDescriptor);
		Indicators.Remove(IndicatorDescriptor);
	}
}
