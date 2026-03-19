// Copyright (c) 2024 UnrealDoom Contributors. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealDoomEditorTarget : TargetRules
{
	public UnrealDoomEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

		ExtraModuleNames.AddRange(new string[] { "UnrealDoom" });
	}
}
