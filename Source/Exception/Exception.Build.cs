// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Exception : ModuleRules
{
	public Exception(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		PublicIncludePaths.AddRange(new string[] {
			"Exception",
			"Exception/Boss",
			"Exception/Combat",
			"Exception/Inventory",
			"Exception/Player",
			"Exception/Save",
			"Exception/Story",
			"Exception/UI",
			"Exception/World",
			"Exception/Variant_Platforming",
			"Exception/Variant_Platforming/Animation",
			"Exception/Variant_Combat",
			"Exception/Variant_Combat/AI",
			"Exception/Variant_Combat/Animation",
			"Exception/Variant_Combat/Gameplay",
			"Exception/Variant_Combat/Interfaces",
			"Exception/Variant_Combat/UI",
			"Exception/Variant_SideScrolling",
			"Exception/Variant_SideScrolling/AI",
			"Exception/Variant_SideScrolling/Gameplay",
			"Exception/Variant_SideScrolling/Interfaces",
			"Exception/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
