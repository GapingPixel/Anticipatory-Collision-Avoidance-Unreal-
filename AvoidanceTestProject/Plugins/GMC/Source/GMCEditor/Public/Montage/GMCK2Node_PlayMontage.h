// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "K2Node_BaseAsyncTask.h"
#include "Internationalization/Text.h"
#include "GMCK2Node_PlayMontage.generated.h"

UCLASS()
class GMCEDITOR_API UGMC_K2Node_PlayMontage : public UK2Node_BaseAsyncTask
{
  GENERATED_BODY()

public:

  UGMC_K2Node_PlayMontage();

  FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
  FLinearColor GetNodeTitleColor() const override;
  FText GetTooltipText() const override;
  FText GetMenuCategory() const override;
  FText GetPinDisplayName(const UEdGraphPin* Pin) const override;
  void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
};
