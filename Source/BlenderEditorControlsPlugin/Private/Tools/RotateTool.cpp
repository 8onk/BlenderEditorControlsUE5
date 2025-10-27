#include "Tools/RotateTool.h"
#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "BaseGizmos/TransformProxy.h"
#include "Style/Style.h"
#include "Tools/SharedPivot.h"
#include "UI/TransformHUD.h"
#include "Utils/MathHelpers.h"

namespace BlenderControls
{
	FRotateTool::FRotateTool(const TSharedRef<FTransformSession>& InSession)
		: FToolBase(InSession, ETransformMode::Rotate, TEXT("Rotate"))
	{
	}

	void FRotateTool::OnBegin()
	{
		FToolBase::OnBegin();
		bTrackballModeEnabled = false;

		StartPivotTransform = VirtualPivot->GetStartTransform();
		PivotStartPosition = VirtualPivot->GetStartTransform().GetLocation();

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));
		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);

		StartDragVector = CurrentMousePosition - PivotViewportPosition;
		LastDragVector = StartDragVector;

		ViewportClient->SetWidgetMode(UE::Widget::WM_Rotate);
		ViewportClient->Invalidate();
		AccumulatedAngleRad = 0.0f;
		TrackballMouseDelta = FVector2D::ZeroVector;
		AngleToApplyRad = 0.0f;

		CursorBrush = FStyle::Get().GetBrush(
			TEXT("BlenderEditorControls.Cursors.DoubleArrow"));
		HudWidget->SetCursorBrush(CursorBrush);
		HudWidget->SetCursorSize(FVector2D(24, 24));
		HudWidget->SetCursorHotspot(FVector2D(12, 12));
		HudWidget->SetCursorOrientation(ECursorOrient::PerpendicularCW);
	}

	void FRotateTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FToolBase::OnActive(CurrentViewportMousePosition);

		if (!bIsToolActive)
		{
			return;
		}
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (HudWidget.IsValid() && !bTrackballModeEnabled)
		{
			HudWidget->SetDashState(/*bEnabled=*/true, /*InOriginPx=*/PivotViewportPosition,
			                                     Session->GetVirtualMousePos());
		}
		else
		{
			HudWidget->SetDashState(/*bEnabled=*/false, /*InOriginPx=*/PivotViewportPosition,
			                                     Session->GetVirtualMousePos());
		}

		if (!GEditor || Session->IsNumericInputActive())
		{
			return;
		}

		if (bTrackballModeEnabled)
		{
			constexpr float MouseDeltaSensitivity = 0.01f;
			TrackballMouseDelta = FVector2D(MouseDelta.X, MouseDelta.Y) * MouseDeltaSensitivity;

			if (bSnappingEnabled)
			{
				//Can use .Yaw or .Pitch or .Roll since the snapping value is the same for all.
				const float SnapAngleDeg = GEditor->GetRotGridSize().Yaw;
				const float SnapAngleRad = FMath::DegreesToRadians(SnapAngleDeg);
				const float SnapIncrement = SnapAngleRad;

				TrackballMouseDelta.X = FMath::GridSnap(TrackballMouseDelta.X, SnapIncrement);
				TrackballMouseDelta.Y = FMath::GridSnap(TrackballMouseDelta.Y, SnapIncrement);
			}

			const FVector RotationAxis = (-ViewUp * TrackballMouseDelta.X) + (-ViewRight * TrackballMouseDelta.Y);
			const float RotationAngle = RotationAxis.Length();
			const FQuat TargetRotation = FQuat(RotationAxis.GetSafeNormal(), RotationAngle);

			FTransform NewTransform = StartPivotTransform;
			const FQuat FinalRotation = TargetRotation * StartPivotTransform.GetRotation();
			NewTransform.SetRotation(FinalRotation);
			NewTransform.NormalizeRotation();
			VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
		}
		else
		{
			const FVector2D CurrentDragVector = Session->GetVirtualMousePos() - PivotViewportPosition;
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

			AngleToApplyRad = SignedAccum;
			if (bSnappingEnabled)
			{
				const float SnapAngleDeg = GEditor->GetRotGridSize().Yaw;
				const float TotalAngleDeg = FMath::RadiansToDegrees(SignedAccum);
				const float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
				AngleToApplyRad = FMath::DegreesToRadians(SnappedAngleDeg);
			}

			VirtualPivot->Rotate(GrabContext, AngleToApplyRad, Session->IsUsingLocalSpace(), Session->GetLockedAxis());
			LastDragVector = CurrentDragVector;
		}

		// UpdateHud();
	}


	void FRotateTool::ApplyNumeric(const double Value)
	{
		FToolBase::ApplyNumeric(Value);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (bTrackballModeEnabled)
		{
			const float Slot1 = Session->GetSlotTotalAtIndex(0);
			const float Slot2 = Session->GetSlotTotalAtIndex(1);

			const float AngleXRad = FMath::DegreesToRadians(Slot2);
			const float AngleYRad = FMath::DegreesToRadians(Slot1);

			const FVector RotationAxis = (-ViewUp * AngleXRad) + (-ViewRight * AngleYRad);
			const float RotationAngle = RotationAxis.Length();

			// Check for zero rotation to avoid issues with GetSafeNormal()
			if (FMath::IsNearlyZero(RotationAngle))
			{
				UpdateHud();
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
			const double TotalAngleDegrees = Session->GetSlotTotalAtIndex(0);
			const double RadiansToRotate = FMath::DegreesToRadians(TotalAngleDegrees);
			VirtualPivot->Rotate(GrabContext, RadiansToRotate, Session->IsUsingLocalSpace(), Session->GetLockedAxis());
			UpdateHud();
		}
	}

	void FRotateTool::UpdateHud()
	{
		FToolBase::UpdateHud();
	}

	//
	// void FRotateTool::UpdateHud()
	// {
	// }

	void FRotateTool::OnEnd(const bool bApply)
	{
		FToolBase::OnEnd(bApply);
	}

	void FRotateTool::HandleMouseMovement(const FVector2D& CurrentViewportMousePosition)
	{
		FToolBase::HandleMouseMovement(CurrentViewportMousePosition);
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid())
		{
			return;
		}

		if (HudWidget.IsValid() && !bTrackballModeEnabled)
		{
			HudWidget->SetDashState(/*bEnabled=*/true, /*InOriginPx=*/PivotViewportPosition,
			                                     Session->GetVirtualMousePos());
		}
		else
		{
			HudWidget->SetDashState(/*bEnabled=*/false, /*InOriginPx=*/PivotViewportPosition,
			                                     Session->GetVirtualMousePos());
		}
	}

	FText FRotateTool::GetNumericHudText() const
	{
		return FText::GetEmpty();
	}

	void FRotateTool::HandleAxisLock(const EAxisLock AxisPressed)
	{
		if (bTrackballModeEnabled)
		{
			return;
		}

		FToolBase::HandleAxisLock(AxisPressed);
	}

	void FRotateTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		checkf(OwningSession.IsValid(), TEXT("SetGrabContextAxisLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		const FTransform ObjectTransform = VirtualPivot->GetStartTransform();
		const FVector X = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

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
		const bool bPreviousState = bTrackballModeEnabled;

		if (bPreviousState != bEnabled)
		{
			if (bEnabled)
			{
				CursorBrush = BlenderControls::FStyle::Get().GetBrush(
					TEXT("BlenderEditorControls.Cursors.Trackball"));
				HudWidget->SetCursorBrush(CursorBrush);
				HudWidget->SetCursorOrientation(ECursorOrient::None);
			}
			else
			{
				CursorBrush = BlenderControls::FStyle::Get().GetBrush(
					TEXT("BlenderEditorControls.Cursors.DoubleArrow"));
				HudWidget->SetCursorBrush(CursorBrush);
				HudWidget->SetCursorOrientation(ECursorOrient::PerpendicularCW);
			}

			bTrackballModeEnabled = bEnabled;

			UpdateAxisLock();
		}
	}

	bool FRotateTool::GetTrackballRotationMode()
	{
		return bTrackballModeEnabled;
	}

	void FRotateTool::UpdateToolSettingsForAxisLock()
	{
		if (bTrackballModeEnabled)
		{
			NumNumericSlots = 2;
		}
		else
		{
			NumNumericSlots = 1;
		}
	}

	FString FRotateTool::GetFormattedValueForEditing(const FNumericSlotData& Slot) const
	{
		if (Slot.CommittedValue.IsSet())
		{
			// Degree symbol
			static const TCHAR* Unit = TEXT("\u00B0");
			const FString ValueString = FString::Printf(TEXT("%g"), Slot.CommittedValue.Get(0.0));

			return FString::Printf(TEXT("%s%s"), *ValueString, Unit);
		}
		return FString();
	}

	FText FRotateTool::GetLiveTranslationHudText() const
	{
		return FText::FromString("");
	}

	FText FRotateTool::BuildFreeformHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		const FText& MagText) const
	{
		return FText::FromString("");
	}

	FText FRotateTool::BuildSingleAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		const FText& MagText) const
	{
		return FText::FromString("");
	}

	FText FRotateTool::BuildDualAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		const FText& MagText) const
	{
		return FText::FromString("");
	}
} // namespace BlenderControls
