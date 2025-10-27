#include "Tools/ScaleTool.h"

#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Input/Numeric/NumericInputStructs.h"
#include "Kismet/KismetMathLibrary.h"
#include "Style/Style.h"
#include "Tools/SharedPivot.h"
#include "UI/TransformHUD.h"

// NOTE gizmo automatically sets to local for scaling, since UE doesnt support global mode for scaling
//TODO when switching from another tool to scale, object origin gets shifted

namespace BlenderControls
{
	FScaleTool::FScaleTool(const TSharedRef<FTransformSession>& InSession)
		: FToolBase(InSession, ETransformMode::Scale, TEXT("Scale"))
	{
	}

	void FScaleTool::OnBegin()
	{
		FToolBase::OnBegin();

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));
		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);
		PivotStartPosition = VirtualPivot->GetStartTransform().GetLocation();
		SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);

		ScaleFactor = 1.0f;
		InitialMouseToPivotDistance = UKismetMathLibrary::Distance2D(CurrentMousePosition, PivotViewportPosition);
		StartScale = VirtualPivot->GetStartTransform().GetScale3D();
		InitialMousePosition = CurrentMousePosition;

		ViewportClient->SetWidgetMode(UE::Widget::WM_Scale);
		ViewportClient->Invalidate();
		CursorBrush = BlenderControls::FStyle::Get().GetBrush(
			TEXT("BlenderEditorControls.Cursors.DoubleArrow"));

		HudWidget->SetCursorBrush(CursorBrush);
		HudWidget->SetCursorSize(FVector2D(24, 24));
		HudWidget->SetCursorHotspot(FVector2D(12, 12));
		HudWidget->SetCursorOrientation(ECursorOrient::AlongLineToOrigin);
	}

	void FScaleTool::OnActive(const FVector2D& CurrentViewportMousePosition)
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

		const FVector2D ScaledVirtualMousePosition = InitialMousePosition + MouseDelta;
		CurrentMouseToPivotDistance = UKismetMathLibrary::Distance2D(ScaledVirtualMousePosition, PivotViewportPosition);

		if (InitialMouseToPivotDistance > KINDA_SMALL_NUMBER)
		{
			const FVector2D StartVec = InitialMousePosition - PivotViewportPosition;
			const FVector2D CurrentVec = ScaledVirtualMousePosition - PivotViewportPosition;

			const float Sign = FMath::Sign(FVector2D::DotProduct(CurrentVec, StartVec));
			ScaleFactor = Sign * (CurrentMouseToPivotDistance / InitialMouseToPivotDistance);
		}

		FVector FinalScaleMultiplier(1.0f);
		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
			FinalScaleMultiplier.X = ScaleFactor;
			break;
		case EAxisLock::Y:
			FinalScaleMultiplier.Y = ScaleFactor;
			break;
		case EAxisLock::Z:
			FinalScaleMultiplier.Z = ScaleFactor;
			break;
		case EAxisLock::XY:
			FinalScaleMultiplier.X = ScaleFactor;
			FinalScaleMultiplier.Y = ScaleFactor;
			break;
		case EAxisLock::XZ:
			FinalScaleMultiplier.X = ScaleFactor;
			FinalScaleMultiplier.Z = ScaleFactor;
			break;
		case EAxisLock::YZ:
			FinalScaleMultiplier.Y = ScaleFactor;
			FinalScaleMultiplier.Z = ScaleFactor;
			break;
		case EAxisLock::All:
			FinalScaleMultiplier = FVector(ScaleFactor);
			break;
		default:
			break;
		}

		FVector SnappedScaleMultiplier = FinalScaleMultiplier;
		if (bSnappingEnabled)
		{
			const float SnappingIncrement = GEditor->GetScaleGridSize();
			SnappedScaleMultiplier.X = FMath::GridSnap(FinalScaleMultiplier.X, SnappingIncrement);
			SnappedScaleMultiplier.Y = FMath::GridSnap(FinalScaleMultiplier.Y, SnappingIncrement);
			SnappedScaleMultiplier.Z = FMath::GridSnap(FinalScaleMultiplier.Z, SnappingIncrement);
		}

		VirtualPivot->Scale(SnappedScaleMultiplier, Session->IsUsingLocalSpace());

		UpdateHud();
	}

	void FScaleTool::ApplyNumeric(double Value)
	{
		FToolBase::ApplyNumeric(Value);
		const TSharedPtr<FTransformSession> Session = GetSession();

		FNumericInputProcessor* Processor = Session->GetNumericInputProcessor();
		if (!Processor) return;

		FBlenderNumericState& State = Processor->CurrentState;

		FVector ScaleMultiplier = FVector::OneVector;
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

		switch (Session->GetLockedAxis())
		{
		case EAxisLock::All:
			ScaleMultiplier = FVector(Slot0, Slot1, Slot2);
			break;

		case EAxisLock::X:
			ScaleMultiplier.X = Slot0;
			break;
		case EAxisLock::Y:
			ScaleMultiplier.Y = Slot0;
			break;
		case EAxisLock::Z:
			ScaleMultiplier.Z = Slot0;
			break;

		case EAxisLock::XY:
			ScaleMultiplier.X = Slot0;
			ScaleMultiplier.Y = Slot1;
			break;
		case EAxisLock::XZ:
			ScaleMultiplier.X = Slot0;
			ScaleMultiplier.Z = Slot1;
			break;
		case EAxisLock::YZ:
			ScaleMultiplier.Y = Slot0;
			ScaleMultiplier.Z = Slot1;
			break;
		}

		VirtualPivot->Scale(ScaleMultiplier, Session->IsUsingLocalSpace());
		UpdateHud();
	}

	void FScaleTool::UpdateHud()
	{
		FToolBase::UpdateHud();
	}

	//
	// void FScaleTool::UpdateHud()
	// {  
	// 	if (!VirtualPivot || !HudWidget.IsValid() || !VirtualPivot->GetActiveElement().Actor)
	// 	{
	// 		return;
	// 	}
	// }

	void FScaleTool::OnEnd(const bool bApply)
	{
		FToolBase::OnEnd(bApply);
		if (HudWidget.IsValid())
		{
			HudWidget->SetDashState(false, FVector2D::ZeroVector, FVector2D::ZeroVector);
		}
	}

	void FScaleTool::HandleMouseMovement(const FVector2D& CurrentViewportMousePosition)
	{
		FToolBase::HandleMouseMovement(CurrentViewportMousePosition);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (HudWidget.IsValid() && Session.IsValid())
		{
			HudWidget->SetLineEndpoints(PivotViewportPosition, Session->GetVirtualMousePos());
			HudWidget->SetDashState(true, PivotViewportPosition, Session->GetVirtualMousePos());
			HudWidget->Invalidate(EInvalidateWidgetReason::Paint);
		}
	}

	FText FScaleTool::GetNumericHudText() const
	{
		return FText::GetEmpty();
	}

	void FScaleTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		checkf(OwningSession.IsValid(), TEXT("SetGrabContextAxisLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		const FTransform ObjectTransform = VirtualPivot->GetStartTransform();
		const FVector X = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

		switch (AxisLock)
		{
		case EAxisLock::X:
			GrabContext.HelperAxisDir = X;
			break;
		case EAxisLock::Y:
			GrabContext.HelperAxisDir = Y;
			break;
		case EAxisLock::Z:
			GrabContext.HelperAxisDir = Z;
			break;

		case EAxisLock::XY:
			GrabContext.HelperAxisDir = Z;
			break;
		case EAxisLock::YZ:
			GrabContext.HelperAxisDir = X;
			break;
		case EAxisLock::XZ:
			GrabContext.HelperAxisDir = Y;
			break;

		case EAxisLock::All:
			GrabContext.HelperAxisDir = GrabContext.ViewForward;
			break;
		}
	}

	void FScaleTool::UpdateToolSettingsForAxisLock()
	{
		checkf(OwningSession.IsValid(), TEXT("UpdateToolSettingsForAxisLock: Session must be valid for %s"),
		       *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
			NumNumericSlots = 1;
			break;

		case EAxisLock::XY:
		case EAxisLock::XZ:
		case EAxisLock::YZ:
			NumNumericSlots = 2;
			break;

		case EAxisLock::All:
		default:
			NumNumericSlots = 3;
			break;
		}
	}

	FText FScaleTool::GetLiveHudText() const
	{
		return FText::FromString("");
	}

	FText FScaleTool::BuildFreeformHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
	                                       const FText& MagText) const
	{
		return FText::FromString("");
	}

	FText FScaleTool::BuildSingleAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
	                                         const FText& MagText) const
	{
		return FText::FromString("");
	}

	FText FScaleTool::BuildDualAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
	                                       const FText& MagText) const
	{
		return FText::FromString("");
	}
} // namespace BlenderControls
