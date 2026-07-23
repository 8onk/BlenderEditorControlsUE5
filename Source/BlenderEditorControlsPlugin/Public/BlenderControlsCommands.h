// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once
#include "Framework/Commands/Commands.h"
#include "Styling/SlateStyle.h"

namespace BlenderControls
{
	/**
	 * Defines the UI commands and default keybindings for the Blender Editor Controls plugin.
	 * These commands are registered with the Unreal Engine Input system and can be rebound
	 * by users in the Editor Preferences under "Blender Editor Controls". 
	 */
	class FBlenderControlsCommands : public TCommands<FBlenderControlsCommands>
	{
	public:
		FBlenderControlsCommands();
		
		virtual void RegisterCommands() override;

		// --- Session Lifecycle Commands ---
		TSharedPtr<FUICommandInfo> CommandAccept;
		TSharedPtr<FUICommandInfo> CommandAcceptAlt;
		TSharedPtr<FUICommandInfo> CommandCancel;

		// --- Transformation Tool Commands ---
		TSharedPtr<FUICommandInfo> CommandTranslate;
		TSharedPtr<FUICommandInfo> CommandRotate;
		TSharedPtr<FUICommandInfo> CommandToggleTrackball;
		TSharedPtr<FUICommandInfo> CommandScale;

		// --- Numeric Input Helper Commands ---
		TSharedPtr<FUICommandInfo> CommandNumericBackspace;
		TSharedPtr<FUICommandInfo> CommandNumericToggleNegation;
		TSharedPtr<FUICommandInfo> CommandNumericToggleReciprocal;
		TSharedPtr<FUICommandInfo> CommandNumericCycleSlot;

		// --- Axis Constraint Commands ---
		TSharedPtr<FUICommandInfo> CommandAxisX;
		TSharedPtr<FUICommandInfo> CommandAxisY;
		TSharedPtr<FUICommandInfo> CommandAxisZ;

		TSharedPtr<FUICommandInfo> CommandDuplicateAndMove;

		// --- Modifier Commands ---
		TSharedPtr<FUICommandInfo> CommandSnapInvert;
		TSharedPtr<FUICommandInfo> CommandPrecisionMode;
	};
} // namespace BlenderControls
