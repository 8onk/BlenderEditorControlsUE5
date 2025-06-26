#pragma once

#include "CoreMinimal.h"

namespace BlenderControls
{
	/** High-level mode enumeration */
	enum class ETransformMode : uint8
	{
		None,
		Translate,
		Rotate,
		Scale
	};

	enum class ETransformAxis : uint8
	{
		X = 1 << 0,
		Y = 1 << 1,
		Z = 1 << 2,
		All = X | Y | Z
	};

	// Makes bitwise ops work (like |, &, etc.)
	ENUM_CLASS_FLAGS(ETransformAxis)
} // namespace BlenderControls
