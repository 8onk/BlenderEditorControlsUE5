#include "Tools/MoveTool.h"

#include "LevelEditorViewport.h"
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

	void FMoveTool::OnActive(const FVector2D& CurrentViewportMousePosition)
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

		const float PixelDelta = (CurrentViewportMousePosition - CurrentViewportMousePos).Size();
		// or from OnBegin mouse pos
		const float WorldDelta = LiveDelta.Size();

		const float GainRatio = PixelDelta > KINDA_SMALL_NUMBER ? (WorldDelta / PixelDelta) : 0.0f;
		const float ViewPlaneAngleCos = FVector::DotProduct(ViewportClient->GetViewRotation().Vector(),
		                                                    GrabContext.HelperPlaneN);


		const FVector NewPos = Pivot->GetStartTransform().GetLocation() + GrabContext.TotalDelta;

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
