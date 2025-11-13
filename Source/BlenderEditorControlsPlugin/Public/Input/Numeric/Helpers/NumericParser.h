#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	enum class EBlenderNumericContext : uint8;

	class FNumericParser
	{
	public:
		static bool Evaluate(const FString& RawString, bool bIsEquationMode, float& OutResult);
		static TOptional<TCHAR> KeyToNumericChar(const FKey& Key);
		static bool HasInvalidUnitsForContext(const FString& RawInput, EBlenderNumericContext Context);

	private:
		static bool EvaluateSimple(const FString& RawString, float& OutResult);
		static bool EvaluateEquation(const FString& RawString, float& OutResult);
		static float CountOccurrences(const FString& RawString, TCHAR CharToCount);
		static bool EvaluateAdditiveWithUnit(const FString& Trimmed, float& OutResult);
	};
}
