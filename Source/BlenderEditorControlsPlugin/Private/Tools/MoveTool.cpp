#include "Tools/MoveTool.h"
#include "LevelEditorViewport.h"
#include "Style.h"
#include "TransformSession.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Input/Numeric/NumericInputStructs.h"
#include "Pivots/VirtualPivotBase.h"
#include "UI/TransformHUD.h"
#include "Utils/MathHelpers.h"

namespace BlenderControls
{
	FMoveTool::FMoveTool(const TSharedRef<FTransformSession>& InSession)
		: FToolBase(InSession, ETransformMode::Translate, TEXT("Move"))
	{
	}

	void FMoveTool::OnBegin()
	{
		FToolBase::OnBegin();

		ViewportClient->SetWidgetMode(UE::Widget::WM_Translate);
		ViewportClient->Invalidate();

		CursorBrush = FStyle::Get().GetBrush(
			TEXT("BlenderEditorControls.Cursors.Move"));

		HudWidget->SetCursorBrush(CursorBrush);
		HudWidget->SetCursorSize(FVector2D(24, 24));
		HudWidget->SetCursorHotspot(FVector2D(12, 12));
		HudWidget->SetCursorOrientation(ECursorOrient::None);
	}

	void FMoveTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FToolBase::OnActive(CurrentViewportMousePosition);

		if (!bIsToolActive)
		{
			return;
		}

		const TSharedPtr<FTransformSession> Session = GetSession();
		if (Session->IsNumericInputActive())
		{
			return;
		}

		// ParallelCos represents an ~8-degree threshold (arccos(0.990) = 8.1).
		// If the view angle is more parallel to the axis than this, 3D projection 
		// becomes mathematically unstable (floating point explode).
		constexpr float ParallelCos = 0.990f;
		float CosAngle = SMALL_NUMBER;

		//If HelperAxisDir is not 0 then we are in single axis lock. 
		if (!GrabContext.SingleLockAxis.IsNearlyZero())
		{
			// Calculate the alignment between the View Forward and the constrained Axis.
			CosAngle = FMath::Abs(FVector::DotProduct(ViewForward.GetSafeNormal(),
			                                          GrabContext.SingleLockAxis.GetSafeNormal()));
		}

		FVector FinalTotalDelta;
		if (CosAngle <= ParallelCos) //Standard Projection (View is sufficiently angled relative to the axis/plane)
		{
			const FVector UnconstrainedMouseDelta3d = (ViewRight * MouseDelta.X * GrabContext.ScreenToWorldScale) +
				(-ViewUp * MouseDelta.Y * GrabContext.ScreenToWorldScale);

			const FVector ActiveObjectStartLocation = GetActiveElementStartLocation();

			//Intersect ghost pos to get the blender mouse drag "feel", NOT the current mouse pos. 
			const FVector GhostPos = ActiveObjectStartLocation + UnconstrainedMouseDelta3d;

			FVector RayOrigin, RayDir;
			if (ViewportClient->IsPerspective())
			{
				RayOrigin = ViewLocation;
				RayDir = (GhostPos - RayOrigin).GetSafeNormal();
				const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
				FinalTotalDelta = FinalHit - ActiveObjectStartLocation;
			}
			else
			{
				RayOrigin = GhostPos;
				RayDir = ViewForward.GetSafeNormal();

				if (GrabContext.ConstraintMode == FGrabContext::EHelperType::AxisPlane)
				{
					const float PlaneViewDot = FVector::DotProduct(RayDir, GrabContext.PlaneNormal);

					if (FMath::IsNearlyZero(PlaneViewDot, KINDA_SMALL_NUMBER))
					{
						const FVector VisibleAxis = MathHelper::SelectMostPerpendicularAxis(
							GrabContext.PlaneAxisU, GrabContext.PlaneAxisV,
							ViewForward.GetSafeNormal());

						FinalTotalDelta = UnconstrainedMouseDelta3d.ProjectOnToNormal(VisibleAxis);
					}
					else
					{
						const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
						FinalTotalDelta = FinalHit - ActiveObjectStartLocation;
					}
				}
				else
				{
					const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
					FinalTotalDelta = FinalHit - ActiveObjectStartLocation;
				}
			}
		}
		else
		{
			const FVector2D ScreenUpVector(0.0f, -1.0f);
			const float ScreenSpaceFactor = FVector2D::DotProduct(MouseDelta, ScreenUpVector);
			const float ScaledFactor = FMath::Sign(ScreenSpaceFactor) * FMath::Square(ScreenSpaceFactor) * 0.1f;

			FinalTotalDelta = GrabContext.SingleLockAxis * ScaledFactor;
		}

		FVector LiveDelta = FinalTotalDelta;
		if (bSnappingEnabled)
		{
			LiveDelta = GetSnapOffset(LiveDelta);
		}

		// Calculate relative 3D frame delta from the snapped total delta
		FVector FrameDelta = LiveDelta - PreviousFrameTotalDelta;
		
		if (FrameDelta.IsNearlyZero())
		{
			return; // Skip doing work if there is no actual movement!
		}

		FRotator Rot = FRotator::ZeroRotator;
		FVector Scale = FVector::ZeroVector;
		const EAxisList::Type Axis = Session->GetResolvedWidgetAxis();
		// Set this, as InputWidgetDelta() internally calls GetCurrentWidgetAxis() for some checks (like surface snapping)
		ViewportClient->SetCurrentWidgetAxis(Axis);

		// Let the active polymorphic ViewportClient (Level, SCS, etc.) handle the drag delta
		bool bHandledManually = false;
		if (VirtualPivot.IsValid())
		{
			bHandledManually = VirtualPivot->ApplyManualTransformDelta(FrameDelta, Rot, Scale);
		}

		if (!bHandledManually)
		{
			ViewportClient->InputWidgetDelta(ViewportClient->Viewport, Axis, FrameDelta, Rot, Scale);
		}

		PreviousFrameTotalDelta = LiveDelta;
		UpdateHud();
	}

	void FMoveTool::ApplyNumeric(double Value)
	{
		FToolBase::ApplyNumeric(Value);
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid()) return;

		FNumericInputProcessor* Processor = Session->GetNumericInputProcessor();
		if (!Processor) return;

		FBlenderNumericState& State = Processor->CurrentState;

		float Slot0 = 0.f;
		float Slot1 = 0.f;
		float Slot2 = 0.f;

		if (State.Slots.IsValidIndex(0))
		{
			Processor->EvaluateSlot(State.Slots[0], Slot0);
		}
		if (State.Slots.IsValidIndex(1))
		{
			Processor->EvaluateSlot(State.Slots[1], Slot1);
		}
		if (State.Slots.IsValidIndex(2))
		{
			Processor->EvaluateSlot(State.Slots[2], Slot2);
		}

		FVector NumericDelta = FVector::ZeroVector;

		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
			NumericDelta.X = Slot0;
			break;
		case EAxisLock::Y:
			NumericDelta.Y = Slot0;
			break;
		case EAxisLock::Z:
			NumericDelta.Z = Slot0;
			break;

		case EAxisLock::XY: // Shift+Z
			NumericDelta.X = Slot0;
			NumericDelta.Y = Slot1;
			break;
		case EAxisLock::XZ: // Shift+Y
			NumericDelta.X = Slot0;
			NumericDelta.Z = Slot1;
			break;
		case EAxisLock::YZ: // Shift+X
			NumericDelta.Y = Slot0;
			NumericDelta.Z = Slot1;
			break;

		case EAxisLock::All:
		default:
			NumericDelta.X = Slot0;
			NumericDelta.Y = Slot1;
			NumericDelta.Z = Slot2;
			break;
		}
		FVector FrameDelta = NumericDelta - PreviousFrameTotalDelta;
		if (FrameDelta.IsNearlyZero()) return;

		FRotator Rot = FRotator::ZeroRotator;
		FVector Scale = FVector::ZeroVector;
		const EAxisList::Type Axis = Session->GetResolvedWidgetAxis();
		ViewportClient->SetCurrentWidgetAxis(Axis);

		ViewportClient->InputWidgetDelta(ViewportClient->Viewport, Axis, FrameDelta, Rot, Scale);

		PreviousFrameTotalDelta = NumericDelta;
	}

	void FMoveTool::UpdateHud()
	{
		FToolBase::UpdateHud();
	}

	void FMoveTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		// if (!HasValidPivotInternal())
		// {
		// 	return;
		// }
		const FTransform ActiveObjectTransform = GetActiveElementStartTransform();
		const FVector X = Session->IsUsingLocalSpace()
			                  ? ActiveObjectTransform.GetUnitAxis(EAxis::X)
			                  : FVector::XAxisVector;
		const FVector Y = Session->IsUsingLocalSpace()
			                  ? ActiveObjectTransform.GetUnitAxis(EAxis::Y)
			                  : FVector::YAxisVector;
		const FVector Z = Session->IsUsingLocalSpace()
			                  ? ActiveObjectTransform.GetUnitAxis(EAxis::Z)
			                  : FVector::ZAxisVector;

		switch (AxisLock)
		{
		case EAxisLock::All:
			GrabContext.ConstraintMode = FGrabContext::EHelperType::ViewPlane;
			GrabContext.PlaneNormal = -GrabContext.ViewForward;
			break;

		case EAxisLock::X:
			GrabContext.ConstraintMode = FGrabContext::EHelperType::AxisLine;
			GrabContext.SingleLockAxis = X;
			GrabContext.PlaneNormal = MathHelper::SelectMostParallelPlaneNormal(Y, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Y:
			GrabContext.ConstraintMode = FGrabContext::EHelperType::AxisLine;
			GrabContext.SingleLockAxis = Y;
			GrabContext.PlaneNormal = MathHelper::SelectMostParallelPlaneNormal(X, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Z:
			GrabContext.ConstraintMode = FGrabContext::EHelperType::AxisLine;
			GrabContext.SingleLockAxis = Z;
			GrabContext.PlaneNormal = MathHelper::SelectMostParallelPlaneNormal(X, Y, GrabContext.ViewForward);
			break;

		case EAxisLock::XY:
			GrabContext.ConstraintMode = FGrabContext::EHelperType::AxisPlane;
			GrabContext.SingleLockAxis = FVector::ZeroVector;
			GrabContext.PlaneNormal = Z;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Y;
			break;

		case EAxisLock::XZ:
			GrabContext.ConstraintMode = FGrabContext::EHelperType::AxisPlane;
			GrabContext.SingleLockAxis = FVector::ZeroVector;
			GrabContext.PlaneNormal = Y;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Z;
			break;

		case EAxisLock::YZ:
			GrabContext.ConstraintMode = FGrabContext::EHelperType::AxisPlane;
			GrabContext.SingleLockAxis = FVector::ZeroVector;
			GrabContext.PlaneNormal = X;
			GrabContext.PlaneAxisU = Y;
			GrabContext.PlaneAxisV = Z;
			break;
		}
	}

	FVector FMoveTool::GetSnapOffset(const FVector LiveDelta) const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (!GEditor)
		{
			return FVector::ZeroVector;
		}

		const float GridSize = GEditor->GetGridSize();
		FVector SnapOffset;
		if (Session->GetLockedAxis() == EAxisLock::X || Session->GetLockedAxis() == EAxisLock::Y || Session->
			GetLockedAxis() == EAxisLock::Z)
		{
			const FVector SnapAxis = GrabContext.SingleLockAxis;
			const float DistanceAlongAxis = FVector::DotProduct(LiveDelta, SnapAxis);
			const float SnappedDistance = FMath::GridSnap(DistanceAlongAxis, GridSize);
			SnapOffset = SnapAxis * SnappedDistance;
		}
		else if (Session->GetLockedAxis() == EAxisLock::XY || Session->GetLockedAxis() == EAxisLock::XZ || Session->
			GetLockedAxis() == EAxisLock::YZ)
		{
			const float DistanceAlongU = FVector::DotProduct(LiveDelta, GrabContext.PlaneAxisU);
			const float DistanceAlongV = FVector::DotProduct(LiveDelta, GrabContext.PlaneAxisV);

			const float SnappedDistanceU = FMath::GridSnap(DistanceAlongU, GridSize);
			const float SnappedDistanceV = FMath::GridSnap(DistanceAlongV, GridSize);

			SnapOffset = (GrabContext.PlaneAxisU * SnappedDistanceU) + (GrabContext.PlaneAxisV * SnappedDistanceV);
		}
		else
		{
			SnapOffset = LiveDelta / GridSize;
			SnapOffset = MathHelper::RoundVectorToInt(SnapOffset);
			SnapOffset *= GridSize;
		}

		return SnapOffset;
	}

	FText FMoveTool::GetNumericHudText() const
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

		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		FNumberFormattingOptions NumFmt;
		NumFmt.MaximumFractionalDigits = 3;
		NumFmt.MinimumFractionalDigits = 3;
		NumFmt.UseGrouping = false;

		FFormatOrderedArguments HudArgs;

		const float Magnitude = Processor->GetTotalMagnitude();
		const FText MagText = FText::Format(
			NSLOCTEXT("MoveHUD", "MagFmt", "({0} cm)"),
			FText::AsNumber(Magnitude, &NumFmt));

		const FText Context = Session->IsUsingLocalSpace()
			                      ? NSLOCTEXT("MoveHUD", "Local", "local")
			                      : NSLOCTEXT("MoveHUD", "Global", "global");

		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
			{
				FString SlotString = Processor->BuildSlotDisplayString(0);
				const FText D = FText::FromString(TEXT("D: ") + SlotString);

				FText AxisName;
				if (Session->GetLockedAxis() == EAxisLock::X) AxisName = NSLOCTEXT("MoveHUD", "AxisX", "X");
				else if (Session->GetLockedAxis() == EAxisLock::Y) AxisName = NSLOCTEXT("MoveHUD", "AxisY", "Y");
				else AxisName = NSLOCTEXT("MoveHUD", "AxisZ", "Z");

				const FText Along = FText::Format(
					NSLOCTEXT("MoveHUD", "AlongFmt", "along {0} {1}"),
					Context, AxisName);

				HudArgs.Add(D);
				HudArgs.Add(MagText);
				HudArgs.Add(Along);
				break;
			}

		case EAxisLock::XY: // Shift+Z
		case EAxisLock::XZ: // Shift+Y
		case EAxisLock::YZ: // Shift+X
			{
				FString Slot0 = Processor->BuildSlotDisplayString(0);
				FString Slot1 = Processor->BuildSlotDisplayString(1);

				const FText D1 = FText::FromString(TEXT("D: ") + Slot0);
				const FText D2 = FText::FromString(TEXT("D: ") + Slot1);

				FText LockingAxisName;
				if (Session->GetLockedAxis() == EAxisLock::XY) LockingAxisName = NSLOCTEXT("MoveHUD", "AxisZ", "Z");
				else if (Session->GetLockedAxis() == EAxisLock::XZ)
					LockingAxisName =
						NSLOCTEXT("MoveHUD", "AxisY", "Y");
				else LockingAxisName = NSLOCTEXT("MoveHUD", "AxisX", "X");

				const FText Locking = FText::Format(
					NSLOCTEXT("MoveHUD", "LockingFmt", "locking {0} {1}"),
					Context, LockingAxisName);

				HudArgs.Add(D1);
				HudArgs.Add(D2);
				HudArgs.Add(MagText);
				HudArgs.Add(Locking);
				break;
			}

		case EAxisLock::All:
		default:
			{
				FString Dx_Str = Processor->BuildSlotDisplayString(0);
				FString Dy_Str = Processor->BuildSlotDisplayString(1);
				FString Dz_Str = Processor->BuildSlotDisplayString(2);

				const FText Dx = FText::FromString(FString::Printf(TEXT("Dx: %s"), *Dx_Str));
				const FText Dy = FText::FromString(FString::Printf(TEXT("Dy: %s"), *Dy_Str));
				const FText Dz = FText::FromString(FString::Printf(TEXT("Dz: %s"), *Dz_Str));

				HudArgs.Add(Dx);
				HudArgs.Add(Dy);
				HudArgs.Add(Dz);
				HudArgs.Add(MagText);
				break;
			}
		}

		return FText::Join(FText::FromString(Gap), HudArgs);
	}


	FText FMoveTool::GetLiveHudText() const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid())
		{
			return FText::GetEmpty();
		}

		const FVector LiveDelta = GetActiveElementCurrentLocation() - GetPivotStartLocation();

		FNumberFormattingOptions NumFmt;
		NumFmt.MaximumFractionalDigits = 3;
		NumFmt.MinimumFractionalDigits = 3;
		NumFmt.UseGrouping = false;

		const float Magnitude = LiveDelta.Size();
		const FText MagText = FText::Format(
			FText::FromString(TEXT("({0} cm)")),
			FText::AsNumber(Magnitude, &NumFmt));

		FText HudText;
		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
			HudText = BuildSingleAxisHudText(LiveDelta, NumFmt, MagText);
			break;

		case EAxisLock::XY:
		case EAxisLock::XZ:
		case EAxisLock::YZ:
			HudText = BuildDualAxisHudText(LiveDelta, NumFmt, MagText);
			break;

		case EAxisLock::All:
		default:
			HudText = BuildFreeformHudText(LiveDelta, NumFmt, MagText);
			break;
		}

		return HudText;
	}

	FText FMoveTool::BuildFreeformHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
	                                      const FText& MagText) const
	{
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		const FText Dx = FText::Format(
			FText::FromString(TEXT("Dx: {0} cm")),
			FText::AsNumber(LiveDelta.X, &NumFmt));

		const FText Dy = FText::Format(
			FText::FromString(TEXT("Dy: {0} cm")),
			FText::AsNumber(LiveDelta.Y, &NumFmt));

		const FText Dz = FText::Format(
			FText::FromString(TEXT("Dz: {0} cm")),
			FText::AsNumber(LiveDelta.Z, &NumFmt));

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(Dx);
		HudArgs.Add(Dy);
		HudArgs.Add(Dz);
		HudArgs.Add(MagText);

		return FText::Join(FText::FromString(Gap), HudArgs);
	}

	FText FMoveTool::BuildSingleAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
	                                        const FText& MagText) const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		const FVector AxisVector = GetAxisVector(Session->GetLockedAxis());
		const float SignedDelta = FVector::DotProduct(LiveDelta, AxisVector);

		const FText D = FText::Format(
			FText::FromString(TEXT("D: {0} cm")),
			FText::AsNumber(SignedDelta, &NumFmt));

		FText AxisName;
		if (Session->GetLockedAxis() == EAxisLock::X) AxisName = FText::FromString("X");
		else if (Session->GetLockedAxis() == EAxisLock::Y) AxisName = FText::FromString("Y");
		else AxisName = FText::FromString("Z");

		const FText Space = Session->IsUsingLocalSpace()
			                    ? FText::FromString("local")
			                    : FText::FromString("global");

		const FText Along = FText::Format(FText::FromString(TEXT("along {0} {1}")),
		                                  Space,
		                                  AxisName);

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(D);
		HudArgs.Add(MagText);
		HudArgs.Add(Along);

		return FText::Join(FText::FromString(Gap), HudArgs);
	}

	FText FMoveTool::BuildDualAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
	                                      const FText& MagText) const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		FVector Axis1, Axis2;
		FText LockingAxisName;

		if (Session->GetLockedAxis() == EAxisLock::XY)
		{
			Axis1 = GetAxisVector(EAxisLock::X);
			Axis2 = GetAxisVector(EAxisLock::Y);
			LockingAxisName = FText::FromString("Z");
		}
		else if (Session->GetLockedAxis() == EAxisLock::XZ)
		{
			Axis1 = GetAxisVector(EAxisLock::X);
			Axis2 = GetAxisVector(EAxisLock::Z);
			LockingAxisName = FText::FromString("Y");
		}
		else
		{
			Axis1 = GetAxisVector(EAxisLock::Y);
			Axis2 = GetAxisVector(EAxisLock::Z);
			LockingAxisName = FText::FromString("X");
		}

		const float Delta1 = FVector::DotProduct(LiveDelta, Axis1);
		const float Delta2 = FVector::DotProduct(LiveDelta, Axis2);

		const FText D1 = FText::Format(
			FText::FromString("D: {0} cm"),
			FText::AsNumber(Delta1, &NumFmt));

		const FText D2 = FText::Format(
			FText::FromString("D: {0} cm"),
			FText::AsNumber(Delta2, &NumFmt));

		const FText Context = Session->IsUsingLocalSpace()
			                      ? FText::FromString("local")
			                      : FText::FromString("global");

		const FText Locking = FText::Format(
			FText::FromString("locking {0} {1}"),
			Context, LockingAxisName);

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(D1);
		HudArgs.Add(D2);
		HudArgs.Add(MagText);
		HudArgs.Add(Locking);

		return FText::Join(FText::FromString(Gap), HudArgs);
	}
}
