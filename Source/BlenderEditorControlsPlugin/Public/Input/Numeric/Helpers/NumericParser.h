#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	class FNumericParser
	{
	public:
		static bool Evaluate(const FString& RawString, bool bIsEquationMode, float& OutResult);

	private:
		static bool EvaluateSimple(const FString& RawString, float& OutResult);
		static bool EvaluateEquation(const FString& RawString, float& OutResult);
		static float CountOccurrences(const FString& RawString, TCHAR CharToCount);
	};
}
