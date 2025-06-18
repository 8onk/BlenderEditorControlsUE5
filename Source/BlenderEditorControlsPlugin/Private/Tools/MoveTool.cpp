#include "Tools/MoveTool.h"

namespace BlenderControls
{
    FMoveTool::FMoveTool(ETransformAxis InAxis)
        : FBlenderToolBase(ETransformMode::Translate, InAxis, TEXT("Move"))
    {
    }

    void FMoveTool::Tick(const FVector2D &MouseDelta) {}
    void FMoveTool::ApplyNumeric(float Value) {}
    void FMoveTool::OnBegin()
    {
        FBlenderToolBase::OnBegin();
    }
    void FMoveTool::OnEnd(bool bApply) {}
    void FMoveTool::HandleDelta(const FVector2D &MouseDelta) {}
} // namespace BlenderControls