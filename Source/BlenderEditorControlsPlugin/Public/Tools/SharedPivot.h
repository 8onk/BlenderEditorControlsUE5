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
		void RotateBy(const FQuat& Delta);
		void ScaleBy(const FVector& Scale, bool bUniform);
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
		UTransformGizmo* Gizmo = nullptr;
		UTransformProxy* TransformProxy = nullptr;
	};
} // namespace BlenderControls
