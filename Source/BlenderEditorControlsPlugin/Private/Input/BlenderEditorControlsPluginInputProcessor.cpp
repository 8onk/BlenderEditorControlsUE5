#include "Input/BlenderEditorControlsPluginInputProcessor.h"
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
	}

	void FBlenderControlsInputProcessor::Tick(const float DeltaTime, FSlateApplication& App, TSharedRef<ICursor>)
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

	bool FBlenderControlsInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent)
	{
		if (CommandList.IsValid() && CommandList->ProcessCommandBindings(KeyEvent))
		{
			return true; // G/R/S (or remapped key) handled
		}

		if (CurrentTool.IsValid())
		{
			const FKey Key = KeyEvent.GetKey();
			const bool bShift = KeyEvent.IsShiftDown();

			// Only handle if not already down
			if ((Key == EKeys::X || Key == EKeys::Y || Key == EKeys::Z) && !AxisLockKeysDown.Contains(Key))
			{
				AxisLockKeysDown.Add(Key);

				if (Key == EKeys::X)
				{
					CurrentTool->HandleAxisLock(bShift ? (EAxisLock::Y | EAxisLock::Z) : EAxisLock::X);
					return true;
				}
				if (Key == EKeys::Y)
				{
					CurrentTool->HandleAxisLock(bShift ? (EAxisLock::X | EAxisLock::Z) : EAxisLock::Y);
					return true;
				}
				if (Key == EKeys::Z)
				{
					CurrentTool->HandleAxisLock(bShift ? (EAxisLock::X | EAxisLock::Y) : EAxisLock::Z);
					return true;
				}
			}

			if (KeyEvent.GetKey() == EKeys::Escape)
			{
				CurrentTool->Cancel();
				return true;
			}
		}

		return false;
	}

	bool FBlenderControlsInputProcessor::HandleKeyUpEvent(FSlateApplication&, const FKeyEvent& KeyEvent)
	{
		const FKey Key = KeyEvent.GetKey();
		if (Key == EKeys::X || Key == EKeys::Y || Key == EKeys::Z)
		{
			AxisLockKeysDown.Remove(Key);
		}
		return false;
	}

	bool FBlenderControlsInputProcessor::HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& MouseEvent)
	{
		if (!bActive || !CurrentTool.IsValid() || bNumericInput)
		{
			return false; // plugin disabled or numeric typing
		}

		FVector2D CurrentViewportMousePosition;
		MathHelper::GetMousePosToViewportPos(MouseEvent.GetScreenSpacePosition(),
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

	void FBlenderControlsInputProcessor::BeginTool(ETransformMode Mode)
	{
		if (GEditor->GetSelectedActorCount() == 0 || CurrentTool.IsValid())
		{
			return;
		}

		constexpr EAxisLock InitialAxis = EAxisLock::All;

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
		StartMousePos = FSlateApplication::Get().GetCursorPos();

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
		StartMousePos = FVector2D::ZeroVector;

		// stop drawing
		if (Overlay.IsValid())
		{
			Overlay->SetContext(nullptr);
		}

		// FSlateApplication::Get().GetPlatformCursor()->Show(true);
	}

	void FBlenderControlsInputProcessor::WrapMouse(const FVector2D& CurrentViewportMousePosition) const
	{
		UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
		if (!EditorEngine)
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
			if (NewX < 0) NewX += ViewportSizeX;

			int NewY = static_cast<int>(CurrentViewportMousePosition.Y) % ViewportSizeY;
			if (NewY < 0) NewY += ViewportSizeY;

			EditorViewport->SetMouse(NewX, NewY);

			if (CurrentTool.IsValid())
			{
				CurrentTool->NotifyMouseWrap();
			}
		}
	}
} // namespace BlenderControls
