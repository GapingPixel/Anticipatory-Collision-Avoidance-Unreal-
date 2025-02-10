// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "ADogVerbMessageHelpers.generated.h"

//struct FGameplayCueParameters;
struct FADogVerbMessage;

class APlayerController;
class APlayerState;
class UObject;
struct FFrame;

UCLASS()
class ALPHADOGGAME_API UADogVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "AlphaDog")
	static APlayerState* GetPlayerStateFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "AlphaDog")
	static APlayerController* GetPlayerControllerFromObject(UObject* Object);

	/* TODO We aren't using GameplayCues, should replace with whatever our Cue is
	 *UFUNCTION(BlueprintCallable, Category = "AlphaDog")
	static FGameplayCueParameters VerbMessageToCueParameters(const FADogVerbMessage& Message);

	UFUNCTION(BlueprintCallable, Category = "AlphaDog")
	static FADogVerbMessage CueParametersToVerbMessage(const FGameplayCueParameters& Params);
	*/
};
