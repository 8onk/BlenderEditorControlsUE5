#include "Utils/MathHelpers.h"
#include "Math/Vector.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "DrawDebugHelpers.h"
#include "VectorUtil.h"
#include "Tools/GrabContext.h"

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
		if (!UE::Geometry::VectorUtil::IsFinite(RayOrigin) ||
			!UE::Geometry::VectorUtil::IsFinite(RayDir) || RayDir.IsNearlyZero() ||
			!UE::Geometry::VectorUtil::IsFinite(GC.HelperPlaneN) || GC.HelperPlaneN.IsNearlyZero())
		{
			UE_LOG(LogTemp, Warning, TEXT("IntersectHelper: invalid input; returning StartLocation."));
			return GC.StartLocation;
		}

		const FVector PlaneOrigin = GC.StartLocation;
		const FVector MouseRayStart = RayOrigin - (RayDir * WORLD_MAX);
		const FVector MouseRayEnd = RayOrigin + (RayDir * WORLD_MAX);

		FVector IntersectionPoint = FMath::LinePlaneIntersection(MouseRayStart, MouseRayEnd, PlaneOrigin,
		                                                         GC.HelperPlaneN);

		if (GC.HelperType == FGrabContext::EHelperType::AxisLine)
		{
			const FVector AxisLineStart = GC.StartLocation - (GC.HelperAxisDir * WORLD_MAX);
			const FVector AxisLineEnd = GC.StartLocation + (GC.HelperAxisDir * WORLD_MAX);
			IntersectionPoint = FMath::ClosestPointOnInfiniteLine(AxisLineStart, AxisLineEnd, IntersectionPoint);
		}

		return IntersectionPoint;
	}

	FVector IntersectHelper(const FVector& PlaneOrigin, const FVector& RayOrigin, const FVector& RayDir,
	                        const FVector& PlaneNormal)
	{
		const float Denom = FVector::DotProduct(PlaneNormal.GetSafeNormal(), RayDir.GetSafeNormal());
		if (FMath::Abs(Denom) < KINDA_SMALL_NUMBER) // Check if ray is parallel to the plane surface
		{
			return FVector::ZeroVector;
		}

		const float DistanceAlongRayToHit = FVector::DotProduct(PlaneOrigin - RayOrigin, PlaneNormal) / Denom;
		const FVector HitPoint = RayOrigin + RayDir * DistanceAlongRayToHit;
		return HitPoint;
	}

	bool IsRayParallelToNormal(const float Denom)
	{
		return FMath::Abs(Denom) < KINDA_SMALL_NUMBER;
	}

	FVector SelectMostParallelPlaneNormal(const FVector& A, const FVector& B,
	                                      const FVector& ViewForward)
	{
		const float DotA = FMath::Abs(FVector::DotProduct(ViewForward, A));
		const float DotB = FMath::Abs(FVector::DotProduct(ViewForward, B));
		return (DotA > DotB) ? A : B;
	}

	FVector SelectMostPerpendicularAxis(const FVector& A, const FVector& B,
	                                    const FVector& ViewForward)
	{
		const float DotA = FMath::Abs(FVector::DotProduct(ViewForward, A));
		const float DotB = FMath::Abs(FVector::DotProduct(ViewForward, B));
		return (DotA < DotB) ? A : B;
	}

	float GetSignedAngle2D(const FVector2D& From, const FVector2D& To)
	{
		const FVector2D FromNorm = From.GetSafeNormal();
		const FVector2D ToNorm = To.GetSafeNormal();

		const float Dot = FVector2D::DotProduct(FromNorm, ToNorm);
		//Clamp Dot even though both vectors are normalized so shouldn't be needed
		float Angle = FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f));

		const float CrossZ = FromNorm.X * ToNorm.Y - FromNorm.Y * ToNorm.X;
		//counter-clockwise rotations are negative and clockwise rotations are positive.
		if (CrossZ > 0)
		{
			Angle = -Angle;
		}
		return Angle;
	}

	float GetSignedAngle3D(const FVector& StartVec, const FVector& EndVec)
	{
		const FVector StartVecNorm = StartVec.GetSafeNormal();
		const FVector EndVecNorm = EndVec.GetSafeNormal();
		const float Dot = FVector::DotProduct(StartVecNorm, EndVecNorm);
		return FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f));
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
