// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GoHome : ModuleRules
{
	public GoHome(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara" });

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemSteam", "SteamSockets", "CinematicCamera" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// OnlineSubsystemSteam/SteamSockets are enabled via the Plugins section of GoHome.uproject.
	}
}
