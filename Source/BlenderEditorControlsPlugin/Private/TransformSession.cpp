// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#include "TransformSession.h"
#include "Editor.h"
#include "Selection.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandInfo.h"
#include "BlenderControlsCommands.h"
#include "SSubobjectEditor.h"
#include "Animation/SkeletalMeshActor.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
#include "BlenderControlsSettings.h"
#endif
#include "Input/Numeric/NumericInputProcessor.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
#include "Settings/EditorStyleSettings.h"
#else
#include "Classes/EditorStyleSettings.h"
#endif
#include "Tools/ToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Pivots/ActorPivot.h"
#include "Pivots/SCSPivot.h"
#include "Pivots/ControlRigPivot.h"
#include "Utils/MathHelpers.h"
#include "ControlRig/ControlRigSelectionHelper.h"
#include "BlueprintEditor.h"
#include "SEditorViewport.h"
#include "Settings/EditorLoadingSavingSettings.h"

// Dummy class to bypass the 'protected' access modifier for bIsTracking. 
// Not the cleanest solution, but this seems to be the only way (that I could think of) to make it such that 
// auto save is pending until a transform session is complete, vs aborting it altogether.
#include "Utils/ViewportExposer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTransformSession, Log, All);

namespace BlenderControls
{
	FTransformSession::FTransformSession(ETransformMode InStartMode, bool bDuplicateSelection)
	{
		if (!ValidateEditorState())
		{
			bHasSessionTerminated = true;
			return;
		}

		// Tracks duplication state to prevent calling Cancel() on ScopedTransaction.
		// Since edactDuplicateSelected commits its own transaction, calling Cancel() 
		// here would revert the transform but leave the duplicated actors orphaned in the scene.
		// Note: Duplication is only supported for actors, not Control Rig elements.
		bStartedWithDuplicate = bDuplicateSelection && (SelectionType == ESelectionType::Actors);

		if (bStartedWithDuplicate)
		{
			PerformDuplicateSelected();
		}

		InitializePivot();
		InitializeMouseState();

		NumericInputProcessor = MakeUnique<FNumericInputProcessor>();

		const bool bAutoSaveEnable = GetDefault<UEditorLoadingSavingSettings>()->bAutoSaveEnable;
		FViewportClientExposer::SetViewportState(ActiveViewportClient, /*bInTracking=*/bAutoSaveEnable,
		                                         /*bAxisControlledByDrag=*/true);

		// Necessary to prevent hit proxies from triggering in blueprint viewport. For Level viewport, 
		// setting bAxisControlledByDrag = true is enough to prevent this. However, for blueprint level viewport,
		// due to custom overrides, this does nothing to prevent calculating hit proxies which results in lag. 
		if (ActiveViewportClient && ActiveViewportClient->GetEditorViewportWidget().IsValid())
		{
			ActiveViewportClient->GetEditorViewportWidget()->SetVisibility(EVisibility::HitTestInvisible);
		}
	}

	FTransformSession::~FTransformSession()
	{
		if (CurrentTool.IsValid())
		{
			CurrentTool->Cancel();
			CurrentTool.Reset();
		}
		if (ScopedTransaction.IsValid())
		{
			ScopedTransaction->Cancel();
			ScopedTransaction.Reset();
		}
	}

	FEditorViewportClient* GetHoveredViewportClient()
	{
		constexpr bool bIgnoreEnabledStatus = true;
		const FWidgetPath WidgetPath = FSlateApplication::Get().LocateWindowUnderMouse(
			FSlateApplication::Get().GetCursorPos(),
			FSlateApplication::Get().GetInteractiveTopLevelWindows(),
			bIgnoreEnabledStatus
		);

		if (WidgetPath.IsValid())
		{
			for (FEditorViewportClient* ViewportClient : GEditor->GetAllViewportClients())
			{
				if (!ViewportClient)
				{
					continue;
				}
				TSharedPtr<SEditorViewport> ViewportWidget = ViewportClient->GetEditorViewportWidget();
				if (ViewportWidget.IsValid() && WidgetPath.ContainsWidget(ViewportWidget.Get()))
				{
					return ViewportClient;
				}
			}
		}

		return nullptr;
	}

	FBlueprintEditor* FTransformSession::GetActiveBlueprintEditor() const
	{
		if (!GEditor)
		{
			return nullptr;
		}

		UWorld* ViewportWorld = ActiveViewportClient->GetWorld();

		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (!AssetEditorSubsystem)
		{
			return nullptr;
		}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
		TArray<IAssetEditorInstance*> OpenEditors;
		TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets();

		for (UObject* Asset : EditedAssets)
		{
			TArray<IAssetEditorInstance*> EditorsForAsset = AssetEditorSubsystem->FindEditorsForAsset(Asset);
			for (IAssetEditorInstance* Editor : EditorsForAsset)
			{
				OpenEditors.AddUnique(Editor);
			}
		}
#else
		//Seems like this is the only straightforward route to get a hold of FBlueprintEditor* (ie by iterating over all open editors)
		const TArray<IAssetEditorInstance*> OpenEditors = AssetEditorSubsystem->GetAllOpenEditors();
#endif

		for (IAssetEditorInstance* Editor : OpenEditors)
		{
			if (Editor && Editor->GetEditorName() == TEXT("BlueprintEditor"))
			{
				// Find the Blueprint Editor that owns the preview world matching our hovered viewport
				FBlueprintEditor* BPEditor = static_cast<FBlueprintEditor*>(Editor);
				if (AActor* PreviewActor = BPEditor->GetPreviewActor())
				{
					if (PreviewActor->GetWorld() == ViewportWorld)
					{
						return BPEditor;
					}
				}
			}
		}

		return nullptr;
	}

	bool FTransformSession::ValidateEditorState()
	{
		if (!GEditor)
		{
			UE_LOG(LogTransformSession, Error, TEXT("[%hs]: GEditor is null."), __FUNCTION__);
			return false;
		}

		//Is any viewport focused or hovered?
		ActiveViewportClient = GetHoveredViewportClient();
		if (!ActiveViewportClient)
		{
			return false;
		}

		if (ActiveViewportClient->IsLevelEditorClient())
		{
			const bool bControlRigModeActive = FControlRigSelectionHelper::IsControlRigEditModeActive();
			const bool bHasRigElements = FControlRigSelectionHelper::HasSelectedRigElements();

			if (bControlRigModeActive && bHasRigElements)
			{
				SelectionType = ESelectionType::ControlRig;
				return true;
			}

			if (GEditor->GetSelectedComponentCount() > 0)
			{
				SelectionType = ESelectionType::Components;
				return true;
			}

			if (GEditor->GetSelectedActorCount() > 0)
			{
				SelectionType = ESelectionType::Actors;
				return true;
			}
		}
		else
		{
			if (const FBlueprintEditor* BPEditor = GetActiveBlueprintEditor())
			{
				const auto& SelectedNodes = BPEditor->GetSelectedSubobjectEditorTreeNodes();
				bool bHasValidComponentNode = false;

				for (const auto& Node : SelectedNodes)
				{
					// We must ignore root actor selection (as it cannot be transformed since it has no editor gizmo). 
					if (Node.IsValid() && Node->IsComponentNode())
					{
						// Also exclude default scene root as it is not supposed to be transformed (it only has scale). 
						if (Node->GetVariableName() != FName("DefaultSceneRoot"))
						{
							bHasValidComponentNode = true;
							break;
						}
					}
				}

				if (bHasValidComponentNode)
				{
					SelectionType = ESelectionType::SCSTreeNodes;
					return true;
				}
				else
				{
					UE_LOG(LogTransformSession, Error,
					       TEXT("[%hs]: Selected Blueprint node is invalid (Root Actor or DefaultSceneRoot)."),
					       __FUNCTION__);
				}
			}
			else
			{
				UE_LOG(LogTransformSession, Error, TEXT("[%hs]: Could not find active Blueprint Editor."),
				       __FUNCTION__);
			}
		}

		SelectionType = ESelectionType::None;
		UE_LOG(LogTransformSession, Error,
		       TEXT("[%hs]: No valid transformable selection found (Actors, Components, or ControlRig)."),
		       __FUNCTION__);
		return false;
	}

	void FTransformSession::PerformDuplicateSelected()
	{
		// Start a transaction immediately for duplication to ensure the 'Spawn' 
		// and 'Move' are part of the same Undo step.
		InitializeTransaction(TEXT("Duplicate Selection"));

		if (const UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			ULevel* Level = World->GetCurrentLevel();
			constexpr bool bOffsetLocations = false;
			GEditor->edactDuplicateSelected(Level, bOffsetLocations);
		}
		else
		{
			UE_LOG(LogTransformSession, Error, TEXT("[%hs]: Failed to get Editor World Context. Duplication aborted."),
			       __FUNCTION__);
		}
	}

	void FTransformSession::InitializeMouseState()
	{
		FIntPoint MousePosInt;
		ActiveViewportClient->Viewport->GetMousePos(MousePosInt);

		StartMousePos = FVector2D(MousePosInt);
		CursorAnchorPoint = StartMousePos;
		VirtualMousePosition = StartMousePos;
		WrappedMousePosition = StartMousePos;

		AccumulatedMouseDelta = FVector2D::ZeroVector;
		GlobalCursorAnchor = FSlateApplication::Get().GetCursorPos();
	}

	void FTransformSession::InitializePivot()
	{
		SelectedActors.Empty();
		SelectedRigElements.Empty();
		SelectedComponents.Empty();
		VirtualPivot.Reset();

		constexpr EPivotMode PivotMode = EPivotMode::MedianPoint;

		if (SelectionType == ESelectionType::ControlRig)
		{
			FControlRigSelectionHelper::GetSelectedRigElements(SelectedRigElements);

			VirtualPivot = MakeShared<FControlRigPivot>(SelectedRigElements, PivotMode);
		}
		else if (SelectionType == ESelectionType::SCSTreeNodes)
		{
			FBlueprintEditor* BPEditor = GetActiveBlueprintEditor();
			if (!BPEditor)
			{
				UE_LOG(LogTransformSession, Error,
				       TEXT("[%hs]: Failed to get active Blueprint Editor for SCSTreeNodes."), __FUNCTION__);
				return;
			}

			TArray<TSharedPtr<FSubobjectEditorTreeNode>> SelectedNodes = BPEditor->
				GetSelectedSubobjectEditorTreeNodes();

			// Filter out the DefaultSceneRoot before passing to FSCSPivot (its transform is not supposed to change, 
			// but it does if it is passed in)
			TArray<TSharedPtr<FSubobjectEditorTreeNode>> FilteredNodes;
			for (const TSharedPtr<FSubobjectEditorTreeNode>& Node : SelectedNodes)
			{
				if (Node.IsValid() && Node->IsComponentNode() && Node->GetVariableName() != FName("DefaultSceneRoot"))
				{
					FilteredNodes.Add(Node);
				}
			}

			VirtualPivot = MakeShared<FSCSPivot>(BPEditor, FilteredNodes);
		}
		else
		{
			if (SelectionType == ESelectionType::Components)
			{
				USelection* ComponentSelection = GEditor->GetSelectedComponents();
				for (FSelectionIterator It(*ComponentSelection); It; ++It)
				{
					if (USceneComponent* Comp = Cast<USceneComponent>(*It))
					{
						SelectedComponents.Add(TWeakObjectPtr<USceneComponent>(Comp));
					}
				}
			}
			else
			{
				USelection* ActorSelection = GEditor->GetSelectedActors();
				for (FSelectionIterator It(*ActorSelection); It; ++It)
				{
					if (AActor* Actor = Cast<AActor>(*It))
					{
						SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
						if (USceneComponent* RootComp = Actor->GetRootComponent())
						{
							SelectedComponents.Add(TWeakObjectPtr<USceneComponent>(RootComp));
						}
					}
				}
			}

			VirtualPivot = MakeShared<FActorPivot>(SelectedComponents, PivotMode);
		}
	}

	void FTransformSession::InitializeTransaction(const FString& InTransactionName)
	{
		if (ScopedTransaction)
		{
			return;
		}

		// Check if a transaction is already active (e.g., from Animation Mode's "Select Control")
		// Animation Mode starts a transaction on click that can conflict with ours.
		// We need to end it first to avoid our changes being reverted when their transaction ends.
		if (GEditor && GEditor->IsTransactionActive())
		{
			UE_LOG(LogTransformSession, Warning, TEXT("[%hs]: Ending existing active transaction to avoid conflict"),
			       __FUNCTION__);
			GEditor->EndTransaction();
		}

		// Now create our own transaction
		FText TransactionName = FText::FromString(InTransactionName + TEXT(" - BlenderEditorControls"));
		ScopedTransaction = MakeUnique<FScopedTransaction>(TransactionName);

		// Is handled in FTransformSession::End()
		if (SelectionType == ESelectionType::ControlRig)
		{
			return;
		}

		if (SelectionType == ESelectionType::Components)
		{
			for (auto CompPtr : SelectedComponents)
			{
				if (USceneComponent* Comp = CompPtr.Get())
				{
					Comp->Modify();
				}
			}
		}
		else
		{
			// Standard actor modification - call Modify() to register with OUR transaction
			for (auto Actor : SelectedActors)
			{
				if (Actor.IsValid())
				{
					Actor->Modify();
					if (USceneComponent* RootComp = Actor->GetRootComponent())
					{
						RootComp->Modify();
					}
				}
			}
		}
	}

	void FTransformSession::SwitchTool(ETransformMode NewMode)
	{
		if (bIsSwitchingTools)
		{
			return;
		}
		TGuardValue<bool> SwitchGuard(bIsSwitchingTools, true);

		if (ActiveMode == NewMode && CurrentTool.IsValid())
		{
			return;
		}

		//Capture OldState for NumericProcessor and EndTool 
		FBlenderNumericState OldState;
		if (CurrentTool.IsValid())
		{
			if (VirtualPivot.IsValid())
			{
				VirtualPivot->RevertTransformToStartState();
			}

			// Clear existing HUD and axis lines. 
			CurrentTool->OnSwitch();
			if (NumericInputProcessor.IsValid())
			{
				OldState = NumericInputProcessor->CurrentState;
				bIsFirstTool = false;
			}

			// Prevents the "Modify components" transaction called inside FSCSEditorViewportClient::HandleBeginTransform()
			// from appearing in the undo history. 
			FViewportClientExposer::CallStopTracking(ActiveViewportClient);

			// If we switch tools mid-session, we want the undo history to reflect the new tool.
			// However, if we started by duplicating, we must keep the original transaction so we don't delete the duplicate.
			if (!bStartedWithDuplicate && ScopedTransaction.IsValid())
			{
				ScopedTransaction->Cancel();
				ScopedTransaction.Reset();
			}

			// If a recursive event (e.g. from Cancel()) terminated the session, abort tool switch.
			if (bHasSessionTerminated)
			{
				return;
			}
		}

		ActiveMode = NewMode;
		int NumNumericSlots = 3;
		EBlenderNumericContext NumericContext = EBlenderNumericContext::Distance;

		switch (NewMode)
		{
		case ETransformMode::Translate:
			CurrentTool = MakeShared<FMoveTool>(AsShared());
			NumericContext = EBlenderNumericContext::Distance;
			break;
		case ETransformMode::Rotate:
			CurrentTool = MakeShared<FRotateTool>(AsShared());
			NumNumericSlots = 1;
			NumericContext = EBlenderNumericContext::Angle_Degrees;
			break;
		case ETransformMode::Scale:
			CurrentTool = MakeShared<FScaleTool>(AsShared());
			NumericContext = EBlenderNumericContext::Scale;
			break;
		default:
			CurrentTool.Reset();
			break;
		}

		if (CurrentTool.IsValid() && NumericInputProcessor.IsValid())
		{
			if (!ScopedTransaction)
			{
				InitializeTransaction(CurrentTool->GetDisplayName());
			}

			if (!CurrentTool->OnBegin())
			{
				CurrentTool->OnEnd(false);
				CurrentTool.Reset();
				bHasSessionTerminated = true;
				return;
			}

			if (bIsFirstTool)
			{
				NumericInputProcessor->Initialize(NumNumericSlots, NumericContext);
			}
			else
			{
				NumericInputProcessor->OnToolSwitch(NumNumericSlots, NumericContext, OldState);
			}

			if (NumericInputProcessor->IsInNumericMode())
			{
				CurrentTool->ApplyNumeric();
			}

			CurrentTool->UpdateHud();

			CurrentTool->OnActive(WrappedMousePosition);
		}
	}

	void FTransformSession::End(bool bApply)
	{
		if (bHasSessionTerminated || !CurrentTool.IsValid())
		{
			return;
		}
		bHasSessionTerminated = true;

		// Only keep transaction if transform actually changed, or duplicated object. 
		if (bApply && !bStartedWithDuplicate)
		{
			bool bHasModifications = false;
			if (!AccumulatedMouseDelta.IsNearlyZero())
			{
				bHasModifications = true;
			}
			else if (IsNumericInputActive())
			{
				bHasModifications = true;
			}

			if (!bHasModifications)
			{
				bApply = false;
			}
		}

		if (bApply)
		{
			CurrentTool->Accept();

			for (auto ActorPtr : SelectedActors)
			{
				if (AActor* Actor = ActorPtr.Get())
				{
					// If the actor is a SkeletalMeshActor, it's highly likely to be driven by a Control Rig.
					// If we fire PostEditChangeProperty on it, Sequencer will ForceEvaluate and wipe any un-keyframed
					// Control Rig offsets. Thus, we skip the property spoofing for Skeletal Mesh Actors.
					if (Actor->IsA<ASkeletalMeshActor>())
					{
						// Without the following code, transforming StaticMeshActors in level viewport, attached to a sequencer
						// resets the transform on save. We need to notify the engine's property system that the relative
						// transform properties have changed.
						if (USceneComponent* RootComp = Actor->GetRootComponent())
						{
							// Fetch transform properties
							FProperty* RelLocProp = USceneComponent::StaticClass()->FindPropertyByName(
								TEXT("RelativeLocation"));
							FProperty* RelRotProp = USceneComponent::StaticClass()->FindPropertyByName(
								TEXT("RelativeRotation"));
							FProperty* RelScaleProp = USceneComponent::StaticClass()->FindPropertyByName(
								TEXT("RelativeScale3D"));

							// Notify pre-edit
							RootComp->Modify();
							RootComp->PreEditChange(RelLocProp);
							RootComp->PreEditChange(RelRotProp);
							RootComp->PreEditChange(RelScaleProp);

							// Notify post-edit
							FPropertyChangedEvent LocEvent(RelLocProp, EPropertyChangeType::ValueSet);
							RootComp->PostEditChangeProperty(LocEvent);

							FPropertyChangedEvent RotEvent(RelRotProp, EPropertyChangeType::ValueSet);
							RootComp->PostEditChangeProperty(RotEvent);

							FPropertyChangedEvent ScaleEvent(RelScaleProp, EPropertyChangeType::ValueSet);
							RootComp->PostEditChangeProperty(ScaleEvent);

							Actor->PostEditMove(true);
							Actor->PostEditChange();
						}
					}

					Actor->PostEditMove(true);
					Actor->PostEditChange();
				}
			}
		}
		else
		{
			CurrentTool->Cancel();

			// If we didn't duplicate, we must explicitly cancel the transaction 
			// to revert moved actors to their start positions.
			if (!bStartedWithDuplicate && ScopedTransaction.IsValid())
			{
				ScopedTransaction->Cancel();
			}
		}

		ScopedTransaction.Reset();
		CurrentTool.Reset();

		FViewportClientExposer::SetViewportState(ActiveViewportClient, /*bInTracking=*/false, /*bAxisControlledByDrag=*/
		                                         false);

		// Restore hit proxy calculation. 
		if (ActiveViewportClient && ActiveViewportClient->GetEditorViewportWidget().IsValid())
		{
			ActiveViewportClient->GetEditorViewportWidget()->SetVisibility(EVisibility::Visible);
		}

		NumericInputProcessor.Reset();

		if (GEditor)
		{
			const UEditorStyleSettings* StyleSettings = GetDefault<UEditorStyleSettings>();
			if (StyleSettings)
			{
				FLinearColor DefaultSelectionColor = StyleSettings->SelectionColor;
				GEditor->SetSelectionOutlineColor(DefaultSelectionColor);
			}
		}
	}

	void FTransformSession::Tick(const float DeltaTime, const FSlateApplication& SlateApp)
	{
		if (!CurrentTool.IsValid())
		{
			return;
		}

		const bool bShift = SlateApp.GetModifierKeys().IsShiftDown();
		CurrentTool->SetPrecisionModeActive(bShift);

		const bool bIsPositionSnapEnabled = GetDefault<ULevelEditorViewportSettings>()->GridEnabled;
		const bool bIsRotationSnapEnabled = GetDefault<ULevelEditorViewportSettings>()->RotGridEnabled;
		const bool bIsScalingSnapEnabled = GetDefault<ULevelEditorViewportSettings>()->SnapScaleEnabled;

		const bool bCtrl = SlateApp.GetModifierKeys().IsControlDown();
		bool bIsSnapEnabled = false;

		switch (ActiveMode)
		{
		case ETransformMode::Translate:
			bIsSnapEnabled = bCtrl ? !bIsPositionSnapEnabled : bIsPositionSnapEnabled;
			break;
		case ETransformMode::Rotate:
			bIsSnapEnabled = bCtrl ? !bIsRotationSnapEnabled : bIsRotationSnapEnabled;
			break;
		case ETransformMode::Scale:
			bIsSnapEnabled = bCtrl ? !bIsScalingSnapEnabled : bIsScalingSnapEnabled;
			break;
		default:
			break;
		}

		CurrentTool->SetSnappingEnabled(bIsSnapEnabled);
		CurrentTool->Tick();

		if (bHasPendingMouseMovement)
		{
			if (NumericInputProcessor->IsInNumericMode())
			{
				CurrentTool->HandleMouseMovement(PendingMousePosition);
			}
			else
			{
				CurrentTool->OnActive(PendingMousePosition);
			}
			bHasPendingMouseMovement = false;
		}

		const bool bAutoSaveEnable = GetDefault<UEditorLoadingSavingSettings>()->bAutoSaveEnable;
		FViewportClientExposer::SetViewportState(ActiveViewportClient, /*bInTracking=*/bAutoSaveEnable,
		                                         /*bAxisControlledByDrag=*/true);
	}

	bool FTransformSession::HandleKeyDownEvent(const FKeyEvent& KeyEvent)
	{
		if (bHasSessionTerminated || !CurrentTool.IsValid())
		{
			return false;
		}
		const auto& Cmd = FBlenderControlsCommands::Get();
		const FKey PressedKey = KeyEvent.GetKey();

		// Helper lambda to check if the pressed key matches either the primary or secondary binding of a command.
		auto KeyMatchesCommand = [&](const TSharedPtr<FUICommandInfo>& Command) -> bool
		{
			if (!Command.IsValid())
			{
				return false;
			}
			const FInputChord PrimaryChord = Command->GetActiveChord(EMultipleKeyBindingIndex::Primary).Get();
			const FInputChord SecondaryChord = Command->GetActiveChord(EMultipleKeyBindingIndex::Secondary).Get();

			return (PrimaryChord.IsValidChord() && PrimaryChord.Key == PressedKey) || (SecondaryChord.IsValidChord() &&
				SecondaryChord.Key == PressedKey);
		};

		if (KeyMatchesCommand(Cmd.CommandTranslate))
		{
			SwitchTool(ETransformMode::Translate);
			return true;
		}
		if (KeyMatchesCommand(Cmd.CommandRotate))
		{
			if (ActiveMode == ETransformMode::Rotate)
			{
				if (!KeyEvent.IsRepeat())
				{
					if (const TSharedPtr<FRotateTool> RotateTool = StaticCastSharedPtr<FRotateTool>(CurrentTool))
					{
						const bool bTrackballRotationState = RotateTool->GetTrackballRotationMode();
						RotateTool->SetTrackballRotationMode(!bTrackballRotationState);
					}
				}
			}
			else
			{
				SwitchTool(ETransformMode::Rotate);
			}
			return true;
		}
		if (KeyMatchesCommand(Cmd.CommandScale))
		{
			SwitchTool(ETransformMode::Scale);
			return true;
		}

		if (KeyMatchesCommand(Cmd.CommandAccept) || KeyMatchesCommand(Cmd.CommandAcceptAlt))
		{
			End(/*bApply=*/true);
			return true;
		}
		if (KeyMatchesCommand(Cmd.CommandCancel))
		{
			End(/*bApply=*/false);
			return true;
		}

		if (KeyMatchesCommand(Cmd.CommandAxisX))
		{
			CurrentTool->HandleAxisLock(KeyEvent.IsShiftDown() ? EAxisLock::YZ : EAxisLock::X);
			return true;
		}
		if (KeyMatchesCommand(Cmd.CommandAxisY))
		{
			CurrentTool->HandleAxisLock(KeyEvent.IsShiftDown() ? EAxisLock::XZ : EAxisLock::Y);
			return true;
		}
		if (KeyMatchesCommand(Cmd.CommandAxisZ))
		{
			CurrentTool->HandleAxisLock(KeyEvent.IsShiftDown() ? EAxisLock::XY : EAxisLock::Z);
			return true;
		}

		if (!NumericInputProcessor.IsValid())
		{
			return false;
		}
		if (NumericInputProcessor->HandleInput(KeyEvent))
		{
			if (NumericInputProcessor->IsInNumericMode())
			{
				CurrentTool->ApplyNumeric();
			}
			CurrentTool->UpdateHud();
			return true;
		}

		return true; // Disable all other inputs while any tool is active
	}

	bool FTransformSession::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
	{
		if (bHasSessionTerminated || !CurrentTool.IsValid())
		{
			return false;
		}

		FVector2D CurrentViewportMousePosition;
		MathHelper::GetMousePosToViewportPos(FSlateApplication::Get().GetCursorPos(),
		                                     CurrentViewportMousePosition);

		PendingMousePosition = CurrentViewportMousePosition;
		bHasPendingMouseMovement = true;

		return true;
	}

	EAxisList::Type FTransformSession::GetResolvedWidgetAxis() const
	{
		switch (LockedAxis)
		{
		case EAxisLock::X:
			return EAxisList::X;
		case EAxisLock::Y:
			return EAxisList::Y;
		case EAxisLock::Z:
			return EAxisList::Z;
		case EAxisLock::XY:
			return EAxisList::XY;
		case EAxisLock::XZ:
			return EAxisList::XZ;
		case EAxisLock::YZ:
			return EAxisList::YZ;
		case EAxisLock::All:
		default:
			return EAxisList::XYZ;
		}
	}

	bool FTransformSession::HandleMouseButtonDownEvent(const FPointerEvent& MouseEvent)
	{
		if (bHasSessionTerminated || !CurrentTool.IsValid())
		{
			return false;
		}

		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			End(/*bApply=*/true);
			return true;
		}
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			End(/*bApply=*/false);
			return true;
		}
		return false;
	}
} // namespace BlenderControls
