// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class BlenderEditorControlsPlugin : ModuleRules
{
	public BlenderEditorControlsPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Necessary to hide overlay position setting for versions older than 5.6
		if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 6)
		{
			PublicDefinitions.Add("UE_BEFORE_5_6=1");

			PrivateDependencyModuleNames.Add("SequencerWidgets");
		}
		else
		{
			PublicDefinitions.Add("UE_BEFORE_5_6=0");
		}

		if (Target.Version.MajorVersion == 5)
		{
			if (Target.Version.MinorVersion == 3)
			{
				// Needed for ControlRigEditMode.h include in ControlRigSelectionHelper.cpp
				PrivateDependencyModuleNames.Add("Persona");
			}
			else if (Target.Version.MinorVersion == 1 || Target.Version.MinorVersion == 2)
			{
				PrivateDependencyModuleNames.Add("Persona");
				PrivateDependencyModuleNames.Add("AnimationEditMode");
			}
			else if (Target.Version.MinorVersion == 0)
			{
				// UE 5.0 requires no additional dependencies here
			}
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
				"ControlRig",
				"ControlRigEditor",
				"RigVM",
				"AnimationCore",
				"SubobjectEditor",
				"Kismet",
				"BlueprintGraph",
				"SubobjectDataInterface",
				"MainFrame"
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