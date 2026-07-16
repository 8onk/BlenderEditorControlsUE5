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

		//~ FToolBase Interface
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value = 0.0f) override;
		virtual bool OnBegin() override;
		virtual void OnEnd(bool bApply) override;
		virtual FText GetNumericHudText() const override;
		virtual void Tick() override;
		virtual void HandleAxisLock(EAxisLock AxisPressed) override;

	protected:
		//~ FToolBase Interface
		virtual UE::Widget::EWidgetMode GetDesiredWidgetMode() const override { return UE::Widget::WM_Rotate; }

	private:
		FVector2D StartDragVector;
		FVector2D LastDragVector;
		FVector PivotStartPosition;
		FVector2D PivotViewportPosition;
		FTransform StartPivotTransform;
		
		//~ FToolBase Interface
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;
		virtual void SetTrackballRotationMode(const bool bEnabled) override;
		virtual bool GetTrackballRotationMode() override;
		virtual void UpdateNumActiveSlots() override;
		virtual FText GetLiveHudText() const override;

		bool bTrackballModeEnabled;
		EAxisLock PreviousAxisLock = EAxisLock::All;
		float AccumulatedAngleRad;
		FVector2D TrackballMouseDelta;
		float AngleToApplyRad;
		EAxisLock CachedAxisLockPreTrackball = EAxisLock::All;

		FText BuildTrackballHudText(double LiveAngleX, double LiveAngleY, const FNumberFormattingOptions& NumFmt) const;
		FText BuildFreeformHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const;
		FText BuildSingleAxisHudText(double LiveAngleDeg, const FNumberFormattingOptions& NumFmt) const;
	};
} // namespace BlenderControls
