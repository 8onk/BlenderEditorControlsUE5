#include "TransformSession.h"
#include "Editor.h"
#include "Selection.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Tools/SharedPivot.h"
#include "Tools/ToolBase.h"
#include "Tools/MoveTool.h"
#include "Tools/RotateTool.h"
#include "Tools/ScaleTool.h"
#include "Utils/MathHelpers.h"

namespace BlenderControls
{
	FTransformSession::FTransformSession(ETransformMode InStartMode, bool bDuplicateSelection)
	{
		if (!GEditor || GEditor->GetSelectedActorCount() == 0)
		{
			bIsSessionFinished = true;
			return;
		}

		bStartedWithDuplicate = bDuplicateSelection;

		if (bDuplicateSelection)
		{
			InitializeTransaction(TEXT("Duplicate Selection"));

			UWorld* World = GEditor->GetEditorWorldContext().World();
			if (World)
			{
				ULevel* Level = World->GetCurrentLevel();
				constexpr bool bOffsetLocations = false;
				GEditor->edactDuplicateSelected(Level, bOffsetLocations);
			}
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

		NumericInputProcessor = MakeUnique<FNumericInputProcessor>();
	}

	FTransformSession::~FTransformSession()
	{
		if (CurrentTool.IsValid())
		{
			CurrentTool->OnEnd(/*bApply=*/false);
		}

		NumericInputProcessor.Reset();
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

	void FTransformSession::InitializeTransaction(const FString& InTransactionName)
	{
		if (ScopedTransaction)
		{
			return;
		}

		FText TransactionName = FText::FromString(InTransactionName + TEXT(" - BlenderEditorControls"));
		ScopedTransaction = MakeUnique<FScopedTransaction>(TransactionName);
		for (auto Actor : SelectedActors)
		{
			Actor->Modify();
		}
	}

	void FTransformSession::SwitchTool(ETransformMode NewMode)
	{
		if (ActiveMode == NewMode && CurrentTool.IsValid()) return;

		//Capture OldState for NumericProcessor and EndTool 
		FBlenderNumericState OldState;
		if (CurrentTool.IsValid())
		{
			CurrentTool->OnSwitch();
			if (NumericInputProcessor.IsValid())
			{
				OldState = NumericInputProcessor->CurrentState;
				bIsFirstTool = false;
			}
		}

		ActiveMode = NewMode;
		int NumNumericSlots = 3;
		EBlenderNumericContext NumericContext = EBlenderNumericContext::Distance;

		switch (NewMode)
		{
		case ETransformMode::Translate:
			CurrentTool = MakeShared<FMoveTool>(AsShared());
			NumericContext = EBlenderNumericContext::Distance;
			break;
		case ETransformMode::Rotate:
			CurrentTool = MakeShared<FRotateTool>(AsShared());
			NumNumericSlots = 1;
			NumericContext = EBlenderNumericContext::Angle_Degrees;
			break;
		case ETransformMode::Scale:
			CurrentTool = MakeShared<FScaleTool>(AsShared());
			NumericContext = EBlenderNumericContext::Scale;
			break;
		default:
			CurrentTool.Reset();
			break;
		}

		if (CurrentTool.IsValid() && NumericInputProcessor.IsValid())
		{
			CurrentTool->OnBegin();

			if (bIsFirstTool)
			{
				NumericInputProcessor->Initialize(NumNumericSlots, NumericContext);
			}
			else
			{
				NumericInputProcessor->OnToolSwitch(NumNumericSlots, NumericContext, OldState);
			}

			if (NumericInputProcessor->IsInNumericMode())
			{
				CurrentTool->ApplyNumeric();
			}

			CurrentTool->UpdateHud();

			if (!ScopedTransaction)
			{
				InitializeTransaction(CurrentTool->GetDisplayName());
			}

			CurrentTool->OnActive(WrappedMousePosition);
		}
	}

	void FTransformSession::End(bool bApply)
	{
		if (bIsSessionFinished) return;

		if (CurrentTool.IsValid())
		{
			if (bApply)
			{
				CurrentTool->Accept();
				ScopedTransaction.Reset();
			}
			else
			{
				CurrentTool->Cancel();

				if (!bStartedWithDuplicate)
				{
					ScopedTransaction->Cancel();
				}
				ScopedTransaction.Reset();
			}
		}

		bIsSessionFinished = true;
	}

	void FTransformSession::Tick(const float DeltaTime, FSlateApplication& SlateApp) const
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
		CurrentTool->Tick();
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
			if (ActiveMode == ETransformMode::Rotate)
			{
				const bool bTrackballRotationModeStat = CurrentTool->GetTrackballRotationMode();
				CurrentTool->SetTrackballRotationMode(!bTrackballRotationModeStat);
			}
			else
			{
				SwitchTool(ETransformMode::Rotate);
			}
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

		if (!NumericInputProcessor.IsValid()) return false;

		// if (KeyMatchesCommand(Cmd.CommandNumericCycleSlot))
		// {
		// 	UE_LOG(LogTemp, Log, TEXT("COMMAND ACTIVATED"));
		// 	NumericInputProcessor->HandleInput(KeyEvent);
		// 	if (NumericInputProcessor->IsInNumericMode())
		// 	{
		// 		if (CurrentTool.IsValid())
		// 		{
		// 			CurrentTool->ApplyNumeric();
		// 			CurrentTool->UpdateHud();
		// 		}
		// 		return true;
		// 	}
		// }

		if (NumericInputProcessor->HandleInput(KeyEvent))
		{
			if (NumericInputProcessor->IsInNumericMode())
			{
				CurrentTool->ApplyNumeric();
			}
			CurrentTool->UpdateHud();
			return true;
		}

		return true; // Disable all other inputs while any tool is active
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
		if (NumericInputProcessor->IsInNumericMode())
		{
			CurrentTool->HandleMouseMovement(CurrentViewportMousePosition);
		}
		else
		{
			CurrentTool->OnActive(CurrentViewportMousePosition);
		}

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
