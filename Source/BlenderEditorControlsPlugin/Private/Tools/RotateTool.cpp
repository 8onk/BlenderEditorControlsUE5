#include "Tools/RotateTool.h"
#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "BaseGizmos/TransformProxy.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Input/Numeric/NumericInputStructs.h"
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
		HudWidget->SetCursorHotspot(FVector2D(16, 16));
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
			//if locked axis is “backwards” relative to the camera, flip the sign
			float SignedAccum = AccumulatedAngleRad;
			bool bShouldCheckFlip = true;
			if (ViewportClient && !ViewportClient->IsPerspective())
			{
				bShouldCheckFlip = false;
			}

			if (bShouldCheckFlip && FVector::DotProduct(ViewToPivot, RotationAxis) < 0)
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

		UpdateHud();
	}


	void FRotateTool::ApplyNumeric(const double Value)
	{
		FToolBase::ApplyNumeric(Value);
		const TSharedPtr<FTransformSession> Session = GetSession();
		FNumericInputProcessor* Processor = Session->GetNumericInputProcessor();
		if (!Processor)
		{
			return;
		}
		FBlenderNumericState& State = Processor->CurrentState;

		if (bTrackballModeEnabled)
		{
			float Slot0 = 0.f;
			float Slot1 = 0.f;

			if (State.Slots.IsValidIndex(0))
			{
				Processor->EvaluateSlot(State.Slots[0], Slot0);
			}
			if (State.Slots.IsValidIndex(1))
			{
				Processor->EvaluateSlot(State.Slots[1], Slot1);
			}

			const float AngleXRad = FMath::DegreesToRadians(Slot1);
			const float AngleYRad = FMath::DegreesToRadians(Slot0);

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
		}
		else
		{
			float Slot0 = 0.f;

			if (State.Slots.IsValidIndex(0))
			{
				Processor->EvaluateSlot(State.Slots[0], Slot0);
			}

			const double RadiansToRotate = FMath::DegreesToRadians(Slot0);
			VirtualPivot->Rotate(GrabContext, RadiansToRotate, Session->IsUsingLocalSpace(), Session->GetLockedAxis());
		}
	}

	void FRotateTool::UpdateHud()
	{
		FToolBase::UpdateHud();
	}

	void FRotateTool::OnEnd(const bool bApply)
	{
		FToolBase::OnEnd(bApply);
	}

	// void FRotateTool::HandleMouseMovement(const FVector2D& CurrentViewportMousePosition)
	// {
	// 	FToolBase::HandleMouseMovement(CurrentViewportMousePosition);
	// 	const TSharedPtr<FTransformSession> Session = GetSession();
	// 	if (!Session.IsValid())
	// 	{
	// 		return;
	// 	}
	//
	// 	// if (HudWidget.IsValid() && !bTrackballModeEnabled)
	// 	// {
	// 	// 	HudWidget->SetDashState(/*bEnabled=*/true, /*InOriginPx=*/PivotViewportPosition,
	// 	// 	                                     Session->GetVirtualMousePos());
	// 	// }
	// 	// else
	// 	// {
	// 	// 	HudWidget->SetDashState(/*bEnabled=*/false, /*InOriginPx=*/PivotViewportPosition,
	// 	// 	                                     Session->GetVirtualMousePos());
	// 	// }
	// }

	FText FRotateTool::GetNumericHudText() const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid())
		{
			return FText::GetEmpty();
		}

		FNumericInputProcessor* Processor = Session->GetNumericInputProcessor();
		if (!Processor)
		{
			return FText::GetEmpty();
		}

		// --- Setup ---
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');
		FFormatOrderedArguments HudArgs;

		// --- Dispatch based on axis lock ---
		switch (Session->GetLockedAxis())
		{
		// --- CASE 1: AXIS-LOCKED (SINGLE SLOT) ---
		// Dual-axis lock is treated as single-axis rotation
		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
		case EAxisLock::XY:
		case EAxisLock::XZ:
		case EAxisLock::YZ:
			{
				// Safety check: Ensure the processor has the slot we need
				if (!Processor->CurrentState.Slots.IsValidIndex(0))
				{
					return FText::GetEmpty();
				}

				// 1. Get the formatted slot string from the processor
				FString SlotString = Processor->BuildSlotDisplayString(0);
				HudArgs.Add(FText::FromString(TEXT("Rotation: ") + SlotString));

				// 2. Get the axis name (copied from your Live HUD logic)
				FText AxisName;
				EAxisLock LockedAxis = Session->GetLockedAxis();
				if (LockedAxis == EAxisLock::X || LockedAxis == EAxisLock::YZ)
					AxisName = NSLOCTEXT(
						"RotateHUD", "AxisX", "X");
				else if (LockedAxis == EAxisLock::Y || LockedAxis == EAxisLock::XZ)
					AxisName = NSLOCTEXT(
						"RotateHUD", "AxisY", "Y");
				else if (LockedAxis == EAxisLock::Z || LockedAxis == EAxisLock::XY)
					AxisName = NSLOCTEXT(
						"RotateHUD", "AxisZ", "Z");

				// 3. Get the context (global/local)
				const FText Context = Session->IsUsingLocalSpace()
					                      ? NSLOCTEXT("RotateHUD", "Local", "local")
					                      : NSLOCTEXT("RotateHUD", "Global", "global");

				// 4. Format the "along" string
				const FText Along = FText::Format(
					NSLOCTEXT("RotateHUD", "AlongFmt", "along {0} {1}"),
					Context, AxisName);

				HudArgs.Add(Along);
				break;
			}

		// --- CASE 2: FREEFORM (CHECK TRACKBALL) ---
		case EAxisLock::All:
		default:
			{
				if (bTrackballModeEnabled)
				{
					// Trackball Mode (2 Slots)
					// Safety check: Ensure the processor has 2 slots
					if (!Processor->CurrentState.Slots.IsValidIndex(1))
					{
						UE_LOG(LogTemp, Error,
						       TEXT("Slot index 1 is NOT valid in BTrackballMode! Current number of slots: %d"),
						       Processor->CurrentState.Slots.Num());
						return FText::GetEmpty();
					}

					FString Slot0 = Processor->BuildSlotDisplayString(0);
					FString Slot1 = Processor->BuildSlotDisplayString(1);

					HudArgs.Add(FText::FromString(TEXT("Trackball:")));
					HudArgs.Add(FText::FromString(Slot0)); // Slot 0 is X Angle
					HudArgs.Add(FText::FromString(Slot1)); // Slot 1 is Y Angle
				}
				else
				{
					// View-Aligned Mode (1 Slot)
					// Safety check
					if (!Processor->CurrentState.Slots.IsValidIndex(0))
					{
						return FText::GetEmpty();
					}

					FString SlotString = Processor->BuildSlotDisplayString(0);
					HudArgs.Add(FText::FromString(TEXT("Rotation: ") + SlotString));
					// No "along..." text for freeform view rotation
				}
				break;
			}
		}
		
		return FText::Join(FText::FromString(Gap), HudArgs);
	}

	void FRotateTool::Tick()
	{
		if (!OwningSession.IsValid() || !HudWidget.IsValid())
		{
			return;
		}

		const TSharedPtr<FTransformSession> Session = GetSession();
		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));
		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);

		if (!bTrackballModeEnabled)
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

	FText FRotateTool::BuildTrackballHudText(double LiveAngleX, double LiveAngleY,
	                                         const FNumberFormattingOptions& NumFmt) const
	{
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		const FText AngleX = FText::AsNumber(LiveAngleX, &NumFmt);
		const FText AngleY = FText::AsNumber(LiveAngleY, &NumFmt);

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(FText::FromString(TEXT("Trackball:")));
		HudArgs.Add(AngleX);
		HudArgs.Add(AngleY);

		return FText::Join(FText::FromString(Gap), HudArgs);
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

		if (bPreviousState == bEnabled)
		{
			return;
		}

		bTrackballModeEnabled = bEnabled;
		if (bEnabled)
		{
			CursorBrush = FStyle::Get().GetBrush(
				TEXT("BlenderEditorControls.Cursors.Trackball"));
			HudWidget->SetCursorBrush(CursorBrush);
			HudWidget->SetCursorOrientation(ECursorOrient::None);
			ClearAxisGizmos();
			UpdateAxisLock();
		}
		else
		{
			CursorBrush = FStyle::Get().GetBrush(
				TEXT("BlenderEditorControls.Cursors.DoubleArrow"));
			HudWidget->SetCursorBrush(CursorBrush);
			HudWidget->SetCursorOrientation(ECursorOrient::PerpendicularCW);
			UpdateAxisLock();
		}

		const TSharedPtr<FTransformSession> Session = GetSession();
		if (Session.IsValid() && Session->IsNumericInputActive())
		{
			ApplyNumeric();
		}

		UpdateHud();
		OnActive(CurrentViewportMousePos);
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

	FText FRotateTool::GetLiveHudText() const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid() || !VirtualPivot.IsValid())
		{
			return FText::GetEmpty();
		}

		FNumberFormattingOptions NumFmt;
		NumFmt.MaximumFractionalDigits = 2;
		NumFmt.MinimumFractionalDigits = 2;
		NumFmt.UseGrouping = false;

		FText HudText;
		switch (Session->GetLockedAxis())
		{
		case EAxisLock::All: // Trackball (Freeform)
		default:
			{
				if (bTrackballModeEnabled)
				{
					const double LiveAngleY = FMath::RadiansToDegrees(TrackballMouseDelta.X);
					const double LiveAngleX = FMath::RadiansToDegrees(TrackballMouseDelta.Y);
					HudText = BuildTrackballHudText(LiveAngleX, LiveAngleY, NumFmt);
				}
				else
				{
					const double LiveAngleDeg = FMath::RadiansToDegrees(AngleToApplyRad) * -1;
					HudText = BuildFreeformHudText(LiveAngleDeg, NumFmt);
				}
			}
			break;

		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
		case EAxisLock::XY:
		case EAxisLock::XZ:
		case EAxisLock::YZ:
			const double LiveAngleDeg = FMath::RadiansToDegrees(AngleToApplyRad) * -1;
			HudText = BuildSingleAxisHudText(LiveAngleDeg, NumFmt);
			break;
		}

		return HudText;
	}

	FText FRotateTool::BuildSingleAxisHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid()) return FText::GetEmpty();

		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		const FText Rotation = FText::Format(
			NSLOCTEXT("RotateHUD", "RotationFmt", "Rotation: {0}"),
			FText::AsNumber(LiveAngleDeg, &NumFmt));

		FText AxisName;
		EAxisLock LockedAxis = Session->GetLockedAxis();

		if (LockedAxis == EAxisLock::X || LockedAxis == EAxisLock::YZ) AxisName = NSLOCTEXT("RotateHUD", "AxisX", "X");
		else if (LockedAxis == EAxisLock::Y || LockedAxis == EAxisLock::XZ)
			AxisName = NSLOCTEXT(
				"RotateHUD", "AxisY", "Y");
		else if (LockedAxis == EAxisLock::Z || LockedAxis == EAxisLock::XY)
			AxisName = NSLOCTEXT(
				"RotateHUD", "AxisZ", "Z");

		const FText Context = Session->IsUsingLocalSpace()
			                      ? NSLOCTEXT("RotateHUD", "Local", "local")
			                      : NSLOCTEXT("RotateHUD", "Global", "global");

		const FText Along = FText::Format(
			NSLOCTEXT("RotateHUD", "AlongFmt", "along {0} {1}"),
			Context, AxisName);

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(Rotation);
		HudArgs.Add(Along);

		return FText::Join(FText::FromString(Gap), HudArgs);
	}

	FText FRotateTool::BuildFreeformHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const
	{
		return FText::Format(
			NSLOCTEXT("RotateHUD", "RotationFmt", "Rotation: {0}"),
			FText::AsNumber(LiveAngleDeg, &NumFmt));
	}
} // namespace BlenderControls
