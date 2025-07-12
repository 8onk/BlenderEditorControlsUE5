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
		FVector PivotStartPosition;
		FVector2D PivotViewportPosition;
		FTransform StartPivotTransform;
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;
		FVector RotationAxisWorld;
		bool bTrackballModeEnabled;
		float RotationAngleAnchor;

	protected:
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);
		virtual void OnMouseWrap() override;
		virtual void SetTrackballRotationMode(const bool bEnabled) override;
		virtual bool GetTrackballRotationMode();

		FQuat ApplyRotationAroundAxis(
			const FQuat& CurrentRotation,
			const FVector& Axis, // Either local or world
			float AngleRad,
			bool bUseLocalAxis);
	};
} // namespace BlenderControls
