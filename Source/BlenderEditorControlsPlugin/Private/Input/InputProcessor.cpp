#include "Input/InputProcessor.h"
#include "TransformSession.h"
#include "BlenderControlsCommands.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "ScopedTransaction.h"
#include "SLevelViewport.h"

namespace BlenderControls
{
	FInputProcessor::FInputProcessor(TSharedPtr<FUICommandList> InCommandList)
		: CommandList(InCommandList)
	{
	}

	void FInputProcessor::BindCommands()
	{
		const auto& Cmd = FBlenderControlsCommands::Get();

		CommandList->MapAction(
			Cmd.CommandTranslate,
			FExecuteAction::CreateLambda([this]() { OnTransformStart(ETransformMode::Translate); }),
			FCanExecuteAction::CreateSP(this, &FInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandRotate,
			FExecuteAction::CreateLambda([this]() { OnTransformStart(ETransformMode::Rotate); }),
			FCanExecuteAction::CreateSP(this, &FInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandScale,
			FExecuteAction::CreateLambda([this]() { OnTransformStart(ETransformMode::Scale); }),
			FCanExecuteAction::CreateSP(this, &FInputProcessor::CanStartTool)
		);

		CommandList->MapAction(
			Cmd.CommandDuplicateAndMove,
			FExecuteAction::CreateSP(this, &FInputProcessor::DuplicateAndMovePressed),
			FCanExecuteAction::CreateSP(this, &FInputProcessor::CanStartTool)
		);
	}

	//MAIN ENTRY POINT, this is called when a registered command is fired.
	void FInputProcessor::OnTransformStart(ETransformMode Mode, bool bDuplicateSelection)
	{
		if (ActiveSession.IsValid())
		{
			return;
		}

		TSharedRef<FTransformSession> NewSession = MakeShared<FTransformSession>(Mode, bDuplicateSelection);

		//Session is terminated if failed the initialization process during construction phase. 
		if (NewSession->HasSessionTerminated())
		{
			return;
		}

		ActiveSession = NewSession;
		ActiveSession->SwitchTool(Mode);
	}

	void FInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp,
	                           TSharedRef<ICursor> Cursor)
	{
		if (ActiveSession.IsValid() && ActiveSession->HasSessionTerminated())
		{
			ActiveSession.Reset();
		}

		if (ActiveSession.IsValid())
		{
			ActiveSession->Tick(DeltaTime, SlateApp);
		}
	}

	bool FInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent)
	{
		if (ActiveSession.IsValid())
		{
			return ActiveSession->HandleKeyDownEvent(KeyEvent);
		}

		if (PressedKeys.Contains(KeyEvent.GetKey()))
		{
			return false; // Prevent re-handling from key-repeat
		}
		PressedKeys.Add(KeyEvent.GetKey());

		if (ShouldHandleHotkeys(SlateApp))
		{
			return CommandList->ProcessCommandBindings(KeyEvent);
		}

		return false;
	}

	bool FInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent)
	{
		PressedKeys.Remove(KeyEvent.GetKey());
		return false; // Never consume KeyUp, let other systems use it.
	}

	bool FInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp,
	                                           const FPointerEvent& MouseEvent)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[FInputProcessor::HandleMouseMoveEvent] Delta: %s"), *MouseEvent.GetCursorDelta().ToString());

		if (ActiveSession.IsValid())
		{
			return ActiveSession->HandleMouseMoveEvent(SlateApp, MouseEvent);
		}
		return false;
	}

	bool FInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp,
	                                                 const FPointerEvent& MouseEvent)
	{
		if (ActiveSession.IsValid())
		{
			return ActiveSession->HandleMouseButtonDownEvent(MouseEvent);
		}
		return false;
	}

	bool FInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp,
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

	bool FInputProcessor::CanStartTool() const
	{
		return !ActiveSession.IsValid();
	}

	void FInputProcessor::DuplicateAndMovePressed()
	{
		if (GEditor && GEditor->GetSelectedActorCount() > 0)
		{
			OnTransformStart(ETransformMode::Translate, /*bDuplicateSelection*/true);
		}
	}

	bool FInputProcessor::ShouldHandleHotkeys(FSlateApplication& SlateApp) const
	{
		if (!IsMouseOverAnyViewport() || SlateApp.AnyMenusVisible())
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

	bool FInputProcessor::IsMouseOverAnyViewport() const
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
