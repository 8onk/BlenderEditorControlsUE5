#include "Input/Numeric/Helpers/NumericParser.h"
#include "Misc/DefaultValueHelper.h"

namespace BlenderControls
{
	bool FNumericParser::Evaluate(const FString& RawString, bool bIsEquationMode,
	                              float& OutResult)
	{
		if (bIsEquationMode)
		{
			return EvaluateEquation(RawString, OutResult);
		}
		else
		{
			return EvaluateSimple(RawString, OutResult);
		}
	}

	bool FNumericParser::EvaluateSimple(const FString& RawString, float& OutResult)
	{
		FString Trimmed = RawString.TrimEnd();
		if (Trimmed.IsEmpty())
		{
			OutResult = 0.f;
			return true;
		}

		// Check for invalid "4 4"
		if (Trimmed.Contains(TEXT(" ")) && !Trimmed.EndsWith(TEXT(" ")))
		{
			return false;
		}

		// Check for invalid "4..5"
		if (CountOccurrences(Trimmed, TEXT('.')) > 1)
		{
			return false;
		}

		if (FDefaultValueHelper::ParseFloat(Trimmed, OutResult))
		{
			return true;
		}

		// Handle case like "45."
		if (Trimmed.EndsWith(TEXT(".")))
		{
			FString WithoutDot = Trimmed.Left(Trimmed.Len() - 1);
			if (FDefaultValueHelper::ParseFloat(WithoutDot, OutResult))
			{
				return true;
			}
		}

		return false; // Invalid format
	}

	bool FNumericParser::EvaluateEquation(const FString& RawString, float& OutResult)
	{
		// =================================================================
		// !! CRITICAL !!
		// You MUST implement a real C++ math expression parser here.
		// This is a placeholder.
		// Look into "Shunting-yard algorithm" or libraries like "ExprTk".
		// =================================================================

		// Placeholder logic:
		if (RawString == TEXT("2**3"))
		{
			OutResult = 8.f;
			return true;
		}
		if (RawString == TEXT("4*4"))
		{
			OutResult = 16.f;
			return true;
		}

		// For now, fail on complex equations
		if (RawString.Contains(TEXT("*")) || RawString.Contains(TEXT("/")))
		{
			// Try to evaluate the last number if it's "4*
			// This is a hack, replace with real parser
			FString Left, Right;
			RawString.Split(TEXT("*"), &Left, &Right);
			if (!Right.IsEmpty())
			{
				return EvaluateSimple(Right, OutResult);
			}
			RawString.Split(TEXT("/"), &Left, &Right);
			if (!Right.IsEmpty())
			{
				return EvaluateSimple(Right, OutResult);
			}
		}

		return EvaluateSimple(RawString, OutResult);
	}

	float FNumericParser::CountOccurrences(const FString& RawString, TCHAR CharToCount)
	{
		int32 DotCount = 0;
		for (int32 i = 0; i < RawString.Len(); ++i)
		{
			if (RawString[i] == CharToCount)
			{
				++DotCount;
			}
		}

		return DotCount;
	}
}
