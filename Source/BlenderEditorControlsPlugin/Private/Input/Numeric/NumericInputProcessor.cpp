#include "Input/Numeric/NumericInputProcessor.h"
#include "Input/Numeric/Helpers/NumericParser.h"
#include "Input/Numeric/Helpers/UnitFormatter.h"

namespace BlenderControls
{
	void FNumericInputProcessor::Initialize(int32 NumSlots, EBlenderNumericContext Context)
	{
		CurrentState.Initialize(NumSlots, Context);
	}

	void FNumericInputProcessor::OnToolSwitch(int32 NewNumSlots, EBlenderNumericContext NewContext,
	                                          const FBlenderNumericState& OldState)
	{
		CurrentState = OldState;
		CurrentState.ToolContext = NewContext;
		CurrentState.NumActiveSlots = NewNumSlots;
		CurrentState.ActiveSlotIndex = FMath::Min(CurrentState.ActiveSlotIndex, NewNumSlots);

		if (CurrentState.Slots.Num() < NewNumSlots)
		{
			CurrentState.Slots.SetNum(NewNumSlots);
		}

		if (CurrentState.Slots[CurrentState.ActiveSlotIndex].RawString.IsEmpty() && !CurrentState.Slots[CurrentState.
			ActiveSlotIndex].bIsAdditive)
		{
			CurrentState.InitialContext = NewContext;
		}
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
		if (Key == EKeys::Left || Key == EKeys::Right) return HandleNavigation(Key);

		if (HandleModifiers(KeyEvent)) return true;
		if (HandleCharacter(Key)) return true;

		return false; // Key not handled by numeric system
	}

	void FNumericInputProcessor::UpdateActiveNumSlots(int32 NewNumSlots)
	{
		CurrentState.NumActiveSlots = NewNumSlots;
	}

	bool FNumericInputProcessor::HandleCharacter(const FKey& Key)
	{
		if (TOptional<TCHAR> MaybeChar = FNumericParser::KeyToNumericChar(Key))
		{
			const TCHAR ParsedChar = MaybeChar.GetValue();

			//FOR EQUATION MODE
			// const bool bIsValidChar =
			// 	FChar::IsDigit(ParsedChar) || ParsedChar == TEXT('.') ||
			// 	(CurrentState.bIsEquationMode && (ParsedChar == TEXT('*') || ParsedChar == TEXT('+') || ParsedChar ==
			// 		TEXT('-') || ParsedChar ==
			// 		TEXT('/')));

			const bool bIsValidChar = FChar::IsDigit(ParsedChar) || ParsedChar == TEXT('.');

			if (!bIsValidChar)
			{
				return false;
			}

			FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

			if (ActiveSlot.bIsEmpty)
			{
				ActiveSlot.bIsEmpty = false;
				ActiveSlot.RawString = "";
				ActiveSlot.CursorIndex = 0;
			}

			ActiveSlot.RawString.InsertAt(ActiveSlot.CursorIndex, FString::Chr(ParsedChar));
			ActiveSlot.CursorIndex++;
			if (CurrentState.bIsUniformScaleMode)
			{
				PropagateUniformScale(CurrentState.ActiveSlotIndex);
			}

			return true;
		}
		return false;
	}

	bool FNumericInputProcessor::HandleBackspace()
	{
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		//ActiveSlot.DebugPrint();

		if (ActiveSlot.RawString.Len() > 0 && ActiveSlot.CursorIndex > 0)
		{
			// Case 1: Deleting from RawString
			ActiveSlot.RawString.RemoveAt(ActiveSlot.CursorIndex - 1);
			ActiveSlot.CursorIndex--;
		}
		else if (ActiveSlot.bIsAdditive)
		{
			// Case 2: We are in [45 m |] and hit backspace
			//PERHAPS NEED TO STORE UNIT AND RETRIEVE IT, otheriwse, when we subtract 45 cm, we get 45. 
			ActiveSlot.bIsAdditive = false;
			ActiveSlot.RawString = FString::SanitizeFloat(ActiveSlot.BaseValue);
			ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
			ActiveSlot.BaseValue = 0.f;
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
		const bool bWasSlotEmpty = ActiveSlot.bIsEmpty;

		float FinalValue = 0.f;
		EvaluateSlot(ActiveSlot, FinalValue);

		ActiveSlot.Finalize(FinalValue, bWasSlotEmpty);

		if (CurrentState.InitialContext != CurrentState.ToolContext)
		{
			CurrentState.InitialContext = CurrentState.ToolContext;
		}

		CurrentState.ActiveSlotIndex = (CurrentState.ActiveSlotIndex + 1) % CurrentState.NumActiveSlots;

		return true;
	}

	bool FNumericInputProcessor::HandleModifiers(const FKeyEvent& KeyEvent)
	{
		FString Char = KeyEvent.GetKey().GetDisplayName().ToString();
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		// --- Equation Mode Toggle (disabled for now) ---
		// if (Char == TEXT("Num *") && !CurrentState.bIsEquationMode)
		// {
		// 	float CurrentValue = 0.f;
		// 	EvaluateSlot(ActiveSlot, CurrentValue);
		//
		// 	CurrentState.bIsEquationMode = true;
		// 	ActiveSlot.bIsAdditive = false;
		// 	ActiveSlot.bIsNegative = false;
		// 	ActiveSlot.bIsReciprocal = false;
		// 	ActiveSlot.RawString = FString::SanitizeFloat(CurrentValue) + TEXT("*");
		// 	ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
		// 	if (ActiveSlot.bIsEmpty) ActiveSlot.bIsEmpty = false;
		// 	return true;
		// }
		//
		// if (KeyEvent.GetKey() == EKeys::Equals && KeyEvent.IsControlDown())
		// {
		// 	float CurrentValue = 0.f;
		// 	EvaluateSlot(ActiveSlot, CurrentValue);
		//
		// 	CurrentState.bIsEquationMode = false;
		// 	ActiveSlot.bIsNegative = false;
		// 	ActiveSlot.bIsReciprocal = false;
		// 	ActiveSlot.RawString = FString::SanitizeFloat(CurrentValue);
		// 	ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
		// 	return true;
		// }

		// --- Simple Modifiers ---
		if (CurrentState.bIsEquationMode) return false; // Handled by HandleCharacter

		if (Char == TEXT("Hyphen"))
		{
			if (ActiveSlot.bIsEmpty) ActiveSlot.bIsEmpty = false; // Activate slot
			ActiveSlot.bIsNegative = !ActiveSlot.bIsNegative;
			return true;
		}
		if (Char == TEXT("Num /"))
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
		if (ActiveSlot.bIsEmpty) return false;

		if (ActiveSlot.bIsAdditive)
		{
			// "Flatten" the state, just like Backspace does
			ActiveSlot.bIsAdditive = false;

			FString Suffix;
			switch (CurrentState.ToolContext)
			{
			case EBlenderNumericContext::Angle_Degrees:
				Suffix = TEXT("°");
				break;
			case EBlenderNumericContext::Distance:
				Suffix = TEXT("cm");
				break;
			case EBlenderNumericContext::Scale:
			default:
				Suffix = TEXT("");
				break;
			}
			ActiveSlot.RawString = FString::SanitizeFloat(ActiveSlot.BaseValue) + Suffix;
			ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
			ActiveSlot.BaseValue = 0.f;
		}

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

	FString FNumericInputProcessor::BuildSlotDisplayString(int32 SlotIndex)
	{
		if (!CurrentState.Slots.IsValidIndex(SlotIndex))
		{
			return FString();
		}

		FNumericInputSlot& Slot = CurrentState.Slots[SlotIndex];
		const bool bIsActiveSlot = (SlotIndex == CurrentState.ActiveSlotIndex);

		//Render master slot for all slave slots if they are empty for scale tool
		if (CurrentState.ToolContext == EBlenderNumericContext::Scale &&
			SlotIndex > 0 &&
			Slot.bIsEmpty &&
			!bIsActiveSlot)
		{
			return BuildSlotDisplayString(0);
		}

		//Slot.DebugPrint();

		// State 1: |NONE|
		if (Slot.bIsEmpty)
		{
			return (SlotIndex == CurrentState.ActiveSlotIndex) ? TEXT("|NONE|") : TEXT("NONE");
		}

		// State 2: Active Slot [ ... | ... ]
		if (SlotIndex == CurrentState.ActiveSlotIndex)
		{
			FString InnerDisplayString = TEXT("");

			// Additive prefix: [45 m 
			if (Slot.bIsAdditive)
			{
				float BaseForDisplay = Slot.BaseValue;
				EBlenderNumericContext ContextForFormatting = CurrentState.InitialContext;
				if (CurrentState.ToolContext == EBlenderNumericContext::Scale)
				{
					if (CurrentState.InitialContext != EBlenderNumericContext::Scale)
					{
						BaseForDisplay = FUnitFormatter::ConvertOnToolSwitch(
							BaseForDisplay, CurrentState.InitialContext, EBlenderNumericContext::Scale);
					}
					ContextForFormatting = EBlenderNumericContext::Scale;
				}

				InnerDisplayString += FUnitFormatter::FormatValue(BaseForDisplay, ContextForFormatting);
			}

			// RawString with cursor
			FString CursoredString = Slot.RawString.Left(Slot.CursorIndex) + TEXT("|") + Slot.RawString.RightChop(
				Slot.CursorIndex);
			InnerDisplayString += CursoredString;

			FString OuterDisplayString = TEXT("");
			// Modifiers: [-(1/(...
			if (Slot.bIsNegative) OuterDisplayString += TEXT("-(");
			if (Slot.bIsReciprocal) OuterDisplayString += TEXT("1/(");

			OuterDisplayString += InnerDisplayString;

			if (Slot.bIsReciprocal) OuterDisplayString += TEXT(")");
			if (Slot.bIsNegative) OuterDisplayString += TEXT(")");

			// Result: ] = 50 m
			float EvaluatedValue = 0.f;
			FString ResultString;
			if (EvaluateSlot(Slot, EvaluatedValue))
			{
				ResultString += FString::Printf(
					TEXT("] = %s"), *FUnitFormatter::FormatValue(EvaluatedValue, CurrentState.ToolContext));
			}
			else
			{
				ResultString += TEXT("] = INVALID");
			}
			return FString::Printf(TEXT("[%s%s"), *OuterDisplayString, *ResultString);
		}

		// State 3: Inactive, Set Slot
		float FinalValue = 0.f;
		EvaluateSlot(Slot, FinalValue); // This will just be BaseValue
		return FUnitFormatter::FormatValue(FinalValue, CurrentState.ToolContext);
	}

	bool FNumericInputProcessor::EvaluateSlot(FNumericInputSlot& Slot, float& OutResult) const
	{
		if (CurrentState.ToolContext == EBlenderNumericContext::Scale)
		{
			int32 CurrentSlotIndex = &Slot - &CurrentState.Slots[0];

			if (CurrentSlotIndex > 0 && Slot.bIsEmpty)
			{
				if (CurrentState.ActiveSlotIndex == CurrentSlotIndex)
				{
					OutResult = 1.0f;
					Slot.LastValidValue = 1.0f;
					return true;
				}
				else
				{
					FNumericInputSlot& MasterSlot = const_cast<FNumericInputSlot&>(CurrentState.Slots[0]);
					return EvaluateSlot(MasterSlot, OutResult);
				}
			}
		}

		float ParsedValue = 0.f;

		// 1. Get value from parser
		if (!FNumericParser::Evaluate(Slot.RawString, CurrentState.bIsEquationMode, ParsedValue))
		{
			OutResult = Slot.LastValidValue;
			return false; // INVALID
		}

		// 2. Handle conversion from previous tool
		float BaseValue = Slot.BaseValue;
		if (CurrentState.InitialContext != CurrentState.ToolContext)
		{
			BaseValue = FUnitFormatter::ConvertOnToolSwitch(BaseValue, CurrentState.InitialContext,
			                                                CurrentState.ToolContext);
			ParsedValue = FUnitFormatter::ConvertOnToolSwitch(ParsedValue, CurrentState.InitialContext,
			                                                  CurrentState.ToolContext);
		}

		// 3. Apply Additive
		float Value = Slot.bIsAdditive ? (BaseValue + ParsedValue) : ParsedValue;

		// 4. Apply Modifiers
		if (Slot.bIsReciprocal)
		{
			if (FMath::IsNearlyZero(Value)) return false;
			Value = 1.0f / Value;
		}
		if (Slot.bIsNegative)
		{
			Value = -Value;
		}

		OutResult = Value;
		Slot.LastValidValue = Value;
		return true;
	}

	float FNumericInputProcessor::GetTotalMagnitude()
	{
		float SumOfSquares = 0.f;

		float Slot0 = 0.f;
		float Slot1 = 0.f;
		float Slot2 = 0.f;

		if (CurrentState.Slots.IsValidIndex(0)) EvaluateSlot(CurrentState.Slots[0], Slot0);
		if (CurrentState.Slots.IsValidIndex(1)) EvaluateSlot(CurrentState.Slots[1], Slot1);
		if (CurrentState.Slots.IsValidIndex(2)) EvaluateSlot(CurrentState.Slots[2], Slot2);

		SumOfSquares += FMath::Square(Slot0);
		SumOfSquares += FMath::Square(Slot1);
		SumOfSquares += FMath::Square(Slot2);

		return FMath::Sqrt(SumOfSquares);
	}
}
