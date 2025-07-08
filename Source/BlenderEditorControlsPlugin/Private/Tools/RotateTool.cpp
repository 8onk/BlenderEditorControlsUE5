#include "Tools/RotateTool.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
	FRotateTool::FRotateTool(EAxisLock InAxis)
		: FBlenderToolBase(ETransformMode::Rotate, InAxis, TEXT("Rotate"))
	{
	}

	void FRotateTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);

		const FVector Hit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDirection);
		StartDragVector = Hit - Pivot->GetStartTransform().GetLocation();
		LastDragVector = StartDragVector;
	}

	void FRotateTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);

		const FVector Hit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDirection);
		const FVector CurrentDragVector = Hit - Pivot->GetStartTransform().GetLocation();

		if (!CurrentDragVector.IsNearlyZero() && !CurrentDragVector.ContainsNaN())
		{
			const FVector RotationAxis = GrabContext.HelperAxisDir;

			const float DeltaAngleRad = MathHelper::GetSignedAngleOnAxis(
				LastDragVector, CurrentDragVector, RotationAxis);

			if (!FMath::IsNaN(DeltaAngleRad))
			{
				const FQuat DeltaRotation(RotationAxis, DeltaAngleRad);
				FTransform CurrentTransform = Pivot->GetTransformProxy()->GetTransform();

				CurrentTransform.ConcatenateRotation(DeltaRotation);
				Pivot->GetTransformProxy()->SetTransform(CurrentTransform);
			}

			LastDragVector = CurrentDragVector;
		}
	}

	void FRotateTool::ApplyNumeric(float Value)
	{
	}

	void FRotateTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FRotateTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
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

		case EAxisLock::All: GrabContext.HelperAxisDir = -GrabContext.ViewForward;
			break;
		}
	}
} // namespace BlenderControls
