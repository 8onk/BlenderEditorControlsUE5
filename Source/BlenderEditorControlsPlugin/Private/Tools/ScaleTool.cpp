#include "Tools/ScaleTool.h"
#include "LevelEditorViewport.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/BlenderMathHelpers.h"

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

		CurrentMouseToPivotDistance = UKismetMathLibrary::Distance2D(VirtualMousePosition, PivotViewportPosition);

		if (InitialMouseToPivotDistance > KINDA_SMALL_NUMBER)
		{
			ScaleFactor = CurrentMouseToPivotDistance / InitialMouseToPivotDistance;
		}

		FTransform CurrentTransform = VirtualPivot->GetStartTransform();
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
		default: // EAxisLock::All
			FinalScaleMultiplier = FVector(ScaleFactor);
			break;
		}

		if (bUsingLocalSpace || LockedAxis == EAxisLock::All)
		{
			FVector NewLocalScale = StartScale * FinalScaleMultiplier;

			if (bSnappingEnabled)
			{
				const float SnappingIncrement = GEditor->GetScaleGridSize();
				NewLocalScale.X = FMath::GridSnap(NewLocalScale.X, SnappingIncrement);
				NewLocalScale.Y = FMath::GridSnap(NewLocalScale.Y, SnappingIncrement);
				NewLocalScale.Z = FMath::GridSnap(NewLocalScale.Z, SnappingIncrement);
			}

			CurrentTransform.SetScale3D(NewLocalScale);
		}
		else
		{
			const FQuat InitialRotation = CurrentTransform.GetRotation();
			const FRotator Rot = InitialRotation.Rotator();
			const FMatrix RotationMatrix = FRotationMatrix(Rot);

			const FMatrix GlobalScaleMatrix = FScaleMatrix(FinalScaleMultiplier);
			const FMatrix LocalEquivalentMatrix = RotationMatrix.Inverse() * GlobalScaleMatrix * RotationMatrix;
			const FVector LocalScaleToAdd = LocalEquivalentMatrix.GetScaleVector();
			const FVector NewScale = StartScale * LocalScaleToAdd;

			CurrentTransform.SetScale3D(NewScale);
		}

		VirtualPivot->GetTransformProxy()->SetTransform(CurrentTransform);
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
