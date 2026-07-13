#include "Pivots/SCSPivot.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "BlueprintEditor.h"
#include "SSubobjectEditor.h"
#include "SubobjectData.h"
#include "EditorViewportClient.h"
#include "Enums.h"
#include "Tools/GrabContext.h"

namespace BlenderControls
{
	FSCSPivot::FSCSPivot(FBlueprintEditor* InBlueprintEditor,
	                     const TArray<TSharedPtr<FSubobjectEditorTreeNode>>& InNodes)
		: BlueprintEditorPtr(InBlueprintEditor)
	{
		if (!BlueprintEditorPtr)
		{
			return;
		}

		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		for (const TSharedPtr<FSubobjectEditorTreeNode>& Node : InNodes)
		{
			if (!Node.IsValid())
			{
				continue;
			}

			const FSubobjectData* Data = Node->GetDataSource();
			if (!Data)
			{
				continue;
			}

			FSCSNodeInfo Info;
			Info.CachedData = Data;

			USceneComponent* PreviewInstance =
				const_cast<USceneComponent*>(Cast<USceneComponent>(Data->FindComponentInstanceInActor(PreviewActor)));
			USceneComponent* TemplateComponent =
				const_cast<USceneComponent*>(Data->GetObjectForBlueprint<USceneComponent>(Blueprint));

			if (PreviewInstance)
			{
				Info.StartTransform = PreviewInstance->GetComponentTransform();
				Nodes.Add(Info);
			}
			// Resort to template component in case PreviewInstance fails. 
			else if (TemplateComponent)
			{
				Info.StartTransform = TemplateComponent->GetComponentTransform();
				Nodes.Add(Info);
			}
		}

		if (Nodes.Num() > 0)
		{
			ActiveCachedData = Nodes.Last().CachedData;
			ActiveComponentStartTransform = Nodes.Last().StartTransform;
			ComputeMedianPivot();
		}
	}

	FVector FSCSPivot::GetActiveElementCurrentLocation() const
	{
		if (BlueprintEditorPtr && ActiveCachedData)
		{
			AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();
			if (USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
				ActiveCachedData->FindComponentInstanceInActor(PreviewActor))))
			{
				return LivePreview->GetComponentLocation();
			}
		}
		return FVector::ZeroVector;
	}

	void FSCSPivot::RevertToStartState()
	{
		if (!BlueprintEditorPtr)
		{
			return;
		}

		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		for (const FSCSNodeInfo& Info : Nodes)
		{
			if (!Info.CachedData)
			{
				continue;
			}

			// DYNAMICALLY re-fetch the Template
			USceneComponent* LiveTemplate = const_cast<USceneComponent*>(
				Info.CachedData->GetObjectForBlueprint<USceneComponent>(Blueprint));

			if (LiveTemplate)
			{
				LiveTemplate->SetWorldTransform(Info.StartTransform);
			}

			// DYNAMICALLY re-fetch the Preview Instance (this guarantees we get the newly spawned one)
			USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
				Info.CachedData->FindComponentInstanceInActor(PreviewActor)));

			if (LivePreview)
			{
				LivePreview->SetWorldTransform(Info.StartTransform);
			}
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
			if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()
				->GetClient()))
			{
				ViewportClient->Invalidate();
			}
		}
	}

	void FSCSPivot::ComputeMedianPivot()
	{
		FVector Accum = FVector::ZeroVector;
		for (const FSCSNodeInfo& Info : Nodes)
		{
			Accum += Info.StartTransform.GetLocation();
		}

		const FVector PivotLocation = Accum / Nodes.Num();
		StartPivotTransform.SetLocation(PivotLocation);

		if (Nodes.Num() == 1)
		{
			StartPivotTransform.SetRotation(Nodes[0].StartTransform.GetRotation());
		}
		else
		{
			StartPivotTransform.SetRotation(FQuat::Identity);
		}
	}

	void FSCSPivot::ComputeActiveElementPivot()
	{
		if (ActiveCachedData)
		{
			StartPivotTransform = ActiveComponentStartTransform;
		}
	}

	void FSCSPivot::ApplyRotation(const FGrabContext& GC, float AngleToRotateRad, bool bUsingLocalSpace,
	                              EAxisLock LockedAxis)
	{
		if (!BlueprintEditorPtr) return;

		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		const FVector PivotPosition = GetStartLocation();
		const FVector RotationAxis = GC.SingleLockAxis;

		const FQuat TargetRotation = FQuat(RotationAxis, AngleToRotateRad);
		FQuat ResultQuat = TargetRotation * StartPivotTransform.GetRotation();
		ResultQuat.Normalize();

		for (const FSCSNodeInfo& Info : Nodes)
		{
			if (!Info.CachedData) continue;

			FTransform NewTransform = Info.StartTransform;

			if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
			{
				const FTransform ChildTransform = Info.StartTransform;
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
			}
			else
			{
				const FVector CurrentLocation = Info.StartTransform.GetLocation();
				const FVector NewLocation = PivotPosition + TargetRotation.
					RotateVector(CurrentLocation - PivotPosition);
				const FQuat NewRotation = (TargetRotation * Info.StartTransform.GetRotation()).GetNormalized();

				NewTransform.SetLocation(NewLocation);
				NewTransform.SetRotation(NewRotation);
			}

			USceneComponent* LiveTemplate = const_cast<USceneComponent*>(Info.CachedData->GetObjectForBlueprint<
				USceneComponent>(Blueprint));
			if (LiveTemplate) LiveTemplate->SetWorldTransform(NewTransform);

			USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
				Info.CachedData->FindComponentInstanceInActor(PreviewActor)));
			if (LivePreview) LivePreview->SetWorldTransform(NewTransform);
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
			if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()
				->GetClient()))
			{
				ViewportClient->Invalidate();
			}
		}
	}

	void FSCSPivot::ApplyScale(const FVector& ScaleMultiplier, bool bUsingLocalSpace)
	{
		if (!BlueprintEditorPtr) return;

		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		for (const FSCSNodeInfo& Info : Nodes)
		{
			if (!Info.CachedData) continue;

			const FTransform ComponentInitialTransform = Info.StartTransform;
			const FQuat ComponentRotation = ComponentInitialTransform.GetRotation();
			const FVector PivotToComponentVec = ComponentInitialTransform.GetLocation() - GetStartLocation();
			FTransform NewTransform = ComponentInitialTransform;

			FVector NewPosition, NewScale;
			if (bUsingLocalSpace)
			{
				const FVector LocalPivotToComponentVec = ComponentRotation.UnrotateVector(PivotToComponentVec);
				const FVector ScaledLocalPivotToComponentVec = LocalPivotToComponentVec * ScaleMultiplier;
				const FVector GlobalPivotToComponentVec = ComponentRotation.
					RotateVector(ScaledLocalPivotToComponentVec);
				NewPosition = GetStartLocation() + GlobalPivotToComponentVec;

				NewScale = ComponentInitialTransform.GetScale3D() * ScaleMultiplier;
			}
			else
			{
				const FQuat InitialRotation = ComponentInitialTransform.GetRotation();
				const FMatrix RotationMatrix = FRotationMatrix(InitialRotation.Rotator());

				const FMatrix GlobalScaleMatrix = FScaleMatrix(ScaleMultiplier);

				const FMatrix LocalEquivalentMatrix = RotationMatrix.Inverse() * GlobalScaleMatrix * RotationMatrix;
				const FVector LocalScaleToAdd = LocalEquivalentMatrix.GetScaleVector();

				FVector LocalScaleToAddSigned = LocalScaleToAdd;
				if (ScaleMultiplier.X < 0) LocalScaleToAddSigned.X *= -1.f;
				if (ScaleMultiplier.Y < 0) LocalScaleToAddSigned.Y *= -1.f;
				if (ScaleMultiplier.Z < 0) LocalScaleToAddSigned.Z *= -1.f;

				NewScale = ComponentInitialTransform.GetScale3D() * LocalScaleToAddSigned;

				const FVector ScaledRelativePosition = PivotToComponentVec * ScaleMultiplier;
				NewPosition = GetStartLocation() + ScaledRelativePosition;
			}

			NewTransform.SetLocation(NewPosition);
			NewTransform.SetScale3D(NewScale);

			USceneComponent* LiveTemplate = const_cast<USceneComponent*>(Info.CachedData->GetObjectForBlueprint<
				USceneComponent>(Blueprint));
			if (LiveTemplate) LiveTemplate->SetWorldTransform(NewTransform);

			USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
				Info.CachedData->FindComponentInstanceInActor(PreviewActor)));
			if (LivePreview) LivePreview->SetWorldTransform(NewTransform);
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
			if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()
				->GetClient()))
			{
				ViewportClient->Invalidate();
			}
		}
	}

	void FSCSPivot::ForEachElementTransform(
		TFunctionRef<void(const FTransform& StartTransform, bool bIsActive)> Callback) const
	{
		for (const FSCSNodeInfo& Info : Nodes)
		{
			const bool bIsActive = (Info.CachedData == ActiveCachedData);
			Callback(Info.StartTransform, bIsActive);
		}
	}

	void FSCSPivot::ApplyTranslation(const FVector& LocalDelta, bool bUsingLocalSpace)
	{
		if (!BlueprintEditorPtr) return;
		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		for (const FSCSNodeInfo& Info : Nodes)
		{
			if (!Info.CachedData) continue;

			FVector WorldSpaceOffset = LocalDelta;
			if (bUsingLocalSpace)
			{
				const FQuat ChildStartRotation = Info.StartTransform.GetRotation();
				WorldSpaceOffset = ChildStartRotation.RotateVector(LocalDelta);
			}

			FTransform NewTransform = Info.StartTransform;
			NewTransform.SetLocation(Info.StartTransform.GetLocation() + WorldSpaceOffset);

			USceneComponent* LiveTemplate = const_cast<USceneComponent*>(Info.CachedData->GetObjectForBlueprint<
				USceneComponent>(Blueprint));
			if (LiveTemplate) LiveTemplate->SetWorldTransform(NewTransform);

			USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
				Info.CachedData->FindComponentInstanceInActor(PreviewActor)));
			if (LivePreview) LivePreview->SetWorldTransform(NewTransform);
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
			if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()
				->GetClient()))
			{
				ViewportClient->Invalidate();
			}
		}
	}

	void FSCSPivot::ApplyTranslation(const FVector& WorldDelta, bool bUsingLocalSpace, EAxisLock LockedAxis)
	{
		if (!BlueprintEditorPtr) return;
		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		if (bUsingLocalSpace && LockedAxis != EAxisLock::All)
		{
			const FQuat ActiveObjectStartRotation = ActiveComponentStartTransform.GetRotation();
			const FVector LocalSpaceDelta = ActiveObjectStartRotation.UnrotateVector(WorldDelta);

			for (const FSCSNodeInfo& Info : Nodes)
			{
				if (!Info.CachedData) continue;

				const FTransform StartTransform = Info.StartTransform;
				const FVector WorldOffset = StartTransform.TransformPositionNoScale(LocalSpaceDelta);

				FTransform NewTransform = StartTransform;
				if (Info.CachedData == ActiveCachedData)
				{
					NewTransform.SetLocation(StartTransform.GetLocation() + WorldDelta);
				}
				else
				{
					NewTransform.SetLocation(WorldOffset);
				}

				USceneComponent* LiveTemplate = const_cast<USceneComponent*>(Info.CachedData->GetObjectForBlueprint<
					USceneComponent>(Blueprint));
				if (LiveTemplate) LiveTemplate->SetWorldTransform(NewTransform);

				USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
					Info.CachedData->FindComponentInstanceInActor(PreviewActor)));
				if (LivePreview) LivePreview->SetWorldTransform(NewTransform);
			}
		}
		else
		{
			for (const FSCSNodeInfo& Info : Nodes)
			{
				if (!Info.CachedData) continue;

				FTransform NewTransform = Info.StartTransform;
				NewTransform.SetLocation(Info.StartTransform.GetLocation() + WorldDelta);

				USceneComponent* LiveTemplate = const_cast<USceneComponent*>(Info.CachedData->GetObjectForBlueprint<
					USceneComponent>(Blueprint));
				if (LiveTemplate) LiveTemplate->SetWorldTransform(NewTransform);

				USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
					Info.CachedData->FindComponentInstanceInActor(PreviewActor)));
				if (LivePreview) LivePreview->SetWorldTransform(NewTransform);
			}
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
			if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()
				->GetClient()))
			{
				ViewportClient->Invalidate();
			}
		}
	}

	bool FSCSPivot::ApplyManualTransformDelta(const FVector& InDrag, const FRotator& InRot, const FVector& InScale)
	{
		if (!BlueprintEditorPtr) return false;

		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		for (const FSCSNodeInfo& Info : Nodes)
		{
			if (!Info.CachedData) continue;

			USceneComponent* LiveTemplate = const_cast<USceneComponent*>(
				Info.CachedData->GetObjectForBlueprint<USceneComponent>(Blueprint));

			USceneComponent* LivePreview = const_cast<USceneComponent*>(
				Cast<USceneComponent>(Info.CachedData->FindComponentInstanceInActor(PreviewActor)));

			if (LiveTemplate)
			{
				FTransform CurrentTransform = LiveTemplate->GetComponentTransform();

				if (!InDrag.IsNearlyZero())
				{
					CurrentTransform.SetLocation(CurrentTransform.GetLocation() + InDrag);
				}
				if (!InRot.IsNearlyZero())
				{
					CurrentTransform.SetRotation((InRot.Quaternion() * CurrentTransform.GetRotation()).GetNormalized());
				}
				if (!InScale.IsNearlyZero())
				{
					CurrentTransform.SetScale3D(CurrentTransform.GetScale3D() + InScale);
				}

				LiveTemplate->SetWorldTransform(CurrentTransform);
			}

			if (LivePreview)
			{
				FTransform CurrentTransform = LivePreview->GetComponentTransform();

				if (!InDrag.IsNearlyZero())
				{
					CurrentTransform.SetLocation(CurrentTransform.GetLocation() + InDrag);
				}
				if (!InRot.IsNearlyZero())
				{
					CurrentTransform.SetRotation((InRot.Quaternion() * CurrentTransform.GetRotation()).GetNormalized());
				}
				if (!InScale.IsNearlyZero())
				{
					CurrentTransform.SetScale3D(CurrentTransform.GetScale3D() + InScale);
				}

				LivePreview->SetWorldTransform(CurrentTransform);
			}
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
			if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient()))
			{
				ViewportClient->Invalidate();
			}
		}

		return true;
	}
} // namespace BlenderControls
