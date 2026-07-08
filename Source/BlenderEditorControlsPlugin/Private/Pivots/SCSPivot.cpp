#include "Pivots/SCSPivot.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "BlueprintEditor.h"
#include "SSubobjectEditor.h"
#include "SubobjectData.h"
#include "EditorViewportClient.h"

namespace BlenderControls
{
	FSCSPivot::FSCSPivot(FBlueprintEditor* InBlueprintEditor, const TArray<TSharedPtr<FSubobjectEditorTreeNode>>& InNodes)
		: BlueprintEditorPtr(InBlueprintEditor)
	{
		if (!BlueprintEditorPtr) return;

		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		for (const TSharedPtr<FSubobjectEditorTreeNode>& Node : InNodes)
		{
			if (!Node.IsValid()) continue;

			const FSubobjectData* Data = Node->GetDataSource();
			if (!Data) continue;

			FSCSNodeInfo Info;
			Info.CachedData = Data;

			// We still need to fetch once here to get the StartTransform
			USceneComponent* TemplateComponent = const_cast<USceneComponent*>(Data->GetObjectForBlueprint<USceneComponent>(Blueprint));
			USceneComponent* PreviewInstance = const_cast<USceneComponent*>(Cast<USceneComponent>(Data->FindComponentInstanceInActor(PreviewActor)));

			if (TemplateComponent)
			{
				Info.StartTransform = TemplateComponent->GetComponentTransform();
				Nodes.Add(Info);
			}
			else if (PreviewInstance)
			{
				Info.StartTransform = PreviewInstance->GetComponentTransform();
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
			if (USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(ActiveCachedData->FindComponentInstanceInActor(PreviewActor))))
			{
				return LivePreview->GetComponentLocation();
			}
		}
		return FVector::ZeroVector;
	}

	void FSCSPivot::RevertToStartState()
	{
		if (!BlueprintEditorPtr) return;

		UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj();
		AActor* PreviewActor = BlueprintEditorPtr->GetPreviewActor();

		for (const FSCSNodeInfo& Info : Nodes)
		{
			if (!Info.CachedData) continue;

			// 1. DYNAMICALLY re-fetch the Template
			USceneComponent* LiveTemplate = const_cast<USceneComponent*>(
				Info.CachedData->GetObjectForBlueprint<USceneComponent>(Blueprint));

			if (LiveTemplate)
			{
				LiveTemplate->SetWorldTransform(Info.StartTransform);
			}

			// 2. DYNAMICALLY re-fetch the Preview Instance (this guarantees we get the newly spawned one)
			USceneComponent* LivePreview = const_cast<USceneComponent*>(Cast<USceneComponent>(
				Info.CachedData->FindComponentInstanceInActor(PreviewActor)));

			if (LivePreview)
			{
				LivePreview->SetWorldTransform(Info.StartTransform);
			}
		}

		// 3. Force viewport redraw
		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
			if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient()))
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
} // namespace BlenderControls
