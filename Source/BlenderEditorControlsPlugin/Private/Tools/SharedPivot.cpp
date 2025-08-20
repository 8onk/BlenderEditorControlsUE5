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
			PivotTransform.SetScale3D(Children[0].Transform.GetScale3D());
		}
		else
		{
			PivotTransform.SetRotation(FQuat::Identity);
		}

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

	void FSharedPivot::Translate(const FVector& Delta, const bool bInUsingLocalSpace)
	{
		for (const FChildInfo& Child : Children)
		{
			FVector WorldSpaceOffset = Delta;

			if (bInUsingLocalSpace)
			{
				const FQuat ChildStartRotation = Child.Transform.GetRotation();
				WorldSpaceOffset = ChildStartRotation.RotateVector(Delta);
			}

			const FVector StartPos = Child.Transform.GetLocation();
			Child.Actor->SetActorLocation(StartPos + WorldSpaceOffset);
		}
	}

	void FSharedPivot::Translate(const bool bUsingLocalSpace, const EAxisLock LockedAxis, const FVector& Delta)
	{
		if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
		{
			const FQuat ActiveObjectStartRotation = GetActiveElement().Transform.GetRotation();
			const FVector LocalSpaceDelta = ActiveObjectStartRotation.UnrotateVector(Delta);

			for (const FChildInfo Child : Children)
			{
				const FTransform StartTransform = Child.Transform;
				const FVector WorldOffset = StartTransform.TransformPositionNoScale(LocalSpaceDelta);

				if (Child.Actor == GetActiveElement().Actor)
				{
					Child.Actor->SetActorLocation(StartTransform.GetLocation() + Delta);
				}
				else
				{
					Child.Actor->SetActorLocation(WorldOffset);
				}
			}
		}
		else
		{
			const FVector NewPos = GetStartTransform().GetLocation() + Delta;
			SetPosition(NewPos);
		}
	}

	void FSharedPivot::Rotate(FGrabContext GC, float AngleToRotateRad, bool bUsingLocalSpace, EAxisLock LockedAxis)
	{
		const FVector PivotPosition = GetLocation();
		const FVector RotationAxis = GC.HelperAxisDir;

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
				}
				const FQuat TargetRot(LocalRotationAxis, AngleToRotateRad);

				const FVector CurrentLocation = ChildTransform.GetLocation();
				const FVector CurrentScale = ChildTransform.GetScale3D();
				const FVector NewLocation = PivotPosition + TargetRot.RotateVector(CurrentLocation - PivotPosition);
				const FQuat NewRotation = (TargetRot * ChildTransform.GetRotation()).GetNormalized();

				NewTransform.SetScale3D(CurrentScale);
				NewTransform.SetLocation(NewLocation);
				NewTransform.SetRotation(NewRotation);
				Child.Actor->SetActorTransform(NewTransform);
			}
		}
		else
		{
			NewTransform.SetRotation(ResultQuat);
			TransformProxy->SetTransform(NewTransform);
		}
	}

	void FSharedPivot::Scale(FVector ScaleMultiplier, bool bUsingLocalSpace)
	{
		for (FChildInfo Child : Children)
		{
			const FTransform ActorInitialTransform = Child.Transform;
			const FQuat ActorRotation = ActorInitialTransform.GetRotation();
			const FVector PivotToActorVec = ActorInitialTransform.GetLocation() - GetLocation();
			FTransform NewTransform = ActorInitialTransform;

			FVector NewPosition, NewScale;
			if (bUsingLocalSpace)
			{
				const FVector LocalPivotToActorVec = ActorRotation.UnrotateVector(PivotToActorVec);
				const FVector ScaledLocalPivotToActorVec = LocalPivotToActorVec * ScaleMultiplier;
				const FVector GlobalPivotToActorVec = ActorRotation.RotateVector(ScaledLocalPivotToActorVec);
				NewPosition = GetLocation() + GlobalPivotToActorVec;

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
				NewPosition = GetLocation() + ScaledRelativePosition;
			}

			NewTransform.SetLocation(NewPosition);
			NewTransform.SetScale3D(NewScale);

			Child.Actor->SetActorTransform(NewTransform);
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
