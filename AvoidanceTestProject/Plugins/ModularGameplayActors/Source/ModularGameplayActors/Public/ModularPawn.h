// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GMCPawn.h"
#include "GameFramework/Pawn.h"

#include "ModularPawn.generated.h"

class UObject;

/** Minimal class that supports extension by game feature plugins
 *  Similar to ModularPlayerPawn in Lyra's ModularGameplayActors but is based on
 *  GeneralMovementComponent's GMC_Pawn  
 */
UCLASS(Blueprintable)
class MODULARGAMEPLAYACTORS_API AModularPawn : public AGMC_Pawn
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface

};
