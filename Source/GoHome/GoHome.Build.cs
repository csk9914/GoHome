// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GoHome : ModuleRules
{
	public GoHome(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] 
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"Niagara",
			"CableComponent",
			"GeometryCollectionEngine",
			"NavigationSystem",
			"UMG",
			"Slate",
			"SlateCore"
		});

        PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystem", 
			"OnlineSubsystemUtils",
			"OnlineSubsystemSteam", 
			"SteamSockets", 
			"CinematicCamera",
			"AIModule"
		});

		// 프로필 아바타(UPlayerAvatarSubsystem) — OSS v1 에 아바타 API 가 없어 ISteamFriends 직접 호출
		AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
	}
}
