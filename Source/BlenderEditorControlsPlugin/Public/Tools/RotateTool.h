#pragma once
#include "Tools/BlenderToolBase.h"
#include "Math/Quat.h"
#include "Input/BlenderEditorControlsPluginInputProcessor.h"

namespace BlenderControls
{
    /**
     * Rotation tool – spawned when the input-processor enters ETransformMode::Rotate.
     * Handles axis-locked rotation, track-ball rotation, numeric entry, and
     * angle-snap (e.g. Ctrl for 5° increments).
     */
    class FRotateTool : public FBlenderToolBase
    {
    public:
        /** Axis = All means track-ball by default, overridden by SetAxis() calls. */
        explicit FRotateTool(ETransformAxis InAxis = ETransformAxis::All);

        /* ---------- FBlenderToolBase overrides ---------- */
        virtual void Tick(const FVector2D &MouseDelta) override;
        virtual void ApplyNumeric(float Value) override;
        virtual void OnBegin() override;
        virtual void OnEnd(bool bApply) override;

    protected:
        virtual void HandleDelta(const FVector2D &MouseDelta) override;

    private:
        /* ——— helpers ——— */
        FVector ScreenToWorldVector(const FVector2D &ScreenPos) const;
        void UpdateTrackball(const FVector2D &MouseDelta);
        FQuat BuildAxisQuat(float Radians) const;

        /* ——— state ——— */
        FVector PivotWS = FVector::ZeroVector;      // cached centre
        FVector LastVectorWS = FVector::ZeroVector; // for track-ball
        bool bTrackballMode = false;
        float AngleSnapIncrementDeg = 5.f;
        float AngleAccumulatorDeg = 0.f; // used when snapping
    };

} // namespace BlenderControls
