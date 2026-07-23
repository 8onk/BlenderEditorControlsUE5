// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	/** 
	 * Caches the initial state of the camera and mouse when a transform operation begins. 
	 * Used to convert 2D mouse deltas into 3D world space transformations.
	 */
	struct FGrabContext
	{
		/** Screen position of the mouse at the start of the grab. */
		FVector2D StartMousePos;
		
		/** World space location of the pivot at the start of the grab. */
		FVector   StartLocation;
		
		/** The normalized axis vector if locked to a single axis. */
		FVector   SingleLockAxis;
		
		/** The normal vector of the plane if locked to a 2D plane (or camera view plane). */
		FVector   PlaneNormal;
		
		/** The primary vector spanning the constraint plane. */
		FVector   PlaneAxisU;
		
		/** The secondary vector spanning the constraint plane. */
		FVector   PlaneAxisV;
		
		/** The forward direction of the active viewport camera. */
		FVector   ViewForward;
		
		/** Ratio used to convert screen-space pixels to world-space units based on depth. */
		float     ScreenToWorldScale;

		/** Determines how the mouse delta is projected into 3D space. */
		enum class EHelperType
		{
			/** Moving relative to the camera's view plane. */
			ViewPlane,
			/** Constrained along a single 1D axis line. */
			AxisLine,
			/** Constrained across a 2D plane. */
			AxisPlane
		} ConstraintMode;
	};
}
