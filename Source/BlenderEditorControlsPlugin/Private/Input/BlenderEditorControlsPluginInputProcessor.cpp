#include "Input/BlenderEditorControlsPluginInputProcessor.h"

#include "LevelEditor.h"
#include "Selection.h"
#include "SLevelViewport.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Tools/BlenderToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Containers/Ticker.h"
#include "Utils/BlenderMathHelpers.h"
#include "Editor/UnrealEd/Classes/Settings/LevelEditorViewportSettings.h"
#include "Editor/UnrealEd/Public/EditorViewportClient.h"
#include "Misc/DefaultValueHelper.h"
#include "Tools/SharedPivot.h"
//TODO issue with mouse wrapping, sometimes the current mouse pos is stale due to race condition which makes delta incorrect. 

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
        const auto& Commands = FBlenderEditorControlsPluginCommands::Get();

        // G  – Translate
        CommandList->MapAction(
            Commands.CommandTranslate,
            FExecuteAction::CreateSP(SharedThis(this), &FBlenderControlsInputProcessor::TranslatePressed),
            FCanExecuteAction());

        // R – Rotate
        CommandList->MapAction(
            Commands.CommandRotate,
            FExecuteAction::CreateSP(SharedThis(this), &FBlenderControlsInputProcessor::RotatePressed),
            FCanExecuteAction());

        // S – Scale
        CommandList->MapAction(
            Commands.CommandScale,
            FExecuteAction::CreateSP(SharedThis(this), &FBlenderControlsInputProcessor::ScalePressed),
            FCanExecuteAction());

        CommandList->MapAction(
            Commands.CommandDuplicateAndMove,
            FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::DuplicateAndMovePressed),
            FCanExecuteAction());
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

        // Overlay may want to animate a fade, so pass DeltaTime
        /*if (Overlay.IsValid())
        {
            Overlay->Tick(DeltaTime);
        }*/
    }

    bool FBlenderControlsInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent)
    {
        if (!ShouldHandleToolHotkeys(SlateApp))
        {
            return false; // Let the editor / focused widget handle it
        }

        const FKey PressedKey = KeyEvent.GetKey();
        const bool bShift = KeyEvent.IsShiftDown();
        if (PressedKeys.Contains(PressedKey))
        {
            return false;
        }
        PressedKeys.Add(PressedKey);

        const auto& Commands = FBlenderEditorControlsPluginCommands::Get();
        const FKey RotateKey = Commands.CommandRotate->GetActiveChord(EMultipleKeyBindingIndex::Primary)->Key;

        if (PressedKey == RotateKey && ActiveMode == ETransformMode::Rotate)
        {
            const bool bTrackballRotationModeStat = CurrentTool->GetTrackballRotationMode();
            CurrentTool->SetTrackballRotationMode(!bTrackballRotationModeStat);
        }

        if (CommandList.IsValid() && CommandList->ProcessCommandBindings(KeyEvent))
        {
            return true; // G/R/S (or remapped key) handled
        }

        if (!CurrentTool.IsValid())
        {
            return false;
        }

        if (TCHAR Char; TryMapKeyToNumericChar(PressedKey, Char))
        {
            if (!bNumericInput)
            {
                if (!FChar::IsDigit(Char) && Char != '-' && Char != '.')
                {
                    return false;
                }

                for (int i = 0; i < UE_ARRAY_COUNT(CurrentSession->NumericSlots); ++i)
                {
                    CurrentSession->NumericSlots[i] = FNumericSlotData();
                }
                CurrentSession->CurrentNumericSlotIndex = 0;

                bNumericInput = true;
                CurrentTool->BeginNumericMode();
            }

            if (Char == '-')
            {
                if (NumericBuffer.Contains(TEXT("-")))
                {
                    NumericBuffer.RemoveFromStart(TEXT("-"));
                }
                else
                {
                    NumericBuffer.InsertAt(0, TEXT("-"));
                }
            }
            else
            {
                NumericBuffer.AppendChar(Char);
            }

            CurrentTool->UpdateActiveNumericSlot(Char);
            return true;
        }

        if (bNumericInput)
        {
            if (PressedKey == EKeys::BackSpace)
            {
                CurrentTool->HandleBackspace();
                bNumericInput = CurrentSession->bIsNumericInputActive;
                return true;
            }

            if (PressedKey == EKeys::Hyphen // main keyboard "-"
                || PressedKey == EKeys::Subtract // numpad "-"
                || PressedKey == EKeys::Underscore // (covers some layouts with Shift)
            )
            {
                CurrentTool->ToggleNegation();
                return true;
            }

            if (PressedKey == EKeys::Slash || PressedKey == EKeys::Divide)
            {
                CurrentTool->ToggleReciprocal();
                return true;
            }

            if (PressedKey == EKeys::Tab)
            {
                CurrentTool->CycleNumericInputSlot();
                return true;
            }
        }

        if (PressedKey == EKeys::X || PressedKey == EKeys::Y || PressedKey == EKeys::Z)
        {
            PressedKeys.Add(PressedKey);
            bool bKeySwallowed = false;

            if (PressedKey == EKeys::X)
            {
                CurrentTool->HandleAxisLock(bShift ? (EAxisLock::Y | EAxisLock::Z) : EAxisLock::X);
                bKeySwallowed = true;
            }
            if (PressedKey == EKeys::Y)
            {
                CurrentTool->HandleAxisLock(bShift ? (EAxisLock::X | EAxisLock::Z) : EAxisLock::Y);
                bKeySwallowed = true;
            }
            if (PressedKey == EKeys::Z)
            {
                CurrentTool->HandleAxisLock(bShift ? (EAxisLock::X | EAxisLock::Y) : EAxisLock::Z);
                bKeySwallowed = true;
            }

            if (bNumericInput)
            {
                double ParsedValue = 0.0f;
                FDefaultValueHelper::ParseDouble(NumericBuffer, ParsedValue);
                CurrentTool->ApplyNumeric(ParsedValue);
            }

            return bKeySwallowed;
        }

        if (PressedKey == EKeys::SpaceBar || PressedKey == EKeys::Enter)
        {
            EndTool(/*bApply=*/true);
            return true;
        }
        if (KeyEvent.GetKey() == EKeys::Escape)
        {
            EndTool(/*bApply=*/false);
            return true;
        }

        return false;
    }

    static bool IsTextEntryWidget(const TSharedPtr<SWidget>& W)
    {
        if (!W.IsValid()) return false;

        // Cheap + robust check: name-based since Slate types aren't all public headers.
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

        // 1) Menus/popups up? bail. (UE 5.x uses AnyMenusVisible)
        if (SlateApp.AnyMenusVisible())
        {
            return false;
        }

        // 2) If user is typing in any text field, don't eat keys.
        const TSharedPtr<SWidget> Focused = SlateApp.GetKeyboardFocusedWidget();
        if (IsTextEntryWidget(Focused))
        {
            return false;
        }

        // 3) Only handle when a Level Editor viewport is focused
        FViewport* ActiveViewport = GEditor ? GEditor->GetActiveViewport() : nullptr;
        // if (!ActiveViewport || !ActiveViewport->HasFocus())
        // {
        //     return false;
        // }

        FEditorViewportClient* Client = ActiveViewport
                                            ? static_cast<FEditorViewportClient*>(ActiveViewport->GetClient())
                                            : nullptr;
        if (!Client || !Client->IsLevelEditorClient())
        {
            return false; // not a Level Editor viewport
        }

        // 4) Don’t steal keys during PIE/SIE
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

    bool FBlenderControlsInputProcessor::HandleKeyUpEvent(FSlateApplication&, const FKeyEvent& KeyEvent)
    {
        const FKey PressedKey = KeyEvent.GetKey();
        PressedKeys.Remove(PressedKey);

        return false;
    }

    bool FBlenderControlsInputProcessor::HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& MouseEvent)
    {
        if (!bActive || !CurrentTool.IsValid() || bNumericInput)
        {
            return false; // plugin disabled or numeric typing
        }

        FVector2D CurrentViewportMousePosition;
        MathHelper::GetMousePosToViewportPos(FSlateApplication::Get().GetCursorPos(),
                                             CurrentViewportMousePosition);

        CurrentTool->OnActive(CurrentViewportMousePosition);
        WrapMouse(CurrentViewportMousePosition);

        // Track if mouse was wrapped and inform the tool
        // CurrentTool->SetWrapped(bWrapped);

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

        if (!CurrentSession.IsValid())
        {
            CurrentSession = MakeShared<FTransformSession>();

            if (FViewport* Viewport = GEditor->GetActiveViewport())
            {
                FIntPoint MousePosInt;
                Viewport->GetMousePos(MousePosInt);
                CurrentSession->StartMousePos = FVector2D(MousePosInt);
            }

            constexpr EPivotMode PivotMode = EPivotMode::MedianPoint;
            CaptureSelection();
            CurrentSession->VirtualPivot = MakeShared<FSharedPivot>(CurrentSession->SelectedActors, PivotMode);

            CurrentSession->LockedAxis = EAxisLock::All;
            CurrentSession->bUsingLocalSpace = false;

            CurrentSession->bIsNumericInputActive = false;
            CurrentSession->NumericBuffer.Reset();
            CurrentSession->CurrentNumericSlotIndex = 0;

            for (int i = 0; i < UE_ARRAY_COUNT(CurrentSession->NumericSlots); ++i)
            {
                CurrentSession->NumericSlots[i] = FNumericSlotData();
            }
        }
        else
        {
            CurrentSession->VirtualPivot->RevertToStartState();
        }

        // Use the session's axis lock as the initial axis for the new tool.
        const EAxisLock InitialAxis = CurrentSession->LockedAxis;

        switch (Mode)
        {
        case ETransformMode::Translate:
            CurrentTool = MakeShared<FMoveTool>(CurrentSession, InitialAxis);
            break;
        case ETransformMode::Rotate:
            CurrentTool = MakeShared<FRotateTool>(CurrentSession, InitialAxis);
            break;
        case ETransformMode::Scale:
            CurrentTool = MakeShared<FScaleTool>(CurrentSession, InitialAxis);
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

        if (CurrentSession->bIsNumericInputActive)
        {
            // Restore the state in the Input Processor
            bNumericInput = true;
            NumericBuffer = CurrentSession->NumericBuffer;

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
        CurrentSession->SelectedActors.Empty();

        if (GEditor)
        {
            USelection* ActorSelection = GEditor->GetSelectedActors();
            for (FSelectionIterator It(*ActorSelection); It; ++It)
            {
                if (AActor* Actor = Cast<AActor>(*It))
                {
                    CurrentSession->SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
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

        CurrentTool->OnEnd(bApply);

        if (bApply)
        {
            CurrentTool->Accept();
        }
        else
        {
            CurrentTool->Cancel();
        }

        CurrentTool.Reset();

        // Reset state
        ActiveMode = ETransformMode::None;
        bNumericInput = false;
        NumericBuffer.Reset();

        // stop drawing
        if (Overlay.IsValid())
        {
            Overlay->SetContext(nullptr);
        }
        CurrentSession.Reset();

        // FSlateApplication::Get().GetPlatformCursor()->Show(true);
    }

    void FBlenderControlsInputProcessor::WrapMouse(const FVector2D& CurrentViewportMousePosition) const
    {
        UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
        if (!EditorEngine || !CurrentTool.IsValid())
        {
            return;
        }
        FViewport* EditorViewport = EditorEngine->GetActiveViewport();
        if (!EditorViewport)
        {
            return;
        }

        const int32 ViewportSizeX = EditorViewport->GetSizeXY().X;
        const int32 ViewportSizeY = EditorViewport->GetSizeXY().Y;

        if (CurrentViewportMousePosition.X < 0 || CurrentViewportMousePosition.X >= ViewportSizeX ||
            CurrentViewportMousePosition.Y < 0 || CurrentViewportMousePosition.Y >= ViewportSizeY)
        {
            int NewX = static_cast<int>(CurrentViewportMousePosition.X) % ViewportSizeX;

            if (NewX < 0)
            {
                NewX += ViewportSizeX;
            }

            int NewY = static_cast<int>(CurrentViewportMousePosition.Y) % ViewportSizeY;
            if (NewY < 0)
            {
                NewY += ViewportSizeY;
            }

            const FVector2D NewMousePosition = FVector2D(NewX, NewY);
            FIntPoint MousePos;
            EditorViewport->SetMouse(NewX, NewY);
            CurrentTool->SetViewportMousePosition(NewMousePosition);
            EditorViewport->GetMousePos(MousePos, true);

            CurrentTool->SetLastMousePosition(FVector2D(NewX, NewY));
            CurrentTool->NotifyMouseWrap();
        }
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
