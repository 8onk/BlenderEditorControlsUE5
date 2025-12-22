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
		 * (e.g., Converting a distance in CM to a rotation in Degrees (radians -> degrees).
		 * 
		 * @param Value - The current raw float from the NumericProcessor.
		 * @param From  - The context of the tool being deactivated.
		 * @param To    - The context of the tool being activated.
		 * @return The converted float scaled for the new context.
		 */
		static float ConvertOnToolSwitch(float Value, EBlenderNumericContext From, EBlenderNumericContext To);

		/**
		 * Converts a raw float into a string containing the unit
		 * based on the current tool.
		 */
		static FString FormatValue(float Value, EBlenderNumericContext Context);
	};
}
