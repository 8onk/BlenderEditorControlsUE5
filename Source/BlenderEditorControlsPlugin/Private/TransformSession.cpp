#include "TransformSession.h"
#include "Editor.h"
#include "Selection.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandInfo.h"
#include "BlenderControlsCommands.h"
#include "SSubobjectEditor.h"
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
class FViewportClientExposer : public FEditorViewportClient
{
public:
	static void SetTracking(FEditorViewportClient* Client, bool bInTracking)
	{
		// Cast the client to our exposer so we can write to the protected variable
		static_cast<FViewportClientExposer*>(Client)->bIsTracking = bInTracking;
	}
};

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

		if (ActiveViewportClient && GetDefault<UEditorLoadingSavingSettings>()->bAutoSaveEnable)
		{
			FViewportClientExposer::SetTracking(ActiveViewportClient, true);
		}
	}

	FTransformSession::~FTransformSession()
	{
		if (ActiveViewportClient)
		{
			FViewportClientExposer::SetTracking(ActiveViewportClient, false);
		}

		if (CurrentTool.IsValid())
		{
			CurrentTool->OnEnd(/*bApply=*/false);
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
				if (!ViewportClient) continue;

				TSharedPtr<SEditorViewport> ViewportWidget = ViewportClient->GetEditorViewportWidget();
				if (ViewportWidget.IsValid() && WidgetPath.ContainsWidget(ViewportWidget.Get()))
				{
					return ViewportClient;
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("WidgetPath NULL"));
		return nullptr;
	}

	FBlueprintEditor* FTransformSession::GetActiveBlueprintEditor() const
	{
		if (!GEditor || !ActiveViewportClient)
		{
			return nullptr;
		}

		UWorld* ViewportWorld = ActiveViewportClient->GetWorld();

		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (!AssetEditorSubsystem)
		{
			return nullptr;
		}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
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

			const int32 ActorCount = GEditor->GetSelectedActorCount();
			if (ActorCount > 0)
			{
				SelectionType = ESelectionType::Actors;
				return true;
			}
		}
		else
		{
			if (const FBlueprintEditor* BPEditor = GetActiveBlueprintEditor())
			{
				if (BPEditor->GetSelectedSubobjectEditorTreeNodes().Num())
				{
					SelectionType = ESelectionType::SCSTreeNodes;
					return true;
				}
			}
		}

		SelectionType = ESelectionType::None;
		return false;
	}

	void FTransformSession::PerformDuplicateSelected()
	{
		// Start a transaction immediately for duplication to ensure the 'Spawn' 
		// and 'Move' are part of the same Undo step.
		InitializeTransaction(TEXT("Duplicate Selection"));

		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			ULevel* Level = World->GetCurrentLevel();
			const bool bOffsetLocations = false;
			GEditor->edactDuplicateSelected(Level, bOffsetLocations);
		}
	}

	void FTransformSession::InitializeMouseState()
	{
		if (ActiveViewportClient && ActiveViewportClient->Viewport)
		{
			FIntPoint MousePosInt;
			ActiveViewportClient->Viewport->GetMousePos(MousePosInt);

			StartMousePos = FVector2D(MousePosInt);
			CursorAnchorPoint = StartMousePos;
			VirtualMousePosition = StartMousePos;
			WrappedMousePosition = StartMousePos;
		}

		AccumulatedMouseDelta = FVector2D::ZeroVector;
		GlobalCursorAnchor = FSlateApplication::Get().GetCursorPos();
	}

	void FTransformSession::InitializePivot()
	{
		SelectedActors.Empty();
		SelectedRigElements.Empty();
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
				return;
			}

			TArray<TSharedPtr<FSubobjectEditorTreeNode>> SelectedNodes = BPEditor->
				GetSelectedSubobjectEditorTreeNodes();
			VirtualPivot = MakeShared<FSCSPivot>(BPEditor, SelectedNodes);
		}
		else
		{
			USelection* ActorSelection = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*ActorSelection); It; ++It)
			{
				if (AActor* Actor = Cast<AActor>(*It))
				{
					SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
				}
			}

			VirtualPivot = MakeShared<FActorPivot>(SelectedActors, PivotMode);
		}
	}

	void FTransformSession::InitializeTransaction(const FString& InTransactionName)
	{
		// SCSTreeNodes are transformed using internal InputWidgetDelta() thus transaction is handled automatically. 
		if (ScopedTransaction || SelectionType == ESelectionType::SCSTreeNodes)
		{
			return;
		}

		// Check if a transaction is already active (e.g., from Animation Mode's "Select Control")
		// Animation Mode starts a transaction on click that can conflict with ours.
		// We need to end it first to avoid our changes being reverted when their transaction ends.
		if (GEditor && GEditor->IsTransactionActive())
		{
			UE_LOG(LogTransformSession, Warning,
			       TEXT("InitializeTransaction: Ending existing active transaction to avoid conflict"));
			GEditor->EndTransaction();
		}

		// Now create our own transaction
		FText TransactionName = FText::FromString(InTransactionName + TEXT(" - BlenderEditorControls"));
		ScopedTransaction = MakeUnique<FScopedTransaction>(TransactionName);

		if (SelectionType == ESelectionType::ControlRig)
		{
			// For Control Rig, mark the rig for modification
			FControlRigSelectionHelper::BeginTransaction(TransactionName);
		}
		else
		{
			// Standard actor modification - call Modify() to register with OUR transaction
			for (auto Actor : SelectedActors)
			{
				if (Actor.IsValid())
				{
					Actor->Modify();
				}
			}
		}
	}

	void FTransformSession::SwitchTool(ETransformMode NewMode)
	{
		if (ActiveMode == NewMode && CurrentTool.IsValid()) return;

		//Capture OldState for NumericProcessor and EndTool 
		FBlenderNumericState OldState;
		if (CurrentTool.IsValid())
		{
			if (VirtualPivot.IsValid())
			{
				VirtualPivot->RevertToStartState();
			}

			CurrentTool->OnSwitch();
			if (NumericInputProcessor.IsValid())
			{
				OldState = NumericInputProcessor->CurrentState;
				bIsFirstTool = false;
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
			CurrentTool->OnBegin();

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

			if (!ScopedTransaction)
			{
				InitializeTransaction(CurrentTool->GetDisplayName());
			}

			CurrentTool->OnActive(WrappedMousePosition);
		}
	}

	void FTransformSession::End(bool bApply)
	{
		if (bHasSessionTerminated || !CurrentTool.IsValid()) return;

		if (bApply)
		{
			CurrentTool->Accept();
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
		bHasSessionTerminated = true;
	}

	void FTransformSession::Tick(const float DeltaTime, FSlateApplication& SlateApp)
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
	}

	bool FTransformSession::HandleKeyDownEvent(const FKeyEvent& KeyEvent)
	{
		if (!CurrentTool.IsValid()) return false;

		const auto& Cmd = FBlenderControlsCommands::Get();
		const FKey PressedKey = KeyEvent.GetKey();

		// Helper lambda to check if the pressed key matches either the primary or secondary binding of a command.
		auto KeyMatchesCommand = [&](const TSharedPtr<FUICommandInfo>& Command) -> bool
		{
			if (!Command.IsValid()) return false;

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
					const bool bTrackballRotationState = CurrentTool->GetTrackballRotationMode();
					CurrentTool->SetTrackballRotationMode(!bTrackballRotationState);
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

		if (!NumericInputProcessor.IsValid()) return false;

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
		if (!CurrentTool.IsValid())
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
