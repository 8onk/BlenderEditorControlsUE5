#pragma once

#include "UObject/WeakObjectPtr.h"
#include "BlenderEditorControlsEnums.h"
#include "SharedPivot.h"
#include "ScopedTransaction.h"

namespace BlenderControls
{
	struct FGrabContext
	{
		FVector2D MousePosStart;
		FVector2D MousePosB;
		FVector TotalDelta;
		FVector PivotStartPosition;
		FVector HelperAxisDir;
		FVector HelperPlaneN;
		FVector ViewForward;
		float ScreenToWorldScale;

		// Helper describing current dragging surface (view-plane, axis-line, dual plane)
		enum class EHelperType
		{
			ViewPlane,
			AxisLine,
			AxisPlane
		} HelperType;
	};

	class FBlenderToolBase : public TSharedFromThis<FBlenderToolBase>
	{
	public:
		FBlenderToolBase(ETransformMode InMode, EAxisLock InAxis, const FString& InDisplayName);
		virtual ~FBlenderToolBase();

		/** Per-frame update from input-processor */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) = 0;

		virtual void Accept();
		virtual void Cancel();

		/** Numeric entry apply */
		virtual void ApplyNumeric(float Value);

		// Getter for DisplayName
		const FString& GetDisplayName() const { return DisplayName; }

		virtual void OnBegin();
		virtual void OnEnd(bool bApply);

		void SetPrecisionModeActive(bool bNewPrecisionModeActive);
		void SetSnappingEnabled(bool bNewSnappingEnabled) { bSnappingEnabled = bNewSnappingEnabled; }
		void HandleAxisLock(EAxisLock AxisPressed);
		bool IsSingleAxisLocked() const;
		void NotifyMouseWrap() { bPendingMouseWrap = true; }

	private:
		void CaptureSelection();

		FLinearColor CachedSelectionColor;
		UE::Widget::EWidgetMode InitialWidgetMode;

		void StartNewLock(EAxisLock NewAxis);
		void UpdateAxisLock();
		static FLinearColor GetAxisColor(EAxisLock InAxis);
		void DrawAxisLine(const EAxisLock InAxis) const;
		void FlushDrawnAxisLines() const;
		float CalculateDynamicThickness(const FVector& Origin) const;


		TWeakObjectPtr<ULineBatchComponent> CachedBatcher;
		float FallbackLineThickness = 2.0f;
		const float MinLineThickness = 1.0f;
		const float MaxLineThickness = 6.0f;
		const float ReferenceDistance = 500.0f;
		FVector2D LastMousePosition;
		bool bPendingMouseWrap = false;

	protected:
		FVector GetAxisVector(EAxisLock InAxis) const;
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock);
		FVector2D CurrentViewportMousePos;
		FVector2D CurrentMousePosition;

		/* Transaction utilities */
		TUniquePtr<FScopedTransaction> ParentTxn;

		ETransformMode Mode;
		EAxisLock LockedAxis;
		FString DisplayName;
		TArray<TWeakObjectPtr<AActor>> SelectedActors;
		FViewport* Viewport = nullptr;
		TSharedPtr<FSharedPivot> Pivot;
		FSceneView* SceneView = nullptr;
		float PrecisionFactor = 0.1f;
		float CurrentPrecisionFactor = 1.0f;
		bool bPrecisionModeActive = false;
		bool bWasPrecisionModeActive = false;
		bool bSnappingEnabled = false;
		FLevelEditorViewportClient* ViewportClient = nullptr;
		bool bIsAxisLockActive = false;
		bool bIsUsingLocalSpace = false;
		bool bLocalSpaceDefault;
		FGrabContext GrabContext;
		FVector2D MouseDelta;
		FVector ViewUp;
		FVector ViewRight;
		FVector ViewLocation;
		FVector ViewForward;
	};
} // namespace BlenderControls
