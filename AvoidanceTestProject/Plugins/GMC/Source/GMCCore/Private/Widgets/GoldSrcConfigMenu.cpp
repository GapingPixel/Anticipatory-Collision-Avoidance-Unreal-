// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GoldSrcConfigMenu.h"
#include "GMCGoldSrcMovementComponent.h"

bool UGMC_GoldSrcConfigMenu::Initialize()
{
  if (Super::Initialize())
  {
    // Update first, otherwise the actual movement component parameter values will be overwritten with the default widget values.
    UpdateInterfaceValues();

#if WITH_EDITOR

    if (Button_UpdateArchetype) Button_UpdateArchetype->OnClicked.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnClicked_UpdateArchetype);

#else

    if (Button_UpdateArchetype) Button_UpdateArchetype->SetVisibility(ESlateVisibility::Hidden);
    if (Text_UpdateArchetype) Text_UpdateArchetype->SetVisibility(ESlateVisibility::Hidden);

#endif

    if (Value_Acceleration) Value_Acceleration->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_Acceleration);
    if (Value_MaxSpeed) Value_MaxSpeed->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_MaxSpeed);
    if (Value_StopSpeed) Value_StopSpeed->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_StopSpeed);
    if (Value_SprintSpeed) Value_SprintSpeed->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_SprintSpeed);
    if (Value_CrouchWalkSpeed) Value_CrouchWalkSpeed->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_CrouchWalkSpeed);
    if (Value_CrouchInterpSpeed) Value_CrouchInterpSpeed->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_CrouchInterpSpeed);
    if (Value_CrouchedHalfHeight) Value_CrouchedHalfHeight->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_CrouchedHalfHeight);
    if (Value_FloorFriction) Value_FloorFriction->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_FloorFriction);
    if (Value_EdgeFrictionScale) Value_EdgeFrictionScale->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_EdgeFrictionScale);
    if (Value_EdgeFrictionMinRequiredHeight) Value_EdgeFrictionMinRequiredHeight->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_EdgeFrictionMinRequiredHeight);
    if (Value_bUseAltEdgeFriction) Value_bUseAltEdgeFriction->OnCheckStateChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bUseAltEdgeFriction);
    if (Value_JumpMode) Value_JumpMode->OnSelectionChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnSelectionChanged_JumpMode);
    if (Value_JumpForce) Value_JumpForce->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_JumpForce);
    if (Value_MaxGroundedVelocityZ) Value_MaxGroundedVelocityZ->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_MaxGroundedVelocityZ);
    if (Value_bApplyJumpSpeedBoost) Value_bApplyJumpSpeedBoost->OnCheckStateChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bApplyJumpSpeedBoost);
    if (Value_SmallJumpSpeedBoostFactor) Value_SmallJumpSpeedBoostFactor->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_SmallJumpSpeedBoostFactor);
    if (Value_LargeJumpSpeedBoostFactor) Value_LargeJumpSpeedBoostFactor->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_LargeJumpSpeedBoostFactor);
    if (Value_AirAcceleration) Value_AirAcceleration->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_AirAcceleration);
    if (Value_AirControl) Value_AirControl->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_AirControl);
    if (Value_WaterControl) Value_WaterControl->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_WaterControl);
    if (Value_SinkSpeed) Value_SinkSpeed->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_SinkSpeed);
    if (Value_WaterJumpForce) Value_WaterJumpForce->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_WaterJumpForce);
    if (Value_WaterJumpPush) Value_WaterJumpPush->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_WaterJumpPush);
    if (Value_bUseAltSwimControls) Value_bUseAltSwimControls->OnCheckStateChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bUseAltSwimControls);
    if (Value_SpeedHardCap) Value_SpeedHardCap->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_SpeedHardCap);
    if (Value_bConsiderMassOnJump) Value_bConsiderMassOnJump->OnCheckStateChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bConsiderMassOnJump);
    if (Value_GravityScale) Value_GravityScale->OnValueChanged.AddDynamic(this, &UGMC_GoldSrcConfigMenu::OnValueChanged_GravityScale);

    return true;
  }

  return false;
}

void UGMC_GoldSrcConfigMenu::UpdateInterfaceValues() const
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    if (Value_Acceleration) Value_Acceleration->SetValue(GoldSrcMovementComponent->Acceleration);
    if (Value_MaxSpeed) Value_MaxSpeed->SetValue(GoldSrcMovementComponent->MaxSpeed);
    if (Value_StopSpeed) Value_StopSpeed->SetValue(GoldSrcMovementComponent->StopSpeed);
    if (Value_SprintSpeed) Value_SprintSpeed->SetValue(GoldSrcMovementComponent->SprintSpeed);
    if (Value_CrouchWalkSpeed) Value_CrouchWalkSpeed->SetValue(GoldSrcMovementComponent->CrouchWalkSpeed);
    if (Value_CrouchInterpSpeed) Value_CrouchInterpSpeed->SetValue(GoldSrcMovementComponent->CrouchInterpSpeed);
    if (Value_CrouchedHalfHeight) Value_CrouchedHalfHeight->SetValue(GoldSrcMovementComponent->CrouchedHalfHeight);
    if (Value_FloorFriction) Value_FloorFriction->SetValue(GoldSrcMovementComponent->FloorFriction);
    if (Value_EdgeFrictionScale) Value_EdgeFrictionScale->SetValue(GoldSrcMovementComponent->EdgeFrictionScale);
    if (Value_EdgeFrictionMinRequiredHeight) Value_EdgeFrictionMinRequiredHeight->SetValue(GoldSrcMovementComponent->EdgeFrictionMinRequiredHeight);
    if (Value_bUseAltEdgeFriction) Value_bUseAltEdgeFriction->SetCheckedState(static_cast<ECheckBoxState>(GoldSrcMovementComponent->bUseAltEdgeFriction));
    if (Value_JumpMode) Value_JumpMode->SetSelectedOption(StaticEnum<EGMC_JumpMode>()->GetNameStringByValue(static_cast<int64>(GoldSrcMovementComponent->JumpMode)));
    if (Value_JumpForce) Value_JumpForce->SetValue(GoldSrcMovementComponent->JumpForce);
    if (Value_MaxGroundedVelocityZ) Value_MaxGroundedVelocityZ->SetValue(GoldSrcMovementComponent->MaxGroundedVelocityZ);
    if (Value_bApplyJumpSpeedBoost) Value_bApplyJumpSpeedBoost->SetCheckedState(static_cast<ECheckBoxState>(GoldSrcMovementComponent->bApplyJumpSpeedBoost));
    if (Value_SmallJumpSpeedBoostFactor) Value_SmallJumpSpeedBoostFactor->SetValue(GoldSrcMovementComponent->SmallJumpSpeedBoostFactor);
    if (Value_LargeJumpSpeedBoostFactor) Value_LargeJumpSpeedBoostFactor->SetValue(GoldSrcMovementComponent->LargeJumpSpeedBoostFactor);
    if (Value_AirAcceleration) Value_AirAcceleration->SetValue(GoldSrcMovementComponent->AirAcceleration);
    if (Value_AirControl) Value_AirControl->SetValue(GoldSrcMovementComponent->AirControl);
    if (Value_WaterControl) Value_WaterControl->SetValue(GoldSrcMovementComponent->WaterControl);
    if (Value_SinkSpeed) Value_SinkSpeed->SetValue(GoldSrcMovementComponent->SinkSpeed);
    if (Value_WaterJumpForce) Value_WaterJumpForce->SetValue(GoldSrcMovementComponent->WaterJumpForce);
    if (Value_WaterJumpPush) Value_WaterJumpPush->SetValue(GoldSrcMovementComponent->WaterJumpPush);
    if (Value_bUseAltSwimControls) Value_bUseAltSwimControls->SetCheckedState(static_cast<ECheckBoxState>(GoldSrcMovementComponent->bUseAltSwimControls));
    if (Value_SpeedHardCap) Value_SpeedHardCap->SetValue(GoldSrcMovementComponent->SpeedHardCap);
    if (Value_bConsiderMassOnJump) Value_bConsiderMassOnJump->SetCheckedState(static_cast<ECheckBoxState>(GoldSrcMovementComponent->bConsiderMassOnJump));
    if (Value_GravityScale) Value_GravityScale->SetValue(GoldSrcMovementComponent->GravityScale);
  }
}

void UGMC_GoldSrcConfigMenu::UpdateArchetypeValues() const
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    if (const auto& Archetype = Cast<UGMC_GoldSrcMovementCmp>(GoldSrcMovementComponent->GetArchetype()))
    {
      if (Value_Acceleration) Archetype->Acceleration = Value_Acceleration->GetValue();
      if (Value_MaxSpeed) Archetype->MaxSpeed = Value_MaxSpeed->GetValue();
      if (Value_StopSpeed) Archetype->StopSpeed = Value_StopSpeed->GetValue();
      if (Value_SprintSpeed) Archetype->SprintSpeed = Value_SprintSpeed->GetValue();
      if (Value_CrouchWalkSpeed) Archetype->CrouchWalkSpeed = Value_CrouchWalkSpeed->GetValue();
      if (Value_CrouchInterpSpeed) Archetype->CrouchInterpSpeed = Value_CrouchInterpSpeed->GetValue();
      if (Value_CrouchedHalfHeight) Archetype->CrouchedHalfHeight = Value_CrouchedHalfHeight->GetValue();
      if (Value_FloorFriction) Archetype->FloorFriction = Value_FloorFriction->GetValue();
      if (Value_EdgeFrictionScale) Archetype->EdgeFrictionScale = Value_EdgeFrictionScale->GetValue();
      if (Value_EdgeFrictionMinRequiredHeight) Archetype->EdgeFrictionMinRequiredHeight = Value_EdgeFrictionMinRequiredHeight->GetValue();
      if (Value_bUseAltEdgeFriction) Archetype->bUseAltEdgeFriction = static_cast<bool>(Value_bUseAltEdgeFriction->GetCheckedState());
      if (Value_JumpMode) Archetype->JumpMode = static_cast<EGMC_JumpMode>(StaticEnum<EGMC_JumpMode>()->GetValueByNameString(Value_JumpMode->GetSelectedOption()));
      if (Value_JumpForce) Archetype->JumpForce = Value_JumpForce->GetValue();
      if (Value_MaxGroundedVelocityZ) Archetype->MaxGroundedVelocityZ = Value_MaxGroundedVelocityZ->GetValue();
      if (Value_bApplyJumpSpeedBoost) Archetype->bApplyJumpSpeedBoost = static_cast<bool>(Value_bApplyJumpSpeedBoost->GetCheckedState());
      if (Value_SmallJumpSpeedBoostFactor) Archetype->SmallJumpSpeedBoostFactor = Value_SmallJumpSpeedBoostFactor->GetValue();
      if (Value_LargeJumpSpeedBoostFactor) Archetype->LargeJumpSpeedBoostFactor = Value_LargeJumpSpeedBoostFactor->GetValue();
      if (Value_AirAcceleration) Archetype->AirAcceleration = Value_AirAcceleration->GetValue();
      if (Value_AirControl) Archetype->AirControl = Value_AirControl->GetValue();
      if (Value_WaterControl) Archetype->WaterControl = Value_WaterControl->GetValue();
      if (Value_SinkSpeed) Archetype->SinkSpeed = Value_SinkSpeed->GetValue();
      if (Value_WaterJumpForce) Archetype->WaterJumpForce = Value_WaterJumpForce->GetValue();
      if (Value_WaterJumpPush) Archetype->WaterJumpPush = Value_WaterJumpPush->GetValue();
      if (Value_bUseAltSwimControls) Archetype->bUseAltSwimControls = static_cast<bool>(Value_bUseAltSwimControls->GetCheckedState());
      if (Value_SpeedHardCap) Archetype->SpeedHardCap = Value_SpeedHardCap->GetValue();
      if (Value_bConsiderMassOnJump) Archetype->bConsiderMassOnJump = static_cast<bool>(Value_bConsiderMassOnJump->GetCheckedState());
      if (Value_GravityScale) Archetype->GravityScale = Value_GravityScale->GetValue();
    }
  }
}

UGMC_GoldSrcMovementCmp* UGMC_GoldSrcConfigMenu::GetLocalGoldSrcMovementComponent() const
{
  if (const auto& World = GetWorld())
  {
    for (auto Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
      const auto& PlayerController = Iterator->Get();
      if (PlayerController && PlayerController->IsLocalController())
      {
        if (const auto& LocalPawn = PlayerController->GetPawn())
        {
          return LocalPawn->FindComponentByClass<UGMC_GoldSrcMovementCmp>();
        }
        break;
      }
    }
  }
  return nullptr;
}

void UGMC_GoldSrcConfigMenu::OnClicked_UpdateArchetype()
{
  UpdateArchetypeValues();
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_Acceleration(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->Acceleration = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_MaxSpeed(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->DefaultSpeed = InValue;
    GoldSrcMovementComponent->MaxDesiredSpeed = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_StopSpeed(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->StopSpeed = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_SprintSpeed(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->SprintSpeed = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_CrouchWalkSpeed(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->CrouchWalkSpeed = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_CrouchInterpSpeed(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->CrouchInterpSpeed = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_CrouchedHalfHeight(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->CrouchedHalfHeight = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_FloorFriction(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->FloorFriction = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_EdgeFrictionScale(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->EdgeFrictionScale = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_EdgeFrictionMinRequiredHeight(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->EdgeFrictionMinRequiredHeight = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bUseAltEdgeFriction(bool bIsChecked)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->bUseAltEdgeFriction = bIsChecked;
  }
}

void UGMC_GoldSrcConfigMenu::OnSelectionChanged_JumpMode(FString SelectedItem, ESelectInfo::Type SelectionType)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    const int64 JumpModeIndex = StaticEnum<EGMC_JumpMode>()->GetValueByNameString(SelectedItem);
    if (JumpModeIndex != INDEX_NONE)
    {
      GoldSrcMovementComponent->JumpMode = static_cast<EGMC_JumpMode>(JumpModeIndex);
      return;
    }
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_JumpForce(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->JumpForce = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_MaxGroundedVelocityZ(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->MaxGroundedVelocityZ = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bApplyJumpSpeedBoost(bool bIsChecked)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->bApplyJumpSpeedBoost = bIsChecked;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_SmallJumpSpeedBoostFactor(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->SmallJumpSpeedBoostFactor = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_LargeJumpSpeedBoostFactor(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->LargeJumpSpeedBoostFactor = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_AirAcceleration(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->AirAcceleration = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_AirControl(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->AirControl = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_WaterControl(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->WaterControl = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_SinkSpeed(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->SinkSpeed = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_WaterJumpForce(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->WaterJumpForce = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_WaterJumpPush(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->WaterJumpPush = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bUseAltSwimControls(bool bIsChecked)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->bUseAltSwimControls = bIsChecked;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_SpeedHardCap(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->SpeedHardCap = InValue;
  }
}

void UGMC_GoldSrcConfigMenu::OnCheckStateChanged_bConsiderMassOnJump(bool bIsChecked)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->bConsiderMassOnJump = bIsChecked;
  }
}

void UGMC_GoldSrcConfigMenu::OnValueChanged_GravityScale(float InValue)
{
  if (const auto& GoldSrcMovementComponent = GetLocalGoldSrcMovementComponent())
  {
    GoldSrcMovementComponent->GravityScale = InValue;
  }
}
