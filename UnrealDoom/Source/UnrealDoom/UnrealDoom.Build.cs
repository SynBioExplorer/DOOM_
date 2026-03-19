// Copyright (c) 2024 UnrealDoom Contributors. All Rights Reserved.

using UnrealBuildTool;

public class UnrealDoom : ModuleRules
{
	public UnrealDoom(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Enable IWYU (Include What You Use) for faster compilation
		bEnforceIWYU = true;

		// C++20 standard for UE5.3+
		CppStandard = CppStandardVersion.Cpp20;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add other private include paths required here ...
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"UMG",
				"Slate",
				"SlateCore",
				"AudioMixer",
				"ProceduralMeshComponent",
				"NavigationSystem"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"RenderCore",
				"RHI"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

		// Disable exceptions to match DOOM's C-style code expectations
		bEnableExceptions = false;

		// Allow unsafe pointer operations needed for DOOM's original C code interop
		bUseUnity = false;
	}
}
