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
        virtual void OnActive(const FVector2D &CurrentViewportMousePosition) override;
        virtual void ApplyNumeric(float Value) override;

        virtual void OnBegin() override;
        virtual void OnEnd(bool bApply) override;

    protected:
        /* Cached pivot + helpers */
        FVector LastIntersectionPoint = FVector::ZeroVector;
        FVector CurrentIntersectionPoint = FVector::ZeroVector;
        FViewport *Viewport = nullptr;
    };
} // namespace BlenderControls
