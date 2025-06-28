#include "Tools/MoveTool.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
	FMoveTool::FMoveTool(ETransformAxis InAxis)
		: FBlenderToolBase(ETransformMode::Translate, InAxis, TEXT("Move"))
	{
	}

	void FMoveTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();
	}

	void FMoveTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		FVector MousePosOffset3D;
		if (bPrecisionModeActive)
		{
			CurrentPrecisionFactor = PrecisionFactor;
			const FVector RawDeltaSinceShift = CurrentPlaneIntersectionPoint - ShiftStartIntersectionPoint;
			MousePosOffset3D = PrecisionAnchor + RawDeltaSinceShift * CurrentPrecisionFactor;
		}
		else
		{
			CurrentPrecisionFactor = 1.0f;
			MousePosOffset3D = CurrentPlaneIntersectionPoint - GrabStartPlaneIntersectionPoint;
		}

		// if (Axis == ETransformAxis::X || Axis == ETransformAxis::Y || Axis == ETransformAxis::Z)
		// {
		// 	FVector AxisDirection;
		// 	switch (Axis)
		// 	{
		// 	case ETransformAxis::X: AxisDirection = FVector::ForwardVector;
		// 		break;
		// 	case ETransformAxis::Y: AxisDirection = FVector::RightVector;
		// 		break;
		// 	case ETransformAxis::Z: AxisDirection = FVector::UpVector;
		// 		break;
		// 	default: break;
		// 	}
		//
		// 	// Reproject mouse ray onto axis line
		// 	const FVector ClosestPointNow = FMath::ClosestPointOnInfiniteLine (
		// 		GrabStartPlaneIntersectionPoint, // origin of line
		// 		AxisDirection,
		// 		WorldOrigin
		// 	);
		//
		// 	const FVector Delta = ClosestPointNow - Pivot->GetStartTransform().GetLocation(); 
		// 	MousePosOffset3D = Delta * CurrentPrecisionFactor;
		// }

		NewPivotPosition = Pivot->GetStartTransform().GetLocation() + MousePosOffset3D;

		FVector TargetPivotPosition;
		if (bSnappingEnabled)
		{
			const FVector SnappedNewPivotPosition = GetSnapOffset(NewPivotPosition);
			TargetPivotPosition = SnappedNewPivotPosition;
		}
		else
		{
			TargetPivotPosition = NewPivotPosition;
		}

		if (Pivot.IsValid())
		{
			Pivot->SetPosition(TargetPivotPosition);
		}
	}

	void FMoveTool::ApplyNumeric(float Value)
	{
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}
}
