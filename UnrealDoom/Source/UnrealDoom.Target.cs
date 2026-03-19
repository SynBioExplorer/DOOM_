// Copyright (c) 2024 UnrealDoom Contributors. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealDoomTarget : TargetRules
{
	public UnrealDoomTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

		ExtraModuleNames.AddRange(new string[] { "UnrealDoom" });

		// Enable logging in shipping builds for debugging DOOM port issues
		bUseLoggingInShipping = true;
	}
}
