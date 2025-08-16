#include "Tools/RotateTool.h"
#include "LevelEditorViewport.h"
#include "Utils/BlenderMathHelpers.h"
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
			FQuat FinalRotation = TargetRotation * StartPivotTransform.GetRotation();
			NewTransform.SetRotation(FinalRotation);
			NewTransform.NormalizeRotation();
			VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
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
				SignedAccum = -SignedAccum;
			}

			//REFACTOR INTO A SNAP OFFSET FUNCTION
			FRotator RotationGridSize = GEditor->GetRotGridSize();
			float SnapAngleDeg = RotationGridSize.Yaw;
			float TotalAngleDeg = FMath::RadiansToDegrees(SignedAccum);
			float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
			float SnappedAngleRad = FMath::DegreesToRadians(SnappedAngleDeg);
			float DegreesToRotate = bSnappingEnabled ? SnappedAngleRad : SignedAccum;

			//Calculate target rotation
			const FQuat TargetRotation = FQuat(RotationAxis, DegreesToRotate);
			FTransform StartTransform = StartPivotTransform;
			FTransform NewTransform = StartTransform;
			FQuat ResultQuat = TargetRotation * NewTransform.GetRotation();
			ResultQuat.Normalize();

			if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
			{
				for (FChildInfo Child : VirtualPivot->GetChildren())
				{
					const FTransform ChildTransform = Child.Transform;
					const FVector X = ChildTransform.GetUnitAxis(EAxis::X);
					const FVector Y = ChildTransform.GetUnitAxis(EAxis::Y);
					const FVector Z = ChildTransform.GetUnitAxis(EAxis::Z);

					FVector LocalRotationAxis;
					switch (LockedAxis)
					{
					case EAxisLock::X: LocalRotationAxis = X;
						break;
					case EAxisLock::Y: LocalRotationAxis = Y;
						break;
					case EAxisLock::Z: LocalRotationAxis = Z;
						break;

					case EAxisLock::XY: LocalRotationAxis = Z;
						break;
					case EAxisLock::YZ: LocalRotationAxis = X;
						break;
					case EAxisLock::XZ: LocalRotationAxis = Y;
						break;
						
					case EAxisLock::All: LocalRotationAxis = GrabContext.ViewForward;
						break;
					}
					const FQuat TargetRot(LocalRotationAxis, DegreesToRotate);
						
					const FVector CurrentLocation = ChildTransform.GetLocation();
					const FVector CurrentScale = ChildTransform.GetScale3D();
					const FVector NewLocation = PivotPosition + TargetRot.RotateVector(CurrentLocation - PivotPosition);
					const FQuat NewRotation = (TargetRot * ChildTransform.GetRotation()).GetNormalized();

					NewTransform.SetScale3D(CurrentScale);
					NewTransform.SetLocation(NewLocation);
					NewTransform.SetRotation(NewRotation);
					Child.Actor->SetActorTransform(NewTransform);
				}
			}
			else
			{
				NewTransform.SetRotation(ResultQuat);
				VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
			}

			LastDragVector = CurrentDragVector;
		}
	}


	void FRotateTool::ApplyNumeric(float Value)
	{
		FBlenderToolBase::ApplyNumeric(Value);
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
		bTrackballModeEnabled = bEnabled;
		UpdateAxisLock();
	}

	bool FRotateTool::GetTrackballRotationMode()
	{
		return bTrackballModeEnabled;
	}
} // namespace BlenderControls
