#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Tools/BlenderToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "LevelEditor.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"

namespace BlenderControls
{
    FBlenderControlsInputProcessor::FBlenderControlsInputProcessor(TSharedPtr<FUICommandList> InCommandList) : CommandList(InCommandList)
    {
    }

    FBlenderControlsInputProcessor::~FBlenderControlsInputProcessor()
    {
    }

    void FBlenderControlsInputProcessor::BindCommands()
    {
        const auto &Commands = FBlenderEditorControlsPluginCommands::Get();

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
    }

    void FBlenderControlsInputProcessor::Tick(const float DeltaTime, FSlateApplication &, TSharedRef<ICursor>)
    {
    }

    bool FBlenderControlsInputProcessor::HandleKeyDownEvent(FSlateApplication &SlateApp, const FKeyEvent &KeyEvent)
    {
        if (CommandList.IsValid() && CommandList->ProcessCommandBindings(KeyEvent))
        {
            UE_LOG(LogBlenderEditorControls, Log, TEXT("Key pressed: %s"), *KeyEvent.GetKey().ToString());
            return true; // G/R/S (or remapped key) handled
        }

        /* --- no command matched --- */
        if (CurrentTool.IsValid())
        {
            // Axis keys, numeric buffer etc. handled here …
        }

        return false;
    }

    bool FBlenderControlsInputProcessor::HandleKeyUpEvent(FSlateApplication &, const FKeyEvent &KeyEvent)
    {
        // UE_LOG(LogBlenderEditorControls, Log, TEXT("KeyUp: %s"), *KeyEvent.GetKey().ToString());
        return false;
    }

    bool FBlenderControlsInputProcessor::HandleMouseMoveEvent(FSlateApplication &, const FPointerEvent &)
    {
        return false;
    }

    bool FBlenderControlsInputProcessor::HandleMouseButtonDownEvent(FSlateApplication &, const FPointerEvent &)
    {
        return false;
    }

    bool FBlenderControlsInputProcessor::HandleMouseButtonUpEvent(FSlateApplication &, const FPointerEvent &)
    {
        return false;
    }

    void FBlenderControlsInputProcessor::BeginTool(ETransformMode Mode)
    {
        if (CurrentTool.IsValid())
        {
            return;
        }

        const ETransformAxis InitialAxis = ETransformAxis::All;

        switch (Mode)
        {
        case ETransformMode::Translate:
            CurrentTool = MakeShared<FMoveTool>(InitialAxis);
            break;
        case ETransformMode::Rotate:
            CurrentTool = MakeShared<FRotateTool>(InitialAxis);
            break;
        case ETransformMode::Scale:
            CurrentTool = MakeShared<FScaleTool>(InitialAxis);
            break;
        case ETransformMode::None:
            return;
        default:
            return;
        }

        ActiveMode = Mode;
        bNumericInput = false;
        NumericBuffer.Reset();

        // Tell overlay to start drawing guides for this tool (Axis lines in level)
        if (Overlay.IsValid())
        {
            Overlay->SetContext(CurrentTool);
        }
    }

    void FBlenderControlsInputProcessor::EndTool(bool bApply)
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

        CurrentTool.Reset();

        // Reset state
        ActiveMode = ETransformMode::None;
        bNumericInput = false;
        NumericBuffer.Reset();
        LastMousePos = FVector2D::ZeroVector;

        // stop drawing
        if (Overlay.IsValid())
        {
            Overlay->SetContext(nullptr);
        }
    }

} // namespace BlenderControls