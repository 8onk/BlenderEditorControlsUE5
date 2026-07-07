#pragma once

#include "UObject/WeakObjectPtr.h"
#include "Enums.h"
#include "GrabContext.h"
#include "ScopedTransaction.h"
#include "ControlRig/ControlRigSelectionHelper.h"

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
		virtual void Accept();
		virtual void Cancel();
		virtual void OnBegin();
		/** Fallback for mouse movement when in numeric mode. */
		virtual void HandleMouseMovement(const FVector2D& CurrentViewportMousePosition);
		/** Per-frame update called by the TransformSession. */
		virtual void Tick()
		{
		}

		virtual void OnEnd(bool bApply);

		virtual void SetTrackballRotationMode(const bool bEnabled)
		{
		}

		virtual bool GetTrackballRotationMode() { return false; }
		virtual void HandleAxisLock(EAxisLock AxisPressed);

		virtual void ApplyNumeric(double Value = 0.0f)
		{
			checkf(OwningSession.IsValid(), TEXT("ApplyNumeric: Session must be valid for %s"), *DisplayName);
		}

		virtual void UpdateHud();
		virtual FText GetNumericHudText() const = 0;

		const FString& GetDisplayName() const { return DisplayName; }

		bool InitializeEditorState();
		void InitializePivot();
		void CacheViewVectors();
		void InitializeGrabContext();
		void InitializeUI();
		void RestorePreviousState();
		void OnSwitch();

		void SetPrecisionModeActive(bool bNewPrecisionModeActive);
		void SetSnappingEnabled(bool bNewSnappingEnabled);
		bool IsSnappingEnabled() const { return bSnappingEnabled; }

		bool IsSingleAxisLocked() const;
		void ClearDrawnAxisLines();

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

	protected:
		FVector GetAxisVector(EAxisLock InAxis) const;

		/** Returns the display string for the HUD during mouse-driven movement (e.g., "D: 12.45cm along Global X"). */
		virtual FText GetLiveHudText() const = 0;
		void ClearAxisGizmos();

		virtual void SetGrabContextAxisLock(EAxisLock AxisLock)
		{
		}

		void UpdateAxisLock();

		virtual void UpdateNumActiveSlots();

		TSharedPtr<FTransformSession> GetSession() const { return OwningSession.Pin(); }

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

		TWeakPtr<FTransformSession> OwningSession;
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
		FVector2D MouseDelta = FVector2D::ZeroVector;
		FVector ViewUp;
		FVector ViewRight;
		FVector ViewLocation;
		FVector ViewForward;
		int32 NumNumericSlots;
		FVector PreviousFrameTotalDelta = FVector::ZeroVector;
	};
} // namespace BlenderControls
