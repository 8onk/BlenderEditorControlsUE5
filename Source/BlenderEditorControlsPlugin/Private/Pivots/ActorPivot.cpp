#include "Pivots/ActorPivot.h"
#include "Enums.h"
#include "BaseGizmos/TransformProxy.h"
#include "Tools/GrabContext.h"

namespace BlenderControls
{
	//Uses median point by default for pivot
	FActorPivot::FActorPivot(const TArray<TWeakObjectPtr<AActor>>& InSelection, const EPivotMode PivotMode)
	{
		TransformProxy = NewObject<UTransformProxy>();
		if (!TransformProxy)
		{
			return;
		}
		TransformProxy->AddToRoot(); //Prevents garbage collection

		for (const TWeakObjectPtr<AActor>& ActorPtr : InSelection)
		{
			if (AActor* Actor = ActorPtr.Get()) 
			{
				const FTransform ActorTransform = Actor->GetTransform();
				const FQuat ActorRotation = ActorTransform.GetRotation();

				FChildInfo Child = {Actor, ActorTransform, ActorRotation};
				Children.Add(Child);

				constexpr bool bModifyComponentOnTransform = true;
				TransformProxy->AddComponent(Actor->GetRootComponent(), bModifyComponentOnTransform);
			}
		}
		if (Children.Num() > 0)
		{
			ActiveChild = Children.Last();
			ComputePivotTransform(PivotMode);
		}
	}

	FActorPivot::~FActorPivot()
	{
		if (TransformProxy)
		{
			TransformProxy->RemoveFromRoot();
			TransformProxy = nullptr;
		}
	}

	void FActorPivot::BeginTransformSequence()
	{
		if (TransformProxy)
		{
			TransformProxy->BeginTransformEditSequence();
		}
	}

	void FActorPivot::EndTransformSequence()
	{
		if (TransformProxy)
		{
			TransformProxy->EndTransformEditSequence();
		}
	}

	void FActorPivot::ComputePivotTransform(const EPivotMode InPivotMode)
	{
		switch (InPivotMode)
		{
		case EPivotMode::MedianPoint:
			ComputeMedianPivot();
			break;
		case EPivotMode::BoundingBoxCenter:
			ComputeBoundingBoxCenterPivot();
			break;
		case EPivotMode::IndividualOrigins:
			StartPivotTransform = FTransform::Identity;
			break;
		case EPivotMode::ActiveElement:
			ComputeActiveElementPivot();
			break;
		}

		if (Children.Num() == 1)
		{
			StartPivotTransform.SetRotation(Children[0].Rotation);
			StartPivotTransform.SetScale3D(Children[0].Transform.GetScale3D());
		}
		else
		{
			StartPivotTransform.SetRotation(FQuat::Identity);
		}

		if (TransformProxy)
		{
			TransformProxy->SetTransform(StartPivotTransform);
		}
	}

	void FActorPivot::ComputeMedianPivot()
	{
		FVector Accum = FVector::ZeroVector;
		for (const auto& Child : Children)
		{
			Accum += Child.Actor->GetActorLocation();
		}

		const FVector PivotLocation = Accum / Children.Num();
		StartPivotTransform.SetLocation(PivotLocation);
	}

	void FActorPivot::ComputeBoundingBoxCenterPivot()
	{
		FBox BoundingBox(ForceInit);

		for (auto& Child : Children)
		{
			FVector Origin, Extent;
			constexpr bool bOnlyCollidingComponents = false;
			Child.Actor->GetActorBounds(bOnlyCollidingComponents, Origin, Extent);
			BoundingBox += Origin;
		}

		StartPivotTransform.SetLocation(BoundingBox.GetCenter());
	}

	void FActorPivot::ComputeActiveElementPivot()
	{
		if (Children.Last().Actor)
		{
			StartPivotTransform = Children.Last().Transform;
		}
	}

	void FActorPivot::RevertToStartState()
	{
		for (const FChildInfo& Child : Children)
		{
			if (Child.Actor)
			{
				Child.Actor->SetActorTransform(Child.Transform);
			}
		}

		if (!TransformProxy)
		{
			TransformProxy->SetTransform(StartPivotTransform);
		}
	}

	TArray<AActor*> FActorPivot::GetSelectedActors() const
	{
		TArray<AActor*> Result;
		Result.Reserve(Children.Num());

		for (const FChildInfo& Child : Children)
		{
			if (Child.Actor)
			{
				Result.Add(Child.Actor);
			}
		}

		return Result;
	}
} // namespace BlenderControls
