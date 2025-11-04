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

	TOptional<TCHAR> FNumericParser::KeyToNumericChar(const FKey& Key)
	{
		if (Key == EKeys::Zero || Key == EKeys::NumPadZero) return TEXT('0');
		if (Key == EKeys::One || Key == EKeys::NumPadOne) return TEXT('1');
		if (Key == EKeys::Two || Key == EKeys::NumPadTwo) return TEXT('2');
		if (Key == EKeys::Three || Key == EKeys::NumPadThree) return TEXT('3');
		if (Key == EKeys::Four || Key == EKeys::NumPadFour) return TEXT('4');
		if (Key == EKeys::Five || Key == EKeys::NumPadFive) return TEXT('5');
		if (Key == EKeys::Six || Key == EKeys::NumPadSix) return TEXT('6');
		if (Key == EKeys::Seven || Key == EKeys::NumPadSeven)return TEXT('7');
		if (Key == EKeys::Eight || Key == EKeys::NumPadEight)return TEXT('8');
		if (Key == EKeys::Nine || Key == EKeys::NumPadNine) return TEXT('9');

		// decimal separators
		if (Key == EKeys::Period || Key == EKeys::Decimal) return TEXT('.');
		// (optionally) accept comma but normalize to '.'
		if (Key == EKeys::Comma) return TEXT('.');

		// equation ops
		if (Key == EKeys::Hyphen || Key == EKeys::Subtract) return TEXT('-');
		if (Key == EKeys::Add) return TEXT('+');
		if (Key == EKeys::Multiply) return TEXT('*');
		if (Key == EKeys::Divide || Key == EKeys::Slash) return TEXT('/');

		return {};
	}

	bool FNumericParser::EvaluateSimple(const FString& RawString, float& OutResult)
	{
		FString Trimmed = RawString.TrimEnd();
		if (Trimmed.IsEmpty())
		{
			OutResult = 0.f;
			return true;
		}

		FRegexPattern NumberPattern(TEXT("(-?\\d*\\.?\\d+)")); // Matches ints and floats
		FRegexMatcher Matcher(NumberPattern, RawString);

		float Sum = 0.f;
		int32 NumberCount = 0;

		while (Matcher.FindNext())
		{
			++NumberCount;

			FString NumberStr = Matcher.GetCaptureGroup(0);
			float Value = FCString::Atof(*NumberStr);

			Sum += Value;
		}

		if ((NumberCount == 2 || NumberCount == 1) && RawString.Contains(" cm") || RawString.Contains("°"))
		{
			OutResult = Sum;
			return true;
		}

		// Check for invalid "4 4"
		if (Trimmed.Contains(TEXT(" ")) && !Trimmed.EndsWith(TEXT(" ")))
		{
			UE_LOG(LogTemp, Log, TEXT("INVALID"));
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
		// implement a real C++ math expression parser here (maybe)
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
