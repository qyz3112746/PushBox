// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PushBoxEditor : ModuleRules
{
	public PushBoxEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"Slate",
			"SlateCore",
			"Blutility"
		});
	}
}
