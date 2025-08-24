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
		explicit FScaleTool(TSharedPtr<FTransformSession> InSession, EAxisLock InAxis = EAxisLock::All);

		/* ---------- FBlenderToolBase overrides ---------- */
		virtual void OnActive(const FVector2D &CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(float Value) override;
		virtual void UpdateHud() override;
		virtual void OnBegin() override;
		virtual void OnEnd(bool bApply) override;

	private:
		FTransform StartPivotTransform;
		FVector PivotStartPosition;
		FVector2D PivotViewportPosition;
		FVector StartHit;
		FVector StartScale;
		FVector2D InitialMousePosition;
		FVector AxisLockProjectionVector;
		float InitialMouseToPivotDistance;
		float LastMouseToPivotDistance;
		float CurrentMouseToPivotDistance;
		float ScaleFactor;

		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;

	protected:
		/* ——— helpers ——— */
		float ComputeScaleDelta(const FVector2D &MouseDelta) const;
		FVector BuildScaleVector(float Scalar) const;

		/* ——— state ——— */
		FVector PivotWS = FVector::ZeroVector; // average loc
		float StartCursorDistance = 1.f;	   // pixels
		float CurrentScalar = 1.f;
		float MinAllowedScale = 0.001f; // safety clamp
	};
} // namespace BlenderControls
