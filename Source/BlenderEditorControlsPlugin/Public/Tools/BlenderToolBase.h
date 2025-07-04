#pragma once

#include "UObject/WeakObjectPtr.h"
#include "BlenderEditorControlsEnums.h"
#include "SharedPivot.h"
#include "ScopedTransaction.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
	struct FGrabContext
	{
		FVector PivotStartPos; // world-space position at G-press
		FVector HitAnchor; // ray/plane or ray/line intersection at G-press
		FVector TotalDelta; // accumulated movement applied so far
		FVector2D MousePosStart;
		FVector2D MousePosB;
		FVector2D MousePosAnchor;
		FVector2D TotalMouseDeltaAtAnchor; 
		float ScreenToWorldScale;

		// Helper describing current dragging surface (view-plane, axis-line, dual plane)
		enum class EHelperType
		{
			ViewPlane,
			AxisLine,
			AxisPlane
		} HelperType;

		FVector HelperAxisDir; // normalized axis vector      (AxisLine)  OR
		FVector HelperPlaneN; // normalized plane normal      (AxisPlane / ViewPlane)
		FVector Pivot; // centre the helper goes through

		// precision mode bookkeeping
		FVector DeltaAnchor;
		FVector ShiftStartHit;
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

		/** Axis helpers */
		EAxisLock GetAxis() const { return LockedAxis; }

		/** Numeric entry apply */
		virtual void ApplyNumeric(float Value);

		// Getter for DisplayName
		const FString& GetDisplayName() const { return DisplayName; }

		virtual void OnBegin();
		virtual void OnEnd(bool bApply);

		void SetPrecisionModeActive(bool bNewPrecisionModeActive);
		void SetSnappingEnabled(bool bNewSnappingEnabled) { bSnappingEnabled = bNewSnappingEnabled; }
		void HandleAxisLock(EAxisLock AxisPressed);

		void OnAxisLockRecalculated(const FVector2D& CurrentViewportMousePosition);

	private:
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

	protected:
		/** Child tools call this to populate Selected & prepare undo */
		void CaptureSelection();
		FVector GetAxisVector(EAxisLock InAxis) const;
		FVector2D CurrentViewportMousePos;
		FVector2D CurrentMousePosition;
		FVector NormalToRemove;
		FVector AccumulatedDelta;

		/* Transaction utilities */
		TUniquePtr<class FScopedTransaction> ParentTxn;

		/** Returns snap offset for the given offset from start position */
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);

		ETransformMode Mode;
		EAxisLock LockedAxis;
		FString DisplayName;
		TArray<TWeakObjectPtr<AActor>> SelectedActors;
		TMap<TWeakObjectPtr<AActor>, FTransform> OriginalTransforms;
		FVector PreviousPlaneIntersectionPoint = FVector::ZeroVector;
		FVector CurrentHit = FVector::ZeroVector;
		FViewport* Viewport = nullptr;
		TSharedPtr<class FSharedPivot> Pivot;
		FSceneView* SceneView = nullptr;
		FPlane DragPlane;
		float PrecisionFactor = 0.1f;
		float CurrentPrecisionFactor = 1.0f;
		bool bPrecisionModeActive = false;
		bool bWasPrecisionModeActive = false;
		bool bSnappingEnabled = false;
		FVector GrabStartPlaneIntersectionPoint;
		FVector NewPivotPosition;
		FVector PrecisionAnchor;
		FVector ShiftStartIntersectionPoint;
		FVector CurrentRayOrigin;
		FVector CurrentRayDirection;
		FLevelEditorViewportClient* ViewportClient = nullptr;
		bool bIsAxisLockActive = false;
		bool bIsUsingLocalSpace = false;
		bool bLocalSpaceDefault;
		FGrabContext GrabContext;
		FVector2D MouseDelta;
		FVector ViewUp;
		FVector ViewRight;
	};
} // namespace BlenderControls
