using UnrealBuildTool;

public class OUUModuleRuleHelpers
{
    public static void AddGameplayDebuggerDependency(ModuleRules Rules, ReadOnlyTargetRules Target)
    {
        if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
        {
            Rules.PrivateDependencyModuleNames.Add("GameplayDebugger");
            Rules.PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
        }
        else
        {
            Rules.PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
        }
    }
}

public class OUUModuleRules : ModuleRules
{
    public OUUModuleRules(ReadOnlyTargetRules Target) : base(Target)
    {
        // Disable PCHs for debug configs to ensure the plugin modules are self-contained and stick to IWYU
        PCHUsage = Target.Configuration == UnrealTargetConfiguration.DebugGame
            ? ModuleRules.PCHUsageMode.NoPCHs
            : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        IWYUSupport = IWYUSupport.Full;

        // Also disable unity builds.
        // Unfortunately even this is needed to ensure that all includes are correct when building the plugin by itself.
        bUseUnity = false;

        OUUModuleRuleHelpers.AddGameplayDebuggerDependency(this, Target);
    }
}

public class BeansTestUtilities : OUUModuleRules
{
    public BeansTestUtilities(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UMG",
                "SignificanceManager",
                "AutomationController",
                "EngineSettings",
            }
        );

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.Add("UnrealEd");
        }
    }
}