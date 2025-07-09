#include "Tools/RotateTool.h"
#include "Utils/BlenderMathHelpers.h"
//TODO fix the rotation behaving oddly when mouse wraps around
//TODO draw the rotation gizmo handle
//TODO cancelling resets the rotation. Make the shared pivot logic be more robust. 

namespace BlenderControls
{
	FRotateTool::FRotateTool(EAxisLock InAxis)
		: FBlenderToolBase(ETransformMode::Rotate, InAxis, TEXT("Rotate"))
	{
	}

	void FRotateTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);

		StartPivotTransform = Pivot->GetStartTransform();
		PivotStartPos = Pivot->GetStartTransform().GetLocation();

		SceneView->WorldToPixel(PivotStartPos, PivotViewportLocation);

		StartDragVector = CurrentMousePosition - PivotViewportLocation;
	}

	void FRotateTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		const FVector2D CurrentDragVector = CurrentMousePosition - PivotViewportLocation;

		const FVector RotationAxis = GrabContext.HelperAxisDir;
		const float ViewAlignmentWithAxis = FVector::DotProduct(GrabContext.ViewForward, RotationAxis);
		float TotalAngleRadians = MathHelper::GetSignedAngle2D(StartDragVector, CurrentDragVector);
		const bool bIsRotationAxisFacingCamera = ViewAlignmentWithAxis > 0;
		if (!bIsRotationAxisFacingCamera)
		{
			TotalAngleRadians = -TotalAngleRadians;
		}

		if (!GEditor)
		{
			return;
		}

		//REFACTOR INTO A SNAP OFFSET FUNCTION
		FRotator RotationGridSize = GEditor->GetRotGridSize();
		float SnapAngleDeg = RotationGridSize.Yaw;
		float TotalAngleDeg = FMath::RadiansToDegrees(TotalAngleRadians);
		float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
		float SnappedAngleRad = FMath::DegreesToRadians(SnappedAngleDeg);
		float DegreesToRotate = bSnappingEnabled ? SnappedAngleRad : TotalAngleRadians;
		const FQuat TargetRotation = FQuat(RotationAxis, DegreesToRotate);

		FTransform NewTransform = StartPivotTransform;
		NewTransform.ConcatenateRotation(TargetRotation);
		Pivot->GetTransformProxy()->SetTransform(NewTransform);
	}

	void FRotateTool::ApplyNumeric(float Value)
	{
	}

	void FRotateTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FRotateTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		const FTransform ObjectTransform = Pivot->GetStartTransform();
		const FVector X = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

		switch (AxisLock)
		{
		case EAxisLock::X: GrabContext.HelperAxisDir = X;
			break;
		case EAxisLock::Y: GrabContext.HelperAxisDir = Y;
			break;
		case EAxisLock::Z: GrabContext.HelperAxisDir = Z;
			break;

		case EAxisLock::XY: GrabContext.HelperAxisDir = Z;
			break;
		case EAxisLock::YZ: GrabContext.HelperAxisDir = X;
			break;
		case EAxisLock::XZ: GrabContext.HelperAxisDir = Y;
			break;

		case EAxisLock::All: GrabContext.HelperAxisDir = GrabContext.ViewForward;
			break;
		}
	}

	FVector FRotateTool::GetSnapOffset(const FVector OffsetFromStart)
	{
		return FVector::ZeroVector;
		// FVector SnapOffset = OffsetFromStart / RotationGridSize;
		// SnapOffset = MathHelper::RoundVectorToInt(SnapOffset);
		// SnapOffset *= RotationGridSize;
		//
		// return SnapOffset;
	}
} // namespace BlenderControls
