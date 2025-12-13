#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "Input/Numeric/NumericInputProcessor.h"

struct FKeyEvent;
struct FPointerEvent;

namespace BlenderControls
{
	class FToolBase;
	class FSharedPivot;
	class STransformHUD;
	class FNumericInputProcessor;

	/**
	 * Manages the state and lifecycle of a single, modal transform operation (e.g., from pressing 'G' to clicking to confirm).
	 * This class acts as a state machine, owning the active tool and all shared state.
	 */
	class FTransformSession : public TSharedFromThis<FTransformSession>
	{
	public:
		/**
		 * Begins a new transform session.
		 * @param InStartMode The initial tool to activate (Translate, Rotate, or Scale).
		 */
		FTransformSession(ETransformMode InStartMode, bool bDuplicateSelection = false);
		~FTransformSession();

		/** Finalizes the operation, either applying or canceling the changes. */
		void End(bool bApply);

		/** Returns true if the session has been ended and can be destroyed. */
		bool IsSessionFinished() const { return bIsSessionFinished; }

		/** Switches the active tool (e.g., from Move to Rotate). */
		void SwitchTool(ETransformMode NewMode);

		// Input Forwarding
		void Tick(const float DeltaTime, FSlateApplication& SlateApp) const;
		bool HandleKeyDownEvent(const FKeyEvent& KeyEvent);
		bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) const;
		bool HandleMouseButtonDownEvent(const FPointerEvent& MouseEvent);
		bool IsSwitchingTools() const { return bIsSwitchingTools; }

		// Public State Accessors (for Tools)
		TSharedPtr<FSharedPivot> GetPivot() const { return VirtualPivot; }
		EAxisLock GetLockedAxis() const { return LockedAxis; }
		bool IsUsingLocalSpace() const { return bUsingLocalSpace; }
		bool IsAxisLockActive() const { return bIsAxisLockActive; }

		bool IsNumericInputActive() const
		{
			if (NumericInputProcessor.IsValid()) return NumericInputProcessor->IsInNumericMode();
			else return false;
		}

		bool HasValidPivot() const { return VirtualPivot.IsValid(); }

		const TArray<TWeakObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }
		const FVector2D& GetVirtualMousePos() const { return VirtualMousePosition; }
		const FVector2D& GetWrappedCursorPos() const { return WrappedMousePosition; }
		const FVector2D& GetStartMousePos() const { return StartMousePos; }

		// State Setters (for Tools)
		void SetAxisLockActive(bool bActive) { bIsAxisLockActive = bActive; }
		void SetUsingLocalSpace(bool bUsing) { bUsingLocalSpace = bUsing; }
		void SetLockedAxis(EAxisLock InAxis) { LockedAxis = InAxis; }
		void SetWrappedMousePos(FVector2D InVector) { WrappedMousePosition = InVector; }
		void SetVirtualMousePos(FVector2D InVector) { VirtualMousePosition = InVector; }
		void SetStartMousePos(FVector2D InVector) { StartMousePos = InVector; }

		FNumericInputProcessor* GetNumericInputProcessor() const { return NumericInputProcessor.Get(); }

	private:
		/** Captures the initial selection and calculates the pivot. */
		void InitializePivot();
		void InitializeTransaction(const FString& InTransactionName);

		/** The current active tool (Move, Rotate, or Scale). */
		TSharedPtr<FToolBase> CurrentTool;

		TUniquePtr<FNumericInputProcessor> NumericInputProcessor;
		bool bIsFirstTool = true;
		bool bStartedWithDuplicate = false;
		bool bIsPrecisionModeHeld = false;
		bool bIsSnapInvertHeld = false;

		/** The pivot point manager for the selected actors. */
		TSharedPtr<FSharedPivot> VirtualPivot;

		// Shared State 
		ETransformMode ActiveMode = ETransformMode::None;
		EAxisLock LockedAxis = EAxisLock::All;
		bool bUsingLocalSpace = false;
		bool bIsAxisLockActive = false;
		TArray<TWeakObjectPtr<AActor>> SelectedActors;
		int32 CurrentNumericSlotIndex = 0;
		FVector2D VirtualMousePosition;
		FVector2D CursorAnchorPoint = FVector2D::ZeroVector;
		FVector2D WrappedMousePosition = FVector2D::ZeroVector;
		FVector2D StartMousePos = FVector2D::ZeroVector;
		bool bIsSwitchingTools = false;

		TUniquePtr<FScopedTransaction> ScopedTransaction;

		// Session Lifecycle 
		bool bIsSessionFinished = false;

		// Friend class declaration so FToolBase can access and modify session state
		friend class FToolBase;
	};
}
