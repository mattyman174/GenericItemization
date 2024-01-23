// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ItemizationSample : ModuleRules
{
	public ItemizationSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "AIModule", "Niagara", "EnhancedInput" });
    }
}
