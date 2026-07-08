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
			if (Children.Num() > 0)
			{
				ActiveChild = Children.Last();
				ComputePivotTransform(PivotMode);
			}
		}
	}

	void FActorPivot::ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace)
	{
		for (const FChildInfo& Child : Children)
		{
			FVector WorldSpaceOffset = LocalDelta;

			if (bUsingLocalSpace)
			{
				const FQuat ChildStartRotation = Child.Transform.GetRotation();
				WorldSpaceOffset = ChildStartRotation.RotateVector(LocalDelta);
			}

			const FVector StartPos = Child.Transform.GetLocation();
			if (Child.Actor)
			{
				Child.Actor->SetActorLocation(StartPos + WorldSpaceOffset);
			}
		}
	}

	void FActorPivot::ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis)
	{
		if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
		{
			const FChildInfo& ActiveElement = Children.Last();
			const FQuat ActiveObjectStartRotation = ActiveElement.Transform.GetRotation();
			const FVector LocalSpaceDelta = ActiveObjectStartRotation.UnrotateVector(WorldDelta);

			for (const FChildInfo& Child : Children)
			{
				const FTransform StartTransform = Child.Transform;
				const FVector WorldOffset = StartTransform.TransformPositionNoScale(LocalSpaceDelta);

				if (Child.Actor)
				{
					if (Child.Actor == ActiveElement.Actor)
					{
						Child.Actor->SetActorLocation(StartTransform.GetLocation() + WorldDelta);
					}
					else
					{
						Child.Actor->SetActorLocation(WorldOffset);
					}
				}
			}
		}
		else
		{
			const FVector NewPos = StartPivotTransform.GetLocation() + WorldDelta;
			for (const FChildInfo& Child : Children)
			{
				if (Child.Actor)
				{
					Child.Actor->SetActorLocation(Child.Transform.GetLocation() + WorldDelta);
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
		const FVector RotationAxis = GC.SingleLockAxis;

		const FQuat TargetRotation = FQuat(RotationAxis, AngleToRotateRad);
		FTransform StartTransform = StartPivotTransform;
		FTransform NewTransform = StartTransform;
		FQuat ResultQuat = TargetRotation * NewTransform.GetRotation();
		ResultQuat.Normalize();

		if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
		{
			for (FChildInfo Child : Children)
			{
				const FTransform ChildTransform = Child.Transform;
				const FVector X = ChildTransform.GetUnitAxis(EAxis::X);
				const FVector Y = ChildTransform.GetUnitAxis(EAxis::Y);
				const FVector Z = ChildTransform.GetUnitAxis(EAxis::Z);

				FVector LocalRotationAxis;
				switch (LockedAxis)
				{
				case EAxisLock::X: LocalRotationAxis = X;
					break;
				case EAxisLock::Y: LocalRotationAxis = Y;
					break;
				case EAxisLock::Z: LocalRotationAxis = Z;
					break;
				case EAxisLock::XY: LocalRotationAxis = Z;
					break;
				case EAxisLock::YZ: LocalRotationAxis = X;
					break;
				case EAxisLock::XZ: LocalRotationAxis = Y;
					break;
				case EAxisLock::All: LocalRotationAxis = GC.ViewForward;
					break;
				default: LocalRotationAxis = FVector::ZeroVector;
				}

				const FQuat TargetRot(LocalRotationAxis, AngleToRotateRad);

				const FVector CurrentLocation = ChildTransform.GetLocation();
				const FVector CurrentScale = ChildTransform.GetScale3D();
				const FVector NewLocation = PivotPosition + TargetRot.RotateVector(CurrentLocation - PivotPosition);
				const FQuat NewRotation = (TargetRot * ChildTransform.GetRotation()).GetNormalized();

				NewTransform.SetScale3D(CurrentScale);
				NewTransform.SetLocation(NewLocation);
				NewTransform.SetRotation(NewRotation);
				if (Child.Actor)
				{
					Child.Actor->SetActorTransform(NewTransform);
				}
			}
		}
		else
		{
			NewTransform.SetRotation(ResultQuat);
			if (TransformProxy)
			{
				TransformProxy->SetTransform(NewTransform);
			}
		}
	}

	void FActorPivot::ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace)
	{
		for (FChildInfo Child : Children)
		{
			if (!Child.Actor) continue;

			const FTransform ActorInitialTransform = Child.Transform;
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
				if (ScaleMultiplier.X < 0) LocalScaleToAddSigned.X *= -1.f;
				if (ScaleMultiplier.Y < 0) LocalScaleToAddSigned.Y *= -1.f;
				if (ScaleMultiplier.Z < 0) LocalScaleToAddSigned.Z *= -1.f;

				NewScale = ActorInitialTransform.GetScale3D() * LocalScaleToAddSigned;

				const FVector ScaledRelativePosition = PivotToActorVec * ScaleMultiplier;
				NewPosition = GetStartLocation() + ScaledRelativePosition;
			}

			NewTransform.SetLocation(NewPosition);
			NewTransform.SetScale3D(NewScale);

			Child.Actor->SetActorTransform(NewTransform);
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

	void FActorPivot::ForEachElementTransform(
		TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const
	{
		const FChildInfo& ActiveElement = Children.Last();
		for (const FChildInfo& Child : Children)
		{
			if (Child.Actor)
			{
				const bool bIsActive = (Child.Actor == ActiveElement.Actor);
				Callback(Child.Transform, bIsActive);
			}
		}
	}
} // namespace BlenderControls
