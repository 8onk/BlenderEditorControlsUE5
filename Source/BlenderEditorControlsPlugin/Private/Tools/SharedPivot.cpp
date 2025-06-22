#include "Tools/SharedPivot.h"

#include "BlenderEditorControlsPlugin.h"
#include "LevelEditorViewport.h"

namespace BlenderControls
{
	FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>>& Selection)
	{
		TransformProxy = NewObject<UTransformProxy>();

		if (!TransformProxy)
		{
			return;
		}

		for (auto& APtr : Selection)
		{
			if (APtr.IsValid())
			{
				AActor* A = APtr.Get();

				// Uses bounding box centre for now only, maybe expand to be able to choose.
				FVector Origin, Extent;
				A->GetActorBounds(false, Origin, Extent);
				UE_LOG(LogTemp, Log, TEXT("Actor: %s | Location: %s | Bounds Origin: %s"),
				       *A->GetName(),
				       *A->GetActorLocation().ToString(),
				       *Origin.ToString());

				Children.Add({A, A->GetActorTransform(), FVector::ZeroVector});
				TransformProxy->AddComponent(A->GetRootComponent(), true);
				Pivot.GetLocation() += Origin; // use bounds center
			}
		}

		if (Children.Num())
		{
			Pivot.GetLocation() /= Children.Num();
		}
		for (FChildInfo& Child : Children)
		{
			Child.Offset = Child.Actor->GetActorLocation() - Pivot.GetLocation();
		}
	}

	void FSharedPivot::MoveBy(const FVector& Delta)
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
