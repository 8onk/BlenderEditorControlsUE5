#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "Tools/BlenderToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"

namespace BlenderControls
{
    FBlenderControlsInputProcessor::FBlenderControlsInputProcessor(TSharedPtr<FUICommandList> InCommandList) : CommandList(InCommandList)
    {
    }

    FBlenderControlsInputProcessor::~FBlenderControlsInputProcessor()
    {
    }

    void FBlenderControlsInputProcessor::Tick(const float DeltaTime, FSlateApplication &, TSharedRef<ICursor>)
    {
    }

    bool FBlenderControlsInputProcessor::HandleKeyDownEvent(FSlateApplication &, const FKeyEvent &)
    {
        return false;
    }

    bool FBlenderControlsInputProcessor::HandleKeyUpEvent(FSlateApplication &, const FKeyEvent &)
    {
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
            return;

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
            Overlay->SetContext(CurrentTool);
    }

    void FBlenderControlsInputProcessor::EndTool(bool bApply)
    {
        if (!CurrentTool.IsValid())
            return;

        if (bApply)
            CurrentTool->Accept();
        else
            CurrentTool->Cancel();

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