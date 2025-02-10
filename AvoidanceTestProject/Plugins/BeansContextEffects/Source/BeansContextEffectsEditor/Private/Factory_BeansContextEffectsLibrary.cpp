// Copyright Epic Games, Inc. All Rights Reserved.

#include "Factory_BeansContextEffectsLibrary.h"

#include "BeansContextEffectsLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Factory_BeansContextEffectsLibrary)

class FFeedbackContext;
class UClass;
class UObject;

UFactory_BeansContextEffectsLibrary::UFactory_BeansContextEffectsLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UBeansContextEffectsLibrary::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UFactory_BeansContextEffectsLibrary::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UBeansContextEffectsLibrary* BeansContextEffectsLibrary = NewObject<UBeansContextEffectsLibrary>(InParent, Name, Flags);

	return BeansContextEffectsLibrary;
}
