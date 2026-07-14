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
		TWeakObjectPtr<AActor> OwnerActor;
		TWeakObjectPtr<USceneComponent> Component;
		bool bIsRootComponent = false;
		FTransform StartTransform;
	};

	/**
	 * Manages a group of selected actors as a single logical unit.
	 * Responsible for calculating pivot points and reverting state.
	*/
	class FActorPivot : public FVirtualPivotBase
	{
	public:
		explicit FActorPivot(const TArray<TWeakObjectPtr<USceneComponent>>& InSelection, EPivotMode PivotMode);
		virtual ~FActorPivot() override;

		// FVirtualPivotBase interface
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
		// ~FVirtualPivotBase interface

		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		TArray<USceneComponent*> GetSelectedComponents() const;
		const TArray<FSelection>& GetChildren() const { return Children; }

	private:
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
