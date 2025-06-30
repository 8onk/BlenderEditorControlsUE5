#include "Tools/SharedPivot.h"

namespace BlenderControls
{
	FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>> &Selection)
	{
		TransformProxy = NewObject<UTransformProxy>();
		TransformProxy->AddToRoot();
		if (!IsValid(TransformProxy))
		{
			return;
		}

		FVector AverageLocation = FVector::ZeroVector;
		for (auto &APtr : Selection)
		{
			if (APtr.IsValid())
			{
				AActor *A = APtr.Get();

				// Uses bounding box centre for now only, maybe expand to be able to choose.
				FVector Origin, Extent;
				A->GetActorBounds(false, Origin, Extent);

				Children.Add({A, A->GetActorTransform(), FVector::ZeroVector});
				TransformProxy->AddComponent(A->GetRootComponent(), true);
				AverageLocation += Origin; // use bounds center
			}
		}

		if (Children.Num())
		{
			AverageLocation /= Children.Num();
		}

		//NOTE: THIS BEHAVES ODDLY FOR EMPTY OBJECTS LIKE PLAYER START, WHERE X = 56 BUT 0 IN TRANSFORM
		Pivot.SetLocation(AverageLocation);
		StartLocation = Pivot;

		for (FChildInfo &Child : Children)
		{
			Child.Offset = Child.Actor->GetActorLocation() - Pivot.GetLocation();
		}
	}

	FSharedPivot::~FSharedPivot()
	{
		if (TransformProxy)
		{
			TransformProxy->RemoveFromRoot();
			TransformProxy = nullptr;
		}
	}

	void FSharedPivot::SetPosition(const FVector &NewPosition)
	{
		if (IsValid(TransformProxy))
		{
			Pivot.SetLocation(NewPosition);
			TransformProxy->SetTransform(Pivot);
		}
	}

	void FSharedPivot::SetStartTransformPosition(const FVector &InPosition)
	{
		StartLocation.SetLocation(InPosition);
	}
} // namespace BlenderControls
