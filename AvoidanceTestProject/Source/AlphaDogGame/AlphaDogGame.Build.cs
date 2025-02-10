using UnrealBuildTool;

public class AlphaDogGame : ModuleRules
{
    public AlphaDogGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreOnline",
                "ApplicationCore",
                "Niagara",
                "AIModule",
                "GameFeatures",
                "ModularGameplay",
                "ModularGameplayActors",
                "GameplayTags",
                "GameplayTasks",
                "GMCCore",
                "GMCAbilitySystem",
                "CommonLoadingScreen",
                "BeansGameplayTags",
                "BeansIndicators",
                "GameSettings",
                "ControlFlows",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "InputCore",
                "RHI",
                "CoreUObject",
                "Engine",
                "NetCore",
                "Slate",
                "SlateCore",
                "CommonUI",
                "CommonInput",
                "CommonGame",
                "CommonUser",
                "DeveloperSettings",
                "GameplayMessageRuntime",
                "GameSubtitles",
                "EnhancedInput",
                "BeansLogging",
                "AudioModulation",
                "NavigationSystem",
                "AudioMixer",
                "EngineSettings",
                "UMG",
                "UIExtension",
                "AnimGraphRuntime"
            }
        );
    }
}