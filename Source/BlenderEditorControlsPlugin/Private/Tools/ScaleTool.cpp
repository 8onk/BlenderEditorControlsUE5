#include "Tools/ScaleTool.h"
#include "Kismet/KismetMathLibrary.h"

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

		StartPivotTransform = Pivot->GetStartTransform();
		PivotStartPosition = Pivot->GetStartTransform().GetLocation();

		SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);

		ScaleFactor = 1.0f;
		InitialMouseToPivotDistance = UKismetMathLibrary::Distance2D(CurrentMousePosition, PivotViewportPosition);
		StartScale = Pivot->GetStartTransform().GetScale3D();
		InitialMousePosition = CurrentMousePosition;
	}

	void FScaleTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		if (!GEditor)
		{
			return;
		}

		const FVector2D EffectiveMousePosition = InitialMousePosition + MouseDelta;
		CurrentMouseToPivotDistance = UKismetMathLibrary::Distance2D(EffectiveMousePosition, PivotViewportPosition);
		ScaleFactor = CurrentMouseToPivotDistance / InitialMouseToPivotDistance;
		NewScale = StartScale * ScaleFactor;

		if (bSnappingEnabled)
		{
			const float SnappingIncrement = GEditor->GetScaleGridSize();
			NewScale.X = FMath::GridSnap(NewScale.X, SnappingIncrement);
			NewScale.Y = FMath::GridSnap(NewScale.Y, SnappingIncrement);
			NewScale.Z = FMath::GridSnap(NewScale.Z, SnappingIncrement);
		}
		
		FTransform NewTransform = Pivot->GetStartTransform();
		NewTransform.SetScale3D(NewScale);

		Pivot->GetTransformProxy()->SetTransform(NewTransform);
	}

	void FScaleTool::ApplyNumeric(float Value)
	{
	}

	void FScaleTool::OnEnd(bool bApply)
	{
	}

	void FScaleTool::SetGrabContextAxisLock(EAxisLock AxisLock)
	{

		const FTransform ObjectTransform = Pivot->GetStartTransform();
		const FVector X = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

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
