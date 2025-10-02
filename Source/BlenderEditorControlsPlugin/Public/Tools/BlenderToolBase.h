#pragma once

#include "UObject/WeakObjectPtr.h"
#include "BlenderEditorControlsEnums.h"
#include "GrabContext.h"
#include "ScopedTransaction.h"
#include "Input/BlenderEditorControlsPluginInputProcessor.h"

class UAxisLockGizmoComponent;

namespace BlenderControls
{
	class STransformHUD;
	struct FChildInfo;
	class FSharedPivot;

	class FBlenderToolBase : public TSharedFromThis<FBlenderToolBase>
	{
	public:
		FBlenderToolBase(TSharedPtr<FTransformSession> InSession, ETransformMode InMode, EAxisLock InAxis,
		                 const FString& InDisplayName);
		virtual ~FBlenderToolBase();

		/** Per-frame update from input-processor */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) = 0;

		virtual void Accept();
		virtual void Cancel();

		/** Numeric entry apply */
		virtual void ApplyNumeric(double Value = 0.0f) {}

		/** Set HUD string - must be implemented by inheriting classes */
		virtual void UpdateHud() = 0;

		// Getter for DisplayName
		const FString& GetDisplayName() const { return DisplayName; }

		virtual void OnBegin();
		virtual void OnEnd(bool bApply);

		void SetPrecisionModeActive(bool bNewPrecisionModeActive);
		void SetSnappingEnabled(bool bNewSnappingEnabled);

		void SetViewportMousePosition(FVector2D InViewportMousePosition)
		{
			CurrentViewportMousePos = InViewportMousePosition;
		}

		virtual void SetTrackballRotationMode(const bool bEnabled);
		virtual bool GetTrackballRotationMode();

		virtual void HandleAxisLock(EAxisLock AxisPressed);
		bool IsSingleAxisLocked() const;
		void FlushDrawnAxisLines() const;

		void SetLastMousePosition(const FVector2D NewLastMousePosition) { LastMousePosition = NewLastMousePosition; }
		FVector2D GetLastMousePosition() const { return LastMousePosition; }
		void AddMouseDelta(const FVector2D NewDelta) { MouseDelta += NewDelta; }
		void NotifyMouseWrap();

		void BeginNumericMode();
		void CycleNumericInputSlot();
		void ExitNumericMode();
		void ClearLiveNumericValue();
		void UpdateActiveNumericSlot(TCHAR Character);
		void HandleBackspace();
		void ToggleNegation();
		void ToggleReciprocal();

	private:
		void RedrawAxisLines();

		FLinearColor CachedSelectionColor;
		UE::Widget::EWidgetMode InitialWidgetMode;

		void StartNewLock(EAxisLock NewAxis);
		static FLinearColor GetAxisColor(EAxisLock InAxis);
		void DrawAxisLine(const EAxisLock InAxis, const FChildInfo* ChildInfo = nullptr) const;

		TWeakObjectPtr<ULineBatchComponent> CachedBatcher;
		float FallbackLineThickness = 2.0f;
		const float MinLineThickness = 1.0f;
		const float MaxLineThickness = 6.0f;
		const float ReferenceDistance = 500.0f;
		FVector2D LastMousePosition;
		FVector2D PreWrapMousePosition;
		bool bPendingMouseWrap = false;
		int32 CachedMouseSpeed;

		TArray<TWeakObjectPtr<UAxisLockGizmoComponent>> AxisGizmos;
		UAxisLockGizmoComponent* SpawnAxisGizmo(const FVector& Origin,
														  const FVector& AxisDir,
														  const FLinearColor& Color,
														  float ThicknessPx,
														  float LineLength) const;

	protected:
		FVector GetAxisVector(EAxisLock InAxis) const;
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) {}
		virtual FString GetFormattedValueForEditing(const FNumericSlotData& Slot) const;
		void UpdateAxisLock();
		virtual void UpdateToolSettingsForAxisLock() {}

		FVector2D CurrentViewportMousePos;
		FVector2D CurrentMousePosition;
		FVector2D VirtualMousePosition;
		TSharedPtr<STransformHUD> HudWidget;

		float CurrentNonTrackballRotationAngle = 0.0f;
		float CachedNonTrackballRotationAngle = 0.0f;

		//FNumericSlotData NumericSlots[3];
		int32 FirstEditedSlotIndex = -1;
		int32 CurrentNumericSlotIndex;

		FString HudString;

		TUniquePtr<FScopedTransaction> ParentTxn;

		TSharedPtr<FTransformSession> Session;
		ETransformMode Mode;
		EAxisLock LockedAxis;
		FString DisplayName;
		TArray<TWeakObjectPtr<AActor>> SelectedActors;
		FViewport* Viewport = nullptr;
		TSharedPtr<FSharedPivot> VirtualPivot;
		FSceneView* SceneView = nullptr;
		float PrecisionFactor = 0.1f;
		float CurrentPrecisionFactor = 1.0f;
		bool bPrecisionModeActive = false;
		bool bWasPrecisionModeActive = false;
		bool bSnappingEnabled = false;
		FLevelEditorViewportClient* ViewportClient = nullptr;
		bool bIsAxisLockActive = false;
		bool bUsingLocalSpace = false;
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
