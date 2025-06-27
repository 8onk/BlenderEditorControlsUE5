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

		const FVector MousePosDelta3D = bPrecisionModeActive
			                           ? (CurrentPlaneIntersectionPoint - PreviousPlaneIntersectionPoint) * PrecisionFactor
			                           : CurrentPlaneIntersectionPoint - PreviousPlaneIntersectionPoint;

		NewPivotPosition += MousePosDelta3D;
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

		const uint8 AxisBits = static_cast<uint8>(Axis);
		// is number of set bits 1 => single axis lock
		if (FMath::CountBits(AxisBits) == 1)
		{
			FVector AxisVector = GetAxisVector(Axis);
			TargetPivotPosition = FVector::DotProduct(TargetPivotPosition, AxisVector) * AxisVector;
		}

		if (Pivot.IsValid())
		{
			Pivot->SetPosition(TargetPivotPosition);
		}
		PreviousPlaneIntersectionPoint = CurrentPlaneIntersectionPoint;
	}

	void FMoveTool::ApplyNumeric(float Value)
	{
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}
}
