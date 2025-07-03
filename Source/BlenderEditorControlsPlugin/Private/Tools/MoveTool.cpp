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

		FVector UnconstrainedDelta = (ViewRight * MouseDelta.X * GrabContext.ScreenToWorldScale) +
			(-ViewUp * MouseDelta.Y * GrabContext.ScreenToWorldScale);

		FVector FinalTotalDelta = UnconstrainedDelta;

		if (GrabContext.HelperType == FGrabContext::EHelperType::AxisPlane)
		{
			//Intersect ghost pos to get the blender "feel", NOT the current mouse pos. 
			const FVector GhostPos = Pivot->GetStartTransform().GetLocation() + UnconstrainedDelta;

			const FVector RayOrigin = SceneView->ViewLocation;
			const FVector RayDir = (GhostPos - RayOrigin).GetSafeNormal();

			const FVector FinalHit = BlenderControls::Math::IntersectHelper(GrabContext, RayOrigin, RayDir);
			FinalTotalDelta = FinalHit - Pivot->GetStartTransform().GetLocation();
		}
		else if (GrabContext.HelperType == FGrabContext::EHelperType::AxisLine)
		{
			FinalTotalDelta = BlenderControls::Math::ProjectVectorOntoAxis(
				UnconstrainedDelta, GrabContext.HelperAxisDir);
		}

		FVector LiveDelta = FinalTotalDelta;

		if (bSnappingEnabled)
		{
			LiveDelta = GetSnapOffset(LiveDelta);
		}
		GrabContext.TotalDelta = LiveDelta;

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
