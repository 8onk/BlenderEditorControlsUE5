#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Utils/ControlRigSelectionHelper.h"
#include "Tools/ControlRigPivot.h"

struct FKeyEvent;
struct FPointerEvent;

namespace BlenderControls
{
	class FToolBase;
	class FSharedPivot;
	class FControlRigPivot;
	class STransformHUD;
	class FNumericInputProcessor;

	/** Identifies what type of selection we're operating on */
	enum class ESelectionType : uint8
	{
		None,
		Actors,      // Standard AActor selection
		ControlRig   // Control Rig bones/controls in Animation Mode
	};

	/**
	 * Manages the state and lifecycle of a single, modal transform operation (e.g., from pressing 'G' to clicking to confirm).
	 * This class acts as a state machine, owning the active tool and all shared state.
	 */
	class FTransformSession : public TSharedFromThis<FTransformSession>
	{
	public:
		FTransformSession(ETransformMode InStartMode, bool bDuplicateSelection = false);
		~FTransformSession();

		/** Finalizes the session, either committing changes to the Undo stack or reverting them. */
		void End(bool bApply);

		bool IsSessionFinished() const { return bIsSessionFinished; }

		/** Transitions between Move, Rotate, and Scale while preserving numeric input state. */
		void SwitchTool(ETransformMode NewMode);

		// Input Forwarding to FInputProcessor
		void Tick(const float DeltaTime, FSlateApplication& SlateApp) const;
		bool HandleKeyDownEvent(const FKeyEvent& KeyEvent);
		bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) const;
		bool HandleMouseButtonDownEvent(const FPointerEvent& MouseEvent);
		bool IsSwitchingTools() const { return bIsSwitchingTools; }

		// State Accessors
		TSharedPtr<FSharedPivot> GetPivot() const { return VirtualPivot; }
		TSharedPtr<FControlRigPivot> GetControlRigPivot() const { return ControlRigVirtualPivot; }
		EAxisLock GetLockedAxis() const { return LockedAxis; }
		bool IsUsingLocalSpace() const { return bUsingLocalSpace; }
		bool IsAxisLockActive() const { return bIsAxisLockActive; }

		bool IsNumericInputActive() const
		{
			if (NumericInputProcessor.IsValid()) return NumericInputProcessor->IsInNumericMode();
			else return false;
		}

		bool HasValidPivot() const 
		{ 
			if (SelectionType == ESelectionType::ControlRig)
			{
				return ControlRigVirtualPivot.IsValid() && ControlRigVirtualPivot->IsValid();
			}
			return VirtualPivot.IsValid(); 
		}

		const TArray<TWeakObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }
		const FVector2D& GetVirtualMousePos() const { return VirtualMousePosition; }
		const FVector2D& GetWrappedCursorPos() const { return WrappedMousePosition; }
		const FVector2D& GetStartMousePos() const { return StartMousePos; }

		// Selection Type Accessors
		ESelectionType GetSelectionType() const { return SelectionType; }
		bool IsControlRigSelection() const { return SelectionType == ESelectionType::ControlRig; }
		bool IsActorSelection() const { return SelectionType == ESelectionType::Actors; }
		const TArray<FControlRigElementInfo>& GetSelectedRigElements() const { return SelectedRigElements; }

		// State Setters 
		void SetAxisLockActive(bool bActive) { bIsAxisLockActive = bActive; }
		void SetUsingLocalSpace(bool bUsing) { bUsingLocalSpace = bUsing; }
		void SetLockedAxis(EAxisLock InAxis) { LockedAxis = InAxis; }
		void SetWrappedMousePos(FVector2D InVector) { WrappedMousePosition = InVector; }
		void SetVirtualMousePos(FVector2D InVector) { VirtualMousePosition = InVector; }
		void SetStartMousePos(FVector2D InVector) { StartMousePos = InVector; }

		FNumericInputProcessor* GetNumericInputProcessor() const { return NumericInputProcessor.Get(); }

	private:
		bool ValidateEditorState();
		void PerformDuplicateSelected();
		void InitializeMouseState();
		void InitializePivot();
		void InitializeTransaction(const FString& InTransactionName);

		// --- Core Tool Components ---
		TSharedPtr<FToolBase> CurrentTool;
		TUniquePtr<FNumericInputProcessor> NumericInputProcessor;
		TSharedPtr<FSharedPivot> VirtualPivot;
		TSharedPtr<FControlRigPivot> ControlRigVirtualPivot;
		TUniquePtr<FScopedTransaction> ScopedTransaction;

		// --- Interaction State ---
		ETransformMode ActiveMode = ETransformMode::None;
		EAxisLock LockedAxis = EAxisLock::All;
		bool bUsingLocalSpace = false;
		bool bIsAxisLockActive = false;
		bool bIsPrecisionModeHeld = false;
		bool bIsSnapInvertHeld = false;

		// --- Mouse & Coordinate State ---
		TArray<TWeakObjectPtr<AActor>> SelectedActors;
		FVector2D VirtualMousePosition;
		FVector2D CursorAnchorPoint = FVector2D::ZeroVector;
		FVector2D WrappedMousePosition = FVector2D::ZeroVector;
		FVector2D StartMousePos = FVector2D::ZeroVector;

		// --- Selection Type ---
		ESelectionType SelectionType = ESelectionType::None;
		TArray<FControlRigElementInfo> SelectedRigElements;

		// --- Session Control ---
		bool bIsFirstTool = true;
		bool bIsSwitchingTools = false;
		bool bIsSessionFinished = false;
		bool bStartedWithDuplicate = false;

		friend class FToolBase;
	};
}
