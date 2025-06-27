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

		FVector MousePosDelta3D = bPrecisionModeActive
			                          ? (CurrentPlaneIntersectionPoint - PreviousPlaneIntersectionPoint) *
			                          PrecisionFactor
			                          : CurrentPlaneIntersectionPoint - PreviousPlaneIntersectionPoint;

		if (NormalToRemove != FVector::ZeroVector)
		{
			MousePosDelta3D -= NormalToRemove * FVector::DotProduct(MousePosDelta3D, NormalToRemove);
		}
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
