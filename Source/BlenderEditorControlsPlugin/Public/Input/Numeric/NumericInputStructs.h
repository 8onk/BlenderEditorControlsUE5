#pragma once

#include "CoreMinimal.h"
#include "BlenderEditorControls.h"

namespace BlenderControls
{
	// Enum to define the context for parsing and formatting
	enum class EBlenderNumericContext : uint8
	{
		Distance, // Base unit is meters
		Angle_Degrees, // Base unit is degrees
		Scale // Base unit is unitless
	};

	// Represents the state of a SINGLE input slot (e.g., "Dx")
	struct FNumericInputSlot
	{
		FNumericInputSlot()
			: Context(EBlenderNumericContext::Distance) // Provide a sensible default
		{
		}

		FNumericInputSlot(EBlenderNumericContext InContext)
			: Context(InContext)
		{
			Reset(InContext);
		}

		FString RawString = "";
		int32 CursorIndex = 0;
		float BaseValue = 0.f;
		float LastValidValue = BaseValue;
		bool bIsAdditive = false;
		bool bIsEmpty = true;
		bool bIsNegative = false;
		bool bIsReciprocal = false;
		EBlenderNumericContext Context;

		void Reset(EBlenderNumericContext InContext)
		{
			RawString = "";
			CursorIndex = 0;
			BaseValue = 0.f;
			LastValidValue = BaseValue;
			bIsAdditive = false;
			bIsEmpty = true;
			bIsNegative = false;
			bIsReciprocal = false;
			Context = InContext;
		}

		// Resets modifiers and string, but keeps BaseValue
		void Finalize(float FinalValue, bool bWasEmpty)
		{
			RawString = "";
			CursorIndex = 0;
			BaseValue = FinalValue;
			bIsAdditive = !bWasEmpty;
			LastValidValue = FinalValue;
			bIsEmpty = bWasEmpty;
			bIsNegative = false;
			bIsReciprocal = false;
		}

		void DebugPrint(int32 Index = -1) const
		{
			FString Prefix = (Index >= 0) ? FString::Printf(TEXT("Slot[%d]: "), Index) : TEXT("Slot: ");
			UE_LOG(LogBlenderEditorControls, Warning,
			       TEXT("%sRawString='%s', Cursor=%d, BaseValue=%.3f, Additive=%d, Empty=%d, Negative=%d, Reciprocal=%d"
			       ),
			       *Prefix,
			       *RawString,
			       CursorIndex,
			       BaseValue,
			       bIsAdditive,
			       bIsEmpty,
			       bIsNegative,
			       bIsReciprocal
			);
		}
	};

	/**
	 * Represents the ENTIRE state of the numeric input system
	 */
	struct FBlenderNumericState
	{
		TArray<FNumericInputSlot> Slots;
		int32 ActiveSlotIndex = 0;
		int32 NumActiveSlots = 0;
		bool bIsInNumericMode = false;
		bool bIsUniformScaleMode = false;
		bool bSlotUpdatedSinceSwitch = false;
		EBlenderNumericContext CurrentContext = EBlenderNumericContext::Distance;

		void Initialize(int32 NumSlots, EBlenderNumericContext Context)
		{
			Slots.Empty(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i)
			{
				Slots.Emplace(Context);
			}
			ActiveSlotIndex = 0;
			bIsInNumericMode = false;
			bIsUniformScaleMode = false; // Must be set true by ScaleTool
			NumActiveSlots = NumSlots;
			CurrentContext = Context;
		}

		void DebugPrint() const
		{
			for (int32 i = 0; i < Slots.Num(); ++i)
			{
				Slots[i].DebugPrint(i);
			}
		}
	};
}
