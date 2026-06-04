#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

namespace BlenderControls
{
	struct FGrabContext;
}

namespace BlenderControls::MathHelper
{
	void GetMousePosToViewportPos(const FVector2D& ScreenSpacePos, FVector2D& OutViewportPos);

	FVector RoundVectorToInt(const FVector& InVector);

	FVector IntersectHelper(const FGrabContext& GC, const FVector& RayOrigin, const FVector& RayDir);

	FVector SelectMostParallelPlaneNormal(const FVector& A, const FVector& B, const FVector& ViewForward);
	FVector SelectMostPerpendicularAxis(const FVector& A, const FVector& B,
	                                    const FVector& ViewForward);

	float GetSignedAngle2D(const FVector2D& From, const FVector2D& To);
} // namespace BlenderControls::MathHelper
