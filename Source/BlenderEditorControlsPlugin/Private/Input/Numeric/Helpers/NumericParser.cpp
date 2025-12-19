#include "Input/Numeric/Helpers/NumericParser.h"

#include "Input/Numeric/NumericInputProcessor.h"
#include "Misc/DefaultValueHelper.h"

namespace BlenderControls
{
	bool FNumericParser::Evaluate(const FString& RawString,
	                              float& OutResult)
	{
		return EvaluateSimple(RawString, OutResult);
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

		//Normalize locale-specific commas (from FText::AsNumber)
		Trimmed.ReplaceInline(TEXT(","), TEXT("."));

		// Check for invalid "4 4"
		if (Trimmed.Contains(TEXT(" ")) && !(Trimmed.Contains(TEXT(" cm")) || Trimmed.Contains(TEXT("°"))))
		{
			return false;
		}

		// Check for invalid "4..5"
		if (CountOccurrences(Trimmed, TEXT('.')) > 1)
		{
			return false;
		}

		if (EvaluateAdditiveWithUnit(Trimmed, OutResult))
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

		if (FDefaultValueHelper::ParseFloat(Trimmed, OutResult))
		{
			return true;
		}

		return false; // Invalid format
	}

	bool FNumericParser::HasInvalidUnitsForContext(const FString& RawInput, EBlenderNumericContext Context)
	{
		const bool bContainsDeg = RawInput.Contains(TEXT("°"));
		const bool bContainsCm = RawInput.Contains(TEXT("cm"), ESearchCase::IgnoreCase);

		// Invalid: Typing "cm" when in Angle mode
		if (Context == EBlenderNumericContext::Angle_Degrees && bContainsCm)
		{
			return true;
		}

		// Invalid: Typing "°" when in Distance or Scale mode
		if ((Context == EBlenderNumericContext::Distance || Context == EBlenderNumericContext::Scale) && bContainsDeg)
		{
			return true;
		}

		return false; // All good
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

	bool FNumericParser::EvaluateAdditiveWithUnit(const FString& Trimmed, float& OutResult)
	{
		int32 UnitIndex = -1;
		int32 UnitLength = 0;

		UnitIndex = Trimmed.Find(TEXT("cm"), ESearchCase::IgnoreCase);
		if (UnitIndex != -1)
		{
			UnitLength = 2; // length of "cm"
		}
		else
		{
			UnitIndex = Trimmed.Find(TEXT("°"));
			if (UnitIndex != -1)
			{
				UnitLength = 1; // length of "°"
			}
		}

		if (UnitIndex == -1)
		{
			if (FDefaultValueHelper::ParseFloat(Trimmed, OutResult))
			{
				return true;
			}
		}
		else
		{
			// Get everything BEFORE the unit
			FString LeftPart = Trimmed.Left(UnitIndex);
			// Get everything AFTER the unit
			FString RightPart = Trimmed.RightChop(UnitIndex + UnitLength);

			float LeftValue = 0.f;
			float RightValue = 0.f;

			// ParseFloat on LeftPart.TrimEnd() handles both "4" (from "4cm")
			// and "4 " (from "4 cm"). It also handles "cm4" (LeftPart=""), which fails.
			if (FDefaultValueHelper::ParseFloat(LeftPart.TrimEnd(), LeftValue))
			{
				// LeftPart is valid. Now parse RightPart.
				if (RightPart.TrimEnd().IsEmpty())
				{
					RightValue = 0.f; // e.g., "4cm" or "4 cm"
				}
				else if (!FDefaultValueHelper::ParseFloat(RightPart.TrimEnd(), RightValue))
				{
					return false; // e.g., "4cmHello"
				}

				// Both parts are valid, return the sum.
				OutResult = LeftValue + RightValue;
				return true;
			}
			else
			{
				// LeftPart was not a valid float (e.g., "cm4")
				return false;
			}
		}

		return false;
	}
}
