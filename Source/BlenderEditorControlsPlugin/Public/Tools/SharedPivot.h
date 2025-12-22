#pragma once

#include "CoreMinimal.h"

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
	 * Responsible for calculating pivot points (Median, Active, etc. which dont exist yet) and
	 * applying bulk transformations (Translate/Rotate/Scale) to children.
	*/
	class FSharedPivot
	{
	public:
		explicit FSharedPivot(const TArray<TWeakObjectPtr<AActor>>& InSelection, EPivotMode PivotMode);
		~FSharedPivot();

		const FTransform& GetPivot() const { return PivotTransform; }
		FVector GetStartLocation() const { return StartPivotTransform.GetLocation(); }
		UTransformProxy* GetTransformProxy() const { return TransformProxy; }
		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		const FChildInfo& GetActiveElement() const { return ActiveChild; }
		TArray<AActor*> GetSelectedActors() const;
		const TArray<FChildInfo>& GetChildren() const { return Children; }

		void SetPosition(const FVector& NewPosition);
		void Translate(const FVector& Delta, const bool bInUsingLocalSpace);
		void Translate(bool bUsingLocalSpace, EAxisLock LockedAxis, const FVector& Delta);
		void Rotate(FGrabContext GC, float AngleToRotateRad, bool bUsingLocalSpace, EAxisLock LockedAxis);
		void Scale(FVector ScaleMultiplier, bool bUsingLocalSpace);

		void RevertToStartState();

	private:
		void ComputePivotTransform(EPivotMode InPivotMode);
		void ComputeMedianPivot();
		void ComputeBoundingBoxCenterPivot();
		void ComputeActiveElementPivot();

		FTransform PivotTransform = FTransform::Identity;
		FTransform StartPivotTransform = FTransform::Identity;
		TArray<FChildInfo> Children;
		FChildInfo ActiveChild;
		UTransformProxy* TransformProxy = nullptr;
	};
} // namespace BlenderControls
