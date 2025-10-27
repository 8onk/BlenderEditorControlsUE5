#include "Input/Numeric/Helpers/UnitFormatter.h"
#include "Input/Numeric/NumericInputStructs.h"

namespace BlenderControls
{
	float FUnitFormatter::ConvertOnToolSwitch(float Value, EBlenderNumericContext From,
	                                          EBlenderNumericContext To)
	{
		if (From == To) return Value;

		// Move/Scale (Meters) -> Rotate (Degrees)
		// We interpret Meters as Radians
		if ((From == EBlenderNumericContext::Distance || From == EBlenderNumericContext::Scale) && To ==
			EBlenderNumericContext::Angle_Degrees)
		{
			return FMath::RadiansToDegrees(Value);
		}

		// Rotate (Degrees) -> Move/Scale (Meters)
		// We interpret Degrees as Degrees
		if (From == EBlenderNumericContext::Angle_Degrees && (To == EBlenderNumericContext::Distance || To ==
			EBlenderNumericContext::Scale))
		{
			return FMath::DegreesToRadians(Value);
		}

		return Value;
	}

	FString FUnitFormatter::FormatValue(float Value, EBlenderNumericContext Context)
	{
		//Will gives some precision error, but blender displays it like 4 m 43 cm etc.
		auto FormatSmart = [](float V) -> FString
		{
			// Check if it's effectively an integer (e.g. 5.0)
			if (FMath::IsNearlyEqual(V, FMath::RoundToFloat(V)))
			{
				return FString::Printf(TEXT("%.0f"), V);
			}
			else
			{
				return FString::Printf(TEXT("%.3f"), V);
			}
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
