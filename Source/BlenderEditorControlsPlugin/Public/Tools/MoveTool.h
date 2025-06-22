#pragma once

#include "BlenderToolBase.h"
#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "BlenderEditorControlsEnums.h"

namespace BlenderControls
{
    class FMoveTool : public FBlenderToolBase
    {
    public:
        FMoveTool(ETransformAxis InAxis);

        /* FBlenderToolBase */
        virtual void Tick(const FPointerEvent &MouseEvent) override;
        virtual void ApplyNumeric(float Value) override;

        virtual void OnBegin() override;
        virtual void OnEnd(bool bApply) override;

    protected:
        virtual void HandleDelta(const FVector2D &CurrentMousePos) override;

    private:
        /* Cached pivot + helpers */
        FVector LastIntersectionPoint = FVector::ZeroVector;
        FVector CurrentIntersectionPoint = FVector::ZeroVector;
        FViewport *Viewport = nullptr;
    };
} // namespace BlenderControls
