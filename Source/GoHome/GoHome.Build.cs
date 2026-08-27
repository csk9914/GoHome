// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GoHome : ModuleRules
{
	public GoHome(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystem", 
			"OnlineSubsystemUtils",
			"OnlineSubsystemSteam", 
			"SteamSockets", 
			"CinematicCamera",
			"AIModule"
		});
	}
}
