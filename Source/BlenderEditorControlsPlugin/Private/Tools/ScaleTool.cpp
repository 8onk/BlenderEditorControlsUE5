#include "Tools/ScaleTool.h"

namespace BlenderControls
{
    FScaleTool::FScaleTool(EAxisLock InAxis)
        : FBlenderToolBase(ETransformMode::Scale, InAxis, TEXT("Scale"))
    {
    }

    void FScaleTool::OnActive(const FVector2D &CurrentViewportMousePosition) {}
    void FScaleTool::ApplyNumeric(float Value) {}
    void FScaleTool::OnBegin() {}
    void FScaleTool::OnEnd(bool bApply) {}
} // namespace BlenderControls