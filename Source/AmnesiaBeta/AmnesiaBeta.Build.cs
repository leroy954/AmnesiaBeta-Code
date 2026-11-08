// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AmnesiaBeta : ModuleRules
{
	public AmnesiaBeta(ReadOnlyTargetRules Target) : base(Target)
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
			"UMG",
			"Slate",
				"SlateCore",
				"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// editor only level building helper
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Json", "AssetTools", "RenderCore", "RHI" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"AmnesiaBeta",
			"AmnesiaBeta/Amnesia",
			"AmnesiaBeta/Variant_Platforming",
			"AmnesiaBeta/Variant_Platforming/Animation",
			"AmnesiaBeta/Variant_Combat",
			"AmnesiaBeta/Variant_Combat/AI",
			"AmnesiaBeta/Variant_Combat/Animation",
			"AmnesiaBeta/Variant_Combat/Gameplay",
			"AmnesiaBeta/Variant_Combat/Interfaces",
			"AmnesiaBeta/Variant_Combat/UI",
			"AmnesiaBeta/Variant_SideScrolling",
			"AmnesiaBeta/Variant_SideScrolling/AI",
			"AmnesiaBeta/Variant_SideScrolling/Gameplay",
			"AmnesiaBeta/Variant_SideScrolling/Interfaces",
			"AmnesiaBeta/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
