#pragma once

#include "UObject/WeakObjectPtr.h"
#include "BlenderEditorControlsEnums.h"
#include "SharedPivot.h"
#include "ScopedTransaction.h"

namespace BlenderControls
{
	class FBlenderToolBase : public TSharedFromThis<FBlenderToolBase>
	{
	public:
		FBlenderToolBase(ETransformMode InMode, ETransformAxis InAxis, const FString& InDisplayName);
		virtual ~FBlenderToolBase();

		/** Per-frame update from input-processor */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) = 0;

		virtual void Accept();
		virtual void Cancel();

		/** Axis helpers */
		ETransformAxis GetAxis() const { return Axis; }

		/** Numeric entry apply */
		virtual void ApplyNumeric(float Value);

		// Getter for DisplayName
		const FString& GetDisplayName() const { return DisplayName; }

		virtual void OnBegin();
		virtual void OnEnd(bool bApply);

		void SetPrecisionModeActive(bool bNewPrecisionModeActive) { bPrecisionModeActive = bNewPrecisionModeActive; }
		void SetSnappingEnabled(bool bNewSnappingEnabled) { bSnappingEnabled = bNewSnappingEnabled; }
		void HandleAxisLock(ETransformAxis AxisPressed);

		void OnAxisLockRecalculated(const FVector2D& CurrentViewportMousePosition);

	private:
		FLinearColor CachedSelectionColor;
		UE::Widget::EWidgetMode InitialWidgetMode;
		void StartNewLock(ETransformAxis NewAxis);
		void UpdateDragPlane();
		static FLinearColor GetAxisColor(ETransformAxis InAxis);
		void DrawAxisLine(const ETransformAxis InAxis) const;
		void FlushDrawnAxisLines() const;
		float CalculateDynamicThickness(const FVector& Origin) const;
		TWeakObjectPtr<ULineBatchComponent> CachedBatcher;
		float FallbackLineThickness = 2.0f;
		const float MinLineThickness = 1.0f;
		const float MaxLineThickness = 6.0f;
		const float ReferenceDistance = 500.0f;

	protected:
		/** Child tools call this to populate Selected & prepare undo */
		void CaptureSelection();
		FVector GetAxisVector(ETransformAxis InAxis) const;
		FVector2D CurrentViewportMousePos;
		FVector NormalToRemove;

		/* Transaction utilities */
		TUniquePtr<class FScopedTransaction> ParentTxn;

		/** Returns snap offset for the given offset from start position */
		virtual FVector GetSnapOffset(const FVector OffsetFromStart);

		ETransformMode Mode;
		ETransformAxis Axis;
		FString DisplayName;
		TArray<TWeakObjectPtr<AActor>> SelectedActors;
		TMap<TWeakObjectPtr<AActor>, FTransform> OriginalTransforms;
		FVector PreviousPlaneIntersectionPoint = FVector::ZeroVector;
		FVector CurrentPlaneIntersectionPoint = FVector::ZeroVector;
		FViewport* Viewport = nullptr;
		TSharedPtr<class FSharedPivot> Pivot;
		FSceneView* SceneView = nullptr;
		FPlane DragPlane;
		float PrecisionFactor = 0.1f;
		bool bPrecisionModeActive = false;
		bool bSnappingEnabled = false;
		FVector GrabStartPlaneIntersectionPoint;
		FVector NewPivotPosition;
		FLevelEditorViewportClient* ViewportClient = nullptr;
		bool bIsAxisLockActive = false;
		bool bIsUsingLocalSpace = false;
		bool bLocalSpaceDefault;
	};
} // namespace BlenderControls
