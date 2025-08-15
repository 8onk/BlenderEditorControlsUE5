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

	enum class EInputMode : uint8
	{
		Numeric,
		Mouse,
	};

	enum class EAxisLock : uint8
	{
		X = 1 << 0,
		Y = 1 << 1,
		Z = 1 << 2,
		XY = X | Y,
		XZ = X | Z,
		YZ = Y | Z,
		All = X | Y | Z
	};

	enum class EPivotMode : uint8
	{
		MedianPoint,
		BoundingBoxCenter,
		//ThreeDCursor maybe in the future?
		IndividualOrigins,
		ActiveElement
	};

	// Makes bitwise ops work (like |, &, etc.)
	ENUM_CLASS_FLAGS(EAxisLock)
} // namespace BlenderControls
