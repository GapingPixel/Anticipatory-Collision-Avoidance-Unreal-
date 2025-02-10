// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Common/ADogTabButtonBase.h"

#include "CommonLazyImage.h"
#include "..\..\..\Public\UI\Common\ADogTabListWidgetBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogTabButtonBase)

class UObject;
struct FSlateBrush;

void UADogTabButtonBase::SetIconFromLazyObject(TSoftObjectPtr<UObject> LazyObject)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrushFromLazyDisplayAsset(LazyObject);
	}
}

void UADogTabButtonBase::SetIconBrush(const FSlateBrush& Brush)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrush(Brush);
	}
}

void UADogTabButtonBase::SetTabLabelInfo_Implementation(const FADogTabDescriptor& TabLabelInfo)
{
	SetButtonText(TabLabelInfo.TabText);
	SetIconBrush(TabLabelInfo.IconBrush);
}

