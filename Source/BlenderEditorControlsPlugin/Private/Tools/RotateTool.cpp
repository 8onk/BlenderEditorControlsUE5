#include "Tools/RotateTool.h"

namespace BlenderControls
{
    FRotateTool::FRotateTool(ETransformAxis InAxis)
        : FBlenderToolBase(ETransformMode::Rotate, InAxis, TEXT("Rotate"))
    {
    }

    void FRotateTool::OnActive(const FVector2D &CurrentViewportMousePosition) {}
    void FRotateTool::ApplyNumeric(float Value) {}
    void FRotateTool::OnBegin() {}
    void FRotateTool::OnEnd(bool bApply) {}
} // namespace BlenderControls