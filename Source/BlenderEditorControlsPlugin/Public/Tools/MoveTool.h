#pragma once

#include "BlenderToolBase.h"
#include "BlenderEditorControlsEnums.h"

namespace BlenderControls
{
    class FMoveTool : public FBlenderToolBase
    {
    public:
        FMoveTool(TSharedPtr<FTransformSession> InSession, EAxisLock InAxis);

        /* FBlenderToolBase */
        virtual void OnActive(const FVector2D &CurrentViewportMousePosition) override;
        virtual void ApplyNumeric(float Value) override;
        virtual void UpdateHud() override;

        virtual void OnBegin() override;
        virtual void OnEnd(bool bApply) override;

    private:
        virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;
        virtual FVector GetSnapOffset(const FVector OffsetFromStart) override;

        FVector StartActiveLocation;
        FVector StartVirtualPivotLocation;
        FVector ActiveToPivotOffset;
    };
} // namespace BlenderControls
