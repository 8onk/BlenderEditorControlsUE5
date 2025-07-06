#include "Utils/BlenderMathHelpers.h"
#include "Math/Vector.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "Tools/BlenderToolBase.h"
#include "DrawDebugHelpers.h"

namespace BlenderControls::Math
{
	void GetMousePosToViewportPos(const FVector2D& ScreenSpacePos, FVector2D& OutViewportPos)
	{
		const FViewport* Viewport = GEditor->GetActiveViewport();
		if (!Viewport)
			return;

		const FIntPoint DesktopInt(static_cast<int32>(ScreenSpacePos.X),
		                           static_cast<int32>(ScreenSpacePos.Y));

		const FVector2D Normalized = Viewport->VirtualDesktopPixelToViewport(DesktopInt);
		OutViewportPos = Normalized * FVector2D(Viewport->GetSizeXY());
	}

	FVector RoundVectorToInt(const FVector& InVector)
	{
		return FVector(
			FMath::RoundToInt(InVector.X),
			FMath::RoundToInt(InVector.Y),
			FMath::RoundToInt(InVector.Z));
	}

	FVector IntersectHelper(const FGrabContext& GC, const FVector& RayOrigin, const FVector& RayDir)
	{
		const FVector PlaneOrigin = GC.PivotStartPosition;
		const FVector MouseRayStart = RayOrigin - (RayDir * WORLD_MAX);
		const FVector MouseRayEnd = RayOrigin + (RayDir * WORLD_MAX);

		FVector IntersectionPoint = FMath::LinePlaneIntersection(MouseRayStart, MouseRayEnd, PlaneOrigin,
		                                                         GC.HelperPlaneN);
		if (GC.HelperType == FGrabContext::EHelperType::AxisLine)
		{
			const FVector AxisLineStart = GC.PivotStartPosition - (GC.HelperAxisDir * WORLD_MAX);
			const FVector AxisLineEnd = GC.PivotStartPosition + (GC.HelperAxisDir * WORLD_MAX);
			IntersectionPoint = FMath::ClosestPointOnInfiniteLine(AxisLineStart, AxisLineEnd, IntersectionPoint);
		}

		return IntersectionPoint;
	}

	FVector ProjectVectorOntoPlane(const FVector& Vector, const FVector& PlaneNormal)
	{
		const FVector Normal = PlaneNormal.GetSafeNormal();
		const FVector Projected = Vector - FVector::DotProduct(Vector, Normal) * Normal;

		return Projected;
	}

	FVector ProjectVectorOntoAxis(const FVector& Vector, const FVector& AxisDirection)
	{
		const FVector Axis = AxisDirection.GetSafeNormal();
		return FVector::DotProduct(Vector, Axis) * Axis;
	}
}
