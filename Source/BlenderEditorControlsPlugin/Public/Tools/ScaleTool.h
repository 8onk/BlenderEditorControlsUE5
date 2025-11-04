#pragma once
#include "Tools/ToolBase.h"

namespace BlenderControls
{
	/**
	 * Scaling tool – supports uniform and axis-constrained scaling,
	 * precision mode (Shift), numeric entry, and min-scale clamping.
	 */
	class FScaleTool final : public FToolBase
	{
	public:
		explicit FScaleTool(const TSharedRef<FTransformSession>& InSession);

		/* ---------- FToolBase overrides ---------- */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value) override;
		virtual void UpdateHud() override;
		virtual void OnBegin() override;
		virtual void OnEnd(bool bApply) override;
		virtual void HandleMouseMovement(const FVector2D& CurrentViewportMousePosition) override;

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
		float ScaleFactor = 1.0f;

		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;

	protected:
		virtual void UpdateToolSettingsForAxisLock() override;
		virtual FText GetNumericHudText() const override;
		virtual FText GetLiveHudText() const override;
		FText BuildFreeformHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const;
		FText BuildSingleAxisHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const;
		FText BuildDualAxisHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const;

		/* ——— state ——— */
		FVector PivotWS = FVector::ZeroVector; // average loc
		float StartCursorDistance = 1.f; // pixels
		float CurrentScalar = 1.f;
		float MinAllowedScale = 0.001f; // safety clamp
	};
} // namespace BlenderControls
