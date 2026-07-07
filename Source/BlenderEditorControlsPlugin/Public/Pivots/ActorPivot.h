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

	struct FChildInfo
	{
		AActor* Actor;
		FTransform Transform;
		FQuat Rotation;
	};

	/**
	 * Manages a group of selected actors as a single logical unit.
	 * Responsible for calculating pivot points and reverting state.
	*/
	class FActorPivot : public FVirtualPivotBase
	{
	public:
		explicit FActorPivot(const TArray<TWeakObjectPtr<AActor>>& InSelection, EPivotMode PivotMode);
		virtual ~FActorPivot() override;

		// FVirtualPivotBase interface
		virtual FVector GetStartLocation() const override { return StartPivotTransform.GetLocation(); }
		virtual FTransform GetActiveElementStartTransform() const override { return ActiveChild.Transform; }
		virtual FVector GetActiveElementCurrentLocation() const override { return ActiveChild.Actor ? ActiveChild.Actor->GetActorLocation() : FVector::ZeroVector; }
		virtual void RevertToStartState() override;
		virtual bool IsValid() const override { return Children.Num() > 0; }
		virtual void BeginTransformSequence() override;
		virtual void EndTransformSequence() override;
		// ~FVirtualPivotBase interface

		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		TArray<AActor*> GetSelectedActors() const;
		const TArray<FChildInfo>& GetChildren() const { return Children; }

	private:
		void ComputePivotTransform(EPivotMode InPivotMode);
		void ComputeMedianPivot();
		void ComputeBoundingBoxCenterPivot();
		void ComputeActiveElementPivot();

		FTransform StartPivotTransform = FTransform::Identity;
		TArray<FChildInfo> Children;
		FChildInfo ActiveChild;
		UTransformProxy* TransformProxy = nullptr;
	};
} // namespace BlenderControls
