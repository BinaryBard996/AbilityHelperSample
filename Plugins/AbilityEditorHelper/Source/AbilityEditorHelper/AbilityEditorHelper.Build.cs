// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AbilityEditorHelper : ModuleRules
{
	public AbilityEditorHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Json",
				"JsonUtilities",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"GameplayAbilities",
				"GameplayTags",
				"DeveloperSettings",
				"EditorSubsystem",
				"Projects"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		// 仅在编辑器构建时引入编辑器相关依赖
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",      // KismetEditorUtilities 等
					"Kismet",
					"AssetTools",
					"AssetRegistry"
				}
			);
		}

		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
