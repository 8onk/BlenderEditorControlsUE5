#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	enum class EBlenderNumericContext : uint8;

	/**
	 * Utility class for sanitizing and evaluating user-typed strings into float values.
	 * Supports raw numbers, basic arithmetic, and unit-aware parsing (e.g., "50cm").
	 */
	class FNumericParser
	{
	public:
		/**
		 * Main entry point for string-to-float conversion.
		 * @param RawString - The raw user input from the NumericProcessor.
		 * @param OutResult - The final calculated float value.
		 * @return True if the string was successfully parsed into a valid number.
		 */
		static bool Evaluate(const FString& RawString, float& OutResult);
		static TOptional<TCHAR> KeyToNumericChar(const FKey& Key);
		static bool HasInvalidUnitsForContext(const FString& RawInput, EBlenderNumericContext Context);

	private:
		static bool EvaluateSimple(const FString& RawString, float& OutResult);
		static float CountOccurrences(const FString& RawString, TCHAR CharToCount);
		static bool EvaluateAdditiveWithUnit(const FString& Trimmed, float& OutResult);
	};
}
