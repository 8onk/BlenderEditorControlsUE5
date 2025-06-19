#include "Tools/SharedPivot.h"

namespace BlenderControls
{
    FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>> &Selection)
    {
        for (auto &APtr : Selection)
        {
            if (APtr.IsValid())
            {
                AActor *A = APtr.Get();

                // Uses bounding box centre for now only, maybe expand to be able to choose.
                FVector Origin, Extent;
                A->GetActorBounds(false, Origin, Extent);

                Children.Add({A, A->GetActorTransform(), FVector::ZeroVector});
                Pivot += Origin; // use bounds center
            }
        }

        if (Children.Num())
        {
            Pivot /= Children.Num();
        }
        for (FChildInfo &Child : Children)
        {
            Child.Offset = Child.Actor->GetActorLocation() - Pivot;
        }
    }

    void FSharedPivot::MoveBy(const FVector &Delta)
    {
        Pivot += Delta;
        for (FChildInfo &Child : Children)
        {
            Child.Actor->Modify(); // undo safety
            Child.Actor->SetActorLocation(Pivot + Child.Offset + Delta, false, nullptr, ETeleportType::None);
        }
    }
} // namespace BlenderControls
