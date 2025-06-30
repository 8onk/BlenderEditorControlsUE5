#include "Utils/BlenderMathHelpers.h"
#include "Math/Vector.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "Tools/BlenderToolBase.h"
#include "DrawDebugHelpers.h"

namespace BlenderControls::Math
{
	void GetMousePosToViewportPos(const FVector2D &ScreenSpacePos, FVector2D &OutViewportPos)
	{
		FViewport *Viewport = GEditor->GetActiveViewport();
		if (!Viewport)
			return;

		const FIntPoint DesktopInt(static_cast<int32>(ScreenSpacePos.X),
								   static_cast<int32>(ScreenSpacePos.Y));

		const FVector2D Normalized = Viewport->VirtualDesktopPixelToViewport(DesktopInt);
		OutViewportPos = Normalized * FVector2D(Viewport->GetSizeXY());
	}

	FVector RoundVectorToInt(const FVector &InVector)
	{
		return FVector(
			FMath::RoundToInt(InVector.X),
			FMath::RoundToInt(InVector.Y),
			FMath::RoundToInt(InVector.Z));
	}

	FVector IntersectHelper(const FGrabContext &GC, const FVector &RayOrigin, const FVector &RayDir)
	{
		const FVector PlaneOrigin = GC.PivotStartPos;
		const float LineLength = 1000000.0f;
		const FVector MouseRayStart = RayOrigin - (RayDir * LineLength);
		const FVector MouseRayEnd = RayOrigin + (RayDir * LineLength);

		if (GC.HelperType == FGrabContext::EHelperType::ViewPlane || GC.HelperType == FGrabContext::EHelperType::AxisPlane)
		{
			FVector result = FMath::LinePlaneIntersection(MouseRayStart, MouseRayEnd, PlaneOrigin, GC.HelperPlaneN);
			return result;
		}

		FVector AxisStart = GC.PivotStartPos - GC.HelperAxisDir * LineLength;
		FVector AxisEnd = GC.PivotStartPos + GC.HelperAxisDir * LineLength;

		FVector ClosestOnRay, ClosestOnAxis;
		FMath::SegmentDistToSegment(MouseRayStart, MouseRayEnd, AxisStart, AxisEnd, ClosestOnRay, ClosestOnAxis);
		return ClosestOnAxis;
	}

	FVector ProjectVectorOntoPlane(const FVector &Vector, const FVector &PlaneNormal)
	{
		FVector Normal = PlaneNormal.GetSafeNormal();
		FVector Projected = Vector - FVector::DotProduct(Vector, Normal) * Normal;

		return Projected;
	}

	FVector ProjectVectorOntoAxis(const FVector &Vector, const FVector &AxisDirection)
	{
		FVector Axis = AxisDirection.GetSafeNormal();
		return FVector::DotProduct(Vector, Axis) * Axis;
	}
}
