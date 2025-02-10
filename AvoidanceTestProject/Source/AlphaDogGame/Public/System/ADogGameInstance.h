// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonGameInstance.h"

#include "ADogGameInstance.generated.h"

class AADogPlayerController;
class UObject;

UCLASS(Config = Game)
class ALPHADOGGAME_API UADogGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:

	UADogGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	AADogPlayerController* GetPrimaryPlayerController() const;
	
	virtual bool CanJoinRequestedSession() const override;
	virtual void HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext) override;

protected:

	virtual void Init() override;
	virtual void Shutdown() override;

	void OnPreClientTravelToSession(FString& URL);
	
};
