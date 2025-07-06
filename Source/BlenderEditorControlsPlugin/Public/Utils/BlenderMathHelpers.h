#pragma once

#include "CoreMinimal.h"
#include "BlenderEditorControlsEnums.h"
#include "Misc/Optional.h"
#include "Math/Plane.h"

namespace BlenderControls
{
	struct FGrabContext;
}

namespace BlenderControls::Math
{
	void GetMousePosToViewportPos(const FVector2D& ScreenSpacePos, FVector2D& OutViewportPos);

	/** Rounds all components of a vector to their nearest integer values */
	FVector RoundVectorToInt(const FVector& InVector);
	FVector GetAxisVector(EAxisLock InAxis);
	FVector IntersectHelper(const FGrabContext& GC, const FVector& RayOrigin, const FVector& RayDir);
	FVector ProjectVectorOntoPlane(const FVector& Vector, const FVector& PlaneNormal);
	FVector ProjectVectorOntoAxis(const FVector& Vector, const FVector& AxisDirection);
} // namespace BlenderControls::Math
