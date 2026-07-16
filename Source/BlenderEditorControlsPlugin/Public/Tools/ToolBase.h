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
	 * Implements shared logic for all transform tools.
	 */
	class FToolBase : public TSharedFromThis<FToolBase>
	{
	public:
		FToolBase(const TSharedRef<FTransformSession>& InSession, ETransformMode InMode,
		          const FString& InDisplayName);
		virtual ~FToolBase() = default;

		/** Primary update loop for mouse-driven transformation. */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) = 0;
		
		/** Commits the current transformation and ends the tool. */
		virtual void Accept();
		
		/** Aborts the transformation and reverts any changes made by this tool. */
		virtual void Cancel();
		
		/** Initializes tool state when activated. @return True if initialization succeeded. */
		virtual bool OnBegin();
		
		/** Fallback for mouse movement when in numeric mode. */
		virtual void HandleMouseMovement(const FVector2D& CurrentViewportMousePosition);
		
		/** Per-frame update called by the TransformSession. */
		virtual void Tick() {}

		/** Called when the tool is ending, either by accepting or cancelling. */
		virtual void OnEnd(bool bApply);

		/** Toggles trackball rotation mode (Rotate tool only). */
		virtual void SetTrackballRotationMode(const bool bEnabled)
		{
		}

		/** @return True if trackball mode is active. */
		virtual bool GetTrackballRotationMode() { return false; }
		
		/** Processes an axis constraint key press (X, Y, Z). */
		virtual void HandleAxisLock(EAxisLock AxisPressed);

		/** Applies the explicitly typed numeric value to the transform. */
		virtual void ApplyNumeric(double Value = 0.0f)
		{
			checkf(Session != nullptr, TEXT("ApplyNumeric: Session must be valid for %s"), *DisplayName);
		}

		/** Updates the HUD display strings. */
		virtual void UpdateHud();
		
		/** @return The display text for the HUD when in numeric input mode. */
		virtual FText GetNumericHudText() const = 0;

		/** @return The user-facing name of the tool (e.g. "Translate"). */
		const FString& GetDisplayName() const { return DisplayName; }

		/** Sets whether precision mode (slower movement) is active. */
		void SetPrecisionModeActive(bool bNewPrecisionModeActive);
		
		/** Sets whether grid/angle snapping is active. */
		void SetSnappingEnabled(bool bNewSnappingEnabled);
		
		/** @return True if grid/angle snapping is active. */
		bool IsSnappingEnabled() const { return bSnappingEnabled; }

		/** @return True if the transformation is constrained to exactly one axis. */
		bool IsSingleAxisLocked() const;
		
		/** Removes any visible axis constraint lines from the viewport. */
		void ClearDrawnAxisLines();
		
		/** Called when transitioning away from this tool to another one. */
		void OnSwitch();

	private:
		void RedrawAxisLines();
		void OnExitNumericMode();

		FLinearColor CachedSelectionColor;
		UE::Widget::EWidgetMode InitialWidgetMode;

		void StartNewLock(EAxisLock NewAxis) const;
		static FLinearColor GetAxisColor(EAxisLock InAxis);

		TArray<TWeakObjectPtr<UAxisLockGizmoComponent>> AxisGizmos;
		UAxisLockGizmoComponent* SpawnAxisGizmo(const FVector& Origin,
		                                        const FVector& AxisDir,
		                                        const FLinearColor& Color,
		                                        float ThicknessPx,
		                                        float LineLength) const;
		
		bool InitializeEditorState();
		bool InitializePivot();
		bool CacheViewVectors();
		bool InitializeGrabContext();
		bool InitializeUI();
		void RestorePreviousState();

	protected:
		virtual UE::Widget::EWidgetMode GetDesiredWidgetMode() const = 0;
		FVector GetAxisVector(EAxisLock InAxis) const;

		/** Returns the display string for the HUD during mouse-driven movement (e.g., "D: 12.45cm along Global X"). */
		virtual FText GetLiveHudText() const = 0;
		void ClearAxisGizmos();

		virtual void SetGrabContextAxisLock(EAxisLock AxisLock)
		{
		}

		void UpdateAxisLock();

		virtual void UpdateNumActiveSlots();

		// --- Pivot Helper Methods ---
		// These methods abstract pivot operations to work with both Actor and Control Rig selections


		/** Gets the start location of the active element (works with both pivot types) */
		FVector GetActiveElementStartLocation() const;

		/** Gets the start transform of the active element */
		FTransform GetActiveElementStartTransform() const;

		/** Gets the current location of the active element */
		FVector GetActiveElementCurrentLocation() const;

		/** Gets the start location of the pivot */
		FVector GetPivotStartLocation() const;



		const FSlateBrush* CursorBrush = nullptr;
		FVector2D CurrentViewportMousePos;
		FVector2D CurrentMousePosition;
		TSharedPtr<STransformHUD> HudWidget;

		bool bIsToolActive = true;
		FString HudString;

		FTransformSession* Session = nullptr;
		ETransformMode Mode;
		FString DisplayName;
		FViewport* Viewport = nullptr;
		TSharedPtr<FVirtualPivotBase> VirtualPivot;
		float CurrentPrecisionFactor = 1.0f;
		bool bPrecisionModeActive = false;
		bool bWasPrecisionModeActive = false;
		bool bSnappingEnabled = false;
		FEditorViewportClient* ViewportClient = nullptr;
		bool bLocalSpaceDefault;
		FGrabContext GrabContext;
		FVector ViewUp;
		FVector ViewRight;
		FVector ViewLocation;
		FVector ViewForward;
		int32 NumNumericSlots;
		FVector PreviousFrameTotalDelta = FVector::ZeroVector;
	};
} // namespace BlenderControls
