// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ChartCreation.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ADogPerformanceStatSubsystem.generated.h"

enum class EADogDisplayablePerformanceStat : uint8;

class FSubsystemCollectionBase;
class UADogPerformanceStatSubsystem;
class UObject;
struct FFrame;

//////////////////////////////////////////////////////////////////////

// Observer which caches the stats for the previous frame
struct FADogPerformanceStatCache : public IPerformanceDataConsumer
{
public:
	FADogPerformanceStatCache(UADogPerformanceStatSubsystem* InSubsystem)
		: MySubsystem(InSubsystem)
	{
	}

	//~IPerformanceDataConsumer interface
	virtual void StartCharting() override;
	virtual void ProcessFrame(const FFrameData& FrameData) override;
	virtual void StopCharting() override;
	//~End of IPerformanceDataConsumer interface

	double GetCachedStat(EADogDisplayablePerformanceStat Stat) const;

protected:
	IPerformanceDataConsumer::FFrameData CachedData;
	UADogPerformanceStatSubsystem* MySubsystem;

	float CachedServerFPS = 0.0f;
	float CachedPingMS = 0.0f;
	float CachedPacketLossIncomingPercent = 0.0f;
	float CachedPacketLossOutgoingPercent = 0.0f;
	float CachedPacketRateIncoming = 0.0f;
	float CachedPacketRateOutgoing = 0.0f;
	float CachedPacketSizeIncoming = 0.0f;
	float CachedPacketSizeOutgoing = 0.0f;
};

//////////////////////////////////////////////////////////////////////

// Subsystem to allow access to performance stats for display purposes
UCLASS()
class ALPHADOGGAME_API UADogPerformanceStatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	double GetCachedStat(EADogDisplayablePerformanceStat Stat) const;

	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of USubsystem interface

protected:
	TSharedPtr<FADogPerformanceStatCache> Tracker;
};
