#include "Input/Numeric/Helpers/UnitFormatter.h"
#include "Input/Numeric/NumericInputStructs.h"

namespace BlenderControls
{
	float FUnitFormatter::ConvertOnToolSwitch(float Value, EBlenderNumericContext From,
	                                          EBlenderNumericContext To)
	{
		if (From == To)
		{
			return Value;
		}
		if ((From == EBlenderNumericContext::Distance || From == EBlenderNumericContext::Scale) && To ==
			EBlenderNumericContext::Angle_Degrees)
		{
			return FMath::RadiansToDegrees(Value);
		}

		if (From == EBlenderNumericContext::Angle_Degrees && (To == EBlenderNumericContext::Distance || To ==
			EBlenderNumericContext::Scale))
		{
			return FMath::DegreesToRadians(Value);
		}

		return Value;
	}

	FString FUnitFormatter::FormatValue(float Value, EBlenderNumericContext Context)
	{
		auto FormatSmart = [](const float V) -> FString
		{
			FNumberFormattingOptions Opt;
			Opt.MinimumFractionalDigits = 0;
			Opt.MaximumFractionalDigits = 3;
			Opt.UseGrouping = false;
			return FText::AsNumber(V, &Opt).ToString();
		};

		switch (Context)
		{
		case EBlenderNumericContext::Angle_Degrees:
			return FormatSmart(Value) + TEXT("°");

		case EBlenderNumericContext::Distance:
			return FormatSmart(Value) + TEXT(" cm");

		default:
			return FormatSmart(Value);
		}
	}
}
