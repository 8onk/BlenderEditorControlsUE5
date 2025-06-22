#include "Tools/MoveTool.h"
#include "Utils/BlenderMathHelpers.h"
#include "BlenderEditorControlsPlugin.h"

namespace BlenderControls
{
    FMoveTool::FMoveTool(ETransformAxis InAxis)
        : FBlenderToolBase(ETransformMode::Translate, InAxis, TEXT("Move"))
    {
    }

    void FMoveTool::Tick(const FVector2D &MouseDelta)
    {
        HandleDelta(MouseDelta);
    }

    void FMoveTool::ApplyNumeric(float Value) {}

    void FMoveTool::OnBegin()
    {
        FBlenderToolBase::OnBegin();
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
        float Depth = FVector::Dist(Group->GetPivot().GetLocation(), ViewClient->GetViewLocation());
        const FVector DeltaWS = Math::ScreenDeltaToWorld(MouseDelta, Depth);
        Group->MoveBy(DeltaWS);
    }
} // namespace BlenderControls|