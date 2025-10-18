#include "Tools/ScaleTool.h"
#include "LevelEditorViewport.h"
#include "Kismet/KismetMathLibrary.h"
#include "Style/BlenderControlsStyle.h"
#include "Tools/SharedPivot.h"
#include "UI/TransformHUD.h"

// NOTE gizmo automatically sets to local for scaling, since UE doesnt support global mode for scaling
//TODO when switching from another tool to scale, object origin gets shifted

namespace BlenderControls
{
	FScaleTool::FScaleTool(TSharedPtr<FTransformSession> InSession, EAxisLock InAxis)
		: FBlenderToolBase(InSession, ETransformMode::Scale, InAxis, TEXT("Scale"))
	{
	}

	void FScaleTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();

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
		CursorBrush = BlenderEditorControls::FBlenderControlsStyle::Get().GetBrush(
			TEXT("BlenderEditorControls.Cursors.DoubleArrow"));

		HudWidget->SetCursorBrush(CursorBrush);
		HudWidget->SetCursorSize(FVector2D(24, 24));
		HudWidget->SetCursorHotspot(FVector2D(12, 12));
		HudWidget->SetCursorOrientation(ECursorOrient::AlongLineToOrigin);
	}

	void FScaleTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		if (!GEditor || !SceneView || Session->bIsNumericInputActive)
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
		switch (LockedAxis)
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

		VirtualPivot->Scale(SnappedScaleMultiplier, bUsingLocalSpace);

		if (HudWidget.IsValid())
		{
			HudWidget->SetLineEndpoints(PivotViewportPosition, Session->VirtualMousePosition);
			HudWidget->SetDashState(true, PivotViewportPosition, Session->VirtualMousePosition);
			HudWidget->Invalidate(EInvalidateWidgetReason::Paint);
		}

		UpdateHud();
	}

	void FScaleTool::ApplyNumeric(double Value)
	{
		FBlenderToolBase::ApplyNumeric(Value);

		FVector ScaleMultiplier = FVector::OneVector;
		const double Slot1 = Session->NumericSlots[0].GetTotal();
		const double Slot2 = Session->NumericSlots[1].GetTotal();
		const double Slot3 = Session->NumericSlots[2].GetTotal();

		switch (LockedAxis)
		{
		case EAxisLock::All:
			ScaleMultiplier = FVector(Slot1, Slot2, Slot3);
			break;

		case EAxisLock::X:
			ScaleMultiplier.X = Slot1;
			break;
		case EAxisLock::Y:
			ScaleMultiplier.Y = Slot1;
			break;
		case EAxisLock::Z:
			ScaleMultiplier.Z = Slot1;
			break;

		case EAxisLock::XY:
			ScaleMultiplier.X = Slot1;
			ScaleMultiplier.Y = Slot2;
			break;
		case EAxisLock::XZ:
			ScaleMultiplier.X = Slot1;
			ScaleMultiplier.Z = Slot2;
			break;
		case EAxisLock::YZ:
			ScaleMultiplier.Y = Slot1;
			ScaleMultiplier.Z = Slot2;
			break;
		}

		VirtualPivot->Scale(ScaleMultiplier, bUsingLocalSpace);
		UpdateHud();
	}

	void FScaleTool::UpdateHud()
	{
		if (!VirtualPivot || !HudWidget.IsValid() || !VirtualPivot->GetActiveElement().Actor)
		{
			return;
		}

		const FVector CurrentScale = VirtualPivot->GetActiveElement().Actor->GetActorScale3D();
		const FVector StartScaleVec = VirtualPivot->GetStartTransform().GetScale3D();

		// Safely calculate the live scale multiplier, avoiding division by zero
		FVector LiveScaleMultiplier;
		LiveScaleMultiplier.X = FMath::IsNearlyZero(StartScaleVec.X) ? 1.0f : CurrentScale.X / StartScaleVec.X;
		LiveScaleMultiplier.Y = FMath::IsNearlyZero(StartScaleVec.Y) ? 1.0f : CurrentScale.Y / StartScaleVec.Y;
		LiveScaleMultiplier.Z = FMath::IsNearlyZero(StartScaleVec.Z) ? 1.0f : CurrentScale.Z / StartScaleVec.Z;

		static const TCHAR* Unit = TEXT(""); // Scale is unitless
		static const TCHAR* Sep = TEXT("\u2003"); // EM SPACE
		const bool bNumeric = Session->bIsNumericInputActive;
		const FString Space = bUsingLocalSpace ? TEXT("local") : TEXT("global");

		enum class EValueSlot : int32
		{
			X = 0,
			Y = 1,
			Z = 2
		};

		struct FHudFieldData
		{
			FString Label;
			double LiveValue;
			EValueSlot SlotIndex;
		};

		TArray<FHudFieldData> HudFields;
		FString Suffix;
		FString Separator = Sep;

		switch (LockedAxis)
		{
		case EAxisLock::All:
			HudFields.Add({TEXT("Scale X"), LiveScaleMultiplier.X, EValueSlot::X});
			HudFields.Add({TEXT("Y"), LiveScaleMultiplier.Y, EValueSlot::Y});
			HudFields.Add({TEXT("Z"), LiveScaleMultiplier.Z, EValueSlot::Z});
			//Halve separation to make spacing between axis' uniform across tools since units are missing
			Separator = TEXT("\u2002");
			break;

		case EAxisLock::X:
			HudFields.Add({TEXT("Scale"), LiveScaleMultiplier.X, EValueSlot::X});
			Suffix = FString::Printf(TEXT("along %s X"), *Space);
			break;

		case EAxisLock::Y:
			HudFields.Add({TEXT("Scale"), LiveScaleMultiplier.Y, EValueSlot::X});
			Suffix = FString::Printf(TEXT("along %s Y"), *Space);
			break;

		case EAxisLock::Z:
			HudFields.Add({TEXT("Scale"), LiveScaleMultiplier.Z, EValueSlot::X});
			Suffix = FString::Printf(TEXT("along %s Z"), *Space);
			break;

		case EAxisLock::XY:
			HudFields.Add({TEXT("Scale"), LiveScaleMultiplier.X, EValueSlot::X});
			HudFields.Add({TEXT(""), LiveScaleMultiplier.Y, EValueSlot::Y});
			Suffix = FString::Printf(TEXT("locking %s Z"), *Space);
			Separator = TEXT(" : ");
			break;

		case EAxisLock::XZ:
			HudFields.Add({TEXT("Scale"), LiveScaleMultiplier.X, EValueSlot::X});
			HudFields.Add({TEXT(""), LiveScaleMultiplier.Z, EValueSlot::Y});
			Suffix = FString::Printf(TEXT("locking %s Y"), *Space);
			Separator = TEXT(" : ");
			break;

		case EAxisLock::YZ:
			HudFields.Add({TEXT("Scale"), LiveScaleMultiplier.Y, EValueSlot::X});
			HudFields.Add({TEXT(""), LiveScaleMultiplier.Z, EValueSlot::Y});
			Suffix = FString::Printf(TEXT("locking %s X"), *Space);
			Separator = TEXT(" : ");
			break;
		}

		TArray<FString> FormattedFields;
		for (const FHudFieldData& FieldData : HudFields)
		{
			const int32 SlotIndexInt = static_cast<int32>(FieldData.SlotIndex);
			FNumericSlotData DataToFormat;

			if (bNumeric)
			{
				DataToFormat = Session->NumericSlots[SlotIndexInt];
			}
			else
			{
				DataToFormat.SlotState = ESlotState::Committed;
				DataToFormat.CommittedValue = FieldData.LiveValue;
			}

			const bool bIsActive = bNumeric && (SlotIndexInt == Session->CurrentNumericSlotIndex);

			FormattedFields.Add(HudWidget->FormatOneField(
				FieldData.Label,
				DataToFormat,
				Unit,
				bIsActive
			));
		}

		const FString FieldsString = FString::Join(FormattedFields, *Separator);

		HudString = FString::Printf(TEXT("%s %s"), *FieldsString, *Suffix).TrimEnd();
		HudWidget->Update(FText::FromString(HudString));
	}

	void FScaleTool::OnEnd(const bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
		if (HudWidget.IsValid())
		{
			HudWidget->SetDashState(false, FVector2D::ZeroVector, FVector2D::ZeroVector);
		}
	}

	void FScaleTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		const FTransform ObjectTransform = VirtualPivot->GetStartTransform();
		const FVector X = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

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
		switch (LockedAxis)
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
} // namespace BlenderControls
