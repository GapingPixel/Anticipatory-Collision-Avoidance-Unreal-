// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameSettingValueDiscrete.h"

#include "ADogSettingValueDiscrete_PerfStat.generated.h"

enum class EADogDisplayablePerformanceStat : uint8;
enum class EADogStatDisplayMode : uint8;

class UObject;

UCLASS()
class UADogSettingValueDiscrete_PerfStat : public UGameSettingValueDiscrete
{
	GENERATED_BODY()
	
public:

	UADogSettingValueDiscrete_PerfStat();

	void SetStat(EADogDisplayablePerformanceStat InStat);

	/** UGameSettingValue */
	virtual void StoreInitial() override;
	virtual void ResetToDefault() override;
	virtual void RestoreToInitial() override;

	/** UGameSettingValueDiscrete */
	virtual void SetDiscreteOptionByIndex(int32 Index) override;
	virtual int32 GetDiscreteOptionIndex() const override;
	virtual TArray<FText> GetDiscreteOptions() const override;

protected:
	/** UGameSettingValue */
	virtual void OnInitialized() override;
	
	void AddMode(FText&& Label, EADogStatDisplayMode Mode);
protected:
	TArray<FText> Options;
	TArray<EADogStatDisplayMode> DisplayModes;

	EADogDisplayablePerformanceStat StatToDisplay;
	EADogStatDisplayMode InitialMode;
};
