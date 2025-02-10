#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;

class FAlphaDogEditorModule : public FDefaultGameModuleImpl
{
    typedef FAlphaDogEditorModule ThisClass;
    
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void OnBeginPIE(bool bIsSimulating);
    void OnEndPIE(bool bIsSimulating);
    
protected:

    static void BindGameplayAbilitiesEditorDelegates();
    static void UnbindGameplayAbilitiesEditorDelegates();
    void ModulesChangedCallback(FName ModuleThatChanged, EModuleChangeReason ReasonForChange);

private:
    TWeakPtr<IAssetTypeActions> AlphaDogContextEffectsLibraryAssetAction;
    FDelegateHandle ToolMenusHandle;
};
