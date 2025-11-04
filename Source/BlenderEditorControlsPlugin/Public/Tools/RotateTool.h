#pragma once
#include "Tools/ToolBase.h"
#include "Math/Quat.h"

namespace BlenderControls
{
	/**
	 * Rotation tool – spawned when the input-processor enters ETransformMode::Rotate.
	 * Handles axis-locked rotation, track-ball rotation, numeric entry, and
	 * angle-snap (e.g. Ctrl for 5° increments).
	 */
	class FRotateTool final : public FToolBase
	{
	public:
		explicit FRotateTool(const TSharedRef<FTransformSession>& InSession);

		/* ---------- FToolBase overrides ---------- */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value = 0.0f) override;
		virtual void UpdateHud() override;
		virtual void OnBegin() override;
		virtual void OnEnd(bool bApply) override;
		virtual void HandleMouseMovement(const FVector2D& CurrentViewportMousePosition) override;
		virtual FText GetNumericHudText() const override;

		virtual void HandleAxisLock(EAxisLock AxisPressed) override;

	private:
		FVector2D StartDragVector;
		FVector2D LastDragVector;
		FVector PivotStartPosition;
		FVector2D PivotViewportPosition;
		FTransform StartPivotTransform;
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;
		bool bTrackballModeEnabled;
		EAxisLock PreviousAxisLock = EAxisLock::All;
		float AccumulatedAngleRad;
		FVector2D TrackballMouseDelta;
		float AngleToApplyRad;

		FText BuildTrackballHudText(double LiveAngleX, double LiveAngleY, const FNumberFormattingOptions& NumFmt) const;

		virtual FVector GetSnapOffset(const FVector OffsetFromStart);
		virtual void SetTrackballRotationMode(const bool bEnabled) override;
		virtual bool GetTrackballRotationMode() override;
		virtual void UpdateToolSettingsForAxisLock() override;

		virtual FText GetLiveHudText() const override;
		FText BuildFreeformHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const;
		FText BuildSingleAxisHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const;
	};
} // namespace BlenderControls
