#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"

class FAlphaDogGameModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

FORCEINLINE static AActor* FindFirstActorWithTag(const UWorld* World, const FName Tag)
{
    TArray<AActor*> ActorsWithTag;
    UGameplayStatics::GetAllActorsWithTag(World, Tag, ActorsWithTag);
    if (ActorsWithTag.Num() > 0)
    {
        return ActorsWithTag[0];  // Return the first actor with the tag
    }
    return nullptr;
}