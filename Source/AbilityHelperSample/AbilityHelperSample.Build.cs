// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AbilityHelperSample : ModuleRules
{
	public AbilityHelperSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"DeveloperSettings",
			"EditorSubsystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AbilityEditorHelper"
		});

		// Editor-only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd"
			});
		}
	}
}
