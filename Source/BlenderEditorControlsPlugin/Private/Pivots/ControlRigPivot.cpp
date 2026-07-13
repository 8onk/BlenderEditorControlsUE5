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
			FControlRigSelectionHelper::SetElementLocalValue(Element.OwningControlRig.Get(), Element.ElementKey, Element.StartLocalValue);
		}
	}

	bool FControlRigPivot::ApplyManualTransformDelta(const FVector& InDrag, const FRotator& InRot, const FVector& InScale)
	{
		if (Elements.Num() == 0) return false;

		for (const FControlRigElementInfo& Element : Elements)
		{
			FTransform CurrentTransform = FControlRigSelectionHelper::GetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey);
			FTransform NewTransform = CurrentTransform;
			
			NewTransform.SetLocation(CurrentTransform.GetLocation() + InDrag);

			if (!InRot.IsZero())
			{
				NewTransform.SetRotation(InRot.Quaternion() * CurrentTransform.GetRotation());
			}

			if (!InScale.IsNearlyZero())
			{
				NewTransform.SetScale3D(CurrentTransform.GetScale3D() + InScale);
			}

			FControlRigSelectionHelper::SetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey, NewTransform, true);
		}

		return true;
	}

	FVector FControlRigPivot::GetActiveElementCurrentLocation() const
	{
		return FControlRigSelectionHelper::GetElementGlobalTransform(ActiveElement.OwningControlRig.Get(), ActiveElement.ElementKey).GetLocation();
	}

	void FControlRigPivot::ApplyRotation(const FGrabContext& GC, float AngleToRotateRad, bool bUsingLocalSpace, EAxisLock LockedAxis)
	{
		const FVector PivotPosition = GetStartLocation();
		const FVector RotationAxis = GC.SingleLockAxis;
		const FQuat TargetRotation = FQuat(RotationAxis, AngleToRotateRad);

		if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
		{
			for (const FControlRigElementInfo& Element : Elements)
			{
				const FTransform ElementTransform = Element.StartTransform;
				const FVector X = ElementTransform.GetUnitAxis(EAxis::X);
				const FVector Y = ElementTransform.GetUnitAxis(EAxis::Y);
				const FVector Z = ElementTransform.GetUnitAxis(EAxis::Z);

				// Map axis lock to rotation axis: single axis = rotate around that axis,
				// plane lock (XY/YZ/XZ) = rotate around the normal to that plane
				FVector LocalRotationAxis;
				switch (LockedAxis)
				{
				case EAxisLock::X: LocalRotationAxis = X; break;
				case EAxisLock::Y: LocalRotationAxis = Y; break;
				case EAxisLock::Z: LocalRotationAxis = Z; break;
				case EAxisLock::XY: LocalRotationAxis = Z; break;
				case EAxisLock::YZ: LocalRotationAxis = X; break;
				case EAxisLock::XZ: LocalRotationAxis = Y; break;
				case EAxisLock::All: LocalRotationAxis = GC.ViewForward; break;
				default: LocalRotationAxis = FVector::ZeroVector;
				}

				const FQuat TargetRot(LocalRotationAxis, AngleToRotateRad);
				const FVector CurrentLocation = ElementTransform.GetLocation();
				const FVector CurrentScale = ElementTransform.GetScale3D();
				
				// Rotate position around pivot, and concatenate rotation to existing orientation
				const FVector NewLocation = PivotPosition + TargetRot.RotateVector(CurrentLocation - PivotPosition);
				const FQuat NewRotation = (TargetRot * ElementTransform.GetRotation()).GetNormalized();

				FTransform NewTransform;
				NewTransform.SetScale3D(CurrentScale);
				NewTransform.SetLocation(NewLocation);
				NewTransform.SetRotation(NewRotation);

				FControlRigSelectionHelper::SetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey, NewTransform, true);
			}
		}
		else
		{
			for (const FControlRigElementInfo& Element : Elements)
			{
				const FTransform ElementTransform = Element.StartTransform;
				const FVector CurrentLocation = ElementTransform.GetLocation();
				const FVector NewLocation = PivotPosition + TargetRotation.RotateVector(CurrentLocation - PivotPosition);
				const FQuat NewRotation = (TargetRotation * ElementTransform.GetRotation()).GetNormalized();

				FTransform NewTransform = ElementTransform;
				NewTransform.SetLocation(NewLocation);
				NewTransform.SetRotation(NewRotation);

				FControlRigSelectionHelper::SetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey, NewTransform, true);
			}
		}
	}

	void FControlRigPivot::ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace)
	{
		for (const FControlRigElementInfo& Element : Elements)
		{
			const FTransform ElementInitialTransform = Element.StartTransform;
			const FQuat ElementRotation = ElementInitialTransform.GetRotation();
			const FVector PivotToElementVec = ElementInitialTransform.GetLocation() - GetStartLocation();

			FVector NewPosition, NewScale;
			if (bUsingLocalSpace)
			{
				// Scale the pivot-to-element offset in element's local space
				const FVector LocalPivotToElementVec = ElementRotation.UnrotateVector(PivotToElementVec);
				const FVector ScaledLocalPivotToElementVec = LocalPivotToElementVec * ScaleMultiplier;
				const FVector GlobalPivotToElementVec = ElementRotation.RotateVector(ScaledLocalPivotToElementVec);
				NewPosition = GetStartLocation() + GlobalPivotToElementVec;
				NewScale = ElementInitialTransform.GetScale3D() * ScaleMultiplier;
			}
			else
			{
				// Global scale on a rotated object: convert world-axis scale to local-axis equivalent
				// using similarity transform: R^-1 * S * R
				const FQuat InitialRotation = ElementInitialTransform.GetRotation();
				const FMatrix RotationMatrix = FRotationMatrix(InitialRotation.Rotator());
				const FMatrix GlobalScaleMatrix = FScaleMatrix(ScaleMultiplier);
				const FMatrix LocalEquivalentMatrix = RotationMatrix.Inverse() * GlobalScaleMatrix * RotationMatrix;
				const FVector LocalScaleToAdd = LocalEquivalentMatrix.GetScaleVector();

				// GetScaleVector returns absolute values; restore original signs from ScaleMultiplier
				FVector LocalScaleToAddSigned = LocalScaleToAdd;
				if (ScaleMultiplier.X < 0) LocalScaleToAddSigned.X *= -1.f;
				if (ScaleMultiplier.Y < 0) LocalScaleToAddSigned.Y *= -1.f;
				if (ScaleMultiplier.Z < 0) LocalScaleToAddSigned.Z *= -1.f;

				NewScale = ElementInitialTransform.GetScale3D() * LocalScaleToAddSigned;

				const FVector ScaledRelativePosition = PivotToElementVec * ScaleMultiplier;
				NewPosition = GetStartLocation() + ScaledRelativePosition;
			}

			FTransform NewTransform = ElementInitialTransform;
			NewTransform.SetLocation(NewPosition);
			NewTransform.SetScale3D(NewScale);

			FControlRigSelectionHelper::SetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey, NewTransform, true);
		}
	}

	void FControlRigPivot::ForEachElementTransform(TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const
	{
		for (const FControlRigElementInfo& Element : Elements)
		{
			const bool bIsActive = (Element.ElementKey == ActiveElement.ElementKey);
			Callback(Element.StartTransform, bIsActive);
		}
	}

	void FControlRigPivot::ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace)
	{
		for (const FControlRigElementInfo& Element : Elements)
		{
			FVector WorldSpaceOffset = LocalDelta;

			if (bUsingLocalSpace)
			{
				const FQuat ChildStartRotation = Element.StartTransform.GetRotation();
				WorldSpaceOffset = ChildStartRotation.RotateVector(LocalDelta);
			}

			FTransform NewTransform = Element.StartTransform;
			NewTransform.SetLocation(Element.StartTransform.GetLocation() + WorldSpaceOffset);

			FControlRigSelectionHelper::SetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey, NewTransform, true);
		}
	}

	void FControlRigPivot::ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis)
	{
		if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
		{
			const FQuat ActiveObjectStartRotation = ActiveElement.StartTransform.GetRotation();
			const FVector LocalSpaceDelta = ActiveObjectStartRotation.UnrotateVector(WorldDelta);

			for (const FControlRigElementInfo& Element : Elements)
			{
				const FTransform StartTransform = Element.StartTransform;
				const FVector WorldOffset = StartTransform.TransformPositionNoScale(LocalSpaceDelta);

				FTransform NewTransform = StartTransform;
				if (Element.ElementKey == ActiveElement.ElementKey)
				{
					NewTransform.SetLocation(StartTransform.GetLocation() + WorldDelta);
				}
				else
				{
					NewTransform.SetLocation(WorldOffset);
				}

				FControlRigSelectionHelper::SetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey, NewTransform, true);
			}
		}
		else
		{
			for (const FControlRigElementInfo& Element : Elements)
			{
				FTransform NewTransform = Element.StartTransform;
				NewTransform.SetLocation(Element.StartTransform.GetLocation() + WorldDelta);
				FControlRigSelectionHelper::SetElementGlobalTransform(Element.OwningControlRig.Get(), Element.ElementKey, NewTransform, true);
			}
		}
	}
} // namespace BlenderControls
