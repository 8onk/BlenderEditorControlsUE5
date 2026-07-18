#include "Pivots/ActorPivot.h"
#include "Enums.h"
#include "BaseGizmos/TransformProxy.h"
#include "Tools/GrabContext.h"
#include "Components/SceneComponent.h"

namespace BlenderControls
{
	//Uses median point by default for pivot
	FActorPivot::FActorPivot(const TArray<TWeakObjectPtr<USceneComponent>>& InSelection, const EPivotMode PivotMode)
	{
		TransformProxy = NewObject<UTransformProxy>();
		if (!TransformProxy)
		{
			return;
		}
		TransformProxy->AddToRoot();

		for (const TWeakObjectPtr<USceneComponent>& CompPtr : InSelection)
		{
			if (USceneComponent* Comp = CompPtr.Get())
			{
				FSelection Child;
				Child.Component = Comp;
				Child.StartTransform = Comp->GetComponentTransform();
				
				if (AActor* Owner = Comp->GetOwner())
				{
					Child.OwnerActor = Owner;
					Child.bIsRootComponent = (Owner->GetRootComponent() == Comp);
				}

				Children.Add(Child);

				constexpr bool bModifyComponentOnTransform = true;
				TransformProxy->AddComponent(Comp, bModifyComponentOnTransform);
			}
		}

		if (Children.Num() > 0)
		{
			ActiveChild = Children.Last();
			ComputePivotTransform(PivotMode);
		}
	}

	USceneComponent* FActorPivot::ResolveComponent(const FSelection& Child)
	{
		if (Child.bIsRootComponent)
		{
			if (const AActor* Owner = Child.OwnerActor.Get())
			{
				return Owner->GetRootComponent();
			}
		}
		
		return Child.Component.Get();
	}

	FVector FActorPivot::GetActiveElementCurrentLocation() const
	{
		if (USceneComponent* Comp = ResolveComponent(ActiveChild))
		{
			return Comp->GetComponentLocation();
		}
		return FVector::ZeroVector;
	}

	void FActorPivot::ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace)
	{
		for (const FSelection& Child : Children)
		{
			FVector WorldSpaceOffset = LocalDelta;

			if (bUsingLocalSpace)
			{
				const FQuat ChildStartRotation = Child.StartTransform.GetRotation();
				WorldSpaceOffset = ChildStartRotation.RotateVector(LocalDelta);
			}

			const FVector StartPos = Child.StartTransform.GetLocation();
			if (USceneComponent* Comp = ResolveComponent(Child))
			{
				Comp->SetWorldLocation(StartPos + WorldSpaceOffset);
			}
		}
	}

	void FActorPivot::ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis)
	{
		if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
		{
			const FSelection& ActiveElement = Children.Last();
			const FQuat ActiveObjectStartRotation = ActiveElement.StartTransform.GetRotation();
			const FVector LocalSpaceDelta = ActiveObjectStartRotation.UnrotateVector(WorldDelta);
			
			USceneComponent* ActiveComp = ResolveComponent(ActiveElement);

			for (const FSelection& Child : Children)
			{
				const FTransform StartTransform = Child.StartTransform;
				const FVector WorldOffset = StartTransform.TransformPositionNoScale(LocalSpaceDelta);

				if (USceneComponent* Comp = ResolveComponent(Child))
				{
					if (Comp == ActiveComp)
					{
						Comp->SetWorldLocation(StartTransform.GetLocation() + WorldDelta);
					}
					else
					{
						Comp->SetWorldLocation(WorldOffset);
					}
				}
			}
		}
		else
		{
			const FVector NewPos = StartPivotTransform.GetLocation() + WorldDelta;
			for (const FSelection& Child : Children)
			{
				if (USceneComponent* Comp = ResolveComponent(Child))
				{
					Comp->SetWorldLocation(Child.StartTransform.GetLocation() + WorldDelta);
				}
			}

			if (TransformProxy)
			{
				FTransform NewPivotTransform = StartPivotTransform;
				NewPivotTransform.SetLocation(NewPos);
				TransformProxy->SetTransform(NewPivotTransform);
			}
		}
	}

	void FActorPivot::ApplyRotation(const FGrabContext& GC, float AngleToRotateRad, bool bUsingLocalSpace,
	                                EAxisLock LockedAxis)
	{
		const FVector PivotPosition = GetStartLocation();
		const bool bIsLocalRotation = (bUsingLocalSpace && LockedAxis != EAxisLock::All);

		for (const FSelection& Child : Children)
		{
			if (USceneComponent* Comp = ResolveComponent(Child))
			{
				const FTransform& ChildTransform = Child.StartTransform;

				// Default to the global/freeform lock axis
				FVector Axis = GC.SingleLockAxis;

				if (bIsLocalRotation)
				{
					switch (LockedAxis)
					{
					case EAxisLock::X:
						Axis = ChildTransform.GetUnitAxis(EAxis::X);
						break;
					case EAxisLock::Y:
						Axis = ChildTransform.GetUnitAxis(EAxis::Y);
						break;
					case EAxisLock::Z:
						Axis = ChildTransform.GetUnitAxis(EAxis::Z);
						break;
					case EAxisLock::XY:
						Axis = ChildTransform.GetUnitAxis(EAxis::Z);
						break;
					case EAxisLock::YZ:
						Axis = ChildTransform.GetUnitAxis(EAxis::X);
						break;
					case EAxisLock::XZ:
						Axis = ChildTransform.GetUnitAxis(EAxis::Y);
						break;
					case EAxisLock::All:
						Axis = GC.ViewForward;
						break;
					default:
						Axis = FVector::ZeroVector;
					}
				}

				const FQuat TargetRot(Axis, AngleToRotateRad);
				const FVector OffsetVector = ChildTransform.GetLocation() - PivotPosition;
				
				const FVector NewLocation = PivotPosition + TargetRot.RotateVector(OffsetVector);
				const FQuat NewRotation = (TargetRot * ChildTransform.GetRotation()).GetNormalized();

				FTransform FinalTransform = ChildTransform;
				FinalTransform.SetLocation(NewLocation);
				FinalTransform.SetRotation(NewRotation);

				Comp->SetWorldTransform(FinalTransform);
			}
		}
	}

	void FActorPivot::ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace)
	{
		for (const FSelection& Child : Children)
		{
			USceneComponent* Comp = ResolveComponent(Child);
			if (!Comp)
			{
				continue;
			}
			const FTransform ActorInitialTransform = Child.StartTransform;
			const FQuat ActorRotation = ActorInitialTransform.GetRotation();
			const FVector PivotToActorVec = ActorInitialTransform.GetLocation() - GetStartLocation();
			FTransform NewTransform = ActorInitialTransform;

			FVector NewPosition, NewScale;
			if (bUsingLocalSpace)
			{
				const FVector LocalPivotToActorVec = ActorRotation.UnrotateVector(PivotToActorVec);
				const FVector ScaledLocalPivotToActorVec = LocalPivotToActorVec * ScaleMultiplier;
				const FVector GlobalPivotToActorVec = ActorRotation.RotateVector(ScaledLocalPivotToActorVec);
				NewPosition = GetStartLocation() + GlobalPivotToActorVec;

				NewScale = ActorInitialTransform.GetScale3D() * ScaleMultiplier;
			}
			else
			{
				const FQuat InitialRotation = ActorInitialTransform.GetRotation();
				const FMatrix RotationMatrix = FRotationMatrix(InitialRotation.Rotator());

				const FMatrix GlobalScaleMatrix = FScaleMatrix(ScaleMultiplier);

				const FMatrix LocalEquivalentMatrix = RotationMatrix.Inverse() * GlobalScaleMatrix * RotationMatrix;
				const FVector LocalScaleToAdd = LocalEquivalentMatrix.GetScaleVector();

				FVector LocalScaleToAddSigned = LocalScaleToAdd;
				if (ScaleMultiplier.X < 0)
				{
					LocalScaleToAddSigned.X *= -1.f;
				}
				if (ScaleMultiplier.Y < 0)
				{
					LocalScaleToAddSigned.Y *= -1.f;
				}
				if (ScaleMultiplier.Z < 0)
				{
					LocalScaleToAddSigned.Z *= -1.f;
				}
				NewScale = ActorInitialTransform.GetScale3D() * LocalScaleToAddSigned;

				const FVector ScaledRelativePosition = PivotToActorVec * ScaleMultiplier;
				NewPosition = GetStartLocation() + ScaledRelativePosition;
			}

			NewTransform.SetLocation(NewPosition);

			if (!NewScale.IsNearlyZero())
			{
				NewTransform.SetScale3D(NewScale);
			}
			else
			{
				return;
			}

			Comp->SetWorldTransform(NewTransform);
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
			StartPivotTransform.SetRotation(Children[0].StartTransform.GetRotation());
			StartPivotTransform.SetScale3D(Children[0].StartTransform.GetScale3D());
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
			if (USceneComponent* Comp = ResolveComponent(Child))
			{
				Accum += Comp->GetComponentLocation();
			}
		}

		const FVector PivotLocation = Accum / Children.Num();
		StartPivotTransform.SetLocation(PivotLocation);
	}

	void FActorPivot::ComputeBoundingBoxCenterPivot()
	{
		FBox BoundingBox(ForceInit);

		for (const auto& Child : Children)
		{
			FVector Origin, Extent;
			if (USceneComponent* Comp = ResolveComponent(Child))
			{
				Comp->Bounds.GetBox().GetCenterAndExtents(Origin, Extent);
				BoundingBox += Origin;
			}
		}

		StartPivotTransform.SetLocation(BoundingBox.GetCenter());
	}

	void FActorPivot::ComputeActiveElementPivot()
	{
		if (ResolveComponent(Children.Last()))
		{
			StartPivotTransform = Children.Last().StartTransform;
		}
	}

	void FActorPivot::RevertTransformToStartState()
	{
		for (const FSelection& Child : Children)
		{
			if (USceneComponent* Comp = ResolveComponent(Child))
			{
				Comp->SetWorldTransform(Child.StartTransform);
			}
		}

		if (!TransformProxy)
		{
			TransformProxy->SetTransform(StartPivotTransform);
		}
	}

	TArray<USceneComponent*> FActorPivot::GetSelectedComponents() const
	{
		TArray<USceneComponent*> Result;
		Result.Reserve(Children.Num());

		for (const FSelection& Child : Children)
		{
			if (USceneComponent* Comp = ResolveComponent(Child))
			{
				Result.Add(Comp);
			}
		}

		return Result;
	}

	void FActorPivot::ForEachElementTransform(
		TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const
	{
		const FSelection& ActiveElement = Children.Last();
		USceneComponent* ActiveComp = ResolveComponent(ActiveElement);
		
		for (const FSelection& Child : Children)
		{
			if (USceneComponent* Comp = ResolveComponent(Child))
			{
				const bool bIsActive = (Comp == ActiveComp);
				Callback(Child.StartTransform, bIsActive);
			}
		}
	}
} // namespace BlenderControls
