#pragma once

#include "CoreMinimal.h"
#include "EditorGizmos/TransformGizmo.h"
#include "BaseGizmos/TransformProxy.h"
// #include "InputState.h"

namespace BlenderControls
{
	struct FChildInfo
	{
		AActor *Actor;
		FTransform Transform;
		//FQuat Rotation;
		FVector Offset;
	};

	class FSharedPivot
	{
	public:
		explicit FSharedPivot(const TArray<TWeakObjectPtr<AActor>> &InSelection);
		~FSharedPivot();

		const FTransform &GetPivot() const { return Pivot; }
		const FVector GetCurrentLocation() const { return TransformProxy->GetTransform().GetLocation(); }
		UTransformProxy *GetTransformProxy() const { return TransformProxy; }
		const FTransform &GetStartTransform() const { return StartTransform; }

		void SetPosition(const FVector &NewPosition);
		void RotateBy(const FQuat &Delta);
		void ScaleBy(const FVector &Scale, bool bUniform);

	private:
		void RecalcPivot();

		FTransform Pivot;
		FTransform StartTransform;
		TArray<FChildInfo> Children;
		UTransformGizmo *Gizmo = nullptr;
		UTransformProxy *TransformProxy = nullptr;
	};
} // namespace BlenderControls
