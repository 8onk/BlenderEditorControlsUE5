// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	enum class EBlenderNumericContext : uint8;

	/**
	 * Utility class responsible for cross-context value conversion and 
	 * string formatting for the STransformHUD.
	 */
	class FUnitFormatter
	{
	public:
		/**
		 * Adjusts numeric values when switching tools mid-session.
		 * (e.g., Converting a distance in CM to a rotation in Degrees (radians -> degrees)).
		 * 
		 * @param Value The current raw float from the NumericProcessor.
		 * @param From  The context of the tool being deactivated.
		 * @param To    The context of the tool being activated.
		 * @return The converted float scaled for the new context.
		 */
		static float ConvertOnToolSwitch(float Value, EBlenderNumericContext From, EBlenderNumericContext To);

		/**
		 * Converts a raw float into a formatted string containing the unit.
		 * 
		 * @param Value The raw numeric value.
		 * @param Context The context dictating which unit string to append.
		 * @return A UI-ready formatted string (e.g., "1.5m").
		 */
		static FString FormatValue(float Value, EBlenderNumericContext Context);
	};
}
