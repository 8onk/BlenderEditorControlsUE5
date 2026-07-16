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
		/** 
		 * Constructs the pivot manager for the given Control Rig selection.
		 * 
		 * @param InSelection The selected rig elements to manage.
		 * @param PivotMode The mode used to calculate the shared pivot center.
		 */
		explicit FControlRigPivot(const TArray<FControlRigElementInfo>& InSelection, EPivotMode PivotMode);
		virtual ~FControlRigPivot() override = default;

		//~ FVirtualPivotBase Interface
		virtual FVector GetStartLocation() const override { return StartPivotTransform.GetLocation(); }
		virtual FTransform GetActiveElementStartTransform() const override { return ActiveElement.StartTransform; }
		virtual FVector GetActiveElementCurrentLocation() const override;
		virtual void RevertToStartState() override;
		virtual bool IsValid() const override { return Elements.Num() > 0; }
		virtual void BeginTransformSequence() override {}
		virtual void EndTransformSequence() override {}
		virtual void ApplyRotation(const FGrabContext& GC, float AngleRad, bool bUsingLocalSpace, EAxisLock LockedAxis) override;
		virtual void ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace) override;
		virtual void ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace) override;
		virtual void ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis) override;
		virtual void ForEachElementTransform(TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const override;
		virtual bool ApplyManualTransformDelta(const FVector& InDrag, const FRotator& InRot, const FVector& InScale) override;

		//~ Pivot Accessors
		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		const TArray<FControlRigElementInfo>& GetElements() const { return Elements; }

	private:
		//~ Internal Helpers
		void ComputePivotTransform(EPivotMode InPivotMode);
		void ComputeMedianPivot();
		void ComputeActiveElementPivot();

		FTransform StartPivotTransform = FTransform::Identity;
		TArray<FControlRigElementInfo> Elements;
		FControlRigElementInfo ActiveElement;
	};
} // namespace BlenderControls
