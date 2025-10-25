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
	class FRotateTool : public FToolBase
	{
	public:
		explicit FRotateTool(const TSharedRef<FTransformSession>& InSession);

		/* ---------- FToolBase overrides ---------- */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value) override;
		virtual void UpdateHud() override;
		virtual void OnBegin() override;
		virtual void OnEnd(bool bApply) override;
		virtual void HandleMouseMovement(const FVector2D& CurrentViewportMousePosition) override;

		void SetTrackBallMode(const bool InTrackBallMode) { bTrackballModeEnabled = InTrackBallMode; }

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

	protected:
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);
		virtual void SetTrackballRotationMode(const bool bEnabled) override;
		virtual bool GetTrackballRotationMode() override;
		virtual void UpdateToolSettingsForAxisLock() override;
		virtual FString GetFormattedValueForEditing(const FNumericSlotData& Slot) const override;
		virtual FText GetLiveTranslationHudText() const override;
	};
} // namespace BlenderControls
