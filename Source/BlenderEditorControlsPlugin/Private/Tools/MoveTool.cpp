#include "Tools/MoveTool.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
	FMoveTool::FMoveTool(EAxisLock InAxis)
		: FBlenderToolBase(ETransformMode::Translate, InAxis, TEXT("Move"))
	{
	}

	void FMoveTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();
	}

	void FMoveTool::OnActive(const FVector2D &CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		FVector LiveDelta;
		if (bPrecisionModeActive)
		{
			const FVector FineDelta = (CurrentHit - GrabContext.ShiftStartHit) * CurrentPrecisionFactor;
			LiveDelta = GrabContext.DeltaAnchor + FineDelta;
		}
		else
		{
			const FVector Unscaled = (CurrentHit - GrabContext.StartHit);
			LiveDelta = GrabContext.DeltaAnchor + Unscaled;
		}

		if (bSnappingEnabled)
		{
			LiveDelta = GetSnapOffset(LiveDelta);
		}
		GrabContext.TotalDelta = LiveDelta;

		const FVector NewPos = Pivot->GetStartTransform().GetLocation() + GrabContext.TotalDelta;

		UE_LOG(LogBlenderEditorControls, Log, TEXT("GrabContext State:"));
		UE_LOG(LogBlenderEditorControls, Log, TEXT("  DeltaAnchor: %s"), *GrabContext.DeltaAnchor.ToString());
		UE_LOG(LogBlenderEditorControls, Log, TEXT("  StartHit: %s"), *GrabContext.StartHit.ToString());
		UE_LOG(LogBlenderEditorControls, Log, TEXT("  CurrentHit: %s"), *CurrentHit.ToString());
		UE_LOG(LogBlenderEditorControls, Log, TEXT("  PivotPos: %s"), *Pivot->GetStartTransform().GetLocation().ToString());

		Pivot->SetPosition(NewPos);
	}

	void FMoveTool::ApplyNumeric(float Value)
	{
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}
}
