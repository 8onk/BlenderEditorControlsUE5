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

	void FMoveTool::SetGrabContextAxisLock(EAxisLock AxisLock)
	{
		if (!Pivot)
		{
			return;
		}
		const FTransform ObjectTransform = Pivot->GetStartTransform();
		const FVector X = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

		// const FVector RotationEuler = ObjectTransform.GetRotation().Rotator().Euler();
		// const FVector Scale = ObjectTransform.GetScale3D();

		// UE_LOG(LogTemp, Warning, TEXT("Rotation (Euler): X=%.2f, Y=%.2f, Z=%.2f"),
		//        RotationEuler.X, RotationEuler.Y, RotationEuler.Z);
		//
		// UE_LOG(LogTemp, Warning, TEXT("Scale: X=%.2f, Y=%.2f, Z=%.2f"),
		//        Scale.X, Scale.Y, Scale.Z);


		switch (AxisLock)
		{
		case EAxisLock::All:
			GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
			GrabContext.HelperPlaneN = -GrabContext.ViewForward;
			break;

		case EAxisLock::X:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = X;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Y, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Y:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Y;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(Y, X, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Z:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Z;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(Z, X, Y, GrabContext.ViewForward);
			break;

		case EAxisLock::XY:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Z;
			break;

		case EAxisLock::XZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Y;
			break;

		case EAxisLock::YZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = X;
			break;
		}
	}

	FVector FMoveTool::GetSnapOffset(const FVector OffsetFromStart)
	{
		if (!GEditor)
		{
			return FVector::ZeroVector;
		}

		float GridSize = GEditor->GetGridSize();
		FVector SnapOffset = OffsetFromStart / GridSize;
		SnapOffset = MathHelper::RoundVectorToInt(SnapOffset);
		SnapOffset *= GridSize;

		return SnapOffset;
	}
}
