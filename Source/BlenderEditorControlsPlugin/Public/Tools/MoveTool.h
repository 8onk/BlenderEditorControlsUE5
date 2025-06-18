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
        virtual void Tick(const FVector2D &MouseDelta) override;
        virtual void ApplyNumeric(float Value) override;

    protected:
        virtual void OnBegin() override;
        virtual void OnEnd(bool bApply) override;
        virtual void HandleDelta(const FVector2D &MouseDelta) override;

    private:
        /* Cached pivot + helpers */
        TSharedPtr<class FGroupTransform> Group;
        FVector PrevIntersection = FVector::ZeroVector;
    };
} // namespace BlenderControls
