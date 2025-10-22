#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "TransformSession.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "ScopedTransaction.h"
#include "SLevelViewport.h"

//TODO cant enter trackball rotation mode

namespace BlenderControls
{
	FBlenderControlsInputProcessor::FBlenderControlsInputProcessor(TSharedPtr<FUICommandList> InCommandList)
		: CommandList(InCommandList)
	{
	}

	void FBlenderControlsInputProcessor::BindCommands()
	{
		const auto& Cmd = FBlenderEditorControlsPluginCommands::Get();

		// The FCanExecuteAction ensures they only fire when no session is active (as defined by CanStartTool()).
		CommandList->MapAction(
			Cmd.CommandTranslate,
			FExecuteAction::CreateLambda([this]() { OnTransformPressed(ETransformMode::Translate); }),
			FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandRotate,
			FExecuteAction::CreateLambda([this]() { OnTransformPressed(ETransformMode::Rotate); }),
			FCanExecuteAction::CreateSP(this, &FBlenderControlsInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandScale,
			FExecuteAction::CreateLambda([this]() { OnTransformPressed(ETransformMode::Scale); }),
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
		return !ActiveSession.IsValid();
	}

	void FBlenderControlsInputProcessor::OnTransformPressed(ETransformMode Mode)
	{
		if (!CanStartTool()) return;
		
		ActiveSession = MakeShared<FTransformSession>(Mode);
		ActiveSession->SwitchTool(Mode);
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
			OnTransformPressed(ETransformMode::Translate);
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
} // namespace BlenderControls
