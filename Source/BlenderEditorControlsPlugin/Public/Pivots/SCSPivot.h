#pragma once

#include "CoreMinimal.h"
#include "Pivots/VirtualPivotBase.h"

struct FSubobjectData;
class USceneComponent;

class FBlueprintEditor;
class FSubobjectEditorTreeNode;

namespace BlenderControls
{
	/**
	 * Manages a group of selected Blueprint components (SCSTreeNodes) as a single logical unit.
	 * Acts purely as a cache to remember where things started and what the active element is.
	 */
	class FSCSPivot : public FVirtualPivotBase
	{
	public:
		FSCSPivot(FBlueprintEditor* InBlueprintEditor, const TArray<TSharedPtr<FSubobjectEditorTreeNode>>& InNodes);
		virtual ~FSCSPivot() override = default;

		// FVirtualPivotBase interface
		virtual FVector GetStartLocation() const override { return StartPivotTransform.GetLocation(); }
		virtual FTransform GetActiveElementStartTransform() const override { return ActiveComponentStartTransform; }
		virtual FVector GetActiveElementCurrentLocation() const override;
		virtual void RevertToStartState() override;
		virtual bool IsValid() const override { return Nodes.Num() > 0; }
		virtual void BeginTransformSequence() override {}
		virtual void EndTransformSequence() override {}
		virtual void ApplyRotation(const FGrabContext& GC, float AngleRad, bool bUsingLocalSpace, EAxisLock LockedAxis) override;
		virtual void ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace) override;
		virtual void ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace) override;
		virtual void ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis) override;
		virtual void ForEachElementTransform(TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const override;
		virtual bool ApplyManualTransformDelta(const FVector& InDrag, const FRotator& InRot, const FVector& InScale) override;
		// ~FVirtualPivotBase interface

	private:
		void ComputeMedianPivot();
		void ComputeActiveElementPivot();

		struct FSCSNodeInfo
		{
			const FSubobjectData* CachedData = nullptr;
			FTransform StartTransform;
		};

		FTransform StartPivotTransform = FTransform::Identity;
		FTransform ActiveComponentStartTransform = FTransform::Identity;
		const FSubobjectData* ActiveCachedData = nullptr;
		TArray<FSCSNodeInfo> Nodes;
		FBlueprintEditor* BlueprintEditorPtr = nullptr;
	};
} // namespace BlenderControls
