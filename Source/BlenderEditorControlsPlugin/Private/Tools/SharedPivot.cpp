#include "Tools/SharedPivot.h"
//TODO, Cancelling, resets the pivot's rotation also. 

namespace BlenderControls
{
	FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>>& InSelection)
	{
		TransformProxy = NewObject<UTransformProxy>();
		if (!IsValid(TransformProxy))
		{
			return;
		}
		TransformProxy->AddToRoot();

		FVector AverageLocation = FVector::ZeroVector;
		for (auto& APtr : InSelection)
		{
			if (APtr.IsValid())
			{
				AActor* Actor = APtr.Get();

				// Uses bounding box centre for now only, maybe expand to be able to choose.
				FVector Origin, Extent;
				Actor->GetActorBounds(false, Origin, Extent);

				Children.Add({Actor, Actor->GetActorTransform(), FVector::ZeroVector});
				TransformProxy->AddComponent(Actor->GetRootComponent(), true);
				AverageLocation += Origin; // use bounds center
			}
		}

		if (Children.Num())
		{
			AverageLocation /= Children.Num();
		}

		//NOTE: THIS BEHAVES ODDLY FOR EMPTY OBJECTS LIKE PLAYER START, WHERE X = 56 BUT 0 IN TRANSFORM
		Pivot.SetLocation(AverageLocation);
		StartTransform = Pivot;

		for (FChildInfo& Child : Children)
		{
			Child.Offset = Child.Actor->GetActorLocation() - Pivot.GetLocation();
			TransformProxy->GetTransform().SetRotation(Child.Transform.GetRotation());
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

	void FSharedPivot::SetPosition(const FVector& NewPosition)
	{
		if (IsValid(TransformProxy))
		{
			const FTransform Current = TransformProxy->GetTransform();
			Pivot.SetLocation(NewPosition);
			Pivot.SetRotation(Current.GetRotation());
			Pivot.SetScale3D(Current.GetScale3D());
			TransformProxy->SetTransform(Pivot);
		}
	}
} // namespace BlenderControls
