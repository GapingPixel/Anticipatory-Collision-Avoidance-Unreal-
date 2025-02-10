#include "BeansDebug.h"

#include "NativeGameplayTags.h"

#define LOCTEXT_NAMESPACE "FBeansDebugModule"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_BeansDebug_ClientServer_Channel, "Beans.Debug.Message", "Tag used for the BeansDebug gameplay message channel.")

void FBeansDebugModule::StartupModule()
{
    
}

void FBeansDebugModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FBeansDebugModule, BeansDebug)