// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace BlenderControls
{
	struct FGrabContext;
	enum class EAxisLock : uint8;

	/**
	 * Abstract base class for handling selection state and reverting transformations.
	 * Now that InputWidgetDelta handles the actual transformation math, the pivot classes
	 * act purely as a cache to remember where things started and what the active element is.
	 */
	class FVirtualPivotBase
	{
	public:
		virtual ~FVirtualPivotBase() = default;

		//~ Pivot State Methods
		
		/** @return The calculated center of the selection (for drawing the dashed line). */
		virtual FVector GetStartLocation() const = 0;

		/** @return The start transform of the "Main" selected element (for Local space axes). */
		virtual FTransform GetActiveElementStartTransform() const = 0;

		/** @return The current location of the active element. */
		virtual FVector GetActiveElementCurrentLocation() const = 0;

		/** Resets the objects back to their cached starting positions. */
		virtual void RevertTransformToStartState() = 0;

		/** @return True if there are elements selected. */
		virtual bool IsValid() const = 0;

		//~ Transformation Methods

		/**
		 * Intercepts the delta for manual application if a Pivot doesn't support InputWidgetDelta natively.
		 * 
		 * @param InDrag Translation delta.
		 * @param InRot Rotation delta.
		 * @param InScale Scale delta.
		 * @return True if the transform was applied manually.
		 */
		virtual bool ApplyManualTransformDelta(const FVector& InDrag, const FRotator& InRot, const FVector& InScale) { return false; }

		/**
		 * Applies a rotation around the pivot.
		 * 
		 * @param GC The grab context detailing the pivot location and camera axes.
		 * @param AngleRad The rotation angle in radians.
		 * @param bUsingLocalSpace True if rotating in local space.
		 * @param LockedAxis The locked axis constraints.
		 */
		virtual void ApplyRotation(const FGrabContext& GC, float AngleRad, bool bUsingLocalSpace, EAxisLock LockedAxis) {}
		
		/**
		 * Applies a scale operation from the pivot.
		 * 
		 * @param ScaleMultiplier The vector representing the scale per-axis.
		 * @param bUsingLocalSpace True if scaling along local axes.
		 */
		virtual void ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace) {}

		/**
		 * Applies a local-space numeric translation delta directly.
		 * Used primarily for numeric keyboard input.
		 * 
		 * @param LocalDelta The unrotated local translation vector (e.g. [10, 0, 0] to translate 10 units on the active axis).
		 * @param bUsingLocalSpace If true, translates each element along its own individual local rotation axes.
		 */
		virtual void ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace) {}

		/**
		 * Applies a world-space translation delta derived from mouse drag interactions.
		 * Used primarily for viewport dragging and mouse movement interaction.
		 * 
		 * @param WorldDelta The absolute world-space translation delta.
		 * @param bUsingLocalSpace If true, converts the active element's world delta to local space and maps it to other elements' local coordinate axes.
		 * @param LockedAxis The locked transformation axis or plane restriction.
		 */
		virtual void ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis) {}

		//~ Event Hooks

		/** Optional: hook for transform start tracking. */
		virtual void BeginTransformSequence() {}
		
		/** Optional: hook for transform end tracking. */
		virtual void EndTransformSequence() {}

		/** Iterates through all managed elements, providing their start transform and active state. */
		virtual void ForEachElementTransform(TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const = 0;
	};
}
