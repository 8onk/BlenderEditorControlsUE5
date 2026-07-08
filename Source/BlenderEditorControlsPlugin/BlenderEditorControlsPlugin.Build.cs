// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class BlenderEditorControlsPlugin : ModuleRules
{
	public BlenderEditorControlsPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = ModuleRules.CodeOptimization.Never;

		//Necessary to hide overlay position setting for versions older than 5.6
		if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 6)
		{
			PublicDefinitions.Add("UE_BEFORE_5_6=1");
		}
		else
		{
			PublicDefinitions.Add("UE_BEFORE_5_6=0");
		}
		
		// Check for 5.7 or newer for global sensitivity setting
		if (Target.Version.MajorVersion > 5 || (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 7))
		{
			PublicDefinitions.Add("UE_5_7_OR_LATER=1");
		}
		else
		{
			PublicDefinitions.Add("UE_5_7_OR_LATER=0");
		}

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(EngineDirectory, "Source", "Editor", "Kismet", "Private")
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "EditorInteractiveToolsFramework", "InteractiveToolsFramework", "UMG"
				// ... add other public dependencies that you statically link with here ...
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"DeveloperSettings",
				"UnrealEd",
				"ApplicationCore",
				"AppFramework",
				"Projects",
				"EditorStyle",
				"RenderCore",
				"InteractiveToolsFramework",
				"EditorInteractiveToolsFramework",
				"ComponentVisualizers",
				// Control Rig support for bone/control selection
				"ControlRig",
				"ControlRigEditor",
				"RigVM",
				"AnimationCore",
				"SubobjectEditor",
				"Kismet",
				"BlueprintGraph",
				"SubobjectDataInterface"
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}