// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GMCPlayerController.h"
#include "GameFramework/PlayerController.h"

#include "ModularPlayerController.generated.h"

class UObject;

/** Minimal class that supports extension by game feature plugins
 *  Similar to ModularPlayerController in Lyra's ModularGameplayActors but is based on
 *  GeneralMovementComponent's GMC_PlayerController
 */
UCLASS(Blueprintable)
class MODULARGAMEPLAYACTORS_API AModularPlayerController : public AGMC_PlayerController
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface

	//~ Begin APlayerController interface
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End APlayerController interface
};
