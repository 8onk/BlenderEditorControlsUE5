#include "Utils/BlenderMathHelpers.h"
#include "Math/Vector.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "Tools/BlenderToolBase.h"
#include "DrawDebugHelpers.h"

namespace BlenderControls::MathHelper
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

	FVector SelectMostParallelPlaneNormal(const FVector& Axis, const FVector& A, const FVector& B,
	                                      const FVector& ViewForward)
	{
		const float DotA = FMath::Abs(FVector::DotProduct(ViewForward, A));
		const float DotB = FMath::Abs(FVector::DotProduct(ViewForward, B));
		return (DotA > DotB) ? A : B;
	}

	float GetSignedAngleOnAxis(const FVector& From, const FVector& To, const FVector& Axis)
	{
		const FVector Cross = FVector::CrossProduct(From.GetSafeNormal(), To.GetSafeNormal());
		const float Dot = FVector::DotProduct(From.GetSafeNormal(), To.GetSafeNormal());

		const float Angle = FMath::Atan2(Cross.Size(), Dot); // unsigned angle

		// Determine sign
		const float Sign = FVector::DotProduct(Cross, Axis) < 0 ? -1.0f : 1.0f;
		return Angle * Sign;
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
