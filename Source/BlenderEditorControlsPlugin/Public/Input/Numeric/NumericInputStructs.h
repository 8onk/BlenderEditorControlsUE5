#pragma once

#include "CoreMinimal.h"

namespace BlenderControls
{
	// Enum to define the context for parsing and formatting
	enum class EBlenderNumericContext : uint8
	{
		Distance, // Base unit is meters
		Angle_Degrees, // Base unit is degrees
		Angle_Radians, // Base unit is radians (internal for conversion)
		Scale // Base unit is unitless
	};

	// Represents the state of a SINGLE input slot (e.g., "Dx")
	struct FNumericInputSlot
	{
		FString RawString;
		int32 CursorIndex = 0;
		float BaseValue = 0.f;
		float LastValidValue = BaseValue;
		bool bIsAdditive = false;
		bool bIsEmpty = true; // This is for the |NONE| state
		bool bIsNegative = false;
		bool bIsReciprocal = false;

		void Reset()
		{
			RawString = "";
			CursorIndex = 0;
			BaseValue = 0.f;
			bIsAdditive = false;
			bIsEmpty = true;
			bIsNegative = false;
			bIsReciprocal = false;
			LastValidValue = BaseValue;
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
			UE_LOG(LogTemp, Warning,
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

	// Represents the ENTIRE state of the numeric input system
	struct FBlenderNumericState
	{
		TArray<FNumericInputSlot> Slots;
		int32 ActiveSlotIndex = 0;
		int32 NumActiveSlots = 0;
		bool bIsInNumericMode = false;
		bool bIsUniformScaleMode = false;
		bool bIsEquationMode = false;
		EBlenderNumericContext ToolContext = EBlenderNumericContext::Distance;
		EBlenderNumericContext InitialContext = EBlenderNumericContext::Distance;

		void Initialize(int32 NumSlots, EBlenderNumericContext Context)
		{
			Slots.Empty(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i)
			{
				Slots.Add(FNumericInputSlot());
				NumActiveSlots++;
			}
			ActiveSlotIndex = 0;
			bIsInNumericMode = false;
			bIsUniformScaleMode = false; // Must be set true by ScaleTool
			ToolContext = Context;
			InitialContext = Context;
		}

		void DebugPrint() const
		{
			UE_LOG(LogTemp, Warning, TEXT("=== BlenderNumericState ==="));
			UE_LOG(LogTemp, Warning, TEXT("ActiveSlotIndex=%d, InNumericMode=%d, UniformScale=%d, EquationMode=%d"),
				ActiveSlotIndex,
				bIsInNumericMode,
				bIsUniformScaleMode,
				bIsEquationMode
			);
			UE_LOG(LogTemp, Warning, TEXT("ToolContext=%d, InitialContext=%d"), (int32)ToolContext, (int32)InitialContext);

			for (int32 i = 0; i < Slots.Num(); ++i)
			{
				Slots[i].DebugPrint(i);
			}
			UE_LOG(LogTemp, Warning, TEXT("==========================="));
		}
	};
}
