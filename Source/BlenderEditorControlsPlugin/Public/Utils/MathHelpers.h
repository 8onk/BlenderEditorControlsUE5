// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

namespace BlenderControls
{
	struct FGrabContext;
}

namespace BlenderControls::MathHelper
{
	/** 
	 * Converts absolute screen space coordinates into coordinates relative to the active viewport's internal geometry.
	 * 
	 * @param ScreenSpacePos The absolute mouse position on screen.
	 * @param OutViewportPos The resulting mouse position scaled and offset to the viewport.
	 */
	void GetMousePosToViewportPos(const FVector2D& ScreenSpacePos, FVector2D& OutViewportPos);

	/** @return A copy of the vector with all components rounded to the nearest integer. */
	FVector RoundVectorToInt(const FVector& InVector);

	/** 
	 * Projects a ray onto the active constraint plane to determine 3D intersection.
	 * 
	 * @param GC The grab context containing camera and constraint plane data.
	 * @param RayOrigin The origin of the trace (usually camera location).
	 * @param RayDir The direction of the trace.
	 * @return The 3D world intersection point on the plane.
	 */
	FVector IntersectHelper(const FGrabContext& GC, const FVector& RayOrigin, const FVector& RayDir);

	/** @return The plane normal (from X, Y, or Z) most parallel to the ViewForward direction. */
	FVector SelectMostParallelPlaneNormal(const FVector& A, const FVector& B, const FVector& ViewForward);
	
	/** @return The axis vector (from A or B) most perpendicular to the ViewForward direction. */
	FVector SelectMostPerpendicularAxis(const FVector& A, const FVector& B,
	                                    const FVector& ViewForward);

	/** 
	 * Calculates the signed angle between two 2D vectors.
	 * 
	 * @return The angle in radians, maintaining positive/negative sign based on direction.
	 */
	float GetSignedAngle2D(const FVector2D& From, const FVector2D& To);
} // namespace BlenderControls::MathHelper
