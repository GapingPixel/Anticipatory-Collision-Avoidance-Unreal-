using UnrealBuildTool;

public class AlphaDogEditor : ModuleRules
{
    public AlphaDogEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "AlphaDogEditor"
            }
        );
        
        PrivateIncludePaths.AddRange(
            new string[] {
            }
        );
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "EditorFramework",
                "UnrealEd",
                "GameplayTagsEditor",
                "StudioTelemetry",
                
                "BeansLogging",
                "BeansContextEffectsEditor",
                "AlphaDogGame",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "ToolMenus",
                "EditorStyle",
                "DataValidation",
                "MessageLog",
                "Projects",
                "DeveloperToolSettings",
                "CollectionManager",
                "SourceControl",
            }
        );
    }
}