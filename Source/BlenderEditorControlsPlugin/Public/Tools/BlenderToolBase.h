#pragma once

#include "UObject/WeakObjectPtr.h"
#include "BlenderEditorControlsEnums.h"
#include "GrabContext.h"
#include "ScopedTransaction.h"
#include "ToolSharedState.h"

class UAxisLockGizmoComponent;

namespace BlenderControls
{
	class FTransformSession;
	class STransformHUD;
	struct FChildInfo;
	class FSharedPivot;

	class FBlenderToolBase : public TSharedFromThis<FBlenderToolBase>
	{
	public:
		FBlenderToolBase(const TSharedRef<FTransformSession>& InSession, ETransformMode InMode,
		                 const FString& InDisplayName);
		virtual ~FBlenderToolBase();

		/** Per-frame update from input-processor */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) = 0;

		virtual void Accept();
		virtual void Cancel();

		/** Numeric entry apply */
		virtual void ApplyNumeric(double Value = 0.0f)
		{
			checkf(OwningSession.IsValid(), TEXT("ApplyNumeric: Session must be valid for %s"), *DisplayName);
		}

		/** Set HUD string - must be implemented by inheriting classes */
		virtual void UpdateHud();

		// Getter for DisplayName
		const FString& GetDisplayName() const { return DisplayName; }

		bool InitializeEditorState();
		void InitializeTransaction();
		bool CacheSceneView();
		void CacheViewVectors();
		void InitializeGrabContext(const FVector2D& InMousePos, const FVector& InRayOrigin,
		                           const FVector& InRayDirection);
		void InitializeUI();
		void RestorePreviousState();
		virtual void OnBegin();
		virtual void OnEnd(bool bApply);

		void SetPrecisionModeActive(bool bNewPrecisionModeActive);
		void SetSnappingEnabled(bool bNewSnappingEnabled);
		bool IsSnappingEnabled() const { return bSnappingEnabled; }

		void SetViewportMousePosition(FVector2D InViewportMousePosition)
		{
			CurrentViewportMousePos = InViewportMousePosition;
		}

		virtual void SetTrackballRotationMode(const bool bEnabled);
		virtual bool GetTrackballRotationMode();

		virtual void HandleAxisLock(EAxisLock AxisPressed);
		bool IsSingleAxisLocked() const;
		void ClearDrawnAxisLines();

	private:
		void RedrawAxisLines();

		FLinearColor CachedSelectionColor;
		UE::Widget::EWidgetMode InitialWidgetMode;

		void StartNewLock(EAxisLock NewAxis) const;
		static FLinearColor GetAxisColor(EAxisLock InAxis);

		TWeakObjectPtr<ULineBatchComponent> CachedBatcher;
		float FallbackLineThickness = 2.0f;
		const float MinLineThickness = 1.0f;
		const float MaxLineThickness = 6.0f;
		const float ReferenceDistance = 500.0f;

		TArray<TWeakObjectPtr<UAxisLockGizmoComponent>> AxisGizmos;
		UAxisLockGizmoComponent* SpawnAxisGizmo(const FVector& Origin,
		                                        const FVector& AxisDir,
		                                        const FLinearColor& Color,
		                                        float ThicknessPx,
		                                        float LineLength) const;

	protected:
		FVector GetAxisVector(EAxisLock InAxis) const;
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);

		virtual void SetGrabContextAxisLock(EAxisLock AxisLock)
		{
		}

		virtual FString GetFormattedValueForEditing(const FNumericSlotData& Slot) const;
		void UpdateAxisLock();

		virtual void UpdateToolSettingsForAxisLock()
		{
		}

		TSharedPtr<FTransformSession> GetSession() const { return OwningSession.Pin(); }

		const FSlateBrush* CursorBrush = nullptr;
		FVector2D CurrentViewportMousePos;
		FVector2D CurrentMousePosition;
		TSharedPtr<STransformHUD> HudWidget;

		float CurrentNonTrackballRotationAngle = 0.0f;
		float CachedNonTrackballRotationAngle = 0.0f;

		FString HudString;

		TUniquePtr<FScopedTransaction> ParentTxn;

		TWeakPtr<FTransformSession> OwningSession;
		ETransformMode Mode;
		FString DisplayName;
		FViewport* Viewport = nullptr;
		TSharedPtr<FSharedPivot> VirtualPivot;
		FSceneView* SceneView = nullptr;
		float PrecisionFactor = 0.1f;
		float CurrentPrecisionFactor = 1.0f;
		bool bPrecisionModeActive = false;
		bool bWasPrecisionModeActive = false;
		bool bSnappingEnabled = false;
		FLevelEditorViewportClient* ViewportClient = nullptr;
		bool bLocalSpaceDefault;
		FGrabContext GrabContext;
		FVector2D MouseDelta;
		FVector ViewUp;
		FVector ViewRight;
		FVector ViewLocation;
		FVector ViewForward;
		int32 NumNumericSlots;
	};
} // namespace BlenderControls
