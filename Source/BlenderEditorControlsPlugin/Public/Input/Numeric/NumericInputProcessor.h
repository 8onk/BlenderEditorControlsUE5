#pragma once
#include "NumericInputStructs.h"

namespace BlenderControls
{
	enum class EBlenderNumericContext : uint8;

	class FNumericInputProcessor
	{
	public:
		FBlenderNumericState CurrentState;

		// Call this from your tool's BeginTool()
		void Initialize(int32 NumSlots, EBlenderNumericContext Context);

		bool EvaluateSlot(FNumericInputSlot& Slot, float& OutResult);

		// Call this from your tool's OnToolSwitch()
		void OnToolSwitch(int32 NewNumSlots, EBlenderNumericContext NewContext, const FBlenderNumericState& OldState);

		// Call this from your tool's OnKeyPressed()
		// Returns true if the key was "consumed" by the numeric system.
		bool HandleInput(const FKeyEvent& KeyEvent);

		bool IsInNumericMode() const { return CurrentState.bIsInNumericMode; }

		// Special function for ScaleTool to call on entry
		void SetUniformScaleMode(bool bIsUniform) { CurrentState.bIsUniformScaleMode = bIsUniform; }

		// Fires when backspace on |NONE| exits the mode
		DECLARE_DELEGATE(FOnExitNumericMode);
		FOnExitNumericMode OnExitNumericMode;

		void UpdateActiveNumSlots(int32 NewNumSlots);

		float GetTotalMagnitude();
		FString BuildSlotDisplayString(int32 SlotIndex);

	private:
		// --- Input Handlers ---
		bool HandleCharacter(const FKey& Key);
		void ConvertSlotValue(FNumericInputSlot& ActiveSlot) const;
		bool HandleBackspace();
		bool HandleTab();
		bool HandleModifiers(const FKeyEvent& KeyEvent);
		bool HandleNavigation(const FKey& Key);
		void PropagateUniformScale(int32 SourceSlotIndex);
		void FlattenAdditiveSlotIfEmpty(bool bUpdateContext = true);
		// --- State Management ---
		void EnterNumericMode(const FKeyEvent& KeyEvent);
	};
}
