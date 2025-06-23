#include "Tools/SharedPivot.h"

#include "BlenderEditorControlsPlugin.h"
#include "LevelEditorViewport.h"

namespace BlenderControls
{
	FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>> &Selection)
	{
		TransformProxy = NewObject<UTransformProxy>();
		if (!TransformProxy)
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
		
		Pivot.SetLocation(AverageLocation);
		StartLocation = Pivot;
		
		for (FChildInfo &Child : Children)
		{
			Child.Offset = Child.Actor->GetActorLocation() - Pivot.GetLocation();
		}
	}

	void FSharedPivot::MoveBy(const FVector &Delta)
	{
		if (!TransformProxy)
		{
			return;
		}

		// Update internal pivot state (if needed for your custom logic)
		Pivot.AddToTranslation(Delta);
		TransformProxy->SetTransform(Pivot);
	}
} // namespace BlenderControls
