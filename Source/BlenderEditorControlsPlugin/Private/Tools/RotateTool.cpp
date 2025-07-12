#include "Tools/RotateTool.h"
#include "Utils/BlenderMathHelpers.h"
//TODO fix the rotation behaving oddly when mouse wraps around
//TODO draw the rotation gizmo handle

namespace BlenderControls
{
	FRotateTool::FRotateTool(EAxisLock InAxis)
		: FBlenderToolBase(ETransformMode::Rotate, InAxis, TEXT("Rotate"))
	{
	}

	void FRotateTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();
		bTrackballModeEnabled = false;

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);

		StartPivotTransform = Pivot->GetStartTransform();
		PivotStartPosition = Pivot->GetStartTransform().GetLocation();

		SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);

		StartDragVector = CurrentMousePosition - PivotViewportPosition;
	}

	void FRotateTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		if (!GEditor)
		{
			return;
		}

		if (bTrackballModeEnabled)
		{
			constexpr float MouseDeltaSensitivity = 0.01f;
			FVector2D ScaledMouseDelta = FVector2D(MouseDelta.X, MouseDelta.Y) * MouseDeltaSensitivity;

			if (bSnappingEnabled)
			{
				//Can use .Yaw or .Pitch or .Roll since the snapping value is the same for all.
				const float SnapAngleDeg = GEditor->GetRotGridSize().Yaw;
				const float SnapAngleRad = FMath::DegreesToRadians(SnapAngleDeg);
				const float SnapIncrement = SnapAngleRad;

				ScaledMouseDelta.X = FMath::GridSnap(ScaledMouseDelta.X, SnapIncrement);
				ScaledMouseDelta.Y = FMath::GridSnap(ScaledMouseDelta.Y, SnapIncrement);
			}

			const FVector RotationAxis = (-ViewUp * ScaledMouseDelta.X) + (-ViewRight * ScaledMouseDelta.Y);
			const float RotationAngle = RotationAxis.Length();
			const FQuat TargetRotation = FQuat(RotationAxis.GetSafeNormal(), RotationAngle);

			FTransform NewTransform = StartPivotTransform;
			NewTransform.ConcatenateRotation(TargetRotation);
			Pivot->GetTransformProxy()->SetTransform(NewTransform);
		}
		else
		{
			const FVector2D CurrentDragVector = CurrentMousePosition - PivotViewportPosition;

			const FVector RotationAxis = GrabContext.HelperAxisDir;
			const float ViewAlignmentWithAxis = FVector::DotProduct(GrabContext.ViewForward, RotationAxis);
			float TotalAngleRadians = MathHelper::GetSignedAngle2D(StartDragVector, CurrentDragVector);

			const bool bIsRotationAxisFacingCamera = ViewAlignmentWithAxis > 0;
			if (!bIsRotationAxisFacingCamera)
			{
				TotalAngleRadians = -TotalAngleRadians;
			}

			//REFACTOR INTO A SNAP OFFSET FUNCTION
			FRotator RotationGridSize = GEditor->GetRotGridSize();
			float SnapAngleDeg = RotationGridSize.Yaw;
			float TotalAngleDeg = FMath::RadiansToDegrees(TotalAngleRadians);
			float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
			float SnappedAngleRad = FMath::DegreesToRadians(SnappedAngleDeg);
			float DegreesToRotate = bSnappingEnabled ? SnappedAngleRad : TotalAngleRadians;

			DegreesToRotate += CachedNonTrackballRotationAngle;
			const FQuat TargetRotation = FQuat(RotationAxis, DegreesToRotate);
			CurrentNonTrackballRotationAngle = DegreesToRotate;

			//START PIVOT TRANSFORM ROTATION IS ALWAYS 0, 0, 0. FIX THIS!
			FTransform NewTransform = StartPivotTransform;
			NewTransform.ConcatenateRotation(TargetRotation);
			Pivot->GetTransformProxy()->SetTransform(NewTransform);
			
		}
	}

	void FRotateTool::ApplyNumeric(float Value)
	{
	}

	void FRotateTool::OnEnd(const bool bApply)
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

	void FRotateTool::OnMouseWrap()
	{
		FBlenderToolBase::OnMouseWrap();

		CachedNonTrackballRotationAngle = CurrentNonTrackballRotationAngle;
		StartDragVector = CurrentMousePosition - PivotViewportPosition;
	}

	void FRotateTool::SetTrackballRotationMode(const bool bEnabled)
	{
		bTrackballModeEnabled = bEnabled;
	}

	bool FRotateTool::GetTrackballRotationMode()
	{
		return bTrackballModeEnabled;
	}
} // namespace BlenderControls
