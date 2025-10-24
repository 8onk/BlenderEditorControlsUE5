#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	enum class EBlenderNumericContext : uint8;

	class FUnitFormatter
	{
	public:
		static float ConvertOnToolSwitch(float Value, EBlenderNumericContext From, EBlenderNumericContext To);
		static FString FormatValue(float Value, EBlenderNumericContext Context);
	};
}
