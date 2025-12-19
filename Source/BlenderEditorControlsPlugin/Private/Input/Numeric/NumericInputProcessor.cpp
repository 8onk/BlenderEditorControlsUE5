#include "Input/Numeric/NumericInputProcessor.h"
#include "BlenderControlsCommands.h"
#include "Input/Numeric/Helpers/NumericParser.h"
#include "Input/Numeric/Helpers/UnitFormatter.h"

namespace BlenderControls
{
	static bool MatchesCommand(const FKeyEvent& KeyEvent, const TSharedPtr<FUICommandInfo>& Command)
	{
		if (!Command.IsValid()) return false;

		const FInputChord KeyChord(
			KeyEvent.GetKey(),
			KeyEvent.IsShiftDown(),
			KeyEvent.IsControlDown(),
			KeyEvent.IsAltDown(),
			KeyEvent.IsCommandDown()
		);

		return Command->HasActiveChord(KeyChord);
	}

	void FNumericInputProcessor::Initialize(int32 NumSlots, EBlenderNumericContext Context)
	{
		CurrentState.Initialize(NumSlots, Context);
	}

	void FNumericInputProcessor::OnToolSwitch(int32 NewNumSlots, EBlenderNumericContext NewContext,
	                                          const FBlenderNumericState& OldState)
	{
		CurrentState = OldState;
		CurrentState.CurrentContext = NewContext;
		CurrentState.NumActiveSlots = NewNumSlots;

		if (CurrentState.Slots.Num() < NewNumSlots)
		{
			CurrentState.Slots.SetNum(NewNumSlots);
		}

		CurrentState.bSlotUpdatedSinceSwitch = false;
	}


	bool FNumericInputProcessor::HandleInput(const FKeyEvent& KeyEvent)
	{
		const auto& Cmd = FBlenderControlsCommands::Get();
		FKey Key = KeyEvent.GetKey();
		if (!CurrentState.bIsInNumericMode)
		{
			FString KeyStr = Key.GetDisplayName().ToString();
			if (KeyStr.IsNumeric() || KeyStr == TEXT(".") || KeyStr == TEXT("-"))
			{
				EnterNumericMode(KeyEvent);
				return true;
			}
		}
		if (MatchesCommand(KeyEvent, Cmd.CommandNumericCycleSlot)) return HandleTab();

		if (!CurrentState.Slots.IsValidIndex(CurrentState.ActiveSlotIndex))
		{
			return false;
		}

		if (MatchesCommand(KeyEvent, Cmd.CommandNumericBackspace)) return HandleBackspace();
		if (Key == EKeys::Left || Key == EKeys::Right) return HandleNavigation(Key);

		if (HandleModifiers(KeyEvent))
		{
			CurrentState.bSlotUpdatedSinceSwitch = true;
			return true;
		}

		if (HandleCharacter(Key))
		{
			CurrentState.bSlotUpdatedSinceSwitch = true;
			return true;
		}

		return false;
	}

	void FNumericInputProcessor::UpdateActiveNumSlots(int32 NewNumSlots)
	{
		CurrentState.NumActiveSlots = NewNumSlots;
		
		if (CurrentState.Slots.Num() < NewNumSlots)
		{
			CurrentState.Slots.SetNum(NewNumSlots);
		}
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

			//FlattenAdditiveSlotIfEmpty();
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

	void FNumericInputProcessor::ConvertSlotValue(FNumericInputSlot& ActiveSlot) const
	{
		if (ActiveSlot.Context != CurrentState.CurrentContext)
		{
			ActiveSlot.RawString = FUnitFormatter::FormatValue(ActiveSlot.BaseValue, CurrentState.CurrentContext);
			ActiveSlot.Context = CurrentState.CurrentContext;
		}
	}

	bool FNumericInputProcessor::HandleBackspace()
	{
		FlattenAdditiveSlotIfEmpty();
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		if (ActiveSlot.RawString.Len() > 0 && ActiveSlot.CursorIndex > 0)
		{
			ActiveSlot.RawString.RemoveAt(ActiveSlot.CursorIndex - 1);
			ActiveSlot.CursorIndex--;
		}
		else if (ActiveSlot.bIsAdditive)
		{
			ActiveSlot.bIsAdditive = false;
			ConvertSlotValue(ActiveSlot);
			ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
			ActiveSlot.BaseValue = 0.f;

			//Subtract immediately the fetched string
			ActiveSlot.RawString.RemoveAt(ActiveSlot.CursorIndex - 1);
			ActiveSlot.CursorIndex--;
		}
		else if (!ActiveSlot.bIsEmpty && ActiveSlot.CursorIndex < 1 && ActiveSlot.RawString.Len() < 1)
		{
			// Case 2: In "[|] =" state
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
				ActiveSlot.Reset(CurrentState.CurrentContext); // Resets to bIsEmpty = true
			}
			else
			{
				// Exit numeric mode
				CurrentState.bIsInNumericMode = false;
				OnExitNumericMode.ExecuteIfBound();
			}
		}
		else if (ActiveSlot.bIsEmpty)
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

		CurrentState.bSlotUpdatedSinceSwitch = true;
		return true;
	}

	bool FNumericInputProcessor::HandleTab()
	{
		if (CurrentState.Slots.IsValidIndex(CurrentState.ActiveSlotIndex))
		{
			FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];
			const bool bWasSlotEmpty = ActiveSlot.bIsEmpty;
			float FinalValue = 0.f;
			EvaluateSlot(ActiveSlot, FinalValue);
			CurrentState.Slots[CurrentState.ActiveSlotIndex].Context = CurrentState.CurrentContext;

			ActiveSlot.Finalize(FinalValue, bWasSlotEmpty);
		}

		CurrentState.ActiveSlotIndex = (CurrentState.ActiveSlotIndex + 1) % CurrentState.NumActiveSlots;
		if (CurrentState.Slots[CurrentState.ActiveSlotIndex].Context != CurrentState.CurrentContext)
		{
			CurrentState.Slots[CurrentState.ActiveSlotIndex].Context = CurrentState.CurrentContext;
		}

		return true;
	}

	bool FNumericInputProcessor::HandleModifiers(const FKeyEvent& KeyEvent)
	{
		const auto& Cmd = FBlenderControlsCommands::Get();
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

		if (CurrentState.bIsEquationMode) return false; // Handled by HandleCharacter
		FlattenAdditiveSlotIfEmpty();

		if (MatchesCommand(KeyEvent, Cmd.CommandNumericToggleNegation))
		{
			if (ActiveSlot.bIsEmpty) ActiveSlot.bIsEmpty = false; // Activate slot
			ActiveSlot.bIsNegative = !ActiveSlot.bIsNegative;
			return true;
		}
		if (MatchesCommand(KeyEvent, Cmd.CommandNumericToggleReciprocal))
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

		FlattenAdditiveSlotIfEmpty(/*bUpdateContext=*/false);

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
			Slot.Reset(CurrentState.CurrentContext);
		}

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

	void FNumericInputProcessor::FlattenAdditiveSlotIfEmpty(bool bUpdateContext)
	{
		FNumericInputSlot& ActiveSlot = CurrentState.Slots[CurrentState.ActiveSlotIndex];

		if (ActiveSlot.bIsAdditive && ActiveSlot.RawString.IsEmpty())
		{
			ActiveSlot.bIsAdditive = false;
			ActiveSlot.RawString = FUnitFormatter::FormatValue(ActiveSlot.BaseValue, ActiveSlot.Context);
			ActiveSlot.CursorIndex = ActiveSlot.RawString.Len();
			ActiveSlot.BaseValue = 0.f;
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
		if (CurrentState.CurrentContext == EBlenderNumericContext::Scale &&
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
		if (bIsActiveSlot)
		{
			FString InnerDisplayString = TEXT("");

			// Additive prefix: [45 m 
			if (Slot.bIsAdditive)
			{
				InnerDisplayString += FUnitFormatter::FormatValue(Slot.BaseValue, Slot.Context);
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
					TEXT("] = %s"), *FUnitFormatter::FormatValue(EvaluatedValue, CurrentState.CurrentContext));
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
		return FUnitFormatter::FormatValue(FinalValue, CurrentState.CurrentContext);
	}

	bool FNumericInputProcessor::EvaluateSlot(FNumericInputSlot& Slot, float& OutResult)
	{
		if (CurrentState.CurrentContext == EBlenderNumericContext::Scale)
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

		if (!Slot.RawString.IsEmpty() && CurrentState.bSlotUpdatedSinceSwitch)
		{
			if (FNumericParser::HasInvalidUnitsForContext(Slot.RawString, CurrentState.CurrentContext))
			{
				OutResult = Slot.LastValidValue;
				return false;
			}
			else
			{
				Slot.Context = CurrentState.CurrentContext;
			}
		}

		float ParsedValue = 0.f;

		// 1. Get value from parser
		if (!FNumericParser::Evaluate(Slot.RawString, CurrentState.bIsEquationMode, ParsedValue))
		{
			OutResult = Slot.LastValidValue;
			return false; // INVALID
		}

		// CurrentState.DebugPrint();
		// 2. Handle conversion from previous tool
		float BaseValue = Slot.BaseValue;
		if (Slot.Context != CurrentState.CurrentContext)
		{
			BaseValue = FUnitFormatter::ConvertOnToolSwitch(BaseValue, Slot.Context,
			                                                CurrentState.CurrentContext);
			ParsedValue = FUnitFormatter::ConvertOnToolSwitch(ParsedValue, Slot.Context,
			                                                  CurrentState.CurrentContext);

			int32 CurrentSlotIndex = &Slot - &CurrentState.Slots[0];
			if (CurrentSlotIndex != CurrentState.ActiveSlotIndex)
			{
				Slot.Context = CurrentState.CurrentContext;
				Slot.BaseValue = BaseValue;
			}
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
