#include "Tools/SharedPivot.h"
#include "BlenderEditorControlsEnums.h"

namespace BlenderControls
{
	//Uses median point by default for pivot
	FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>>& InSelection, const EPivotMode PivotMode)
	{
		TransformProxy = NewObject<UTransformProxy>();
		if (!IsValid(TransformProxy))
		{
			return;
		}
		TransformProxy->AddToRoot(); //Prevents garbage collection

		for (const TWeakObjectPtr<AActor>& ActorPtr : InSelection)
		{
			if (AActor* Actor = ActorPtr.Get()) //is actor valid and safe to use?
			{
				const FTransform ActorTransform = Actor->GetTransform();
				const FQuat ActorRotation = ActorTransform.GetRotation();

				FChildInfo Child = {Actor, ActorTransform, ActorRotation};
				Children.Add(Child);

				constexpr bool bModifyComponentOnTransform = true;
				TransformProxy->AddComponent(Actor->GetRootComponent(), bModifyComponentOnTransform);
			}
		}
		ActiveChild = Children.Last();

		ComputePivotTransform(PivotMode);
	}


	FSharedPivot::~FSharedPivot()
	{
		if (TransformProxy)
		{
			TransformProxy->RemoveFromRoot();
			TransformProxy = nullptr;
		}
	}

	void FSharedPivot::ComputePivotTransform(const EPivotMode InPivotMode)
	{
		switch (InPivotMode)
		{
		case EPivotMode::MedianPoint:
			ComputeMedianPivot();
			break;
		case EPivotMode::BoundingBoxCenter:
			ComputeBoundingBoxCenterPivot();
			break;
		// case EPivotMode::ThreeDCursor:
		// 	Compute3DCursorPivot();
		// 	break;
		case EPivotMode::IndividualOrigins:
			PivotTransform = FTransform::Identity;
			break;
		case EPivotMode::ActiveElement:
			ComputeActiveElementPivot();
			break;
		}

		if (Children.Num() == 1)
		{
			PivotTransform.SetRotation(Children[0].Rotation);
		}
		else
		{
			PivotTransform.SetRotation(FQuat::Identity);
		}
		PivotTransform.SetScale3D(Children[0].Transform.GetScale3D());

		TransformProxy->SetTransform(PivotTransform);
		StartPivotTransform = TransformProxy->GetTransform();
	}

	void FSharedPivot::ComputeMedianPivot()
	{
		FVector Accum = FVector::ZeroVector;
		for (const auto& Child : Children)
		{
			Accum += Child.Actor->GetActorLocation();
		}

		const FVector PivotLocation = Accum / Children.Num();

		PivotTransform.SetLocation(PivotLocation);
	}

	void FSharedPivot::ComputeBoundingBoxCenterPivot()
	{
		FBox BoundingBox(ForceInit);

		for (auto& Child : Children)
		{
			FVector Origin, Extent;
			constexpr bool bOnlyCollidingComponents = false;
			Child.Actor->GetActorBounds(bOnlyCollidingComponents, Origin, Extent);
			BoundingBox += Origin;
		}

		PivotTransform.SetLocation(BoundingBox.GetCenter());
	}

	void FSharedPivot::ComputeActiveElementPivot()
	{
		if (Children.Last().Actor)
		{
			PivotTransform = Children.Last().Transform;
		}
	}

	TArray<AActor*> FSharedPivot::GetSelectedActors() const
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

	void FSharedPivot::SetPosition(const FVector& NewPosition)
	{
		if (IsValid(TransformProxy))
		{
			const FTransform CurrentTransform = TransformProxy->GetTransform();
			PivotTransform.SetLocation(NewPosition);
			PivotTransform.SetRotation(CurrentTransform.GetRotation());
			PivotTransform.SetScale3D(CurrentTransform.GetScale3D());
			TransformProxy->SetTransform(PivotTransform);
		}
	}
} // namespace BlenderControls
