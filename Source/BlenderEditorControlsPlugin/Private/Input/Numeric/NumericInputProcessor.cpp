#include "Input/Numeric/NumericInputProcessor.h"
#include "Input/Numeric/Helpers/NumericParser.h"
#include "Input/Numeric/Helpers/UnitFormatter.h"

namespace BlenderControls
{
	void FNumericInputProcessor::Initialize(int32 NumSlots, EBlenderNumericContext Context)
	{
		CurrentState.Initialize(NumSlots, Context);
		OnExitNumericMode.Unbind();
	}

	void FNumericInputProcessor::OnToolSwitch(int32 NewNumSlots, EBlenderNumericContext NewContext,
	                                          const FBlenderNumericState& OldState)
	{
		EBlenderNumericContext PreviousContext = OldState.ToolContext;
		CurrentState = OldState; // Copy the entire state
		CurrentState.ToolContext = NewContext;
		CurrentState.PreviousContext = PreviousContext; // Remember where we came from

		// Fix slot count if it changes (e.g., Trackball to Move)
		if (CurrentState.Slots.Num() != NewNumSlots)
		{
			CurrentState.Slots.SetNum(NewNumSlots); // Adds/removes as needed
		}
		CurrentState.ActiveSlotIndex = FMath::Min(CurrentState.ActiveSlotIndex, NewNumSlots - 1);
	}


	bool FNumericInputProcessor::HandleInput(const FKeyEvent& KeyEvent)
	{
		FKey Key = KeyEvent.GetKey();
		// 1. Check for entry into numeric mode
		if (!CurrentState.bIsInNumericMode)
		{
			// Check for 0-9, ., -
			FString KeyStr = Key.GetDisplayName().ToString();
			if (KeyStr.IsNumeric() || KeyStr == TEXT(".") || KeyStr == TEXT("-"))
			{
				EnterNumericMode(KeyEvent);
				return true;
			}
			return false; // Not a numeric key, tool handles it
		}

		// 2. We are in numeric mode, process input
		if (Key == EKeys::BackSpace) return HandleBackspace();
		if (Key == EKeys::Tab) return HandleTab();
		if (Key == EKeys::Left) return HandleNavigation(Key);
		if (Key == EKeys::Right) return HandleNavigation(Key);

		if (HandleModifiers(KeyEvent)) return true;
		if (HandleCharacter(Key)) return true;

		return false; // Key not handled by numeric system
	}

	FText FNumericInputProcessor::GetHudText() const
	{
		return BuildHudString();
		// This is where you would check `!CurrentState.bIsInNumericMode`
		// and return your live translation HUD string instead.
	}

#pragma region InputHandlers

	void FNumericInputProcessor::ResetForAxisConstraint(int32 NumSlots)
	{
		CurrentState.Slots.Empty(NumSlots);
		for (int32 i = 0; i < NumSlots; ++i)
		{
			CurrentState.Slots.Add(FNumericInputSlot());
		}
		CurrentState.ActiveSlotIndex = 0;
	}

	bool FNumericInputProcessor::HandleCharacter(const FKey& Key)
	{
		FString Char = Key.GetDisplayName().ToString();
		bool bIsValidChar = Char.IsNumeric() || Char == TEXT(".") ||
		(CurrentState.bIsEquationMode && (Char == TEXT("*") || Char == TEXT("+") || Char == TEXT("-") || Char ==
			TEXT("/")));

		if (!bIsValidChar) return false;

		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		// First keypress in a |NONE| slot
		if (ActiveSlot.bIsEmpty)
		{
			ActiveSlot.bIsEmpty = false;
			ActiveSlot.RawString = "";
			ActiveSlot.CursorIndex = 0;
		}

		ActiveSlot.RawString.InsertAt(ActiveSlot.CursorIndex, Char);
		ActiveSlot.CursorIndex++;

		if (CurrentState.bIsUniformScaleMode)
		{
			PropagateUniformScale(CurrentState.ActiveSlotIndex);
		}

		return true;
	}

	bool FNumericInputProcessor::HandleBackspace()
	{
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		if (ActiveSlot.RawString.Len() > 0 && ActiveSlot.CursorIndex > 0)
		{
			// Case 1: Deleting from RawString
			ActiveSlot.RawString.RemoveAt(ActiveSlot.CursorIndex - 1);
			ActiveSlot.CursorIndex--;
		}
		else if (!ActiveSlot.bIsEmpty)
		{
			// Case 2: In [|] = 0 m state
			// Check if any *other* slot has a value
			bool bOtherSlotsHaveValue = false;
			for (int32 i = 0; i < CurrentState.Slots.Num(); ++i)
			{
				if (i == CurrentState.ActiveSlotIndex) continue;
				if (!CurrentState.Slots[i].bIsEmpty)
				{
					bOtherSlotsHaveValue = true;
					break;
				}
			}

			if (bOtherSlotsHaveValue)
			{
				// Go to |NONE|
				ActiveSlot.Reset(); // Resets to bIsEmpty = true
			}
			else
			{
				// Exit numeric mode
				CurrentState.bIsInNumericMode = false;
				OnExitNumericMode.ExecuteIfBound();
			}
		}
		else
		{
			// Case 3: In |NONE| state
			// Always exit
			CurrentState.bIsInNumericMode = false;
			OnExitNumericMode.ExecuteIfBound();
		}

		if (CurrentState.bIsUniformScaleMode)
		{
			PropagateUniformScale(CurrentState.ActiveSlotIndex);
		}

		return true;
	}

	bool FNumericInputProcessor::HandleTab()
	{
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		// 1. Finalize current slot
		float FinalValue = 0.f;
		EvaluateSlot(ActiveSlot, FinalValue); // Get the final value
		ActiveSlot.Finalize(FinalValue); // Resets modifiers, sets BaseValue

		// 2. Move to next slot
		CurrentState.ActiveSlotIndex = (CurrentState.ActiveSlotIndex + 1) % CurrentState.Slots.Num();

		// 3. Disable uniform scale mode. It's only for the first entry.
		CurrentState.bIsUniformScaleMode = false;

		return true;
	}

	bool FNumericInputProcessor::HandleModifiers(const FKeyEvent& KeyEvent)
	{
		FString Char = KeyEvent.GetKey().GetDisplayName().ToString();
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		// --- Equation Mode Toggle ---
		if (Char == TEXT("*") && !CurrentState.bIsEquationMode)
		{
			float CurrentValue = 0.f;
			EvaluateSlot(ActiveSlot, CurrentValue);

			CurrentState.bIsEquationMode = true;
			ActiveSlot.bIsAdditive = false;
			ActiveSlot.bIsNegative = false;
			ActiveSlot.bIsReciprocal = false;
			ActiveSlot.RawString = FString::SanitizeFloat(CurrentValue) + TEXT("*");
			ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
			if (ActiveSlot.bIsEmpty) ActiveSlot.bIsEmpty = false;
			return true;
		}

		if (KeyEvent.GetKey() == EKeys::Equals && KeyEvent.IsControlDown())
		{
			float CurrentValue = 0.f;
			EvaluateSlot(ActiveSlot, CurrentValue);

			CurrentState.bIsEquationMode = false;
			ActiveSlot.bIsNegative = false;
			ActiveSlot.bIsReciprocal = false;
			ActiveSlot.RawString = FString::SanitizeFloat(CurrentValue);
			ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
			return true;
		}

		// --- Simple Modifiers ---
		if (CurrentState.bIsEquationMode) return false; // Handled by HandleCharacter

		if (Char == TEXT("-"))
		{
			if (ActiveSlot.bIsEmpty) ActiveSlot.bIsEmpty = false; // Activate slot
			ActiveSlot.bIsNegative = !ActiveSlot.bIsNegative;
			return true;
		}
		if (Char == TEXT("/"))
		{
			if (ActiveSlot.bIsEmpty) ActiveSlot.bIsEmpty = false; // Activate slot
			ActiveSlot.bIsReciprocal = !ActiveSlot.bIsReciprocal;
			return true;
		}

		return false;
	}

	bool FNumericInputProcessor::HandleNavigation(const FKey& Key)
	{
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];
		if (ActiveSlot.bIsEmpty) return false; // Can't navigate in |NONE|

		if (Key == EKeys::Left)
		{
			ActiveSlot.CursorIndex = FMath::Max(0, ActiveSlot.CursorIndex - 1);
		}
		else if (Key == EKeys::Right)
		{
			ActiveSlot.CursorIndex = FMath::Min(ActiveSlot.RawString.Len(), ActiveSlot.CursorIndex + 1);
		}
		return true;
	}

	void FNumericInputProcessor::EnterNumericMode(const FKeyEvent& KeyEvent)
	{
		CurrentState.bIsInNumericMode = true;

		// Clear all slots
		for (FNumericInputSlot& Slot : CurrentState.Slots)
		{
			Slot.Reset();
		}

		// Make the first slot active and handle the key
		CurrentState.ActiveSlotIndex = 0;
		HandleInput(KeyEvent); // Re-run HandleInput, now that we're in numeric mode
	}

	void FNumericInputProcessor::PropagateUniformScale(int32 SourceSlotIndex)
	{
		if (!CurrentState.bIsUniformScaleMode) return;

		const FNumericInputSlot& SourceSlot = CurrentState.Slots[SourceSlotIndex];
		for (int32 i = 0; i < CurrentState.Slots.Num(); ++i)
		{
			if (i == SourceSlotIndex) continue;
			CurrentState.Slots[i] = SourceSlot;
		}
	}

	FText FNumericInputProcessor::BuildHudString() const
	{
		// This logic is highly specific to your tools.
		// This is a simple example for a 3-slot Move tool.
		if (CurrentState.Slots.Num() == 3)
		{
			FString Dx = BuildSlotDisplayString(0);
			FString Dy = BuildSlotDisplayString(1);
			FString Dz = BuildSlotDisplayString(2);

			// TODO: GetTotalMagnitude()

			return FText::FromString(FString::Printf(
				TEXT("Dx: %-10s Dy: %-10s Dz: %-10s"),
				*Dx, *Dy, *Dz
			));
		}

		if (CurrentState.Slots.Num() == 1)
		{
			FString Rot = BuildSlotDisplayString(0);
			// TODO: Add "along global x" string
			return FText::FromString(FString::Printf(TEXT("Rotation: %s"), *Rot));
		}

		return FText::FromString(TEXT("Unsupported Slot Count"));
	}

	FString FNumericInputProcessor::BuildSlotDisplayString(int32 SlotIndex) const
	{
		const FNumericInputSlot& Slot = CurrentState.Slots[SlotIndex];

		// State 1: |NONE|
		if (Slot.bIsEmpty)
		{
			return (SlotIndex == CurrentState.ActiveSlotIndex) ? TEXT("|NONE|") : TEXT("NONE");
		}

		// State 2: Active Slot [ ... | ... ]
		if (SlotIndex == CurrentState.ActiveSlotIndex)
		{
			FString DisplayString = "";

			// Additive prefix: [45 m 
			if (Slot.bIsAdditive)
			{
				// Note: This BaseValue needs to be formatted in its *original* context
				float BaseForDisplay = Slot.BaseValue;
				if (CurrentState.PreviousContext != CurrentState.ToolContext)
				{
					BaseForDisplay = FUnitFormatter::ConvertOnToolSwitch(
						BaseForDisplay, CurrentState.PreviousContext, CurrentState.ToolContext);
				}
				DisplayString += FUnitFormatter::FormatValue(BaseForDisplay, CurrentState.ToolContext) +
					TEXT(" ");
			}

			// RawString with cursor
			FString CursoredString = Slot.RawString.Left(Slot.CursorIndex) + TEXT("|") + Slot.RawString.RightChop(
				Slot.CursorIndex);

			// Modifiers: [-(1/(...
			if (Slot.bIsNegative) DisplayString += TEXT("-(");
			if (Slot.bIsReciprocal) DisplayString += TEXT("1/(");

			DisplayString += CursoredString;

			if (Slot.bIsReciprocal) DisplayString += TEXT(")");
			if (Slot.bIsNegative) DisplayString += TEXT(")");

			// Result: ] = 50 m
			float EvaluatedValue = 0.f;
			if (EvaluateSlot(Slot, EvaluatedValue))
			{
				DisplayString += FString::Printf(
					TEXT("] = %s"), *FUnitFormatter::FormatValue(EvaluatedValue, CurrentState.ToolContext));
			}
			else
			{
				DisplayString += TEXT("] = INVALID");
			}
			return FString::Printf(TEXT("[%s"), *DisplayString);
		}

		// State 3: Inactive, Set Slot
		float FinalValue = 0.f;
		EvaluateSlot(Slot, FinalValue); // This will just be BaseValue
		return FUnitFormatter::FormatValue(FinalValue, CurrentState.ToolContext);
	}

	bool FNumericInputProcessor::EvaluateSlot(const FNumericInputSlot& Slot, float& OutResult) const
	{
		float ParsedValue = 0.f;

		// 1. Get value from parser
		if (!FNumericParser::Evaluate(Slot.RawString, CurrentState.bIsEquationMode, ParsedValue))
		{
			OutResult = 0.f;
			return false; // INVALID
		}

		// 2. Handle conversion from previous tool
		float BaseValue = Slot.BaseValue;
		if (CurrentState.PreviousContext != CurrentState.ToolContext)
		{
			// We've switched tools. We must convert the BaseValue and ParsedValue
			// e.g., Move (Meters) -> Rotate (Degrees)
			BaseValue = FUnitFormatter::ConvertOnToolSwitch(BaseValue, CurrentState.PreviousContext,
			                                                CurrentState.ToolContext);
			ParsedValue = FUnitFormatter::ConvertOnToolSwitch(ParsedValue, CurrentState.PreviousContext,
			                                                  CurrentState.ToolContext);
		}

		// 3. Apply Additive
		float Value = Slot.bIsAdditive ? (BaseValue + ParsedValue) : ParsedValue;

		// 4. Apply Modifiers
		if (Slot.bIsReciprocal)
		{
			if (FMath::IsNearlyZero(Value)) return false; // Divide by zero
			Value = 1.0f / Value;
		}
		if (Slot.bIsNegative)
		{
			Value = -Value;
		}

		OutResult = Value;
		return true;
	}

	float FNumericInputProcessor::GetTotalMagnitude() const
	{
		return 0.0f;
	}

#pragma endregion InputHandlers
}
