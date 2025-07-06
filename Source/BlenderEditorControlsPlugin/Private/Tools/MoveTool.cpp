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

	void FMoveTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		constexpr float PARALLEL_COS = 0.990f;
		float CosAngle = SMALL_NUMBER;
		//If HelperAxisDir is 0 then we aren't in single axis lock. 
		if (!GrabContext.HelperAxisDir.IsNearlyZero())
		{
			CosAngle = FMath::Abs(FVector::DotProduct(ViewForward.GetSafeNormal(),
			                                          GrabContext.HelperAxisDir.GetSafeNormal()));
		}

		UE_LOG(LogTemp, Log, TEXT("CosAngle: %f"), CosAngle);
		FVector FinalTotalDelta;
		if (CosAngle <= PARALLEL_COS)
		{
			const FVector UnconstrainedDelta = (ViewRight * MouseDelta.X * GrabContext.ScreenToWorldScale) +
				(-ViewUp * MouseDelta.Y * GrabContext.ScreenToWorldScale);

			//Intersect ghost pos to get the blender "feel", NOT the current mouse pos. 
			const FVector GhostPos = Pivot->GetStartTransform().GetLocation() + UnconstrainedDelta;
			const FVector RayOrigin = ViewLocation;
			const FVector RayDir = (GhostPos - RayOrigin).GetSafeNormal();
			const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
			FinalTotalDelta = FinalHit - Pivot->GetStartTransform().GetLocation();
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Fallback!"));
			const FVector2D ScreenUpVector(0.0f, -1.0f);
			const float ScreenSpaceFactor = FVector2D::DotProduct(MouseDelta, ScreenUpVector);
			const float ScaledFactor = FMath::Sign(ScreenSpaceFactor) * FMath::Square(ScreenSpaceFactor) * 0.1f;

			FinalTotalDelta = GrabContext.HelperAxisDir * ScaledFactor;
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
