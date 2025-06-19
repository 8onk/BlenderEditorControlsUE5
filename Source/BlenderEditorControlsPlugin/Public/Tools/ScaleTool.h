#pragma once
#include "Tools/BlenderToolBase.h"
#include "Input/BlenderEditorControlsPluginInputProcessor.h"

namespace BlenderControls
{

    /**
     * Scaling tool – supports uniform and axis-constrained scaling,
     * precision mode (Shift), numeric entry, and min-scale clamping.
     */
    class FScaleTool : public FBlenderToolBase
    {
    public:
        explicit FScaleTool(ETransformAxis InAxis = ETransformAxis::All);

        /* ---------- FBlenderToolBase overrides ---------- */
        virtual void Tick(const FVector2D &MouseDelta) override;
        virtual void ApplyNumeric(float Value) override;

    protected:
        virtual void OnBegin() override;
        virtual void OnEnd(bool bApply) override;
        virtual void HandleDelta(const FVector2D &MouseDelta) override;

    private:
        /* ——— helpers ——— */
        float ComputeScaleDelta(const FVector2D &MouseDelta) const;
        FVector BuildScaleVector(float Scalar) const;

        /* ——— state ——— */
        FVector PivotWS = FVector::ZeroVector; // average loc
        float StartCursorDistance = 1.f;       // pixels
        float CurrentScalar = 1.f;
        float MinAllowedScale = 0.001f; // safety clamp
    };

} // namespace BlenderControls
