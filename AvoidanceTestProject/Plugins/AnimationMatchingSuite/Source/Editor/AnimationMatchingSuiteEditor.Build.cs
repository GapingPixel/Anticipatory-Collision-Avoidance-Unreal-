// Copyright MuuKnighted Games 2024. All rights reserved.

using UnrealBuildTool;
 
public class AnimationMatchingSuiteEditor : ModuleRules
{
	public AnimationMatchingSuiteEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
			{ 
				"Core", 
				"CoreUObject", 
				"Engine", 
				"UnrealEd",
				"AnimationMatchingSuite", 
				"AnimGraph",
				"BlueprintGraph",
				"SlateCore",
				"ToolMenus", 
				"Slate"
				
			}
		);
		
	}
}