#include "Tools/ScaleTool.h"
#include "LevelEditorViewport.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/BlenderMathHelpers.h"

//NOTE gizmo automatically sets to local for scaling, since UE doesnt support global mode for scaling
//TODO snapping and precision mode now doesn't work, it did previously?

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

		for (FChildInfo Child : VirtualPivot->GetChildren())
		{
			const FTransform ActorInitialTransform = Child.Transform;
			const FQuat ActorRotation = ActorInitialTransform.GetRotation();
			const FVector PivotToActorVec = ActorInitialTransform.GetLocation() - VirtualPivot->GetLocation();
			FTransform NewTransform = ActorInitialTransform;

			FVector SnappedScaleMultiplier = FinalScaleMultiplier;
			if (bSnappingEnabled)
			{
				const float SnappingIncrement = GEditor->GetScaleGridSize();
				SnappedScaleMultiplier.X = FMath::GridSnap(FinalScaleMultiplier.X, SnappingIncrement);
				SnappedScaleMultiplier.Y = FMath::GridSnap(FinalScaleMultiplier.Y, SnappingIncrement);
				SnappedScaleMultiplier.Z = FMath::GridSnap(FinalScaleMultiplier.Z, SnappingIncrement);
			}

			FVector NewPosition, NewScale;
			if (bUsingLocalSpace)
			{
				const FVector LocalPivotToActorVec = ActorRotation.UnrotateVector(PivotToActorVec);
				const FVector ScaledLocalPivotToActorVec = LocalPivotToActorVec * SnappedScaleMultiplier;
				const FVector GlobalPivotToActorVec = ActorRotation.RotateVector(ScaledLocalPivotToActorVec);
				NewPosition = VirtualPivot->GetLocation() + GlobalPivotToActorVec;

				NewScale = ActorInitialTransform.GetScale3D() * SnappedScaleMultiplier;
			}
			else
			{
				const FQuat InitialRotation = ActorInitialTransform.GetRotation();
				const FMatrix RotationMatrix = FRotationMatrix(InitialRotation.Rotator());

				const FMatrix GlobalScaleMatrix = FScaleMatrix(SnappedScaleMultiplier);

				const FMatrix LocalEquivalentMatrix = RotationMatrix.Inverse() * GlobalScaleMatrix * RotationMatrix;
				const FVector LocalScaleToAdd = LocalEquivalentMatrix.GetScaleVector();

				FVector LocalScaleToAddSigned = LocalScaleToAdd;
				if (SnappedScaleMultiplier.X < 0) LocalScaleToAddSigned.X *= -1.f;
				if (SnappedScaleMultiplier.Y < 0) LocalScaleToAddSigned.Y *= -1.f;
				if (SnappedScaleMultiplier.Z < 0) LocalScaleToAddSigned.Z *= -1.f;

				NewScale = ActorInitialTransform.GetScale3D() * LocalScaleToAddSigned;

				const FVector ScaledRelativePosition = PivotToActorVec * SnappedScaleMultiplier;
				NewPosition = VirtualPivot->GetLocation() + ScaledRelativePosition;
			}

			NewTransform.SetLocation(NewPosition);
			NewTransform.SetScale3D(NewScale);
			
			Child.Actor->SetActorTransform(NewTransform);
		}
	}

	void FScaleTool::ApplyNumeric(float Value)
	{
		FBlenderToolBase::ApplyNumeric(Value);
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
