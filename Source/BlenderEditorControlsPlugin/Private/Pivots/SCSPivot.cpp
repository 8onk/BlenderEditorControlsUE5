#include "Pivots/SCSPivot.h"
#include "Components/SceneComponent.h"
#include "Editor.h"

namespace BlenderControls
{
	FSCSPivot::FSCSPivot(const TArray<USceneComponent*>& InSelection)
	{
		for (USceneComponent* Component : InSelection)
		{
			if (Component)
			{
				FComponentInfo Info;
				Info.Component = Component;
				Info.StartTransform = Component->GetComponentTransform();
				Components.Add(Info);
			}
		}

		if (Components.Num() > 0)
		{
			ActiveComponent = Components.Last().Component;
			ActiveComponentStartTransform = Components.Last().StartTransform;
			ComputeMedianPivot();
		}
	}

	FVector FSCSPivot::GetActiveElementCurrentLocation() const
	{
		if (ActiveComponent)
		{
			return ActiveComponent->GetComponentLocation();
		}
		return FVector::ZeroVector;
	}

	void FSCSPivot::RevertToStartState()
	{
		for (const FComponentInfo& Info : Components)
		{
			if (Info.Component)
			{
				Info.Component->Modify();
				Info.Component->SetWorldTransform(Info.StartTransform);

				// Notify the editor subsystems (including the viewport) that this object has changed
				Info.Component->PostEditChange();

				// If it is an active instance in the world, force the render state to update
				if (Info.Component->IsRegistered())
				{
					Info.Component->MarkRenderTransformDirty();
					Info.Component->MarkRenderStateDirty();
				}
			}
		}
	}

	void FSCSPivot::ComputeMedianPivot()
	{
		FVector Accum = FVector::ZeroVector;
		for (const FComponentInfo& Info : Components)
		{
			Accum += Info.StartTransform.GetLocation();
		}

		const FVector PivotLocation = Accum / Components.Num();
		StartPivotTransform.SetLocation(PivotLocation);
		
		if (Components.Num() == 1)
		{
			StartPivotTransform.SetRotation(Components[0].StartTransform.GetRotation());
		}
		else
		{
			StartPivotTransform.SetRotation(FQuat::Identity);
		}
	}

	void FSCSPivot::ComputeActiveElementPivot()
	{
		if (ActiveComponent)
		{
			StartPivotTransform = ActiveComponentStartTransform;
		}
	}
} // namespace BlenderControls
