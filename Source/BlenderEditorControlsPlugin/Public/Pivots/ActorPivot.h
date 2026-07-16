#pragma once

#include "CoreMinimal.h"
#include "Pivots/VirtualPivotBase.h"

class UTransformGizmo;
class UTransformProxy;

namespace BlenderControls
{
	enum class EAxisLock : uint8;
	struct FGrabContext;
	enum class EPivotMode : uint8;

	struct FSelection
	{
		/** The owning actor for this selection. */
		TWeakObjectPtr<AActor> OwnerActor;
		
		/** The specific scene component being transformed. */
		TWeakObjectPtr<USceneComponent> Component;
		
		/** True if this component is the root component of the actor. */
		bool bIsRootComponent = false;
		
		/** Cached starting transform of the component. */
		FTransform StartTransform;
	};

	/**
	 * Manages a group of selected actors as a single logical unit.
	 * Responsible for calculating pivot points and reverting state.
	*/
	class FActorPivot : public FVirtualPivotBase
	{
	public:
		/** 
		 * Constructs the pivot manager for the given selection.
		 * 
		 * @param InSelection The selected scene components to manage.
		 * @param PivotMode The mode used to calculate the shared pivot center.
		 */
		explicit FActorPivot(const TArray<TWeakObjectPtr<USceneComponent>>& InSelection, EPivotMode PivotMode);
		virtual ~FActorPivot() override;

		//~ FVirtualPivotBase Interface
		virtual FVector GetStartLocation() const override { return StartPivotTransform.GetLocation(); }
		virtual FTransform GetActiveElementStartTransform() const override { return ActiveChild.StartTransform; }
		virtual FVector GetActiveElementCurrentLocation() const override;
		virtual void RevertToStartState() override;
		virtual bool IsValid() const override { return Children.Num() > 0; }
		virtual void BeginTransformSequence() override;
		virtual void EndTransformSequence() override;
		virtual void ApplyRotation(const FGrabContext& GC, float AngleRad, bool bUsingLocalSpace,
		                           EAxisLock LockedAxis) override;
		virtual void ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace) override;
		virtual void ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace) override;
		virtual void ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis) override;
		virtual void ForEachElementTransform(
			TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const override;

		//~ Pivot Accessors
		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		TArray<USceneComponent*> GetSelectedComponents() const;
		const TArray<FSelection>& GetChildren() const { return Children; }

	private:
		//~ Internal Helpers
		static USceneComponent* ResolveComponent(const FSelection& Child);

		void ComputePivotTransform(EPivotMode InPivotMode);
		void ComputeMedianPivot();
		void ComputeBoundingBoxCenterPivot();
		void ComputeActiveElementPivot();

		FTransform StartPivotTransform = FTransform::Identity;
		TArray<FSelection> Children;
		FSelection ActiveChild;
		UTransformProxy* TransformProxy = nullptr;
	};
} // namespace BlenderControls
