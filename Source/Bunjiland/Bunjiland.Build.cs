// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Bunjiland : ModuleRules
{
	public Bunjiland(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ArticyRuntime" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Json" });
	}
}
