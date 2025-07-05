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
		FViewport* Viewport = GEditor->GetActiveViewport();
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
		const float LINE_LENGTH = 1e12f;
		const FVector MouseRayStart = RayOrigin - (RayDir * LINE_LENGTH);
		const FVector MouseRayEnd = RayOrigin + (RayDir * LINE_LENGTH);

		if (GC.HelperType == FGrabContext::EHelperType::ViewPlane || GC.HelperType ==
			FGrabContext::EHelperType::AxisPlane)
		{
			FVector Result = FMath::LinePlaneIntersection(MouseRayStart, MouseRayEnd, PlaneOrigin, GC.HelperPlaneN);
			return Result;
		}

		FVector AxisStart = GC.PivotStartPosition - GC.HelperAxisDir * LINE_LENGTH;
		FVector AxisEnd = GC.PivotStartPosition + GC.HelperAxisDir * LINE_LENGTH;

		FVector ClosestOnRay, ClosestOnAxis;
		FMath::SegmentDistToSegment(MouseRayStart, MouseRayEnd, AxisStart, AxisEnd, ClosestOnRay, ClosestOnAxis);
		return ClosestOnAxis;
	}

	FVector ProjectVectorOntoPlane(const FVector& Vector, const FVector& PlaneNormal)
	{
		FVector Normal = PlaneNormal.GetSafeNormal();
		FVector Projected = Vector - FVector::DotProduct(Vector, Normal) * Normal;

		return Projected;
	}

	FVector ProjectVectorOntoAxis(const FVector& Vector, const FVector& AxisDirection)
	{
		FVector Axis = AxisDirection.GetSafeNormal();
		return FVector::DotProduct(Vector, Axis) * Axis;
	}
}
