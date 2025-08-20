#include "Tools/ScaleTool.h"
#include "LevelEditorViewport.h"
#include "Kismet/KismetMathLibrary.h"
#include "Tools/SharedPivot.h"

//NOTE gizmo automatically sets to local for scaling, since UE doesnt support global mode for scaling

namespace BlenderControls
{
	FScaleTool::FScaleTool(EAxisLock InAxis)
		: FBlenderToolBase(ETransformMode::Scale, InAxis, TEXT("Scale"))
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
	}

	void FScaleTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		if (!GEditor || !SceneView)
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
	}

	void FScaleTool::ApplyNumeric(float Value)
	{
		FBlenderToolBase::ApplyNumeric(Value);

		FVector ScaleMultiplier = FVector::OneVector;
		const float Slot1 = NumericInputSlots.X == 0 ? 1 : NumericInputSlots.X;
		const float Slot2 = NumericInputSlots.Y == 0 ? 1 : NumericInputSlots.Y;
		const float Slot3 = NumericInputSlots.Z == 0 ? 1 : NumericInputSlots.Z;

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
	}

	void FScaleTool::OnEnd(const bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FScaleTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
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
} // namespace BlenderControls
