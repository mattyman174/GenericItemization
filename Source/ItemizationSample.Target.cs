// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ItemizationSampleTarget : TargetRules
{
	public ItemizationSampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		
		bUseUnityBuild = false;
		
		ExtraModuleNames.Add("ItemizationSample");
	}
}
