#include "Tools/MoveTool.h"
#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Input/Numeric/NumericInputStructs.h"
#include "Style/Style.h"
#include "Tools/SharedPivot.h"
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
		HudWidget->SetCursorHotspot(FVector2D(4, 4));
		HudWidget->SetCursorOrientation(ECursorOrient::None);
	}

	FString FMoveTool::GetFormattedValueForEditing(const FNumericSlotData& Slot) const
	{
		if (Slot.CommittedValue.IsSet())
		{
			static const TCHAR* Unit = TEXT("cm");
			const FString ValueString = FString::Printf(TEXT("%g"), Slot.CommittedValue.Get(0.0));

			return FString::Printf(TEXT("%s %s"), *ValueString, Unit);
		}
		return FString();
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

		constexpr float ParallelCos = 0.990f;
		float CosAngle = SMALL_NUMBER;
		//If HelperAxisDir is not 0 then we are in single axis lock. 
		if (!GrabContext.HelperAxisDir.IsNearlyZero())
		{
			CosAngle = FMath::Abs(FVector::DotProduct(ViewForward.GetSafeNormal(),
			                                          GrabContext.HelperAxisDir.GetSafeNormal()));
		}

		FVector FinalTotalDelta;
		if (CosAngle <= ParallelCos)
		{
			const FVector UnconstrainedMouseDelta3d = (ViewRight * MouseDelta.X * GrabContext.ScreenToWorldScale) +
				(-ViewUp * MouseDelta.Y * GrabContext.ScreenToWorldScale);

			const FVector ActiveObjectLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();

			//Intersect ghost pos to get the blender "feel", NOT the current mouse pos. 
			const FVector GhostPos = ActiveObjectLocation + UnconstrainedMouseDelta3d;

			FVector RayOrigin, RayDir;
			if (ViewportClient->IsPerspective())
			{
				RayOrigin = ViewLocation;
				RayDir = (GhostPos - RayOrigin).GetSafeNormal();
				const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
				FinalTotalDelta = FinalHit - ActiveObjectLocation;
			}
			else
			{
				RayOrigin = GhostPos;
				RayDir = ViewForward.GetSafeNormal();

				if (GrabContext.HelperType == FGrabContext::EHelperType::AxisPlane)
				{
					const float PlaneViewDot = FVector::DotProduct(RayDir, GrabContext.HelperPlaneN);

					//If view forward and plane normal vectors are parallel
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
						FinalTotalDelta = FinalHit - ActiveObjectLocation;
					}
				}
				else
				{
					const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
					FinalTotalDelta = FinalHit - ActiveObjectLocation;
				}
			}
		}
		else
		{
			const FVector2D ScreenUpVector(0.0f, -1.0f);
			const float ScreenSpaceFactor = FVector2D::DotProduct(MouseDelta, ScreenUpVector);
			const float ScaledFactor = FMath::Sign(ScreenSpaceFactor) * FMath::Square(ScreenSpaceFactor) * 0.1f;

			FinalTotalDelta = GrabContext.HelperAxisDir * ScaledFactor;
		}

		FVector LiveDelta = FinalTotalDelta;
		if (bSnappingEnabled)
		{
			LiveDelta = GetSnapOffset(LiveDelta);
		}

		VirtualPivot->Translate(Session->IsUsingLocalSpace(), Session->GetLockedAxis(), LiveDelta);
	}

	void FMoveTool::ApplyNumeric(double Value)
	{
		FToolBase::ApplyNumeric(Value);
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid() || !VirtualPivot.IsValid()) return;

		const FBlenderNumericState& State = Session->GetNumericInputProcessor()->CurrentState;

		float Slot0 = 0.f;
		float Slot1 = 0.f;
		float Slot2 = 0.f;

		Session->GetNumericInputProcessor()->EvaluateSlot(State.Slots[0], Slot0);
		Session->GetNumericInputProcessor()->EvaluateSlot(State.Slots[1], Slot1);
		Session->GetNumericInputProcessor()->EvaluateSlot(State.Slots[2], Slot2);

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

		VirtualPivot->Translate(NumericDelta, Session->IsUsingLocalSpace());
	}

	void FMoveTool::UpdateHud()
	{
		FToolBase::UpdateHud();
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FToolBase::OnEnd(bApply);
	}

	void FMoveTool::UpdateToolSettingsForAxisLock()
	{
		// switch (Session->GetLockedAxis())
		// {
		// case EAxisLock::X:
		// case EAxisLock::Y:
		// case EAxisLock::Z:
		// 	NumNumericSlots = 1;
		// 	break;
		//
		// case EAxisLock::XY:
		// case EAxisLock::XZ:
		// case EAxisLock::YZ:
		// 	NumNumericSlots = 2;
		// 	break;
		//
		// case EAxisLock::All:
		// default:
		// 	NumNumericSlots = 3;
		// 	break;
		// }
	}

	FText FMoveTool::GetLiveTranslationHudText() const
	{
		const FVector LiveDelta = VirtualPivot->GetActiveElement().Actor->GetActorLocation() - VirtualPivot->
			GetStartLocation();

		// Format it (this is your own logic)
		return FText::FromString(FString::Printf(
			TEXT("Dx: %.3f m Dy: %.3f m Dz: %.3f m"),
			LiveDelta.X, LiveDelta.Y, LiveDelta.Z
		));
	}

	void FMoveTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		checkf(OwningSession.IsValid(), TEXT("SetGrabContextAxisLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (!VirtualPivot)
		{
			return;
		}
		const FTransform ActiveObjectTransform = VirtualPivot->GetActiveElement().Transform;
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
			GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
			GrabContext.HelperPlaneN = -GrabContext.ViewForward;
			break;

		case EAxisLock::X:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = X;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(Y, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Y:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Y;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Z:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Z;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Y, GrabContext.ViewForward);
			break;

		case EAxisLock::XY:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Z;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Y;
			break;

		case EAxisLock::XZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Y;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Z;
			break;

		case EAxisLock::YZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = X;
			GrabContext.PlaneAxisU = Y;
			GrabContext.PlaneAxisV = Z;
			break;
		}
	}

	FVector FMoveTool::GetSnapOffset(const FVector OffsetFromStart)
	{
		checkf(OwningSession.IsValid(), TEXT("GetSnapOffset: Session must be valid for %s"), *DisplayName);
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
			const FVector SnapAxis = GrabContext.HelperAxisDir;
			const float DistanceAlongAxis = FVector::DotProduct(OffsetFromStart, SnapAxis);
			const float SnappedDistance = FMath::GridSnap(DistanceAlongAxis, GridSize);
			SnapOffset = SnapAxis * SnappedDistance;
		}
		else if (Session->GetLockedAxis() == EAxisLock::XY || Session->GetLockedAxis() == EAxisLock::XZ || Session->
			GetLockedAxis() == EAxisLock::YZ)
		{
			const float DistanceAlongU = FVector::DotProduct(OffsetFromStart, GrabContext.PlaneAxisU);
			const float DistanceAlongV = FVector::DotProduct(OffsetFromStart, GrabContext.PlaneAxisV);

			const float SnappedDistanceU = FMath::GridSnap(DistanceAlongU, GridSize);
			const float SnappedDistanceV = FMath::GridSnap(DistanceAlongV, GridSize);

			SnapOffset = (GrabContext.PlaneAxisU * SnappedDistanceU) + (GrabContext.PlaneAxisV * SnappedDistanceV);
		}
		else
		{
			SnapOffset = OffsetFromStart / GridSize;
			SnapOffset = MathHelper::RoundVectorToInt(SnapOffset);
			SnapOffset *= GridSize;
		}

		return SnapOffset;
	}
}
