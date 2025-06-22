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

        auto *ViewportClient = static_cast<FLevelEditorViewportClient *>(GEditor->GetActiveViewport()->GetClient());
        if (!ViewportClient)
            return;

        FSceneViewFamilyContext ViewFamily(
            FSceneViewFamily::ConstructionValues(
                ViewportClient->Viewport,
                ViewportClient->GetScene(),
                ViewportClient->EngineShowFlags));

        FSceneView *View = ViewportClient->CalcSceneView(&ViewFamily);
        FVector2D StartMousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GEditor->GetEditorWorldContext().World());
        UE_LOG(LogBlenderEditorControls, Log, TEXT("Mouse Position - X: %f, Y: %f"), StartMousePos.X, StartMousePos.Y);
        FVector WorldOrigin, WorldDirection;
        View->DeprojectFVector2D(StartMousePos, WorldOrigin, WorldDirection);

        float TraceDistance = BIG_NUMBER;
        FVector Intersection = FMath::LinePlaneIntersection(WorldOrigin,
                                                            WorldOrigin + (WorldDirection * BIG_NUMBER),
                                                            Group->GetPivot().GetLocation(),
                                                            -ViewportClient->GetViewRotation().Vector());
    }

    void FMoveTool::Tick(const FPointerEvent &MouseEvent)
    {
        const FVector2D CurrPos = MouseEvent.GetScreenSpacePosition();
        static FVector2D LastPos = CurrPos;
        FVector2D MouseDelta = CurrPos - LastPos;
        LastPos = CurrPos;
        HandleDelta(MouseDelta);
    }

    void FMoveTool::ApplyNumeric(float Value)
    {
    }


    void FMoveTool::OnEnd(bool bApply)
    {
        FBlenderToolBase::OnEnd(bApply);
    }

    void FMoveTool::HandleDelta(const FVector2D &MouseDelta)
    {
        auto *VC = GEditor->GetActiveViewport()->GetClient();

        if (!VC)
        {
            return;
        }

        FEditorViewportClient *ViewClient = static_cast<FEditorViewportClient *>(VC);

        if (!ViewClient)
        {
            return;
        }

        if (!Group)
        {
            return;
        }
        /*
        float Depth = FVector::Dist(Group->GetPivot().GetLocation(), ViewClient->GetViewLocation());
        const FVector DeltaWS = Math::ScreenDeltaToWorld(MouseDelta, Depth);
        Group->MoveBy(DeltaWS);
        */
    }
} // namespace BlenderControls|
