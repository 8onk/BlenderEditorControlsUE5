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
		const FVector PlaneOrigin = Group->GetPivot().GetLocation();
		const FVector PlaneNormal = -SceneView->ViewRotation.Vector();

		DragPlane = FPlane(PlaneOrigin, PlaneNormal);

		LastIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                     WorldOrigin + (WorldDirection *
			                                                     BIG_NUMBER),
		                                                     DragPlane);
	}

	void FMoveTool::Tick(const FPointerEvent& MouseEvent)
	{
		if (!Viewport)
		{
			return;
		}
		FIntPoint CurrentMousePosInt;
		Viewport->GetMousePos(CurrentMousePosInt);
		FVector2D CurrentMousePos = FVector2D(CurrentMousePosInt);

		HandleDelta(CurrentMousePos);
	}

	void FMoveTool::ApplyNumeric(float Value)
	{
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FMoveTool::HandleDelta(const FVector2D& CurrentMousePos)
	{
		auto* ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (!ViewportClient || !Group || !SceneView)
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
		SceneView->DeprojectFVector2D(CurrentMousePos, WorldOrigin, WorldDirection);
		CurrentIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                        WorldOrigin + (WorldDirection * BIG_NUMBER), DragPlane);

		FVector Delta = CurrentIntersectionPoint - LastIntersectionPoint;
		Group->MoveBy(Delta);
		LastIntersectionPoint = CurrentIntersectionPoint;
		UE_LOG(LogTemp, Log, TEXT("World Delta: %s"), *Delta.ToString());
		UE_LOG(LogTemp, Log, TEXT("Current Intersection: %s"), *CurrentIntersectionPoint.ToString());
		UE_LOG(LogTemp, Log, TEXT("Last Intersection: %s"), *LastIntersectionPoint.ToString());
	}
}
