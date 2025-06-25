#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Tools/BlenderToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "BlenderEditorControlsPlugin.h"
#include "SEditorViewport.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Containers/Ticker.h"
#include "Utils/BlenderMathHelpers.h"
#include "Editor/UnrealEd/Classes/Settings/LevelEditorViewportSettings.h"

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

	void FBlenderControlsInputProcessor::Tick(const float DeltaTime, FSlateApplication &App, TSharedRef<ICursor>)
	{
		if (!bActive || !CurrentTool.IsValid())
		{
			return;
		}

		// Update precision mode based on Shift key state
		const bool bShift = App.GetModifierKeys().IsShiftDown();
		CurrentTool->SetPrecisionModeActive(bShift);
		
		bool bIsGridSnapEnabled = GetDefault<ULevelEditorViewportSettings>()->GridEnabled;

		// If Ctrl is pressed, invert the grid snap setting
		const bool bCtrl = App.GetModifierKeys().IsControlDown();
		CurrentTool->SetSnappingEnabled(bCtrl ? !bIsGridSnapEnabled : bIsGridSnapEnabled);

		// Overlay may want to animate a fade, so pass DeltaTime
		/*if (Overlay.IsValid())
		{
			Overlay->Tick(DeltaTime);
		}*/
	}

	bool FBlenderControlsInputProcessor::HandleKeyDownEvent(FSlateApplication &SlateApp, const FKeyEvent &KeyEvent)
	{
		if (CommandList.IsValid() && CommandList->ProcessCommandBindings(KeyEvent))
		{
			return true; // G/R/S (or remapped key) handled
		}

		/* --- no command matched --- */
		if (CurrentTool.IsValid())
		{
			// Axis keys, numeric buffer etc. handled here …
			if (KeyEvent.GetKey() == EKeys::Escape)
			{
				CurrentTool->Cancel();
				return true;
			}
		}

		return false;
	}

	bool FBlenderControlsInputProcessor::HandleKeyUpEvent(FSlateApplication &, const FKeyEvent &KeyEvent)
	{
		return false;
	}

	bool FBlenderControlsInputProcessor::HandleMouseMoveEvent(FSlateApplication &, const FPointerEvent &MouseEvent)
	{
		if (!bActive || !CurrentTool.IsValid() || bNumericInput)
		{
			return false; // plugin disabled or numeric typing
		}

		FVector2D CurrentViewportMousePosition;

		BlenderControls::Math::GetMousePosToViewportPos(MouseEvent.GetScreenSpacePosition(),
														CurrentViewportMousePosition);
		CurrentTool->OnActive(CurrentViewportMousePosition);
		return true;
	}

	bool FBlenderControlsInputProcessor::HandleMouseButtonDownEvent(FSlateApplication &, const FPointerEvent &MouseEvent)
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

	bool FBlenderControlsInputProcessor::HandleMouseButtonUpEvent(FSlateApplication &, const FPointerEvent &)
	{
		return false;
	}

	void FBlenderControlsInputProcessor::BeginTool(ETransformMode Mode)
	{
		if (GEditor->GetSelectedActorCount() == 0 || CurrentTool.IsValid())
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
		LastMousePos = FSlateApplication::Get().GetCursorPos();

		// Tell overlay to start drawing guides for this tool (Axis lines in level)
		if (Overlay.IsValid())
		{
			Overlay->SetContext(CurrentTool);
		}

		if (CurrentTool.IsValid())
		{
			CurrentTool->OnBegin();
		}
	}

	void FBlenderControlsInputProcessor::EndTool(bool bApply)
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
		LastMousePos = FVector2D::ZeroVector;

		// stop drawing
		if (Overlay.IsValid())
		{
			Overlay->SetContext(nullptr);
		}

		FSlateApplication::Get().GetPlatformCursor()->Show(true);
	}

	// void FBlenderControlsInputProcessor::ShowSoftwareGrabCursor()
	// {
	// 	if (!GEditor || !GEditor->GetActiveViewport())
	// 	{
	// 		return;
	// 	}
	//
	// 	FEditorViewportClient *ViewportClient = static_cast<FEditorViewportClient *>(GEditor->GetActiveViewport()->GetClient());
	// 	SLevelViewport* LevelViewportWidget = GEditor->LevelViewpo
	// 	TSharedPtr<SEditorViewport> ViewportWidget = ViewportClient->GetEditorViewportWidget();
	// 	if (ViewportClient && ViewportWidget.IsValid())
	// 	{
	// 		// 1. Hide the actual hardware cursor by setting it to "None"
	// 		// This is a better approach than hiding it globally.
	// 		ViewportWidget->SetCursor(EMouseCursor::None);
	//
	// 		// 2. Get the mouse position within the viewport
	// 		const FVector2D MousePosition = ViewportWidget->GetMousePosition();
	//
	// 		// 3. Create the software cursor widget
	// 		SoftwareCursorWidget = SNew(SImage)
	// 								   .Image(FAppStyle::GetBrush("GrabHand")); // Use the standard "GrabHand" icon
	//
	// 		// 4. Add the widget to the viewport's overlay
	// 		ViewportWidget->AddOverlayWidget(SoftwareCursorWidget.ToSharedRef());
	//
	// 		// 5. Position the widget. We'll handle continuous updates in a Tick function.
	// 		// For the initial position:
	// 		SoftwareCursorWidget->SetRenderTransform(FSlateRenderTransform(MousePosition));
	// 	}
	//}
} // namespace BlenderControls
