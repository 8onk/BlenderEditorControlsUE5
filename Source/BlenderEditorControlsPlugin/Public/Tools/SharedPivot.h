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
		FTransform Original;
		FVector Offset;
	};

	class FSharedPivot
	{
	public:
		explicit FSharedPivot(const TArray<TWeakObjectPtr<AActor>> &InSelection);
		~FSharedPivot();

		const FTransform &GetPivot() const { return Pivot; }
		UTransformProxy *GetTransformProxy() const { return TransformProxy; }
		const FTransform &GetStartTransform() const { return StartLocation; }

		void SetPosition(const FVector &NewPosition);
		void RotateBy(const FQuat &Delta);
		void ScaleBy(const FVector &Scale, bool bUniform);
		void SetStartTransformPosition(const FVector &InPosition);

	private:
		void RecalcPivot();

		FTransform Pivot;
		FTransform StartLocation;
		TArray<FChildInfo> Children;
		UTransformGizmo *Gizmo = nullptr;
		UTransformProxy *TransformProxy = nullptr;
	};
} // namespace BlenderControls
