#pragma once
#include "Tools/ToolBase.h"
#include "Math/Quat.h"

namespace BlenderControls
{
	/**
	 * Implementation of the Blender-style 'Rotate' operation.
	 * Supports Screen-Relative rotation, Single-Axis (constrained) rotation, 
	 * and Trackball (dual-axis) rotation modes.
	 */
	class FRotateTool final : public FToolBase
	{
	public:
		explicit FRotateTool(const TSharedRef<FTransformSession>& InSession);

		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value = 0.0f) override;
		virtual void OnBegin() override;
		virtual void OnEnd(bool bApply) override;
		virtual FText GetNumericHudText() const override;
		virtual void Tick() override;
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
		EAxisLock CachedAxisLockPreTrackball = EAxisLock::All;

		FText BuildTrackballHudText(double LiveAngleX, double LiveAngleY, const FNumberFormattingOptions& NumFmt) const;

		virtual void SetTrackballRotationMode(const bool bEnabled) override;
		virtual bool GetTrackballRotationMode() override;
		virtual void UpdateNumActiveSlots() override;

		virtual FText GetLiveHudText() const override;
		FText BuildFreeformHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const;
		FText BuildSingleAxisHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const;
	};
} // namespace BlenderControls
