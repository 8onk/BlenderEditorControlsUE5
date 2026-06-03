#include "TransformSession.h"
#include "Editor.h"
#include "Selection.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandInfo.h"
#include "BlenderControlsCommands.h"
#include "BlueprintEditorModule.h"
#include "SSubobjectEditor.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Kismet2/BlueprintEditorUtils.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
#include "BlenderControlsSettings.h"
#endif
#include "Input/Numeric/NumericInputProcessor.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
#include "Settings/EditorStyleSettings.h"
#else
#include "Classes/EditorStyleSettings.h"
#endif
#include "Tools/SharedPivot.h"
#include "Tools/ToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Utils/MathHelpers.h"
#include "ControlRig/ControlRigSelectionHelper.h"
#include "ControlRig/ControlRigPivot.h"
#include "LevelEditorViewport.h"
#include "SCSEditorViewportClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogTransformSession, Log, All);

namespace BlenderControls
{
	FTransformSession::FTransformSession(ETransformMode InStartMode, bool bDuplicateSelection)
	{
		if (!ValidateEditorState())
		{
			bIsSessionFinished = true;
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
	}

	FTransformSession::~FTransformSession()
	{
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

	bool IsUserFocusInLevelEditor()
	{
		if (!FSlateApplication::Get().IsInitialized()) return false;

		// Get the specific widget the user is interacting with right now
		TSharedPtr<SWidget> CurrentWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();

		// Walk up the Slate UI tree to see if this widget lives inside the Level Editor
		while (CurrentWidget.IsValid())
		{
			FString WidgetType = CurrentWidget->GetTypeAsString();

			if (WidgetType == "SLevelEditor" || WidgetType == "SLevelViewport")
			{
				return true;
			}

			CurrentWidget = CurrentWidget->GetParentWidget();
		}

		return false;
	}

	IBlueprintEditor* FTransformSession::GetActiveBlueprintEditor()
	{
		if (!GEditor)
		{
			return nullptr;
		}

		//IF Main level viewport is active tab, return nullptr.
		TSharedPtr<SDockTab> ActiveTab = FGlobalTabmanager::Get()->GetActiveTab();
		if (ActiveTab.IsValid())
		{
			FName TabName = ActiveTab->GetLayoutIdentifier().TabType;
			//UE_LOG(LogTransformSession, Log, TEXT("Active tab: %s"), *TabName.ToString());

			//Main level viewport name is: LevelEditorViewport
			if (TabName == FName("LevelEditorViewport"))
			{
				return nullptr;
			}
		}

		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (!AssetEditorSubsystem) return nullptr;

		TArray<IAssetEditorInstance*> OpenEditors = AssetEditorSubsystem->GetAllOpenEditors();

		IBlueprintEditor* FocusedBPEditor = nullptr;
		double MaxLastActivationTime = 0.0;

		for (IAssetEditorInstance* Editor : OpenEditors)
		{
			// Ensure the open editor instance is a Blueprint Editor
			UE_LOG(LogTransformSession, Log, TEXT("EDITOR NAME: %s"), *Editor->GetEditorName().ToString());
			if (Editor && Editor->GetEditorName() == FName("BlueprintEditor"))
			{
				// The editor with the highest activation time is the active/focused one
				if (Editor->GetLastActivationTime() > MaxLastActivationTime)
				{
					MaxLastActivationTime = Editor->GetLastActivationTime();
					FocusedBPEditor = static_cast<IBlueprintEditor*>(Editor);
				}
			}
		}

		return FocusedBPEditor;
	}

	bool FTransformSession::ValidateEditorState()
	{
		if (!GEditor)
		{
			return false;
		}

		// Debug: Log current selection state
		const bool bControlRigModeActive = FControlRigSelectionHelper::IsControlRigEditModeActive();
		const bool bHasRigElements = FControlRigSelectionHelper::HasSelectedRigElements();
		const int32 ActorCount = GEditor->GetSelectedActorCount();

		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (!AssetEditorSubsystem)
		{
			return false;
		}

		if (IBlueprintEditor* BlueprintEditorInst = GetActiveBlueprintEditor())
		{
			UE_LOG(LogTransformSession, Error, TEXT("Blueprint editor instance ACTIVE!"));
			
			FViewport* ActiveViewport = GEditor->GetActiveViewport();
			if (ActiveViewport)
			{
				FViewportClient* BaseClient = ActiveViewport->GetClient();
    
				//So despite casting to FSCSEditorViewportClient, it calls the overload function of FLevelEditorViewport. 
				FSCSEditorViewportClient* SCSClient = dynamic_cast<FSCSEditorViewportClient*>(BaseClient);

				if (SCSClient)
				{
					UE_LOG(LogTransformSession, Log, TEXT("SCSEDITOR ACTIVE!!!!!!!!!!!"));
					FVector Drag(100.0f, 0.0f, 0.0f);
					FRotator Rot = FRotator::ZeroRotator;
					FVector Scale = FVector::ZeroVector;

					SCSClient->InputWidgetDelta(ActiveViewport, EAxisList::X, Drag, Rot, Scale);
				}
			}
		}
		else
		{
			UE_LOG(LogTransformSession, Error, TEXT("Blueprint editor instance NOT active!"));
		}


		if (ActorCount > 0)
		{
			USelection* ActorSelection = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*ActorSelection); It; ++It)
			{
				if (AActor* Actor = Cast<AActor>(*It))
				{
					UE_LOG(LogTransformSession, Warning, TEXT("  Selected Actor: %s (Class: %s)"),
					       *Actor->GetName(), *Actor->GetClass()->GetName());
				}
			}
		}

		// First, check if we have Control Rig elements selected (Animation Mode + Control Rig)
		// This takes priority because the user might have both actors and rig elements selected
		if (bControlRigModeActive && bHasRigElements)
		{
			SelectionType = ESelectionType::ControlRig;
			//UE_LOG(LogTransformSession, Warning, TEXT("ValidateEditorState: RESULT -> Control Rig selection"));
			return true;
		}

		// Fall back to standard actor selection
		if (ActorCount > 0)
		{
			SelectionType = ESelectionType::Actors;
			//UE_LOG(LogTransformSession, Warning, TEXT("ValidateEditorState: RESULT -> Actor selection"));
			return true;
		}

		SelectionType = ESelectionType::None;
		//UE_LOG(LogTransformSession, Warning, TEXT("ValidateEditorState: RESULT -> None"));
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
		if (FViewport* Viewport = GEditor->GetActiveViewport())
		{
			FIntPoint MousePosInt;
			Viewport->GetMousePos(MousePosInt);

			StartMousePos = FVector2D(MousePosInt);
			CursorAnchorPoint = StartMousePos;
			VirtualMousePosition = StartMousePos;
			WrappedMousePosition = StartMousePos;
		}
	}

	void FTransformSession::InitializePivot()
	{
		SelectedActors.Empty();
		SelectedRigElements.Empty();
		VirtualPivot.Reset();
		ControlRigVirtualPivot.Reset();

		constexpr EPivotMode PivotMode = EPivotMode::MedianPoint;

		if (SelectionType == ESelectionType::ControlRig)
		{
			// Get Control Rig element selection
			FControlRigSelectionHelper::GetSelectedRigElements(SelectedRigElements);

			UE_LOG(LogTransformSession, Log, TEXT("InitializePivot: Initialized with %d Control Rig elements"),
			       SelectedRigElements.Num());

			// Create the Control Rig pivot for transformations
			ControlRigVirtualPivot = MakeShared<FControlRigPivot>(SelectedRigElements, PivotMode);
		}
		else
		{
			// Standard actor selection
			USelection* ActorSelection = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*ActorSelection); It; ++It)
			{
				if (AActor* Actor = Cast<AActor>(*It))
				{
					SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
					UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *Actor->GetName());
				}
			}

			VirtualPivot = MakeShared<FSharedPivot>(SelectedActors, PivotMode);
		}

		USelection* SelectedComponents = GEditor->GetSelectedComponents();
		for (FSelectionIterator It(*SelectedComponents); It; ++It)
		{
			if (USceneComponent* Component = Cast<USceneComponent>(*It))
			{
				UE_LOG(LogTemp, Warning, TEXT("Selected Blueprint Component: %s"), *Component->GetName());
			}
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
			// Revert the appropriate pivot type
			if (SelectionType == ESelectionType::ControlRig && ControlRigVirtualPivot.IsValid())
			{
				ControlRigVirtualPivot->RevertToStartState();
			}
			else if (VirtualPivot.IsValid())
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
		if (bIsSessionFinished || !CurrentTool.IsValid()) return;

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
		bIsSessionFinished = true;
	}

	void FTransformSession::Tick(const float DeltaTime, FSlateApplication& SlateApp) const
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

	bool FTransformSession::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) const
	{
		if (!CurrentTool.IsValid())
		{
			return false;
		}

		FVector2D CurrentViewportMousePosition;
		MathHelper::GetMousePosToViewportPos(FSlateApplication::Get().GetCursorPos(),
		                                     CurrentViewportMousePosition);

		// Add a sensitivity multiplier for ue 5.7, because ue 5.7 uses raw input, as mouse acceleration
		// doesn't kick in for some reason in ue 5.7...
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		FVector2D RawDelta = CurrentViewportMousePosition - CursorAnchorPoint;
		CurrentViewportMousePosition = CursorAnchorPoint + (RawDelta * GetDefault<UBlenderControlsSettings>()->
			MouseSensitivity);
#endif

		if (NumericInputProcessor->IsInNumericMode())
		{
			CurrentTool->HandleMouseMovement(CurrentViewportMousePosition);
		}
		else
		{
			CurrentTool->OnActive(CurrentViewportMousePosition);
		}

		return true;
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
