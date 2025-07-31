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

	FVector RoundVectorToInt(const FVector& InVector);
	FVector GetAxisVector(EAxisLock InAxis);

	FVector IntersectHelper(const FGrabContext& GC, const FVector& RayOrigin, const FVector& RayDir);
	FVector IntersectHelper(const FVector& PlaneOrigin, const FVector& RayOrigin, const FVector& RayDir,
	                        const FVector& PlaneNormal);

	FVector SelectMostParallelPlaneNormal(const FVector& A, const FVector& B, const FVector& ViewForward);
	FVector SelectMostPerpendicularAxis(const FVector& A, const FVector& B,
	                                    const FVector& ViewForward);
	bool IsRayParallelToNormal(const float Denom);

	float GetSignedAngle2D(const FVector2D& From, const FVector2D& To);
	float GetSignedAngle3D(const FVector& StartVec, const FVector& EndVec);

	FVector ProjectVectorOntoPlane(const FVector& Vector, const FVector& PlaneNormal);
	FVector ProjectVectorOntoAxis(const FVector& Vector, const FVector& AxisDirection);
} // namespace BlenderControls::Math
