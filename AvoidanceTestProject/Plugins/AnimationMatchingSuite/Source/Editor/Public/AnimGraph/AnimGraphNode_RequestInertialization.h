// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraph/AnimNode_RequestInertialization.h"
#include "AnimGraphNode_RequestInertialization.generated.h"

/**
 * 
 */
UCLASS(MinimalAPI)
class UAnimGraphNode_RequestInertialization : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_RequestInertialization Node;

	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	virtual FString GetNodeCategory() const override;
	virtual void GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const override;
};
