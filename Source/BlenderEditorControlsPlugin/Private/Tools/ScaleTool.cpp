#include "Tools/ScaleTool.h"

#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Input/Numeric/NumericInputStructs.h"
#include "Kismet/KismetMathLibrary.h"
#include "Style.h"
#include "Tools/SharedPivot.h"
#include "Tools/ControlRigPivot.h"
#include "UI/TransformHUD.h"
#include "Utils/ControlRigSelectionHelper.h"

// NOTE gizmo automatically sets to local for scaling, since UE doesn't support global mode for scaling unlike this tool

namespace BlenderControls
{
	FScaleTool::FScaleTool(const TSharedRef<FTransformSession>& InSession)
		: FToolBase(InSession, ETransformMode::Scale, TEXT("Scale"))
	{
	}

	void FScaleTool::OnBegin()
	{
		FToolBase::OnBegin();

		const TSharedPtr<FTransformSession> Session = GetSession();
		Session->GetNumericInputProcessor()->SetUniformScaleMode(true);

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));
		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);
		PivotStartPosition = GetPivotStartLocation();
		SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);

		ScaleFactor = 1.0f;
		InitialMouseToPivotDistance = UKismetMathLibrary::Distance2D(CurrentMousePosition, PivotViewportPosition);
		StartScale = GetActiveElementStartTransform().GetScale3D();
		InitialMousePosition = CurrentMousePosition;

		ViewportClient->SetWidgetMode(UE::Widget::WM_Scale);
		ViewportClient->Invalidate();
		CursorBrush = FStyle::Get().GetBrush(
			TEXT("BlenderEditorControls.Cursors.DoubleArrow"));

		HudWidget->SetCursorBrush(CursorBrush);
		HudWidget->SetCursorHotspot(FVector2D(16, 16));
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

		ScaleElements(SnappedScaleMultiplier, Session->IsUsingLocalSpace());
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

		bool bUsingLocalSpace = Session->GetLockedAxis() == EAxisLock::All ? true : Session->IsUsingLocalSpace();
		ScaleElements(ScaleMultiplier, bUsingLocalSpace);
	}

	void FScaleTool::UpdateHud()
	{
		FToolBase::UpdateHud();
	}

	void FScaleTool::OnEnd(const bool bApply)
	{
		FToolBase::OnEnd(bApply);
		if (HudWidget.IsValid())
		{
			HudWidget->SetDashState(/*bEnabled*/false, FVector2D::ZeroVector, FVector2D::ZeroVector);
		}
	}

	void FScaleTool::HandleMouseMovement(const FVector2D& CurrentViewportMousePosition)
	{
		FToolBase::HandleMouseMovement(CurrentViewportMousePosition);
	}

	void FScaleTool::Tick()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (HudWidget.IsValid() && Session.IsValid())
		{
			FSceneViewFamilyContext ViewFamily(
				FSceneViewFamily::ConstructionValues(
					ViewportClient->Viewport,
					ViewportClient->GetScene(),
					ViewportClient->EngineShowFlags));
			const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
			SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);

			HudWidget->SetLineEndpoints(PivotViewportPosition, Session->GetVirtualMousePos());
			HudWidget->SetDashState(true, PivotViewportPosition, Session->GetVirtualMousePos());
			HudWidget->Invalidate(EInvalidateWidgetReason::Paint);
		}
	}

	void FScaleTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!GetSession().IsValid() || !HasValidPivotInternal())
		{
			return;
		}

		const FTransform ObjectTransform = GetActiveElementStartTransform();
		const FVector X = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = Session->IsUsingLocalSpace() ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

		switch (AxisLock)
		{
		case EAxisLock::X:
			GrabContext.SingleLockAxis = X;
			break;
		case EAxisLock::Y:
			GrabContext.SingleLockAxis = Y;
			break;
		case EAxisLock::Z:
			GrabContext.SingleLockAxis = Z;
			break;

		case EAxisLock::XY:
			GrabContext.SingleLockAxis = Z;
			break;
		case EAxisLock::YZ:
			GrabContext.SingleLockAxis = X;
			break;
		case EAxisLock::XZ:
			GrabContext.SingleLockAxis = Y;
			break;

		case EAxisLock::All:
			GrabContext.SingleLockAxis = GrabContext.ViewForward;
			break;
		}
	}

	FText FScaleTool::GetNumericHudText() const
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
		FFormatOrderedArguments HudArgs;

		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
			{
				if (!Processor->CurrentState.Slots.IsValidIndex(0)) return FText::GetEmpty();

				FString SlotString = Processor->BuildSlotDisplayString(0);
				HudArgs.Add(FText::FromString(TEXT("Scale: ") + SlotString));

				FText AxisName;
				if (Session->GetLockedAxis() == EAxisLock::X) AxisName = FText::FromString("X");
				else if (Session->GetLockedAxis() == EAxisLock::Y) AxisName = FText::FromString("Y");
				else AxisName = FText::FromString("Z");

				const FText Context = Session->IsUsingLocalSpace()
					                      ? FText::FromString("local")
					                      : FText::FromString("global");

				const FText Along = FText::Format(
					FText::FromString("along {0} {1}"),
					Context, AxisName);

				HudArgs.Add(Along);
				break;
			}

		case EAxisLock::XY:
		case EAxisLock::XZ:
		case EAxisLock::YZ:
			{
				if (!Processor->CurrentState.Slots.IsValidIndex(1)) return FText::GetEmpty();

				FString Slot0 = Processor->BuildSlotDisplayString(0);
				FString Slot1 = Processor->BuildSlotDisplayString(1);

				HudArgs.Add(FText::FromString(TEXT("Scale: ") + Slot0));
				HudArgs.Add(FText::FromString(TEXT("Scale: ") + Slot1));

				FText LockingAxisName;
				if (Session->GetLockedAxis() == EAxisLock::XY) LockingAxisName = FText::FromString("Z");
				else if (Session->GetLockedAxis() == EAxisLock::XZ)
					LockingAxisName = FText::FromString("Y");
				else LockingAxisName = FText::FromString("X");

				const FText Context = Session->IsUsingLocalSpace()
					                      ? FText::FromString("local")
					                      : FText::FromString("global");

				const FText Locking = FText::Format(
					FText::FromString("locking {0} {1}"),
					Context, LockingAxisName);

				HudArgs.Add(Locking);
				break;
			}

		case EAxisLock::All:
		default:
			{
				if (!Processor->CurrentState.Slots.IsValidIndex(2)) return FText::GetEmpty();

				FString ScaleX_Str = Processor->BuildSlotDisplayString(0);
				FString ScaleY_Str = Processor->BuildSlotDisplayString(1);
				FString ScaleZ_Str = Processor->BuildSlotDisplayString(2);

				const FText ScaleX =
					FText::FromString(FString::Printf(TEXT("Scale X: %s"), *ScaleX_Str));
				const FText ScaleY =
					FText::FromString(FString::Printf(TEXT("Scale Y: %s"), *ScaleY_Str));
				const FText ScaleZ =
					FText::FromString(FString::Printf(TEXT("Scale Z: %s"), *ScaleZ_Str));

				HudArgs.Add(ScaleX);
				HudArgs.Add(ScaleY);
				HudArgs.Add(ScaleZ);
				break;
			}
		}

		return FText::Join(FText::FromString(Gap), HudArgs);
	}

	FText FScaleTool::GetLiveHudText() const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid() || !HasValidPivotInternal())
		{
			return FText::GetEmpty();
		}

		// For Control Rig, we use the scale factor directly since we don't have Actor scale
		// For actors, compute from current/start scale ratio
		FVector CurrentScale, StartScaleVec;
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			// For Control Rig, get the current transform from the hierarchy
			const FControlRigElementInfo& Element = ControlRigVirtualPivot->GetActiveElement();
			CurrentScale = FControlRigSelectionHelper::GetElementGlobalTransform(Element.ElementKey).GetScale3D();
			StartScaleVec = Element.StartTransform.GetScale3D();
		}
		else if (VirtualPivot.IsValid())
		{
			CurrentScale = VirtualPivot->GetActiveElement().Actor->GetActorScale3D();
			StartScaleVec = VirtualPivot->GetStartTransform().GetScale3D();
		}
		else
		{
			return FText::GetEmpty();
		}

		FVector LiveScale;
		LiveScale.X = FMath::IsNearlyZero(StartScaleVec.X) ? 1.0f : CurrentScale.X / StartScaleVec.X;
		LiveScale.Y = FMath::IsNearlyZero(StartScaleVec.Y) ? 1.0f : CurrentScale.Y / StartScaleVec.Y;
		LiveScale.Z = FMath::IsNearlyZero(StartScaleVec.Z) ? 1.0f : CurrentScale.Z / StartScaleVec.Z;

		FNumberFormattingOptions NumFmt;
		NumFmt.MaximumFractionalDigits = 3;
		NumFmt.MinimumFractionalDigits = 3;
		NumFmt.UseGrouping = false;

		FText HudText;
		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
			HudText = BuildSingleAxisHudText(LiveScale, NumFmt);
			break;

		case EAxisLock::XY:
		case EAxisLock::XZ:
		case EAxisLock::YZ:
			HudText = BuildDualAxisHudText(LiveScale, NumFmt);
			break;

		case EAxisLock::All:
		default:
			HudText = BuildFreeformHudText(LiveScale, NumFmt);
			break;
		}

		return HudText;
	}

	FText FScaleTool::BuildFreeformHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const
	{
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		const FText ScaleX = FText::Format(
			FText::FromString("Scale X: {0}"),
			FText::AsNumber(LiveScale.X, &NumFmt));

		const FText ScaleY = FText::Format(
			FText::FromString("Scale Y: {0}"),
			FText::AsNumber(LiveScale.Y, &NumFmt));

		const FText ScaleZ = FText::Format(
			FText::FromString("Scale Z: {0}"),
			FText::AsNumber(LiveScale.Z, &NumFmt));

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(ScaleX);
		HudArgs.Add(ScaleY);
		HudArgs.Add(ScaleZ);

		return FText::Join(FText::FromString(Gap), HudArgs);
	}

	FText FScaleTool::BuildSingleAxisHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		float ScaleValue = 1.0f;
		FText AxisName;

		if (Session->GetLockedAxis() == EAxisLock::X)
		{
			ScaleValue = LiveScale.X;
			AxisName = FText::FromString("X");
		}
		else if (Session->GetLockedAxis() == EAxisLock::Y)
		{
			ScaleValue = LiveScale.Y;
			AxisName = FText::FromString("Y");
		}
		else if (Session->GetLockedAxis() == EAxisLock::Z)
		{
			ScaleValue = LiveScale.Z;
			AxisName = FText::FromString("Z");
		}

		const FText Scale = FText::Format(
			FText::FromString("Scale: {0}"),
			FText::AsNumber(ScaleValue, &NumFmt));

		const FText Context = Session->IsUsingLocalSpace()
			                      ? FText::FromString("local")
			                      : FText::FromString("global");

		const FText Along = FText::Format(
			FText::FromString("along {0} {1}"),
			Context, AxisName);

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(Scale);
		HudArgs.Add(Along);

		return FText::Join(FText::FromString(Gap), HudArgs);
	}

	FText FScaleTool::BuildDualAxisHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		constexpr int32 Spacing = 3;
		const FString Gap = FString::ChrN(Spacing, ' ');

		float ScaleValue = 1.0f;
		FText LockingAxisName;

		if (Session->GetLockedAxis() == EAxisLock::XY) // Shift+Z
		{
			ScaleValue = LiveScale.X; // X and Y will be the same
			LockingAxisName = FText::FromString("Z");
		}
		else if (Session->GetLockedAxis() == EAxisLock::XZ) // Shift+Y
		{
			ScaleValue = LiveScale.X; // X and Z will be the same
			LockingAxisName = FText::FromString("Y");
		}
		else if (Session->GetLockedAxis() == EAxisLock::YZ) // (Shift+X)
		{
			ScaleValue = LiveScale.Y; // Y and Z will be the same
			LockingAxisName = FText::FromString("X");
		}

		const FText Scale = FText::Format(
			FText::FromString("Scale: {0}"),
			FText::AsNumber(ScaleValue, &NumFmt));

		const FText Context = Session->IsUsingLocalSpace()
			                      ? FText::FromString("local")
			                      : FText::FromString("global");

		const FText Locking = FText::Format(
			FText::FromString("locking {0} {1}"),
			Context, LockingAxisName);

		FFormatOrderedArguments HudArgs;
		HudArgs.Add(Scale);
		HudArgs.Add(Locking);

		return FText::Join(FText::FromString(Gap), HudArgs);
	}
} // namespace BlenderControls
