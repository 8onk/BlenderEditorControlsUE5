#include "Tools/BlenderToolBase.h"

namespace BlenderControls
{
    FBlenderToolBase::FBlenderToolBase(ETransformMode InMode, ETransformAxis InAxis, const FString &InDisplayName)
        : Mode(InMode), Axis(InAxis), DisplayName(InDisplayName)
    {
    }

    FBlenderToolBase::~FBlenderToolBase()
    {
        // Base cleanup logic if needed
    }

    void FBlenderToolBase::Tick(const FVector2D &MouseDelta)
    {
        // Base implementation does nothing
    }

    void FBlenderToolBase::Accept()
    {
        // Base implementation does nothing
    }

    void FBlenderToolBase::Cancel()
    {
        // Base implementation does nothing
    }

    void FBlenderToolBase::ApplyNumeric(float Value)
    {
        // Base implementation does nothing
    }
} // namespace BlenderControls