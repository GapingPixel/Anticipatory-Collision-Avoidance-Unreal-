// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PerformanceStats/ADogPerfStatContainerBase.h"

#include "Blueprint/WidgetTree.h"
#include "UI/PerformanceStats/ADogPerfStatWidgetBase.h"
#include "Settings/ADogSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogPerfStatContainerBase)

//////////////////////////////////////////////////////////////////////
// UADogPerfStatsContainerBase

UADogPerfStatContainerBase::UADogPerfStatContainerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UADogPerfStatContainerBase::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateVisibilityOfChildren();

	UADogSettingsLocal::Get()->OnPerfStatDisplayStateChanged().AddUObject(this, &ThisClass::UpdateVisibilityOfChildren);
}

void UADogPerfStatContainerBase::NativeDestruct()
{
	UADogSettingsLocal::Get()->OnPerfStatDisplayStateChanged().RemoveAll(this);

	Super::NativeDestruct();
}

void UADogPerfStatContainerBase::UpdateVisibilityOfChildren()
{
	UADogSettingsLocal* UserSettings = UADogSettingsLocal::Get();

	const bool bShowTextWidgets = (StatDisplayModeFilter == EADogStatDisplayMode::TextOnly) || (StatDisplayModeFilter == EADogStatDisplayMode::TextAndGraph);
	const bool bShowGraphWidgets = (StatDisplayModeFilter == EADogStatDisplayMode::GraphOnly) || (StatDisplayModeFilter == EADogStatDisplayMode::TextAndGraph);
	
	check(WidgetTree);
	WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		if (UADogPerfStatWidgetBase* TypedWidget = Cast<UADogPerfStatWidgetBase>(Widget))
		{
			const EADogStatDisplayMode SettingMode = UserSettings->GetPerfStatDisplayState(TypedWidget->GetStatToDisplay());

			bool bShowWidget = false;
			switch (SettingMode)
			{
			case EADogStatDisplayMode::Hidden:
				bShowWidget = false;
				break;
			case EADogStatDisplayMode::TextOnly:
				bShowWidget = bShowTextWidgets;
				break;
			case EADogStatDisplayMode::GraphOnly:
				bShowWidget = bShowGraphWidgets;
				break;
			case EADogStatDisplayMode::TextAndGraph:
				bShowWidget = bShowTextWidgets || bShowGraphWidgets;
				break;
			}

			TypedWidget->SetVisibility(bShowWidget ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	});
}

