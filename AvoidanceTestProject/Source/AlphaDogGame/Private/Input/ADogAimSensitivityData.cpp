// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ADogAimSensitivityData.h"

#include "Settings/ADogSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogAimSensitivityData)

UADogAimSensitivityData::UADogAimSensitivityData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SensitivityMap =
	{
		{ EADogGamepadSensitivity::Slow,			0.5f },
		{ EADogGamepadSensitivity::SlowPlus,		0.75f },
		{ EADogGamepadSensitivity::SlowPlusPlus,	0.9f },
		{ EADogGamepadSensitivity::Normal,		1.0f },
		{ EADogGamepadSensitivity::NormalPlus,	1.1f },
		{ EADogGamepadSensitivity::NormalPlusPlus,1.25f },
		{ EADogGamepadSensitivity::Fast,			1.5f },
		{ EADogGamepadSensitivity::FastPlus,		1.75f },
		{ EADogGamepadSensitivity::FastPlusPlus,	2.0f },
		{ EADogGamepadSensitivity::Insane,		2.5f },
	};
}

const float UADogAimSensitivityData::SensitivtyEnumToFloat(const EADogGamepadSensitivity InSensitivity) const
{
	if (const float* Sens = SensitivityMap.Find(InSensitivity))
	{
		return *Sens;
	}

	return 1.0f;
}

