#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "Input/TransformSession.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "ScopedTransaction.h"

namespace BlenderControls
{
	FBlenderControlsInputProcessor::FBlenderControlsInputProcessor(TSharedPtr<FUICommandList> InCommandList)
		: CommandList(InCommandList)
	{
	}

	void FBlenderControlsInputProcessor::BindCommands()
	{
		const auto& Cmd = FBlenderEditorControlsPluginCommands::Get();

		// The FCanExecuteAction ensures they only fire when no session is active.
		CommandList->MapAction(
			Cmd.CommandTranslate,
			FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::TranslatePressed),
			FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandRotate,
			FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::RotatePressed),
			FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandScale,
			FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::ScalePressed),
			FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandDuplicateAndMove,
			FExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::DuplicateAndMovePressed),
			FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::CanStartTool)
		);
	}

	void FBlenderControlsInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp,
	                                          TSharedRef<ICursor> Cursor)
	{
		// If a session exists, check if it has finished its work.
		if (ActiveSession.IsValid() && ActiveSession->IsFinished())
		{
			ActiveSession.Reset();
		}

		// If a session is active, forward the tick to it.
		if (ActiveSession.IsValid())
		{
			ActiveSession->Tick(DeltaTime, SlateApp);
		}
	}

	bool FBlenderControlsInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent)
	{
		if (PressedKeys.Contains(KeyEvent.GetKey()))
		{
			return false; // Prevent re-handling from key-repeat
		}
		PressedKeys.Add(KeyEvent.GetKey());

		// If a session is active, it gets priority and consumes the input.
		if (ActiveSession.IsValid())
		{
			return ActiveSession->HandleKeyDownEvent(KeyEvent);
		}

		// If no session is active, check if we should start one.
		if (ShouldHandleHotkeys(SlateApp))
		{
			// Process bindings like G, R, S, which will create a new ActiveSession.
			return CommandList->ProcessCommandBindings(KeyEvent);
		}

		return false;
	}

	bool FBlenderControlsInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent)
	{
		PressedKeys.Remove(KeyEvent.GetKey());
		return false; // Never consume KeyUp, let other systems use it.
	}

	bool FBlenderControlsInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp,
	                                                          const FPointerEvent& MouseEvent)
	{
		// Forward mouse movement to the active session.
		if (ActiveSession.IsValid())
		{
			return ActiveSession->HandleMouseMoveEvent(SlateApp, MouseEvent);
		}
		return false;
	}

	bool FBlenderControlsInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp,
	                                                                const FPointerEvent& MouseEvent)
	{
		// Forward mouse clicks to the active session.
		if (ActiveSession.IsValid())
		{
			return ActiveSession->HandleMouseButtonDownEvent(MouseEvent);
		}
		return false;
	}

	bool FBlenderControlsInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp,
	                                                                    const FPointerEvent& InWheelEvent,
	                                                                    const FPointerEvent* InGestureEvent)
	{
		// Block mouse wheel (zoom) while a tool is active.
		if (ActiveSession.IsValid())
		{
			return true;
		}
		return false;
	}


	// --- Command Handler Implementations ---

	bool FBlenderControlsInputProcessor::CanStartTool() const
	{
		// A tool can only be started if there isn't one already active.
		return !ActiveSession.IsValid();
	}

	void FBlenderControlsInputProcessor::TranslatePressed()
	{
		if (CanStartTool())
		{
			ActiveSession = MakeShared<FTransformSession>(ETransformMode::Translate);
		}
	}

	void FBlenderControlsInputProcessor::RotatePressed()
	{
		if (CanStartTool())
		{
			ActiveSession = MakeShared<FTransformSession>(ETransformMode::Rotate);
		}
	}

	void FBlenderControlsInputProcessor::ScalePressed()
	{
		if (CanStartTool())
		{
			ActiveSession = MakeShared<FTransformSession>(ETransformMode::Scale);
		}
	}

	void FBlenderControlsInputProcessor::DuplicateAndMovePressed()
	{
		if (CanStartTool() && GEditor && GEditor->GetSelectedActorCount() > 0)
		{
			// The duplication action must happen *before* the session starts.
			const FScopedTransaction Transaction(FText::FromString(TEXT("Duplicate Actors")));
			ULevel* Level = GEditor->GetEditorWorldContext().World()->GetCurrentLevel();
			constexpr bool bOffsetLocations = false; // Blender doesn't offset by default
			GEditor->edactDuplicateSelected(Level, bOffsetLocations);

			// Now, start the session in Translate mode on the new selection.
			ActiveSession = MakeShared<FTransformSession>(ETransformMode::Translate);
		}
	}


	// --- Helper Functions for Input Context ---
	// (These are largely unchanged from your original implementation)

	bool FBlenderControlsInputProcessor::ShouldHandleHotkeys(FSlateApplication& SlateApp) const
	{
		if (!IsMouseOverLevelViewport() || SlateApp.AnyMenusVisible())
		{
			return false;
		}

		// Don't handle if user is typing in a text field
		const TSharedPtr<SWidget> Focused = SlateApp.GetKeyboardFocusedWidget();
		const FString Type = Focused.IsValid() ? Focused->GetTypeAsString() : FString();
		if (Type.Contains(TEXT("EditableText")) || Type.Contains(TEXT("SearchBox")))
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
		FWidgetPath Path = App.LocateWindowUnderMouse(App.GetCursorPos(), App.GetInteractiveTopLevelWindows(), false);

		if (Path.IsValid())
		{
			for (const auto& Widget : Path.Widgets)
			{
				if (Widget.Widget->GetTypeAsString().Contains(TEXT("SLevelViewport")))
				{
					return true;
				}
			}
		}
		return false;
	}
} // namespace BlenderControls
