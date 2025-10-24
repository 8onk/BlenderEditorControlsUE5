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
		if (Context == EBlenderNumericContext::Angle_Degrees)
		{
			// FString::SanitizeFloat formats to 1 decimal place
			return FString::Printf(TEXT("%s°"), *FString::SanitizeFloat(Value));
		}

		if (Context == EBlenderNumericContext::Distance)
		{
			// 4544 m -> 4 km 544 m
			// 0.022 m -> 2 cm 2 mm
			// This is a complex function you'll need to write.
			// For now, a simple "m"
			return FString::Printf(TEXT("%s m"), *FString::SanitizeFloat(Value, 3));
		}

		// Default for Scale
		return FString::Printf(TEXT("%s"), *FString::SanitizeFloat(Value, 3));
	}
}
