#include "Tools/ScaleTool.h"

namespace BlenderControls
{
    FScaleTool::FScaleTool(ETransformAxis InAxis)
        : FBlenderToolBase(ETransformMode::Scale, InAxis, TEXT("Scale"))
    {
    }

    void FScaleTool::Tick(const FVector2D &MouseDelta) { }
    void FScaleTool::ApplyNumeric(float Value) { }
    void FScaleTool::OnBegin() { }
    void FScaleTool::OnEnd(bool bApply) { }
    void FScaleTool::HandleDelta(const FVector2D &MouseDelta) { }
} // namespace BlenderControls