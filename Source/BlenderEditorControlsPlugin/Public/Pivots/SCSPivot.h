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
