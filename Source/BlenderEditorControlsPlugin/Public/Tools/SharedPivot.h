#pragma once

#include "CoreMinimal.h"

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

        const FVector &GetPivot() const { return Pivot; }
        void MoveBy(const FVector &Delta);
        void RotateBy(const FQuat &Delta);
        void ScaleBy(const FVector &Scale, bool bUniform);

    private:
        void RecalcPivot();

        FVector Pivot = FVector::ZeroVector;
        TArray<FChildInfo> Children;
    };
} // namespace BlenderControls
