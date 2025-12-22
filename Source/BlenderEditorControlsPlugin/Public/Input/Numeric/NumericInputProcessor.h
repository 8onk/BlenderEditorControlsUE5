#pragma once
#include "NumericInputStructs.h"

namespace BlenderControls
{
	enum class EBlenderNumericContext : uint8;

	/**
		 * A modal state-processor that captures and validates Blender-style numeric input.
		 *
		 * Logic Flow:
		 * 1. Captures raw FKey events from the Slate Input Stack.
		 * 2. Manages a "CurrentState" consisting of 1-3 numeric slots (X, Y, Z).
		 * 3. Handles "Uniform Mode" (syncing values across slots for scale tool) and "Additive Mode" (sequential operations).
		 * 4. Provides formatted strings for HUD rendering.
		 */
	class FNumericInputProcessor
	{
	public:
		FBlenderNumericState CurrentState;

		/** Prepares the processor for a specific tool context (e.g., 1 slot for Rotation, 3 for Translation). */
		void Initialize(int32 NumSlots, EBlenderNumericContext Context);

		/** Evaluates the string in a slot to a float, returning false if the string is malformed. */
		bool EvaluateSlot(FNumericInputSlot& Slot, float& OutResult);

		/** Migrates and converts existing numeric data when the user switches tools (e.g., G -> R) mid-session. */
		void OnToolSwitch(int32 NewNumSlots, EBlenderNumericContext NewContext, const FBlenderNumericState& OldState);

		/**
		 * The primary entry point for Slate KeyEvents passed from FInputProcessor
		 * 
		 * @return True if the key was handled (consumed) by the numeric system.
		 */
		bool HandleInput(const FKeyEvent& KeyEvent);
		bool IsInNumericMode() const { return CurrentState.bIsInNumericMode; }
		void SetUniformScaleMode(bool bIsUniform) { CurrentState.bIsUniformScaleMode = bIsUniform; }

		/** Delegate fired when the user backspaces out of an empty session, signaling the tool to return to mouse-control. */
		DECLARE_DELEGATE(FOnExitNumericMode);
		FOnExitNumericMode OnExitNumericMode;

		void UpdateActiveNumSlots(int32 NewNumSlots);

		float GetTotalMagnitude();
		/** Returns a formatted string for the HUD (e.g., "X: 15.5cm"). */
		FString BuildSlotDisplayString(int32 SlotIndex);

	private:
		void EnterNumericMode(const FKeyEvent& KeyEvent);
		bool HandleCharacter(const FKey& Key);
		void ConvertSlotValue(FNumericInputSlot& ActiveSlot) const;
		bool HandleBackspace();
		bool HandleTab();
		bool HandleModifiers(const FKeyEvent& KeyEvent);
		bool HandleNavigation(const FKey& Key);

		/** Forces values in other slots to match the modified SourceSlot during Uniform Scaling. */
		void PropagateUniformScale(int32 SourceSlotIndex);

		/**
		 * When tabbing out of a slot, the internal string representing the
		 * slot input gets emptied. When tabbing into a slot containing
		 * input, the string needs to match the slot input to be able to
		 * update the slot string.
		 *
		 * This function therefore loads the HUD input into the string representing this slot. 
		 * 
		 */
		void FlattenAdditiveSlotIfEmpty();
	};
}
