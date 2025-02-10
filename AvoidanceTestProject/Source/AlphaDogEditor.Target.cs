// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AlphaDogEditorTarget : TargetRules
{
	public AlphaDogEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		
		RegisterModulesCreatedByRider();
		
		if (!bBuildAllModules)
		{
			NativePointerMemberBehaviorOverride = PointerMemberBehavior.Disallow;
		}
		
		AlphaDogGameTarget.ApplySharedAlphaDogTargetSettings(this);
	}

	private void RegisterModulesCreatedByRider()
	{
		ExtraModuleNames.AddRange(new string[] { "AlphaDogGame", "AlphaDogEditor" });
	}
}
