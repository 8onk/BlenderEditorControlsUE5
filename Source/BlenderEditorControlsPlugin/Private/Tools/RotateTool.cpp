#include "Tools/RotateTool.h"
#include "LevelEditorViewport.h"
#include "Tools/SharedPivot.h"
#include "Utils/BlenderMathHelpers.h"
//TODO draw the rotation gizmo handle
//TODO apply numeric for rotation tool and scale tool

namespace BlenderControls
{
	FRotateTool::FRotateTool(TSharedPtr<FTransformSession> InSession, EAxisLock InAxis)
		: FBlenderToolBase(InSession, ETransformMode::Rotate, InAxis, TEXT("Rotate"))
	{
	}

	void FRotateTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();
		bTrackballModeEnabled = false;

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);

		StartPivotTransform = VirtualPivot->GetStartTransform();
		PivotStartPosition = VirtualPivot->GetStartTransform().GetLocation();

		SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);
		StartDragVector = CurrentMousePosition - PivotViewportPosition;
		LastDragVector = StartDragVector;

		ViewportClient->SetWidgetMode(UE::Widget::WM_Rotate);
		ViewportClient->Invalidate();
		AccumulatedAngleRad = 0.0f;
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
			const FQuat FinalRotation = TargetRotation * StartPivotTransform.GetRotation();
			NewTransform.SetRotation(FinalRotation);
			NewTransform.NormalizeRotation();
			VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
			UpdateHud();
		}
		else
		{
			const FVector2D CurrentDragVector = VirtualMousePosition - PivotViewportPosition;
			float AngleDeltaRad = MathHelper::GetSignedAngle2D(LastDragVector, CurrentDragVector);
			AccumulatedAngleRad += AngleDeltaRad * CurrentPrecisionFactor;

			const FVector PivotPosition = VirtualPivot->GetTransformProxy()->GetTransform().GetLocation();
			const FVector ViewToPivot = PivotPosition - ViewLocation;

			const FVector RotationAxis = GrabContext.HelperAxisDir;
			//if lock‐axis is “backwards” relative to the camera, flip the sign
			float SignedAccum = AccumulatedAngleRad;
			if (FVector::DotProduct(ViewToPivot, RotationAxis) < 0)
			{
				SignedAccum = -AccumulatedAngleRad;
			}

			float AngleToApplyRad = SignedAccum;
			if (bSnappingEnabled)
			{
				const float SnapAngleDeg = GEditor->GetRotGridSize().Yaw;
				const float TotalAngleDeg = FMath::RadiansToDegrees(SignedAccum);
				const float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
				AngleToApplyRad = FMath::DegreesToRadians(SnappedAngleDeg);
			}

			VirtualPivot->Rotate(GrabContext, AngleToApplyRad, bUsingLocalSpace, LockedAxis);
			UpdateHud();

			LastDragVector = CurrentDragVector;
		}
	}


	void FRotateTool::ApplyNumeric(const float Value)
	{
		FBlenderToolBase::ApplyNumeric(Value);

		if (bTrackballModeEnabled)
		{
			const float Slot1 = NumericInputSlots[0].GetValue();
			const float Slot2 = NumericInputSlots[1].GetValue();

			const float AngleXRad = FMath::DegreesToRadians(Slot2);
			const float AngleYRad = FMath::DegreesToRadians(Slot1);
			
			const FVector RotationAxis = (-ViewUp * AngleXRad) + (-ViewRight * AngleYRad);
			const float RotationAngle = RotationAxis.Length();

			// Check for zero rotation to avoid issues with GetSafeNormal()
			if (FMath::IsNearlyZero(RotationAngle))
			{
				return;
			}

			const FQuat TargetRotation = FQuat(RotationAxis.GetSafeNormal(), RotationAngle);

			FTransform NewTransform = StartPivotTransform;
			const FQuat FinalRotation = TargetRotation * StartPivotTransform.GetRotation();
			NewTransform.SetRotation(FinalRotation);
			NewTransform.NormalizeRotation();
			VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
			UpdateHud();
		}
		else
		{
			const float DegreesToRotate = FMath::DegreesToRadians(Value);
			VirtualPivot->Rotate(GrabContext, DegreesToRotate, bUsingLocalSpace, LockedAxis);
			UpdateHud();
		}
	}

	void FRotateTool::UpdateHud()
	{
		// if (!VirtualPivot)
		// {
		// 	HudString = TEXT("No selection");
		// 	return;
		// }
		//
		// const FQuat CurrentRotation = VirtualPivot->GetActiveElement().Transform.GetRotation();
		// const FQuat StartRotation = VirtualPivot->GetStartTransform().GetRotation();
		// const FQuat DeltaRotation = CurrentRotation * StartRotation.Inverse();
		//
		// FVector EulerAngles = DeltaRotation.Euler();
		//
		// // Format the HUD string with rotation angles
		// HudString = FString::Printf(TEXT("Rx: %.1f°   Ry: %.1f°   Rz: %.1f°"), 
		// 	EulerAngles.X, EulerAngles.Y, EulerAngles.Z);
	}

	void FRotateTool::OnEnd(const bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FRotateTool::HandleAxisLock(const EAxisLock AxisPressed)
	{
		if (bTrackballModeEnabled)
		{
			return;
		}

		FBlenderToolBase::HandleAxisLock(AxisPressed);
	}

	void FRotateTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		const FTransform ObjectTransform = VirtualPivot->GetStartTransform();
		const FVector X = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

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
		// SnapOffset = MathHelper::RoundVect
		// orToInt(SnapOffset);
		// SnapOffset *= RotationGridSize;
		//
		// return SnapOffset;
	}

	void FRotateTool::SetTrackballRotationMode(const bool bEnabled)
	{
		if (!bTrackballModeEnabled)
		{
			PreviousAxisLock = LockedAxis;
			LockedAxis = EAxisLock::All;
		}
		else
		{
			LockedAxis = PreviousAxisLock;
		}
		Session->LockedAxis = LockedAxis;
		bTrackballModeEnabled = bEnabled;
		UpdateAxisLock();
	}

	bool FRotateTool::GetTrackballRotationMode()
	{
		return bTrackballModeEnabled;
	}
} // namespace BlenderControls
