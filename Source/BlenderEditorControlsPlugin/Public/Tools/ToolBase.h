#pragma once

#include "UObject/WeakObjectPtr.h"
#include "Enums.h"
#include "GrabContext.h"
#include "ScopedTransaction.h"

class UAxisLockGizmoComponent;

namespace BlenderControls
{
	class FTransformSession;
	class STransformHUD;
	struct FChildInfo;
	class FVirtualPivotBase;

	/**
	 * Abstract base class for transform operations (Translate, Rotate, Scale).
	 * Implements shared logic for all transform tools such as axis locking, numeric input, and view projection.
	 */
	class FToolBase : public TSharedFromThis<FToolBase>
	{
	public:
		/** 
		 * Constructs the tool.
		 * @param InSession The transform session managing this tool.
		 * @param InMode The transform mode (Translate, Rotate, Scale).
		 * @param InDisplayName The UI display name for the tool.
		 */
		FToolBase(const TSharedRef<FTransformSession>& InSession, ETransformMode InMode, const FString& InDisplayName);

		virtual ~FToolBase() = default;

		/** 
		 * Initializes tool state, caches view vectors, sets up UI, and begins tracking. 
		 * @return True if initialization succeeded, false if a required initialization process failed.
		 */
		virtual bool OnBegin();

		/** 
		 * Primary update loop for mouse-driven transformation. Called when mouse moves.
		 * @param CurrentViewportMousePosition The current mouse position inside the viewport.
		 */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) = 0;

		/** Per-frame update called by the TransformSession. Used for updating the HUD */
		virtual void Tick()
		{
		}

		/** 
		 * Called when the tool is ending, cleaning up UI, restoring editor state, and saving/reverting changes.
		 * @param bApply If true, commits the transformation; if false, reverts to start state.
		 */
		virtual void OnEnd(bool bApply);

		/** Commits the current transformation and ends the tool. */
		virtual void Accept();

		/** Aborts the transformation and reverts any changes made by this tool. */
		virtual void Cancel();

		/** Called when transitioning away from this tool to another one (e.g. from Translate to Rotate). */
		void OnSwitch();

		/** 
		 * Handles raw mouse movement to calculate virtual continuous mouse positions and deltas.
		 * Prevents hardware cursor from hitting screen bounds (wraps around).
		 * @param CurrentViewportMousePosition The hardware cursor position.
		 */
		virtual void HandleMouseMovement(const FVector2D& CurrentViewportMousePosition);

		/** 
		 * Processes an axis constraint key press (X, Y, Z). Handles double-tap for local space toggle.
		 * @param AxisPressed The axis key that was pressed.
		 */
		virtual void HandleAxisLock(EAxisLock AxisPressed);

		/** 
		 * Applies typed numeric values to the transform.
		 * @param Value Optional override value, otherwise reads from numeric processor.
		 */
		virtual void ApplyNumeric(double Value = 0.0f)
		{
		}

		/** 
		 * Sets whether precision mode (slower movement) is active. Adjusts precision factor based on user settings.
		 * @param bNewPrecisionModeActive True to enable precision mode.
		 */
		void SetPrecisionModeActive(bool bNewPrecisionModeActive);

		/** 
		 * Sets whether grid/angle snapping is active. Triggers an update if changing.
		 * @param bNewSnappingEnabled True to enable snapping.
		 */
		void SetSnappingEnabled(bool bNewSnappingEnabled);

		/** @return True if grid/angle snapping is active. */
		bool IsSnappingEnabled() const { return bSnappingEnabled; }


		/** Updates the HUD display strings to reflect current live or numeric text. */
		virtual void UpdateHud();

		/** @return The display text for the HUD when in numeric input mode. */
		virtual FText GetNumericHudText() const = 0;

		/** Removes any visible axis constraint lines (gizmos) from the viewport. */
		void ClearDrawnAxisLines();

		/** @return The user-facing name of the tool (e.g. "Translate"). */
		const FString& GetDisplayName() const { return DisplayName; }

		/** @return True if the transformation is constrained to exactly one axis. */
		bool IsSingleAxisLocked() const;

	protected:
		/** @return The UE widget mode this tool requires (Translate/Rotate/Scale). */
		virtual UE::Widget::EWidgetMode GetDesiredWidgetMode() const = 0;

		/** @return The display string for the HUD during mouse-driven movement. */
		virtual FText GetLiveHudText() const = 0;

		/** 
		 * Configures the grab context for the specified axis lock.
		 * @param AxisLock The axis constraint to apply.
		 */
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock)
		{
		}

		/** Updates the number of expected numeric inputs based on current axis constraints. */
		virtual void UpdateNumActiveSlots();

		/** 
		 * Resolves a global axis lock into a precise directional vector.
		 * @param InAxis The desired axis.
		 * @return The normalized vector representing the axis in world space (handles local space adjustment).
		 */
		FVector GetAxisVector(EAxisLock InAxis) const;

		/** Syncs the lock state and redraws gizmos. */
		void UpdateAxisLock();

		/** Destroys and clears all active axis line gizmos. */
		void ClearAxisGizmos();

		/** Gets the world-space start location of the active element. */
		FVector GetActiveElementStartLocation() const;

		/** Gets the start transform of the active element. */
		FTransform GetActiveElementStartTransform() const;

		/** Gets the current world-space location of the active element. */
		FVector GetActiveElementCurrentLocation() const;

		/** Gets the start world-space location of the combined pivot. */
		FVector GetPivotStartLocation() const;

		/** The session managing tool execution. */
		FTransformSession* Session = nullptr;

		/** The target viewport client. */
		FEditorViewportClient* ViewportClient = nullptr;

		/** The underlying viewport. */
		FViewport* Viewport = nullptr;

		/** Abstraction over the transformed selection (Actor or Control Rig). */
		TSharedPtr<FVirtualPivotBase> VirtualPivot;

		/** Overlay UI widget displaying transform values. */
		TSharedPtr<STransformHUD> HudWidget;

		/** Mode of this tool (Translate/Rotate/Scale). */
		ETransformMode Mode;

		/** Human-readable tool name. */
		FString DisplayName;

		// --- View Caching ---

		/** Camera's upward vector. */
		FVector ViewUp;

		/** Camera's rightward vector. */
		FVector ViewRight;

		/** Camera's location in world space. */
		FVector ViewLocation;

		/** Camera's forward viewing direction. */
		FVector ViewForward;

		/** Start state and projected intersection planes for mouse operations. */
		FGrabContext GrabContext;

		/** Most recent mouse position within the viewport bounds. */
		FVector2D CurrentViewportMousePos;

		/** Most recent absolute screen mouse position. */
		FVector2D CurrentMousePosition;

		/** Cumulative delta from the prior frame. */
		FVector PreviousFrameTotalDelta = FVector::ZeroVector;

		/** Text currently shown in the HUD. */
		FString HudString;

		/** Brush used for custom cursor display. */
		const FSlateBrush* CursorBrush = nullptr;

		/** Number of distinct numeric values being collected (e.g., 3 for XYZ, 1 for X-only). */
		int32 NumNumericSlots;

		/** True if tool updates are actively processing. */
		bool bIsToolActive = true;

		/** Speed multiplier applied during precision mode. */
		float CurrentPrecisionFactor = 1.0f;

		/** True if precision movement is currently engaged. */
		bool bPrecisionModeActive = false;

		/** Previous tick's precision mode state. */
		bool bWasPrecisionModeActive = false;

		/** True if step-snapping is engaged. */
		bool bSnappingEnabled = false;

		/** The user's default coordinate space setting prior to tool invocation. */
		bool bLocalSpaceDefault;

	private:
		/** Destroys existing lines and creates new gizmos reflecting the current lock. */
		void RedrawAxisLines();

		/** Callback for when numeric input mode is aborted. */
		void OnExitNumericMode();

		/** Overwrites session constraints with a newly initiated lock. */
		void StartNewLock(EAxisLock NewAxis) const;

		/** Retrieves the config-defined color for a given axis. */
		static FLinearColor GetAxisColor(EAxisLock InAxis);

		/** Instantiates an axis line component in the world. */
		UAxisLockGizmoComponent* SpawnAxisGizmo(const FVector& Origin, const FVector& AxisDir,
		                                        const FLinearColor& Color, float ThicknessPx, float LineLength) const;

		/** Sets up viewport capture and alters editor selection outlines. */
		void InitializeEditorState();

		/**
		 * Hides and locks the mouse cursor as well as changes the widget mode based on tool. 
		 * @param bIsToolEnding is the tool ending or beginning?
		 */
		void SetViewportState(bool bIsToolEnding) const;

		/** Begins the sequence on the virtual pivot. */
		bool InitializePivot();

		/** Extracts directional vectors from the current view. */
		bool CacheViewVectors();

		/** Sets up the grab context's initial planes and scales based on scene deprojection. */
		bool InitializeGrabContext();

		/** Instantiates the HUD and hides hardware cursor. */
		bool InitializeUI();

		/** Re-applies axis locks that may have been active prior to switching tools. */
		void RestoreAxisLock();

		/** Editor selection color before tool began. */
		FLinearColor CachedSelectionColor;

		/** Active line gizmo components. */
		TArray<TWeakObjectPtr<UAxisLockGizmoComponent>> AxisGizmos;
	};
} // namespace BlenderControls
