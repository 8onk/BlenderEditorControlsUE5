// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace BlenderControls
{
	/** Defines the type of transformation being applied during an active session. */
	enum class ETransformMode : uint8
	{
		None,
		Translate,
		Rotate,
		Scale
	};

	/** Represents the state of a single axis slot (X, Y, or Z) during numeric input. */
	enum class ESlotState
	{
		Pristine,
		FirstEdit,
		Committed,
		Additive,
		InvalidInput
	};

	/** Bitmask representing which axes are currently constrained during a transformation. */
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

	/** Determines how the center of transformation (the pivot point) is calculated for multiple selected objects.
	 *  NOTE: Currently only supports MedianPoint. 
	 */
	enum class EPivotMode : uint8
	{
		/** The mathematical center of all selected object locations. */
		MedianPoint,
		/** The center of the combined bounding box of all selected objects. */
		BoundingBoxCenter,
		//ThreeDCursor maybe in the future?

		/** Each object transforms around its own local origin. */
		IndividualOrigins,
		/** Transforms occur around the last selected (active) element. */
		ActiveElement
	};

	// Makes bitwise ops work (like |, &, etc.)
	ENUM_CLASS_FLAGS(EAxisLock)
} // namespace BlenderControls
