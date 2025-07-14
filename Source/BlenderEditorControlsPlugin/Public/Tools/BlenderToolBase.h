#pragma once

#include "UObject/WeakObjectPtr.h"
#include "BlenderEditorControlsEnums.h"
#include "SharedPivot.h"
#include "ScopedTransaction.h"

namespace BlenderControls
{
	struct FGrabContext
	{
		FVector2D StartMousePos;
		FVector StartLocation;
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

		virtual void SetTrackballRotationMode(const bool bEnabled);
		virtual bool GetTrackballRotationMode();

		virtual void HandleAxisLock(EAxisLock AxisPressed);
		bool IsSingleAxisLocked() const;
		void FlushDrawnAxisLines() const;

		void SetLastMousePosition(const FVector2D NewLastMousePosition)  { LastMousePosition = NewLastMousePosition; }

	private:
		void CaptureSelection();
		void RedrawAxisLines() const;

		FLinearColor CachedSelectionColor;
		UE::Widget::EWidgetMode InitialWidgetMode;

		void StartNewLock(EAxisLock NewAxis);
		static FLinearColor GetAxisColor(EAxisLock InAxis);
		void DrawAxisLine(const EAxisLock InAxis) const;
		float CalculateDynamicThickness(const FVector& Origin) const;
		
		TWeakObjectPtr<ULineBatchComponent> CachedBatcher;
		float FallbackLineThickness = 2.0f;
		const float MinLineThickness = 1.0f;
		const float MaxLineThickness = 6.0f;
		const float ReferenceDistance = 500.0f;
		FVector2D LastMousePosition;
		FVector2D PreWrapMousePosition;
		bool bPendingMouseWrap = false;
		int32 CachedMouseSpeed;

	protected:
		FVector GetAxisVector(EAxisLock InAxis) const;
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock);
		void UpdateAxisLock();
		FVector2D CurrentViewportMousePos;
		FVector2D CurrentMousePosition;
		FVector2D UnscaledMouseDelta;
		FVector2D VirtualMousePosition;

		float CurrentNonTrackballRotationAngle = 0.0f;
		float CachedNonTrackballRotationAngle = 0.0f;

		/* Transaction utilities */
		TUniquePtr<FScopedTransaction> ParentTxn;

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
