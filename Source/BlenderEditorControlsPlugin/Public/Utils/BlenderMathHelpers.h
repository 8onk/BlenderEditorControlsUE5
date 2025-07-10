#pragma once

#include "CoreMinimal.h"
#include "BlenderEditorControlsEnums.h"
#include "Misc/Optional.h"
#include "Math/Plane.h"

namespace BlenderControls
{
	struct FGrabContext;
}

namespace BlenderControls::MathHelper
{
	void GetMousePosToViewportPos(const FVector2D& ScreenSpacePos, FVector2D& OutViewportPos);

	/** Rounds all components of a vector to their nearest integer values */
	FVector RoundVectorToInt(const FVector& InVector);
	FVector GetAxisVector(EAxisLock InAxis);
	FVector IntersectHelper(const FGrabContext& GC, const FVector& RayOrigin, const FVector& RayDir);
	FVector SelectMostParallelPlaneNormal(const FVector &A, const FVector &B, const FVector &ViewForward);
	float GetSignedAngle2D(const FVector2D& From, const FVector2D& To);
	float GetSignedAngle3D(const FVector& StartVec, const FVector& EndVec);
	FVector ProjectVectorOntoPlane(const FVector& Vector, const FVector& PlaneNormal);
	FVector ProjectVectorOntoAxis(const FVector& Vector, const FVector& AxisDirection);
} // namespace BlenderControls::Math
