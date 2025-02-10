/*
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBeansTestUtilitiesModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
*/

#pragma once

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_WORKER

//////////////////////////////////////////////////////////////////////////
// This file should contain nothing except for includes of commonly used
// AutomationTest utilities.
//////////////////////////////////////////////////////////////////////////

    #include "AutomationSpecTestMacros.h"
    #include "AutomationTestParameterParser.h"
    #include "AutomationTestWorld.h"
    #include "CollectionTestFunctions.h"
    #include "LatentAutomationPIEWorldLoader.h"
    #include "OUUTestMacros.h"
    #include "OUUTestObject.h"

#endif