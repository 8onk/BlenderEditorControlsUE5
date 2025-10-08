#pragma once
#include "Framework/Commands/Commands.h"
#include "Styling/SlateStyle.h"

namespace BlenderControls
{
	class FBlenderEditorControlsPluginCommands : public TCommands<FBlenderEditorControlsPluginCommands>
	{
	public:
		FBlenderEditorControlsPluginCommands();

		/** TCommands interface */
		virtual void RegisterCommands() override;

		TSharedPtr<FUICommandInfo> CommandTogglePlugin;

		// Accept / Cancel
		TSharedPtr<FUICommandInfo> CommandAccept;
		TSharedPtr<FUICommandInfo> CommandAcceptAlt;
		TSharedPtr<FUICommandInfo> CommandCancel;

		//Tools
		TSharedPtr<FUICommandInfo> CommandTranslate;
		TSharedPtr<FUICommandInfo> CommandRotate;
		TSharedPtr<FUICommandInfo> CommandToggleTrackball;
		TSharedPtr<FUICommandInfo> CommandScale;

		// Numeric helpers
		TSharedPtr<FUICommandInfo> CommandNumericBackspace;
		TSharedPtr<FUICommandInfo> CommandNumericToggleNegation;
		TSharedPtr<FUICommandInfo> CommandNumericToggleReciprocal;
		TSharedPtr<FUICommandInfo> CommandNumericCycleSlot;

		// Axis locks
		TSharedPtr<FUICommandInfo> CommandAxisX;
		TSharedPtr<FUICommandInfo> CommandAxisY;
		TSharedPtr<FUICommandInfo> CommandAxisZ;

		TSharedPtr<FUICommandInfo> CommandDuplicateAndMove;

		//Modifiers
		TSharedPtr<FUICommandInfo> CommandSnapInvert;
		TSharedPtr<FUICommandInfo> CommandPrecisionMode;
	};
} // namespace BlenderControls
