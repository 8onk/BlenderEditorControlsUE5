#pragma once

#include "CoreMinimal.h"

namespace BlenderControls
{
	/**
	 * Abstract base class for handling selection state and reverting transformations.
	 * Now that InputWidgetDelta handles the actual transformation math, the pivot classes
	 * act purely as a cache to remember where things started and what the active element is.
	 */
	class FVirtualPivotBase
	{
	public:
		virtual ~FVirtualPivotBase() = default;

		// Get the calculated center of the selection (for drawing the dashed line)
		virtual FVector GetStartLocation() const = 0;

		// Get the start transform of the "Main" selected element (for Local space axes)
		virtual FTransform GetActiveElementStartTransform() const = 0;

		// Get the current location of the active element
		virtual FVector GetActiveElementCurrentLocation() const = 0;

		// Snaps the objects back to their cached starting positions when the tool is cancelled/switched
		virtual void RevertToStartState() = 0;

		// Returns true if there are elements selected
		virtual bool IsValid() const = 0;

		// Optional: hooks for transform start/end tracking
		virtual void BeginTransformSequence() {}
		virtual void EndTransformSequence() {}
	};
}
