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

		// 1. Calculate the 'correct-feeling' delta on the view-plane. This is correct.
		FVector UnconstrainedDelta = (ViewRight * MouseDelta.X * GrabContext.ScreenToWorldScale) +
			(-ViewUp * MouseDelta.Y * GrabContext.ScreenToWorldScale);

		// 2. Start with the assumption that our final delta is the unconstrained one.
		FVector FinalTotalDelta = UnconstrainedDelta;

		// 3. If constrained, recalculate the delta using the ray-intersection method.
		if (GrabContext.HelperType == FGrabContext::EHelperType::AxisPlane)
		{
			// Find the "ghost" position on the view-plane.
			const FVector GhostPos = Pivot->GetStartTransform().GetLocation() + UnconstrainedDelta;

			// Get the ray from the camera that passes through this ghost position.
			const FVector RayOrigin = SceneView->ViewLocation;
			const FVector RayDir = (GhostPos - RayOrigin).GetSafeNormal();

			// Intersect THAT ray with the actual constraint plane to find the final hit.
			const FVector FinalHit = BlenderControls::Math::IntersectHelper(GrabContext, RayOrigin, RayDir);

			// The correct delta is the vector from the start pivot to this final hit point.
			FinalTotalDelta = FinalHit - Pivot->GetStartTransform().GetLocation();
		}
		else if (GrabContext.HelperType == FGrabContext::EHelperType::AxisLine)
		{
			// A similar, more complex ray-line intersection would go here for perfect accuracy,
			// but for now, simple projection is a close approximation.
			FinalTotalDelta = BlenderControls::Math::ProjectVectorOntoAxis(
				UnconstrainedDelta, GrabContext.HelperAxisDir);
		}

		// The DeltaAnchor logic for precision mode needs to be handled correctly,
		// but this sets the main delta.
		// For now, let's assume no precision mode to keep it simple.
		FVector LiveDelta = GrabContext.DeltaAnchor + FinalTotalDelta;

		// Apply snapping to the final calculated delta.
		if (bSnappingEnabled)
		{
			LiveDelta = GetSnapOffset(LiveDelta);
		}

		GrabContext.TotalDelta = LiveDelta;

		// Set the final position.
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
