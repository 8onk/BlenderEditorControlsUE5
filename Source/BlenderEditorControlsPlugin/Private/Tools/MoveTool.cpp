#include "Tools/MoveTool.h"
#include "Utils/BlenderMathHelpers.h"
#include "BlenderEditorControlsPlugin.h"
#include "LevelEditorViewport.h"
#include "Blueprint/WidgetLayoutLibrary.h"

namespace BlenderControls
{
	FMoveTool::FMoveTool(ETransformAxis InAxis)
		: FBlenderToolBase(ETransformMode::Translate, InAxis, TEXT("Move"))
	{
	}

	void FMoveTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();

		auto* ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (!ViewportClient)
			return;

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		if (!SceneView)
		{
			return;
		}

		Viewport = ViewportClient->Viewport;
		if (!Viewport)
		{
			return;
		}
		FIntPoint MousePosInt;
		Viewport->GetMousePos(MousePosInt);
		FVector2D MousePos = FVector2D(MousePosInt);

		FVector WorldOrigin, WorldDirection;
		SceneView->DeprojectFVector2D(MousePos, WorldOrigin, WorldDirection);
		const FVector PlaneOrigin = Group->GetStartLocation().GetLocation();
		const FVector PlaneNormal = -SceneView->ViewRotation.Vector();

		DragPlane = FPlane(PlaneOrigin, PlaneNormal);

		PreviousIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                         WorldOrigin + (WorldDirection *
			                                                         BIG_NUMBER),
		                                                         DragPlane);
	}

	void FMoveTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		if (!Viewport)
		{
			return;
		}
		const FIntPoint CurrentMousePosInt = FIntPoint(CurrentViewportMousePosition.X, CurrentViewportMousePosition.Y);
		const FVector2D CurrentMousePos = FVector2D(CurrentMousePosInt);

		auto* ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (!ViewportClient || !Group)
		{
			return;
		}

		FVector WorldOrigin, WorldDirection;
		FSceneViewFamilyContext TempViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		SceneView = ViewportClient->CalcSceneView(&TempViewFamily);
		if (SceneView)
		{
			SceneView->DeprojectFVector2D(CurrentMousePos, WorldOrigin, WorldDirection);
		}

		CurrentIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                        WorldOrigin + (WorldDirection * BIG_NUMBER), DragPlane);
		const FVector Delta = bPrecisionModeActive
			                      ? (CurrentIntersectionPoint - PreviousIntersectionPoint) * PrecisionFactor
			                      : CurrentIntersectionPoint - PreviousIntersectionPoint;
		Group->MoveBy(Delta);
		PreviousIntersectionPoint = CurrentIntersectionPoint;
	}

	void FMoveTool::ApplyNumeric(float Value)
	{
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}
}
