#include "Tools/ScaleTool.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/BlenderMathHelpers.h"
//TODO SCALES CORRECTLY WITH MULTIPLE OBJECTS, BUT SINGLE OBJECT SELECTION ALWAYS STARTS FROM 1, 1, 1
//When mouse wraps around, zooming into the object doesn't scale it down to 0 fully. 
//GLOBAL WORLD SCALING DOESN'T WORK, ONLY LOCAL

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
	}

	void FScaleTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		if (!GEditor || !SceneView)
		{
			return;
		}

		const FVector2D EffectiveMousePosition = InitialMousePosition + MouseDelta;
		CurrentMouseToPivotDistance = UKismetMathLibrary::Distance2D(EffectiveMousePosition, PivotViewportPosition);

		if (InitialMouseToPivotDistance > KINDA_SMALL_NUMBER)
		{
			ScaleFactor = CurrentMouseToPivotDistance / InitialMouseToPivotDistance;
		}
		
		FVector FinalScaleVector = StartScale; //Default to no scaling
		switch (LockedAxis)
		{
		case EAxisLock::X:
			FinalScaleVector.X = ScaleFactor;
			break;
		case EAxisLock::Y:
			FinalScaleVector.Y = ScaleFactor;
			break;
		case EAxisLock::Z:
			FinalScaleVector.Z = ScaleFactor;
			break;
		case EAxisLock::XY:
			FinalScaleVector.X = ScaleFactor;
			FinalScaleVector.Y = ScaleFactor;
			break;
		case EAxisLock::XZ:
			FinalScaleVector.X = ScaleFactor;
			FinalScaleVector.Z = ScaleFactor;
			break;
		case EAxisLock::YZ:
			FinalScaleVector.Y = ScaleFactor;
			FinalScaleVector.Z = ScaleFactor;
			break;
		default:
			FinalScaleVector = FVector(ScaleFactor, ScaleFactor, ScaleFactor);
			break;
		}
		NewScale = StartScale * FinalScaleVector;

		if (bSnappingEnabled)
		{
			const float SnappingIncrement = GEditor->GetScaleGridSize();
			NewScale.X = FMath::GridSnap(NewScale.X, SnappingIncrement);
			NewScale.Y = FMath::GridSnap(NewScale.Y, SnappingIncrement);
			NewScale.Z = FMath::GridSnap(NewScale.Z, SnappingIncrement);
		}

		FTransform NewTransform = VirtualPivot->GetStartTransform();
		NewTransform.SetScale3D(NewScale);
		VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
	}

	void FScaleTool::ApplyNumeric(float Value)
	{
	}

	void FScaleTool::OnEnd(const bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FScaleTool::SetGrabContextAxisLock(EAxisLock AxisLock)
	{
		const FTransform ObjectTransform = VirtualPivot->GetStartTransform();
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
