#include "TransformSession.h"
#include "Editor.h"
#include "Selection.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Tools/SharedPivot.h"
#include "Tools/BlenderToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
	FTransformSession::FTransformSession(ETransformMode InStartMode)
	{
		if (!GEditor || GEditor->GetSelectedActorCount() == 0)
		{
			bIsFinished = true;
			return;
		}

		InitializePivot();

		if (FViewport* Viewport = GEditor->GetActiveViewport())
		{
			FIntPoint MousePosInt;
			Viewport->GetMousePos(MousePosInt);
			StartMousePos = FVector2D(MousePosInt);
			CursorAnchorPoint = StartMousePos;
			VirtualMousePosition = StartMousePos;
			WrappedMousePosition = StartMousePos;
		}

		UE_LOG(LogTemp, Log, TEXT("Constructing TransformSession"));

		//ParentTxn = MakeUnique<FScopedTransaction>(FText::FromString(TEXT("Blender Transform")));

		//SwitchTool(InStartMode);
	}

	FTransformSession::~FTransformSession()
	{
		if (CurrentTool.IsValid())
		{
			CurrentTool->OnEnd(/*bApply=*/false);
		}

		UE_LOG(LogTemp, Warning, TEXT("~FTransformSession"));
	}

	void FTransformSession::InitializePivot()
	{
		SelectedActors.Empty();
		USelection* ActorSelection = GEditor->GetSelectedActors();
		for (FSelectionIterator It(*ActorSelection); It; ++It)
		{
			if (AActor* Actor = Cast<AActor>(*It))
			{
				SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
			}
		}

		constexpr EPivotMode PivotMode = EPivotMode::MedianPoint;
		VirtualPivot = MakeShared<FSharedPivot>(SelectedActors, PivotMode);
	}

	void FTransformSession::SwitchTool(ETransformMode NewMode)
	{
		if (ActiveMode == NewMode && CurrentTool.IsValid()) return;

		if (CurrentTool.IsValid())
		{
			CurrentTool->OnEnd(false);
		}

		ActiveMode = NewMode;

		switch (NewMode)
		{
		case ETransformMode::Translate:
			CurrentTool = MakeShared<FMoveTool>(AsShared());
			break;
		case ETransformMode::Rotate:
			CurrentTool = MakeShared<FRotateTool>(AsShared());
			break;
		case ETransformMode::Scale:
			CurrentTool = MakeShared<FScaleTool>(AsShared());
			break;
		default:
			CurrentTool.Reset();
			break;
		}

		if (CurrentTool.IsValid())
		{
			CurrentTool->OnBegin();
		}
	}

	void FTransformSession::End(bool bApply)
	{
		if (bIsFinished) return;

		if (CurrentTool.IsValid())
		{
			if (bApply) CurrentTool->Accept();
			else CurrentTool->Cancel();
		}

		bIsFinished = true;
	}

	void FTransformSession::Tick(const float DeltaTime, FSlateApplication& SlateApp)
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
	}

	bool FTransformSession::HandleKeyDownEvent(const FKeyEvent& KeyEvent)
	{
		if (!CurrentTool.IsValid()) return false;

		const auto& Cmd = FBlenderEditorControlsPluginCommands::Get();
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

		// --- Tool Switching ---
		if (KeyMatchesCommand(Cmd.CommandTranslate))
		{
			SwitchTool(ETransformMode::Translate);
			return true;
		}
		if (KeyMatchesCommand(Cmd.CommandRotate))
		{
			SwitchTool(ETransformMode::Rotate);
			return true;
		}
		if (KeyMatchesCommand(Cmd.CommandScale))
		{
			SwitchTool(ETransformMode::Scale);
			return true;
		}

		// --- Confirmation / Cancellation ---
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

		// --- Axis Locking ---
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

		// --- Numeric Input and other commands would go here ---

		return false; // Let other systems handle the key if we didn't.
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
		CurrentTool->OnActive(CurrentViewportMousePosition);
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
