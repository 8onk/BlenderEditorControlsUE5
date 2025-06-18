#include "Tools/RotateTool.h"

namespace BlenderControls
{
    FRotateTool::FRotateTool(ETransformAxis InAxis)
        : FBlenderToolBase(ETransformMode::Rotate, InAxis, TEXT("Rotate"))
    {
    }

    void FRotateTool::Tick(const FVector2D &MouseDelta) {}
    void FRotateTool::ApplyNumeric(float Value) {}
    void FRotateTool::OnBegin() {}
    void FRotateTool::OnEnd(bool bApply) {}
    void FRotateTool::HandleDelta(const FVector2D &MouseDelta) {}
} // namespace BlenderControls