#include "Commands/BlenderEditorControlsPluginCommands.h"

#define LOCTEXT_NAMESPACE "FBlenderEditorControlsPluginCommands"

namespace BlenderControls
{
	FBlenderEditorControlsPluginCommands::FBlenderEditorControlsPluginCommands()
		: TCommands<FBlenderEditorControlsPluginCommands>(
			TEXT("BlenderEditorControlsPlugin"), // Plugin context name
			NSLOCTEXT("Contexts", "BlenderEditorControlsPlugin", "Blender Editor Controls"),
			// Display name in keybindings UI
			FName("LevelEditor"), // No Parent context name
			FName(TEXT("DefaultStyle")) // Icon Style Set
		)
	{
	}

	void FBlenderEditorControlsPluginCommands::RegisterCommands()
	{
		UI_COMMAND(CommandTogglePlugin, "Toggle Blender Controls", "Toggles the Blender-style controls on/off",
		           EUserInterfaceActionType::ToggleButton, FInputChord());

		// Confirmation commands
		UI_COMMAND(CommandAcceptAlt, "Accept (Space)", "Accept with Space",
		           EUserInterfaceActionType::Button, FInputChord(EKeys::SpaceBar));
		UI_COMMAND(CommandAccept, "Accept", "Accept the current transformation (Enter)",
		           EUserInterfaceActionType::Button, FInputChord(EKeys::Enter));
		UI_COMMAND(CommandCancel, "Cancel", "Cancel the current transformation (Esc)", EUserInterfaceActionType::Button,
		           FInputChord(EKeys::Escape));

		// Transform mode commands  
		UI_COMMAND(CommandTranslate, "Move", "Switch to translation mode (G)",
		           EUserInterfaceActionType::RadioButton, FInputChord(EKeys::G));
		UI_COMMAND(CommandRotate, "Rotate", "Switch to rotation mode (R)", EUserInterfaceActionType::RadioButton,
		           FInputChord(EKeys::R));
		UI_COMMAND(CommandScale, "Scale", "Switch to scale mode (S)", EUserInterfaceActionType::RadioButton,
		           FInputChord(EKeys::S));

		// Axis constraint commands
		UI_COMMAND(CommandAxisX, "X Axis", "Constrain to X axis (X)", EUserInterfaceActionType::RadioButton,
		           FInputChord(EKeys::X));
		UI_COMMAND(CommandAxisY, "Y Axis", "Constrain to Y axis (Y)", EUserInterfaceActionType::RadioButton,
		           FInputChord(EKeys::Y));
		UI_COMMAND(CommandAxisZ, "Z Axis", "Constrain to Z axis (Z)", EUserInterfaceActionType::RadioButton,
		           FInputChord(EKeys::Z));

		// Numeric mode helpers
		UI_COMMAND(CommandNumericBackspace, "Numeric: Backspace", "Delete last digit",
		           EUserInterfaceActionType::Button, FInputChord(EKeys::BackSpace));
		UI_COMMAND(CommandNumericToggleNegation, "Numeric: Toggle Sign", "Toggle +/-",
		           EUserInterfaceActionType::Button, FInputChord(EKeys::Hyphen));
		UI_COMMAND(CommandNumericToggleReciprocal, "Numeric: Reciprocal", "Toggle 1/x",
		           EUserInterfaceActionType::Button, FInputChord(EKeys::Slash));
		UI_COMMAND(CommandNumericCycleSlot, "Numeric: Cycle Slot", "Cycle X/Y/Z slot",
		           EUserInterfaceActionType::Button, FInputChord(EKeys::Tab));

		UI_COMMAND(CommandDuplicateAndMove, "Duplicate and Move",
		           "Duplicates the selection and immediately begins a move operation.",
		           EUserInterfaceActionType::Button, FInputChord(EKeys::D, EModifierKey::Shift));
	}
} // namespace BlenderControls

#undef LOCTEXT_NAMESPACE
