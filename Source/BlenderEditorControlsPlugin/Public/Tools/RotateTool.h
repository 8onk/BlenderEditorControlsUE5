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
		explicit FRotateTool(EAxisLock InAxis = EAxisLock::All);

		/* ---------- FBlenderToolBase overrides ---------- */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(float Value) override;
		virtual void OnBegin() override;
		virtual void OnEnd(bool bApply) override;

		void SetTrackBallMode(const bool InTrackBallMode) { bTrackballModeEnabled = InTrackBallMode; }

	private:
		FVector2D StartDragVector;
		FVector2D LastDragVector;
		FVector PivotStartPos;
		FVector2D PivotViewportLocation;
		FTransform StartPivotTransform;
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;

		FVector2D RotationCenterScreen; // The object's pivot point projected onto the screen
		FVector2D MouseStartScreen; // The screen position of the mouse when rotation started
		FQuat InitialObjectRotation; // The object's rotation when rotation started
		FVector RotationAxisWorld;

	protected:
		/* ——— helpers ——— */
		FVector ScreenToWorldVector(const FVector2D& ScreenPos) const;
		void UpdateTrackball(const FVector2D& MouseDelta);
		FQuat BuildAxisQuat(float Radians) const;
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);

		/* ——— state ——— */
		FVector PivotWS = FVector::ZeroVector; // cached centre
		FVector LastVectorWS = FVector::ZeroVector; // for track-ball
		bool bTrackballModeEnabled = false;
		float AngleSnapIncrementDeg = 5.f;
		float AngleAccumulatorDeg = 0.f; // used when snapping
	};
} // namespace BlenderControls
