#include "Tools/SharedPivot.h"

namespace BlenderControls
{
    FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>> &Selection)
    {
        for (TWeakObjectPtr<AActor> A : Selection)
        {
            if (A.IsValid())
            {
                Children.Add({A.Get(), A->GetActorTransform(), FVector::ZeroVector});
                Pivot += A->GetActorLocation();
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
