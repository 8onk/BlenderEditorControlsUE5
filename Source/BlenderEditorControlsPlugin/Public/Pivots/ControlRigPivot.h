#pragma once

#include "CoreMinimal.h"
#include "ControlRig/ControlRigSelectionHelper.h"
#include "Pivots/VirtualPivotBase.h"

namespace BlenderControls
{
	enum class EAxisLock : uint8;
	struct FGrabContext;
	enum class EPivotMode : uint8;

	/**
	 * Manages a group of selected Control Rig elements as a single transformable unit.
	 * Analogous to FActorPivot for actors, but operates on Control Rig hierarchy elements.
	 */
	class FControlRigPivot : public FVirtualPivotBase
	{
	public:
		explicit FControlRigPivot(const TArray<FControlRigElementInfo>& InSelection, EPivotMode PivotMode);
		virtual ~FControlRigPivot() override = default;

		// FVirtualPivotBase interface
		virtual FVector GetStartLocation() const override { return StartPivotTransform.GetLocation(); }
		virtual FTransform GetActiveElementStartTransform() const override { return ActiveElement.StartTransform; }
		virtual FVector GetActiveElementCurrentLocation() const override { return FControlRigSelectionHelper::GetElementGlobalTransform(ActiveElement.ElementKey).GetLocation(); }
		virtual void RevertToStartState() override;
		virtual bool IsValid() const override { return Elements.Num() > 0; }
		virtual bool ApplyManualTransformDelta(const FVector& InDrag, const FRotator& InRot, const FVector& InScale) override;
		// ~FVirtualPivotBase interface

		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		const TArray<FControlRigElementInfo>& GetElements() const { return Elements; }

	private:
		void ComputePivotTransform(EPivotMode InPivotMode);
		void ComputeMedianPivot();
		void ComputeActiveElementPivot();

		FTransform StartPivotTransform = FTransform::Identity;
		TArray<FControlRigElementInfo> Elements;
		FControlRigElementInfo ActiveElement;
	};
} // namespace BlenderControls
