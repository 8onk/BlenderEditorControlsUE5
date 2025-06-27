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

		const FVector FrameDelta = bPrecisionModeActive
			                           ? (CurrentIntersectionPoint - PreviousIntersectionPoint) * PrecisionFactor
			                           : CurrentIntersectionPoint - PreviousIntersectionPoint;

		FloatingOrigin += FrameDelta;
		FVector TargetPosition;
		if (bSnappingEnabled)
		{
			const FVector TotalUnsnappedOffset = FloatingOrigin - Group->GetStartTransform().GetLocation();
			const FVector SnappedTotalOffset = GetSnapOffset(TotalUnsnappedOffset);
			TargetPosition = Group->GetStartTransform().GetLocation() + SnappedTotalOffset;
		}
		else
		{
			TargetPosition = FloatingOrigin;
		}

		const uint8 AxisBits = static_cast<uint8>(Axis);
		// is number of set bits 1 => single axis lock
		if (FMath::CountBits(AxisBits) == 1)
		{
			FVector AxisVector = GetAxisVector(Axis);
			TargetPosition = FVector::DotProduct(TargetPosition, AxisVector) * AxisVector;
		}

		if (Group.IsValid())
		{
			Group->SetPosition(TargetPosition);
		}
		PreviousIntersectionPoint = CurrentIntersectionPoint;
	}

	void FMoveTool::ApplyNumeric(float Value)
	{
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}
}
