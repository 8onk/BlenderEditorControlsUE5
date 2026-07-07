#include "Pivots/ControlRigPivot.h"
#include "Enums.h"
#include "Tools/GrabContext.h"
#include "ControlRig/ControlRigSelectionHelper.h"

namespace BlenderControls
{
	FControlRigPivot::FControlRigPivot(const TArray<FControlRigElementInfo>& InSelection, const EPivotMode PivotMode)
	{
		Elements = InSelection;

		if (Elements.Num() > 0)
		{
			ActiveElement = Elements.Last();
		}

		ComputePivotTransform(PivotMode);
	}

	void FControlRigPivot::ComputePivotTransform(const EPivotMode InPivotMode)
	{
		if (Elements.Num() == 0)
		{
			return;
		}

		switch (InPivotMode)
		{
		case EPivotMode::MedianPoint:
			ComputeMedianPivot();
			break;
		case EPivotMode::ActiveElement:
			ComputeActiveElementPivot();
			break;
		default:
			ComputeMedianPivot();
			break;
		}

		// Single selection: inherit element rotation for local-space gizmo alignment
		// Multi-selection: use world axes (identity) since elements may have different orientations
		if (Elements.Num() == 1)
		{
			StartPivotTransform.SetRotation(Elements[0].StartRotation);
		}
		else
		{
			StartPivotTransform.SetRotation(FQuat::Identity);
		}
	}

	void FControlRigPivot::ComputeMedianPivot()
	{
		FVector Accum = FVector::ZeroVector;
		for (const FControlRigElementInfo& Element : Elements)
		{
			Accum += Element.StartTransform.GetLocation();
		}

		const FVector PivotLocation = Accum / Elements.Num();
		StartPivotTransform.SetLocation(PivotLocation);
	}

	void FControlRigPivot::ComputeActiveElementPivot()
	{
		if (ActiveElement.IsValid())
		{
			StartPivotTransform = ActiveElement.StartTransform;
		}
	}

	void FControlRigPivot::RevertToStartState()
	{
		for (const FControlRigElementInfo& Element : Elements)
		{
			FControlRigSelectionHelper::SetElementGlobalTransform(Element.ElementKey, Element.StartTransform, true);
		}
	}
} // namespace BlenderControls
