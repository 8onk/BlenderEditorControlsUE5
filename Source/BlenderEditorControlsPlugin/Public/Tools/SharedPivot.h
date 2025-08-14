#pragma once

#include "CoreMinimal.h"
#include "EditorGizmos/TransformGizmo.h"
#include "BaseGizmos/TransformProxy.h"

namespace BlenderControls
{
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
		FVector GetLocation() const { return TransformProxy->GetTransform().GetLocation(); }
		UTransformProxy* GetTransformProxy() const { return TransformProxy; }
		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		const FChildInfo& GetActiveElement() const { return ActiveChild; }
		TArray<AActor*> GetSelectedActors() const;
		const TArray<FChildInfo>& GetChildren() const { return Children; }
		
		void SetPosition(const FVector& NewPosition);
		void RotateBy(const FQuat& Delta);
		void ScaleBy(const FVector& Scale, bool bUniform);

	private:
		void ComputePivotTransform(EPivotMode InPivotMode);
		void ComputeMedianPivot();
		void ComputeBoundingBoxCenterPivot();
		void ComputeActiveElementPivot();

		FTransform PivotTransform;
		FTransform StartPivotTransform;
		TArray<FChildInfo> Children;
		FChildInfo ActiveChild;
		UTransformGizmo* Gizmo = nullptr;
		UTransformProxy* TransformProxy = nullptr;
	};
} // namespace BlenderControls
