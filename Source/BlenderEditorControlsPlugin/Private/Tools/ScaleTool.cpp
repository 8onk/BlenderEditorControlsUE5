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

		// 1. Calculate the mouse's displacement from its starting point
		const FVector2D MouseDeltaFromStart = CurrentMousePosition - InitialMousePosition;

		// 2. Scale this displacement by the precision factor
		const FVector2D ScaledDelta = MouseDeltaFromStart * CurrentPrecisionFactor;

		// 3. Calculate an "effective" mouse position
		const FVector2D EffectiveMousePosition = InitialMousePosition + ScaledDelta;

		// 4. Use this new effective position for your distance calculation
		CurrentMouseToPivotDistance = UKismetMathLibrary::Distance2D(EffectiveMousePosition, PivotViewportPosition);

		CurrentMouseToPivotDistance = UKismetMathLibrary::Distance2D(CurrentMousePosition, PivotViewportPosition);
		ScaleFactor = CurrentMouseToPivotDistance / InitialMouseToPivotDistance;
		NewScale = StartScale * ScaleFactor;

		FTransform NewTransform = Pivot->GetStartTransform();
		NewTransform.SetScale3D(NewScale);

		// Then do something with NewTransform, e.g. apply it
		Pivot->GetTransformProxy()->SetTransform(NewTransform);
	}

	void FScaleTool::ApplyNumeric(float Value)
	{
	}

	void FScaleTool::OnEnd(bool bApply)
	{
	}
} // namespace BlenderControls
