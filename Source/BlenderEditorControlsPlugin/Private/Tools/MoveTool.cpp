#include "Tools/MoveTool.h"
#include "LevelEditorViewport.h"
#include "Utils/BlenderMathHelpers.h"
//TODO TRANSLATION MODE NOT WORKING AT ALL IN ORTHOGRAPHIC VIEWS

namespace BlenderControls
{
	FMoveTool::FMoveTool(EAxisLock InAxis)
		: FBlenderToolBase(ETransformMode::Translate, InAxis, TEXT("Move"))
	{
	}

	void FMoveTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();

		ViewportClient->SetWidgetMode(UE::Widget::WM_Translate);
		ViewportClient->Invalidate();
	}

	void FMoveTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		constexpr float ParallelCos = 0.990f;
		float CosAngle = SMALL_NUMBER;
		//If HelperAxisDir is not 0 then we are in single axis lock. 
		if (!GrabContext.HelperAxisDir.IsNearlyZero())
		{
			CosAngle = FMath::Abs(FVector::DotProduct(ViewForward.GetSafeNormal(),
			                                          GrabContext.HelperAxisDir.GetSafeNormal()));
		}

		FVector FinalTotalDelta;
		if (CosAngle <= ParallelCos)
		{
			const FVector UnconstrainedMouseDelta3d = (ViewRight * MouseDelta.X * GrabContext.ScreenToWorldScale) +
				(-ViewUp * MouseDelta.Y * GrabContext.ScreenToWorldScale);

			//Intersect ghost pos to get the blender "feel", NOT the current mouse pos. 
			const FVector GhostPos = VirtualPivot->GetStartTransform().GetLocation() + UnconstrainedMouseDelta3d;
			const FVector RayOrigin = ViewLocation;
			const FVector RayDir = (GhostPos - RayOrigin).GetSafeNormal();
			const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
			FinalTotalDelta = FinalHit - VirtualPivot->GetStartTransform().GetLocation();
		}
		else
		{
			const FVector2D ScreenUpVector(0.0f, -1.0f);
			const float ScreenSpaceFactor = FVector2D::DotProduct(MouseDelta, ScreenUpVector);
			const float ScaledFactor = FMath::Sign(ScreenSpaceFactor) * FMath::Square(ScreenSpaceFactor) * 0.1f;

			FinalTotalDelta = GrabContext.HelperAxisDir * ScaledFactor;
		}

		FVector LiveDelta = FinalTotalDelta;

		if (bSnappingEnabled)
		{
			LiveDelta = GetSnapOffset(LiveDelta);
		}

		const FVector NewPos = VirtualPivot->GetStartTransform().GetLocation() + LiveDelta;
		VirtualPivot->SetPosition(NewPos);
	}

	void FMoveTool::ApplyNumeric(float Value)
	{
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FMoveTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		if (!VirtualPivot)
		{
			return;
		}
		const FTransform ObjectTransform = VirtualPivot->GetStartTransform();
		const FVector X = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

		switch (AxisLock)
		{
		case EAxisLock::All:
			GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
			GrabContext.HelperPlaneN = -GrabContext.ViewForward;
			break;

		case EAxisLock::X:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = X;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(Y, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Y:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Y;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Z:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Z;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Y, GrabContext.ViewForward);
			break;

		case EAxisLock::XY:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Z;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Y;
			break;

		case EAxisLock::XZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Y;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Z;
			break;

		case EAxisLock::YZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = X;
			GrabContext.PlaneAxisU = Y;
			GrabContext.PlaneAxisV = Z;
			break;
		}
	}

	FVector FMoveTool::GetSnapOffset(const FVector OffsetFromStart)
	{
		if (!GEditor)
		{
			return FVector::ZeroVector;
		}

		const float GridSize = GEditor->GetGridSize();
		FVector SnapOffset;
		if (LockedAxis == EAxisLock::X || LockedAxis == EAxisLock::Y || LockedAxis == EAxisLock::Z)
		{
			const FVector SnapAxis = GrabContext.HelperAxisDir;
			const float DistanceAlongAxis = FVector::DotProduct(OffsetFromStart, SnapAxis);
			const float SnappedDistance = FMath::GridSnap(DistanceAlongAxis, GridSize);
			SnapOffset = SnapAxis * SnappedDistance;
		}
		else if (LockedAxis == EAxisLock::XY || LockedAxis == EAxisLock::XZ || LockedAxis == EAxisLock::YZ)
		{
			const float DistanceAlongU = FVector::DotProduct(OffsetFromStart, GrabContext.PlaneAxisU);
			const float DistanceAlongV = FVector::DotProduct(OffsetFromStart, GrabContext.PlaneAxisV);

			const float SnappedDistanceU = FMath::GridSnap(DistanceAlongU, GridSize);
			const float SnappedDistanceV = FMath::GridSnap(DistanceAlongV, GridSize);

			SnapOffset = (GrabContext.PlaneAxisU * SnappedDistanceU) + (GrabContext.PlaneAxisV * SnappedDistanceV);
		}
		else
		{
			SnapOffset = OffsetFromStart / GridSize;
			SnapOffset = MathHelper::RoundVectorToInt(SnapOffset);
			SnapOffset *= GridSize;
		}

		return SnapOffset;
	}
}
