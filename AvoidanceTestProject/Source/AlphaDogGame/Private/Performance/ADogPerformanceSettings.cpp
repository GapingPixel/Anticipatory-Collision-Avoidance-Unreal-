// Fill out your copyright notice in the Description page of Project Settings.


#include "Performance/ADogPerformanceSettings.h"
#include "Engine/PlatformSettingsManager.h"
#include "Misc/EnumRange.h"
#include "Performance/ADogPerformanceStatTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogPerformanceSettings)

//////////////////////////////////////////////////////////////////////

UADogPlatformSpecificRenderingSettings::UADogPlatformSpecificRenderingSettings()
{
	MobileFrameRateLimits.Append({ 20, 30, 45, 60, 90, 120 });
}

const UADogPlatformSpecificRenderingSettings* UADogPlatformSpecificRenderingSettings::Get()
{
	UADogPlatformSpecificRenderingSettings* Result = UPlatformSettingsManager::Get().GetSettingsForPlatform<ThisClass>();
	check(Result);
	return Result;
}

//////////////////////////////////////////////////////////////////////

UADogPerformanceSettings::UADogPerformanceSettings()
{
	PerPlatformSettings.Initialize(UADogPlatformSpecificRenderingSettings::StaticClass());

	CategoryName = TEXT("Game");

	DesktopFrameRateLimits.Append({ 30, 60, 120, 144, 160, 165, 180, 200, 240, 360 });

	// Default to all stats are allowed
	FADogPerformanceStatGroup& StatGroup = UserFacingPerformanceStats.AddDefaulted_GetRef();
	for (EADogDisplayablePerformanceStat PerfStat : TEnumRange<EADogDisplayablePerformanceStat>())
	{
		StatGroup.AllowedStats.Add(PerfStat);
	}
}
