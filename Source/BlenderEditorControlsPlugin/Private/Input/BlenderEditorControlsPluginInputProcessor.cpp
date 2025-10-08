#include "Input/BlenderEditorControlsPluginInputProcessor.h"

#include "LevelEditor.h"
#include "Selection.h"
#include "SLevelViewport.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Utils/BlenderMathHelpers.h"
#include "Editor/UnrealEd/Classes/Settings/LevelEditorViewportSettings.h"
#include "Editor/UnrealEd/Public/EditorViewportClient.h"
#include "Misc/DefaultValueHelper.h"
#include "Tools/SharedPivot.h"

/*TODO
- When switching from rotation to scale, convert radians to units and units to degrees
- IMPROVE icons
- ADD settings
- Check version compatability
*/

class SLevelViewport;

namespace BlenderControls
{
	FBlenderControlsInputProcessor::FBlenderControlsInputProcessor(
		TSharedPtr<FUICommandList> InCommandList) : CommandList(InCommandList)
	{
	}

	FBlenderControlsInputProcessor::~FBlenderControlsInputProcessor()
	{
	}

	void FBlenderControlsInputProcessor::BindCommands()
	{
		const auto& Cmd = FBlenderEditorControlsPluginCommands::Get();

		CommandList->MapAction(Cmd.CommandTranslate,
		                       FExecuteAction::CreateSP(SharedThis(this),
		                                                &FBlenderControlsInputProcessor::TranslatePressed),
		                       FCanExecuteAction());

		CommandList->MapAction(Cmd.CommandRotate,
		                       FExecuteAction::CreateSP(SharedThis(this),
		                                                &FBlenderControlsInputProcessor::RotatePressed),
		                       FCanExecuteAction());

		CommandList->MapAction(Cmd.CommandScale,
		                       FExecuteAction::CreateSP(SharedThis(this),
		                                                &FBlenderControlsInputProcessor::ScalePressed),
		                       FCanExecuteAction());

		CommandList->MapAction(Cmd.CommandDuplicateAndMove,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::DuplicateAndMovePressed),
		                       FCanExecuteAction());

		//Accept / Cancel
		CommandList->MapAction(Cmd.CommandAccept,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::AcceptPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));

		CommandList->MapAction(Cmd.CommandAcceptAlt,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::AcceptPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));

		CommandList->MapAction(Cmd.CommandCancel,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::CancelPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));

		//Axis locks
		CommandList->MapAction(Cmd.CommandAxisX,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::AxisXPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));

		CommandList->MapAction(Cmd.CommandAxisY,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::AxisYPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));

		CommandList->MapAction(Cmd.CommandAxisZ,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::AxisZPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));

		//Numeric helpers
		CommandList->MapAction(Cmd.CommandNumericBackspace,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::NumericBackspacePressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsNumericActive));

		CommandList->MapAction(Cmd.CommandNumericToggleNegation,
		                       FExecuteAction::CreateSP(
			                       this, &FBlenderControlsInputProcessor::NumericToggleNegationPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsNumericActive));

		CommandList->MapAction(Cmd.CommandNumericToggleReciprocal,
		                       FExecuteAction::CreateSP(
			                       this, &FBlenderControlsInputProcessor::NumericToggleReciprocalPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsNumericActive));

		CommandList->MapAction(Cmd.CommandNumericCycleSlot,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::NumericCycleSlotPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsNumericActive));

		//Trackball toggle 
		CommandList->MapAction(Cmd.CommandToggleTrackball,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::ToggleTrackballPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsRotateToolActive));

		//Modifier toggles
		CommandList->MapAction(Cmd.CommandPrecisionMode,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::PrecisionPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));

		CommandList->MapAction(Cmd.CommandSnapInvert,
		                       FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::SnapInvertPressed),
		                       FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::IsToolActive));
	}

	void FBlenderControlsInputProcessor::Tick(const float DeltaTime, FSlateApplication& App, TSharedRef<ICursor>)
	{
		if (!bActive || !CurrentTool.IsValid())
		{
			return;
		}

		const bool bShift = App.GetModifierKeys().IsShiftDown();
		CurrentTool->SetPrecisionModeActive(bShift);

		const bool bIsPositionSnapEnabled = GetDefault<ULevelEditorViewportSettings>()->GridEnabled;
		const bool bIsRotationSnapEnabled = GetDefault<ULevelEditorViewportSettings>()->RotGridEnabled;
		const bool bIsScalingSnapEnabled = GetDefault<ULevelEditorViewportSettings>()->SnapScaleEnabled;

		const bool bCtrl = App.GetModifierKeys().IsControlDown();
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
	}

	bool FBlenderControlsInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent)
	{
		if (!ShouldHandleToolHotkeys(SlateApp))
		{
			return false;
		}

		const FKey PressedKey = KeyEvent.GetKey();
		if (PressedKeys.Contains(PressedKey))
		{
			return false;
		}
		PressedKeys.Add(PressedKey);

		if (CommandList.IsValid() && CommandList->ProcessCommandBindings(KeyEvent))
		{
			return true;
		}

		//Shit+X/Y/Z won't trigger because the check for "shift" runs in the
		//HandleAxisKey which only triggers on X/Y/Z alone. So this is needed
		//for the dual axis lock trigger.
		if (bActive && CurrentTool.IsValid())
		{
			const auto& Cmd = FBlenderEditorControlsPluginCommands::Get();

			// If the base axis key was pressed (even with Shift), handle it here:
			if (MatchesCommandKeyIgnoringModifiers(KeyEvent, Cmd.CommandAxisX))
			{
				HandleAxisKey(EKeys::X); 
				return true; 
			}
			if (MatchesCommandKeyIgnoringModifiers(KeyEvent, Cmd.CommandAxisY))
			{
				HandleAxisKey(EKeys::Y);
				return true;
			}
			if (MatchesCommandKeyIgnoringModifiers(KeyEvent, Cmd.CommandAxisZ))
			{
				HandleAxisKey(EKeys::Z);
				return true;
			}
		}


		if (!CurrentTool.IsValid())
		{
			return false;
		}

		if (TCHAR Char; TryMapKeyToNumericChar(PressedKey, Char))
		{
			const bool bIsDigit = FChar::IsDigit(Char);
			const bool bIsDot = (Char == '.');

			if (!bIsDigit && !bIsDot)
			{
				return false;
			}

			if (!Session->bIsNumericInputActive)
			{
				for (int i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
				{
					Session->NumericSlots[i] = FNumericSlotData();
				}
				Session->CurrentNumericSlotIndex = 0;

				CurrentTool->BeginNumericMode();
			}

			NumericBuffer.AppendChar(Char);

			CurrentTool->UpdateActiveNumericSlot(Char);
			return true;
		}

		return false;
	}

	static bool IsTextEntryWidget(const TSharedPtr<SWidget>& W)
	{
		if (!W.IsValid()) return false;

		const FString Type = W->GetTypeAsString();
		return Type.Contains(TEXT("EditableText"), ESearchCase::IgnoreCase)
			|| Type.Contains(TEXT("EditableTextBox"), ESearchCase::IgnoreCase)
			|| Type.Contains(TEXT("MultiLineEditableText"), ESearchCase::IgnoreCase)
			|| Type.Contains(TEXT("SearchBox"), ESearchCase::IgnoreCase)
			|| Type.Contains(TEXT("ConsoleInput"), ESearchCase::IgnoreCase)
			|| Type.Contains(TEXT("NumericEntryBox"), ESearchCase::IgnoreCase);
	}

	bool FBlenderControlsInputProcessor::ShouldHandleToolHotkeys(FSlateApplication& SlateApp) const
	{
		if (!IsMouseOverLevelViewport())
		{
			return false;
		}

		// Menus/popups up? bail. (UE 5.x uses AnyMenusVisible)
		if (SlateApp.AnyMenusVisible())
		{
			return false;
		}

		// If user is typing in any text field, don't eat keys.
		const TSharedPtr<SWidget> Focused = SlateApp.GetKeyboardFocusedWidget();
		if (IsTextEntryWidget(Focused))
		{
			return false;
		}

		FViewport* ActiveViewport = GEditor ? GEditor->GetActiveViewport() : nullptr;
		FEditorViewportClient* Client = ActiveViewport
			                                ? static_cast<FEditorViewportClient*>(ActiveViewport->GetClient())
			                                : nullptr;
		if (!Client || !Client->IsLevelEditorClient())
		{
			return false;
		}

		if (GEditor && GEditor->IsPlayingSessionInEditor())
		{
			return false;
		}

		return true;
	}

	bool FBlenderControlsInputProcessor::IsMouseOverLevelViewport() const
	{
		auto& App = FSlateApplication::Get();
		const FVector2D ScreenPos = App.GetCursorPos();

		FWidgetPath Path = App.LocateWindowUnderMouse(
			ScreenPos,
			App.GetInteractiveTopLevelWindows(),
			true
		);
		if (!Path.IsValid())
		{
			return false;
		}

		if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
		{
			FLevelEditorModule& LevelEd = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

			// Try the explicit LevelViewport widget route
			if (TSharedPtr<SLevelViewport> SLVP = LevelEd.GetFirstActiveLevelViewport())
			{
				if (Path.ContainsWidget(SLVP.Get()))
				{
					return true;
				}
			}

			// Fallback: scan path for SLevelViewport
			for (int32 i = 0; i < Path.Widgets.Num(); ++i)
			{
				const FArrangedWidget& Arranged = Path.Widgets[i];
				const TSharedRef<SWidget>& W = Arranged.Widget;
				const FString Type = W->GetTypeAsString();
				if (Type.Contains(TEXT("SLevelViewport")) || Type.Contains(TEXT("SEditorViewport")))
				{
					return true;
				}
			}
		}

		return false;
	}

	bool FBlenderControlsInputProcessor::MatchesCommandKeyIgnoringModifiers(const FKeyEvent& KeyEvent,
	                                                                        const TSharedPtr<FUICommandInfo>& CmdInfo)
	{
		if (!CmdInfo.IsValid())
		{
			return false;
		}

		const FInputChord& ChordP = CmdInfo->GetActiveChord(EMultipleKeyBindingIndex::Primary).Get();
		const FInputChord& ChordS = CmdInfo->GetActiveChord(EMultipleKeyBindingIndex::Secondary).Get();

		const FKey K = KeyEvent.GetKey();
		const bool MatchPrimary = (K == ChordP.Key);
		const bool MatchSecondary = (K == ChordS.Key);
		return MatchPrimary || MatchSecondary;
	}

	bool FBlenderControlsInputProcessor::HandleKeyUpEvent(FSlateApplication&, const FKeyEvent& KeyEvent)
	{
		const FKey PressedKey = KeyEvent.GetKey();
		PressedKeys.Remove(PressedKey);

		return false;
	}

	bool FBlenderControlsInputProcessor::HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& MouseEvent)
	{
		if (!bActive || !CurrentTool.IsValid() || Session->bIsNumericInputActive)
		{
			return false; // plugin disabled or numeric typing
		}

		FVector2D CurrentViewportMousePosition;
		MathHelper::GetMousePosToViewportPos(FSlateApplication::Get().GetCursorPos(),
		                                     CurrentViewportMousePosition);

		CurrentTool->OnActive(CurrentViewportMousePosition);

		return true;
	}

	bool FBlenderControlsInputProcessor::HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent& MouseEvent)
	{
		if (!bActive || !CurrentTool.IsValid())
		{
			return false;
		}

		/* LMB = accept  —  RMB/Escape = cancel  */
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			EndTool(/*bApply=*/true);
			return true;
		}
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			EndTool(/*bApply=*/false);
			return true;
		}

		return false;
	}

	bool FBlenderControlsInputProcessor::HandleMouseButtonUpEvent(FSlateApplication&, const FPointerEvent&)
	{
		return false;
	}

	bool FBlenderControlsInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp,
	                                                                    const FPointerEvent& InWheelEvent,
	                                                                    const FPointerEvent* InGestureEvent)
	{
		if (bActive && CurrentTool.IsValid()) //Disable zoom whilst a tool is active
		{
			return true;
		}

		return false;
	}

	void FBlenderControlsInputProcessor::BeginTool(const ETransformMode Mode)
	{
		if (GEditor->GetSelectedActorCount() == 0 || Mode == ActiveMode)
		{
			return;
		}

		if (CurrentTool.IsValid())
		{
			CurrentTool->OnEnd(/*bApply=*/false);
		}

		if (!Session.IsValid())
		{
			Session = MakeShared<FTransformSession>();

			if (FViewport* Viewport = GEditor->GetActiveViewport())
			{
				FIntPoint MousePosInt;
				Viewport->GetMousePos(MousePosInt);
				Session->StartMousePos = FVector2D(MousePosInt);
				Session->CursorAnchorPoint = Session->StartMousePos;
				Session->VirtualMousePosition = Session->StartMousePos;
				Session->WrappedCursorPosition = Session->StartMousePos;
			}

			constexpr EPivotMode PivotMode = EPivotMode::MedianPoint;
			CaptureSelection();
			Session->VirtualPivot = MakeShared<FSharedPivot>(Session->SelectedActors, PivotMode);

			Session->LockedAxis = EAxisLock::All;
			Session->bUsingLocalSpace = false;

			Session->bIsNumericInputActive = false;
			Session->NumericBuffer.Reset();
			Session->CurrentNumericSlotIndex = 0;

			for (int i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
			{
				Session->NumericSlots[i] = FNumericSlotData();
			}
		}
		else
		{
			Session->VirtualPivot->RevertToStartState();
		}

		// Use the session's axis lock as the initial axis for the new tool.
		const EAxisLock InitialAxis = Session->LockedAxis;

		switch (Mode)
		{
		case ETransformMode::Translate:
			CurrentTool = MakeShared<FMoveTool>(Session, InitialAxis);
			break;
		case ETransformMode::Rotate:
			CurrentTool = MakeShared<FRotateTool>(Session, InitialAxis);
			break;
		case ETransformMode::Scale:
			CurrentTool = MakeShared<FScaleTool>(Session, InitialAxis);
			break;
		case ETransformMode::None:
			return;
		default:
			return;
		}

		ActiveMode = Mode;
		CurrentTool->OnBegin();

		if (FViewport* Viewport = GEditor->GetActiveViewport())
		{
			FIntPoint MousePosInt;
			Viewport->GetMousePos(MousePosInt);
			const FVector2D CurrentMousePos(MousePosInt);

			//Force an immediate visual update after tool creation
			CurrentTool->OnActive(CurrentMousePos);
		}

		if (Session->bIsNumericInputActive)
		{
			// Restore the state in the Input Processor
			// bNumericInput = true;
			NumericBuffer = Session->NumericBuffer;

			CurrentTool->BeginNumericMode();

			double ParsedValue;
			if (FDefaultValueHelper::ParseDouble(NumericBuffer, ParsedValue))
			{
				CurrentTool->ApplyNumeric(ParsedValue);
			}
		}
	}

	void FBlenderControlsInputProcessor::CaptureSelection() const
	{
		Session->SelectedActors.Empty();

		if (GEditor)
		{
			USelection* ActorSelection = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*ActorSelection); It; ++It)
			{
				if (AActor* Actor = Cast<AActor>(*It))
				{
					Session->SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
				}
			}
		}
	}

	void FBlenderControlsInputProcessor::EndTool(const bool bApply)
	{
		if (!CurrentTool.IsValid())
		{
			return;
		}

		if (bApply)
		{
			CurrentTool->Accept();
		}
		else
		{
			CurrentTool->Cancel();
		}

		CurrentTool->OnEnd(bApply);
		CurrentTool.Reset();

		// Reset state
		ActiveMode = ETransformMode::None;
		Session->bIsNumericInputActive = false;
		NumericBuffer.Reset();

		Session.Reset();
	}

	bool FBlenderControlsInputProcessor::TryMapKeyToNumericChar(const FKey& Key, TCHAR& OutChar)
	{
		// Main row
		if (Key == EKeys::Zero)
		{
			OutChar = '0';
			return true;
		}
		if (Key == EKeys::One)
		{
			OutChar = '1';
			return true;
		}
		if (Key == EKeys::Two)
		{
			OutChar = '2';
			return true;
		}
		if (Key == EKeys::Three)
		{
			OutChar = '3';
			return true;
		}
		if (Key == EKeys::Four)
		{
			OutChar = '4';
			return true;
		}
		if (Key == EKeys::Five)
		{
			OutChar = '5';
			return true;
		}
		if (Key == EKeys::Six)
		{
			OutChar = '6';
			return true;
		}
		if (Key == EKeys::Seven)
		{
			OutChar = '7';
			return true;
		}
		if (Key == EKeys::Eight)
		{
			OutChar = '8';
			return true;
		}
		if (Key == EKeys::Nine)
		{
			OutChar = '9';
			return true;
		}

		// Numpad
		if (Key == EKeys::NumPadZero)
		{
			OutChar = '0';
			return true;
		}
		if (Key == EKeys::NumPadOne)
		{
			OutChar = '1';
			return true;
		}
		if (Key == EKeys::NumPadTwo)
		{
			OutChar = '2';
			return true;
		}
		if (Key == EKeys::NumPadThree)
		{
			OutChar = '3';
			return true;
		}
		if (Key == EKeys::NumPadFour)
		{
			OutChar = '4';
			return true;
		}
		if (Key == EKeys::NumPadFive)
		{
			OutChar = '5';
			return true;
		}
		if (Key == EKeys::NumPadSix)
		{
			OutChar = '6';
			return true;
		}
		if (Key == EKeys::NumPadSeven)
		{
			OutChar = '7';
			return true;
		}
		if (Key == EKeys::NumPadEight)
		{
			OutChar = '8';
			return true;
		}
		if (Key == EKeys::NumPadNine)
		{
			OutChar = '9';
			return true;
		}

		// Decimal separators
		if (Key == EKeys::Decimal || Key == EKeys::Period || Key == EKeys::Delete)
		{
			OutChar = '.';
			return true;
		}

		// if (Key == EKeys::Hyphen)
		// {
		// 	OutChar = '-';
		// 	return true;
		// }

		return false;
	}

	void FBlenderControlsInputProcessor::HandleAxisKey(FKey Key) const
	{
		if (!CurrentTool.IsValid())
		{
			return;
		}

		const bool bShift = FSlateApplication::Get().GetModifierKeys().IsShiftDown();
		if (Key == EKeys::X)
		{
			CurrentTool->HandleAxisLock(bShift ? (EAxisLock::Y | EAxisLock::Z) : EAxisLock::X);
		}
		else if (Key == EKeys::Y)
		{
			CurrentTool->HandleAxisLock(bShift ? (EAxisLock::X | EAxisLock::Z) : EAxisLock::Y);
		}
		else if (Key == EKeys::Z)
		{
			CurrentTool->HandleAxisLock(bShift ? (EAxisLock::X | EAxisLock::Y) : EAxisLock::Z);
		}
	}

	void FBlenderControlsInputProcessor::DuplicateAndMovePressed()
	{
		if (CurrentTool.IsValid() || GEditor->GetSelectedActorCount() == 0)
		{
			return;
		}

		const FScopedTransaction Transaction(FText::FromString(TEXT("Duplicate and Move Actors")));

		USelection* SelectedActorsSet = GEditor->GetSelectedActors();
		TArray<AActor*> ActorsToDuplicate;
		SelectedActorsSet->GetSelectedObjects<AActor>(ActorsToDuplicate);

		if (ActorsToDuplicate.Num() > 0)
		{
			ULevel* Level = GEditor->GetEditorWorldContext().World()->GetCurrentLevel();
			constexpr bool bOffsetLocations = false;
			GEditor->edactDuplicateSelected(Level, bOffsetLocations);
		}

		BeginTool(ETransformMode::Translate);
	}
} // namespace BlenderControls
