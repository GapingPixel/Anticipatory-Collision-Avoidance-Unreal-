// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/CustomSettings/ADogSettingValueDiscrete_MobileFPSType.h"

#include "Performance/ADogPerformanceSettings.h"
#include "Settings/ADogSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogSettingValueDiscrete_MobileFPSType)

#define LOCTEXT_NAMESPACE "ADogSettings"

UADogSettingValueDiscrete_MobileFPSType::UADogSettingValueDiscrete_MobileFPSType()
{
}

void UADogSettingValueDiscrete_MobileFPSType::OnInitialized()
{
	Super::OnInitialized();

	const UADogPlatformSpecificRenderingSettings* PlatformSettings = UADogPlatformSpecificRenderingSettings::Get();
	const UADogSettingsLocal* UserSettings = UADogSettingsLocal::Get();

	for (int32 TestLimit : PlatformSettings->MobileFrameRateLimits)
	{
		if (UADogSettingsLocal::IsSupportedMobileFramePace(TestLimit))
		{
			FPSOptions.Add(TestLimit, MakeLimitString(TestLimit));
		}
	}

	const int32 FirstFrameRateWithQualityLimit = UserSettings->GetFirstFrameRateWithQualityLimit();
	if (FirstFrameRateWithQualityLimit > 0)
	{
		SetWarningRichText(FText::Format(LOCTEXT("MobileFPSType_Note", "<strong>Note: Changing the framerate setting to {0} or higher might lower your Quality Presets.</>"), MakeLimitString(FirstFrameRateWithQualityLimit)));
	}
}

int32 UADogSettingValueDiscrete_MobileFPSType::GetDefaultFPS() const
{
	return UADogSettingsLocal::GetDefaultMobileFrameRate();
}

FText UADogSettingValueDiscrete_MobileFPSType::MakeLimitString(int32 Number)
{
	return FText::Format(LOCTEXT("MobileFrameRateOption", "{0} FPS"), FText::AsNumber(Number));
}

void UADogSettingValueDiscrete_MobileFPSType::StoreInitial()
{
	InitialValue = GetValue();
}

void UADogSettingValueDiscrete_MobileFPSType::ResetToDefault()
{
	SetValue(GetDefaultFPS(), EGameSettingChangeReason::ResetToDefault);
}

void UADogSettingValueDiscrete_MobileFPSType::RestoreToInitial()
{
	SetValue(InitialValue, EGameSettingChangeReason::RestoreToInitial);
}

void UADogSettingValueDiscrete_MobileFPSType::SetDiscreteOptionByIndex(int32 Index)
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);

	int32 NewMode = FPSOptionsModes.IsValidIndex(Index) ? FPSOptionsModes[Index] : GetDefaultFPS();

	SetValue(NewMode, EGameSettingChangeReason::Change);
}

int32 UADogSettingValueDiscrete_MobileFPSType::GetDiscreteOptionIndex() const
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);
	return FPSOptionsModes.IndexOfByKey(GetValue());
}

TArray<FText> UADogSettingValueDiscrete_MobileFPSType::GetDiscreteOptions() const
{
	TArray<FText> Options;
	FPSOptions.GenerateValueArray(Options);

	return Options;
}

int32 UADogSettingValueDiscrete_MobileFPSType::GetValue() const
{
	return UADogSettingsLocal::Get()->GetDesiredMobileFrameRateLimit();
}

void UADogSettingValueDiscrete_MobileFPSType::SetValue(int32 NewLimitFPS, EGameSettingChangeReason InReason)
{
	UADogSettingsLocal::Get()->SetDesiredMobileFrameRateLimit(NewLimitFPS);

	NotifySettingChanged(InReason);
}

#undef LOCTEXT_NAMESPACE

