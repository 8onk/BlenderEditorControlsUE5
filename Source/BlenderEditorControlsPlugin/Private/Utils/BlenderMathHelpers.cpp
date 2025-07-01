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

	FMatrix2x2::FMatrix2x2()
	{
		M[0][0] = 0.0f;
		M[0][1] = 0.0f;
		M[1][0] = 0.0f;
		M[1][1] = 0.0f;
	}

	FMatrix2x2::FMatrix2x2(float m00, float m01, float m10, float m11)
	{
		M[0][0] = m00;
		M[0][1] = m01;
		M[1][0] = m10;
		M[1][1] = m11;
	}

	FMatrix2x2 FMatrix2x2::Inverse() const
	{
		float det = M[0][0] * M[1][1] - M[0][1] * M[1][0];
		if (FMath::IsNearlyZero(det))
		{
			// Return zero matrix if not invertible
			return FMatrix2x2();
		}
		float invDet = 1.0f / det;
		return FMatrix2x2(
			M[1][1] * invDet, -M[0][1] * invDet,
			-M[1][0] * invDet, M[0][0] * invDet);
	}

	FVector2D FMatrix2x2::GetColumn0() const
	{
		return FVector2D(M[0][0], M[1][0]);
	}

	FVector2D FMatrix2x2::GetColumn1() const
	{
		return FVector2D(M[0][1], M[1][1]);
	}
}
