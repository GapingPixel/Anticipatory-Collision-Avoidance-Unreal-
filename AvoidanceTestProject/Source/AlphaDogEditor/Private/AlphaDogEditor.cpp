#include "AlphaDogEditor.h"

#include "ADogEditorStatics.h"
#include "EditorStyling/GameEditorStyle.h"

#define LOCTEXT_NAMESPACE "FAlphaDogEditorModule"

void FAlphaDogEditorModule::StartupModule()
{
	FGameEditorStyle::Initialize();

	if (!IsRunningGame())
	{
		FModuleManager::Get().OnModulesChanged().AddRaw(this, &FAlphaDogEditorModule::ModulesChangedCallback);

		BindGameplayAbilitiesEditorDelegates();

		if (FSlateApplication::IsInitialized())
		{
			ToolMenusHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&UADogEditorStatics::RegisterGameEditorMenus));
		}

		FEditorDelegates::BeginPIE.AddRaw(this, &ThisClass::OnBeginPIE);
		FEditorDelegates::EndPIE.AddRaw(this, &ThisClass::OnEndPIE);
	}
}

void FAlphaDogEditorModule::ShutdownModule()
{
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);

	// Undo UToolMenus
	if (UObjectInitialized() && ToolMenusHandle.IsValid())
	{
		UToolMenus::UnRegisterStartupCallback(ToolMenusHandle);
	}

	UnbindGameplayAbilitiesEditorDelegates();
	FModuleManager::Get().OnModulesChanged().RemoveAll(this);
	FGameEditorStyle::Shutdown();
}

void FAlphaDogEditorModule::OnBeginPIE(bool bIsSimulating)
{
	/*UAlphaDogExperienceManager* ExperienceManager = GEngine->GetEngineSubsystem<UAlphaDogExperienceManager>();
	check(ExperienceManager);
	ExperienceManager->OnPlayInEditorBegun();*/
}

void FAlphaDogEditorModule::OnEndPIE(bool bIsSimulating)
{
}

void FAlphaDogEditorModule::BindGameplayAbilitiesEditorDelegates()
{
	/*IGameplayAbilitiesEditorModule& GameplayAbilitiesEditorModule = IGameplayAbilitiesEditorModule::Get();

	GameplayAbilitiesEditorModule.GetGameplayCueNotifyClassesDelegate().BindStatic(&UADogEditorStatics::GetGameplayCueDefaultClasses);
	GameplayAbilitiesEditorModule.GetGameplayCueInterfaceClassesDelegate().BindStatic(&UADogEditorStatics::GetGameplayCueInterfaceClasses);
	GameplayAbilitiesEditorModule.GetGameplayCueNotifyPathDelegate().BindStatic(&UADogEditorStatics::GetGameplayCuePath);*/
}

void FAlphaDogEditorModule::UnbindGameplayAbilitiesEditorDelegates()
{
	/*if (IGameplayAbilitiesEditorModule::IsAvailable())
	{
		IGameplayAbilitiesEditorModule& GameplayAbilitiesEditorModule = IGameplayAbilitiesEditorModule::Get();
		GameplayAbilitiesEditorModule.GetGameplayCueNotifyClassesDelegate().Unbind();
		GameplayAbilitiesEditorModule.GetGameplayCueInterfaceClassesDelegate().Unbind();
		GameplayAbilitiesEditorModule.GetGameplayCueNotifyPathDelegate().Unbind();
	}*/
}

void FAlphaDogEditorModule::ModulesChangedCallback(FName ModuleThatChanged, EModuleChangeReason ReasonForChange)
{
	if ((ReasonForChange == EModuleChangeReason::ModuleLoaded) && (ModuleThatChanged.ToString() == TEXT("GameplayAbilitiesEditor")))
	{
		BindGameplayAbilitiesEditorDelegates();
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FAlphaDogEditorModule, AlphaDogEditor)