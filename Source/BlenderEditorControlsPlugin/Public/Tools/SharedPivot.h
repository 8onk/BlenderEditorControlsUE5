#pragma once

#include "CoreMinimal.h"
#include "EditorGizmos/TransformGizmo.h"
#include "BaseGizmos/TransformProxy.h"
//#include "InputState.h"

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

        const FTransform &GetPivot() const { return Pivot; }
        UTransformProxy* GetTransformProxy() const { return TransformProxy; }
        
        void MoveBy(const FVector &Delta);
        void RotateBy(const FQuat &Delta);
        void ScaleBy(const FVector &Scale, bool bUniform);

    private:
        void RecalcPivot();

        FTransform Pivot;
        TArray<FChildInfo> Children;
        UTransformGizmo *Gizmo = nullptr;
        UTransformProxy *TransformProxy = nullptr;
    };
} // namespace BlenderControls
