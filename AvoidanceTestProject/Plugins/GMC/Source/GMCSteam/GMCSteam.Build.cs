// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class GMCSteam : ModuleRules
{
  public GMCSteam(ReadOnlyTargetRules Target) : base(Target)
  {
    // The logging setup partially depends on the order of macro definitions (which can break with unity builds). Setting the override to a high value will
    // effectively disable unity builds for this module.
    MinSourceFilesForUnityBuildOverride = Int32.MaxValue;

    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PrivatePCHHeaderFile = "Private/GMCSteam.pch";

    PublicDependencyModuleNames.AddRange(new[] {
      "Core",
      "CoreUObject",
      "Engine",
      "SlateCore",
      "NetCore",
      "OnlineSubsystem",
      "UMG",
    });

    // Public include directories.
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/GameInstance"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Widgets"));

    // Private includes directories.
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/GameInstance"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Widgets"));
  }
}
