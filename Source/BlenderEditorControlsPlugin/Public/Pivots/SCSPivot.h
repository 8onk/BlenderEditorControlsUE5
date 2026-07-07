#pragma once

#include "CoreMinimal.h"
#include "Pivots/VirtualPivotBase.h"

class USceneComponent;

namespace BlenderControls
{
	/**
	 * Manages a group of selected Blueprint components (SCSTreeNodes) as a single logical unit.
	 * Acts purely as a cache to remember where things started and what the active element is.
	 */
	class FSCSPivot : public FVirtualPivotBase
	{
	public:
		explicit FSCSPivot(const TArray<USceneComponent*>& InSelection);
		virtual ~FSCSPivot() override = default;

		// FVirtualPivotBase interface
		virtual FVector GetStartLocation() const override { return StartPivotTransform.GetLocation(); }
		virtual FTransform GetActiveElementStartTransform() const override { return ActiveComponentStartTransform; }
		virtual FVector GetActiveElementCurrentLocation() const override;
		virtual void RevertToStartState() override;
		virtual bool IsValid() const override { return Components.Num() > 0; }
		// ~FVirtualPivotBase interface

	private:
		void ComputeMedianPivot();
		void ComputeActiveElementPivot();

		struct FComponentInfo
		{
			USceneComponent* Component;
			FTransform StartTransform;
		};

		FTransform StartPivotTransform = FTransform::Identity;
		FTransform ActiveComponentStartTransform = FTransform::Identity;
		USceneComponent* ActiveComponent = nullptr;
		TArray<FComponentInfo> Components;
	};
} // namespace BlenderControls
