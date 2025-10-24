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
		}

		// Resets modifiers and string, but keeps BaseValue
		void Finalize(float FinalValue)
		{
			RawString = "";
			CursorIndex = 0;
			BaseValue = FinalValue;
			bIsAdditive = true; // Ready for next additive input
			bIsEmpty = FMath::IsNearlyZero(FinalValue); // Becomes |NONE| if value is 0
			bIsNegative = false;
			bIsReciprocal = false;
		}
	};

	// Represents the ENTIRE state of the numeric input system
	struct FBlenderNumericState
	{
		TArray<FNumericInputSlot> Slots;
		int32 ActiveSlotIndex = 0;
		bool bIsInNumericMode = false;
		bool bIsUniformScaleMode = false;
		bool bIsEquationMode = false;
		EBlenderNumericContext ToolContext = EBlenderNumericContext::Distance;
		EBlenderNumericContext PreviousContext = EBlenderNumericContext::Distance;

		void Initialize(int32 NumSlots, EBlenderNumericContext Context)
		{
			Slots.Empty(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i)
			{
				Slots.Add(FNumericInputSlot());
			}
			ActiveSlotIndex = 0;
			bIsInNumericMode = false;
			bIsUniformScaleMode = false; // Must be set true by ScaleTool
			ToolContext = Context;
			PreviousContext = Context;
		}
	};
}
