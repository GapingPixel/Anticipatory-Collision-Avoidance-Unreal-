// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCK2Node_PlayMontage.h"
#include "GMCPlayMontageCallbackProxy.h"

#define LOCTEXT_NAMESPACE "K2Node"

UGMC_K2Node_PlayMontage::UGMC_K2Node_PlayMontage()
{
  ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UGMC_PlayMontageCallbackProxy, CreateProxyObjectForPlayMontage);
  ProxyFactoryClass = UGMC_PlayMontageCallbackProxy::StaticClass();
  ProxyClass = UGMC_PlayMontageCallbackProxy::StaticClass();
}

FText UGMC_K2Node_PlayMontage::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
  return LOCTEXT("GMCPlayMontage_NodeTitle", "GMC Play Montage");
}

FLinearColor UGMC_K2Node_PlayMontage::GetNodeTitleColor() const
{
  return FLinearColor(0.f, 0.f, 0.f);
}

FText UGMC_K2Node_PlayMontage::GetTooltipText() const
{
  return LOCTEXT("GMCPlayMontage_Tooltip", "Plays a montage.");
}

FText UGMC_K2Node_PlayMontage::GetMenuCategory() const
{
  return LOCTEXT("GMCPlayMontage_Category", "General Movement Component");
}

FText UGMC_K2Node_PlayMontage::GetPinDisplayName(const UEdGraphPin* Pin) const
{
  if (FindPinChecked(TEXT("OnStarted")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnStartedPin", "On Started (Networked)");
  }
  else if (FindPinChecked(TEXT("OnCompleted")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnCompletedPin", "On Completed (Networked)");
  }
  else if (FindPinChecked(TEXT("OnBlendIn")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnBlendInPin", "On Blend In (Networked)");
  }
  else if (FindPinChecked(TEXT("OnBlendOut")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnBlendOutPin", "On Blend Out (Networked)");
  }
  else if (FindPinChecked(TEXT("OnNotifyBegin")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnNotifyBeginPin", "On Notify Begin (Networked)");
  }
  else if (FindPinChecked(TEXT("OnNotifyEnd")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnNotifyEndPin", "On Notify End (Networked)");
  }
  else if (FindPinChecked(TEXT("OnCompleted_Cosmetic")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnCompletedCosmeticPin", "On Completed (Cosmetic)");
  }
  else if (FindPinChecked(TEXT("OnBlendOut_Cosmetic")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnBlendOutCosmeticPin", "On Blend Out (Cosmetic)");
  }
  else if (FindPinChecked(TEXT("OnInterrupted_Cosmetic")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnInterruptedCosmeticPin", "On Interrupted (Cosmetic)");
  }
  else if (FindPinChecked(TEXT("OnNotifyBegin_Cosmetic")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnNotifyBeginCosmeticPin", "On Notify Begin (Cosmetic)");
  }
  else if (FindPinChecked(TEXT("OnNotifyEnd_Cosmetic")) == Pin)
  {
    return LOCTEXT("GMCPlayMontage_OnNotifyEndCosmeticPin", "On Notify End (Cosmetic)");
  }
  else
  {
    return Super::GetPinDisplayName(Pin);
  }
}

void UGMC_K2Node_PlayMontage::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
  if (FindPinChecked(TEXT("MontageToPlay")) == &Pin)
  {
    HoverTextOut = TEXT("The montage to play");
  }
  else if (FindPinChecked(TEXT("StartPosition")) == &Pin)
  {
    HoverTextOut = TEXT("The position to start the montage at.");
  }
  else if (FindPinChecked(TEXT("PlayRate")) == &Pin)
  {
    HoverTextOut = TEXT("The rate to play the montage at.");
  }
  else if (FindPinChecked(TEXT("StartSection")) == &Pin)
  {
    HoverTextOut = TEXT("The section to start the montage at (will nullify the start position if set).");
  }
  else if (FindPinChecked(TEXT("bInterruptOther")) == &Pin)
  {
    HoverTextOut = TEXT("Whether to always interrupt another (but not the same) montage that may already be playing.");
  }
  else if (FindPinChecked(TEXT("bCancelOnBlendIn")) == &Pin)
  {
    HoverTextOut = TEXT("Whether to cancel any (another or the same) montage if it is currently blending in.");
  }
  else if (FindPinChecked(TEXT("bCancelOnBlendOut")) == &Pin)
  {
    HoverTextOut = TEXT("Whether to cancel any (another or the same) montage if it is currently blending out.");
  }
  else
  {
    Super::GetPinHoverText(Pin, HoverTextOut);
  }
}

#undef LOCTEXT_NAMESPACE
