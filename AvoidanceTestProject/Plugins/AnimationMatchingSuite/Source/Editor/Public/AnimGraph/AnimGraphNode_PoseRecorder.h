// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraph/AnimNode_PoseRecorder.h"
#include "AnimGraphNode_PoseRecorder.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_PoseRecorder : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_PoseRecorder Node;

	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	virtual FString GetNodeCategory() const override;
};
