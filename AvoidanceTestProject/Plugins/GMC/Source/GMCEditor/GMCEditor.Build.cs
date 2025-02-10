// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class GMCEditor : ModuleRules
{
  public GMCEditor(ReadOnlyTargetRules Target) : base(Target)
  {
    // The logging setup partially depends on the order of macro definitions (which can break with unity builds). Setting the override to a high value will
    // effectively disable unity builds for this module.
    MinSourceFilesForUnityBuildOverride = Int32.MaxValue;

    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PrivatePCHHeaderFile = "Private/GMCEditor.pch";

    PublicDependencyModuleNames.AddRange(new[] {
      "GMCCore",
      "Core",
      "CoreUObject",
      "Engine",
      "BlueprintGraph"
    });

    // Public include directories.
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Montage"));

    // Private includes directories.
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Montage"));
  }
}
