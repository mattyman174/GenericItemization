// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

using UnrealBuildTool;

public class GenericItemization : ModuleRules
{
	public GenericItemization(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
		new string[] {
				
		});	
		
		PrivateIncludePaths.AddRange(
        new string[] {

        });

        PublicDependencyModuleNames.AddRange(
        new string[]
        {
            "Core",
			"StructUtils",
			"GameplayTags",
        });

        PrivateDependencyModuleNames.AddRange(
		new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
		});
	}
}
