// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "Input/Numeric/NumericInputProcessor.h"
#include "Pivots/VirtualPivotBase.h"

class FBlueprintEditor;
struct FKeyEvent;
struct FPointerEvent;

namespace BlenderControls
{
	struct FControlRigElementInfo;
	class FToolBase;
	class FVirtualPivotBase;
	class STransformHUD;
	class FNumericInputProcessor;

	/** Identifies what type of selection we're operating on */
	enum class ESelectionType : uint8
	{
		/** No valid selection. */
		None,
		/** Blueprint level viewport (Simple Construction Script nodes). */
		SCSTreeNodes,
		/** Standard AActor selection in the level editor. */
		Actors,
		/** Standard USceneComponent selection. */
		Components,
		/** Control Rig bones/controls in Animation Mode. */
		ControlRig
	};

	/**
	 * Manages the state and lifecycle of a single, modal transform operation (e.g., from pressing 'G' to clicking to confirm).
	 * This class acts as a state machine, owning the active tool and all shared state.
	 */
	class FTransformSession : public TSharedFromThis<FTransformSession>
	{
		friend class FToolBase;

	public:
		/** 
		 * Constructs a new transform session.
		 * 
		 * @param InStartMode The initial transform mode (Translate, Rotate, Scale).
		 * @param bDuplicateSelection Whether to duplicate the selection before transforming.
		 */
		FTransformSession(ETransformMode InStartMode, bool bDuplicateSelection = false);

		~FTransformSession();

		/** 
		 * Finalizes the session, either committing the changes or reverting them.
		 * 
		 * @param bApply If true, confirms the transform. If false, reverts to original state.
		 */
		void End(bool bApply);

		/** 
		 * Checks if the session has concluded.
		 * 
		 * @return True if the session has ended and should be cleaned up.
		 */
		bool HasSessionTerminated() const { return bHasSessionTerminated; }

		/** 
		 * Transitions between Move, Rotate, and Scale while preserving numeric input state.
		 * 
		 * @param NewMode The transform mode to switch to.
		 */
		void SwitchTool(ETransformMode NewMode);

		// Input Forwarding to FInputProcessor

		/** 
		 * Forwards the engine tick to the active tool and updates continuous states.
		 * 
		 * @param DeltaTime The time in seconds since the last tick.
		 * @param SlateApp A reference to the application for querying current input states.
		 */
		void Tick(const float DeltaTime, const FSlateApplication& SlateApp);

		/** 
		 * Processes a keyboard down event to handle numeric inputs, axis locks, or modifiers.
		 * 
		 * @param KeyEvent The generated key event data.
		 * @return True if the event was handled and should not bubble up, false otherwise.
		 */
		bool HandleKeyDownEvent(const FKeyEvent& KeyEvent);

		/** 
		 * Handles mouse movement, calculating delta from the virtual cursor to drive the transformation.
		 * 
		 * @param SlateApp A reference to the slate application handling the pointer.
		 * @param MouseEvent The generated pointer event data.
		 * @return True if the movement was consumed by the transform session.
		 */
		bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);

		/** 
		 * Handles mouse clicks, typically to confirm (LMB) or cancel (RMB) the operation.
		 * 
		 * @param MouseEvent The generated pointer event data.
		 * @return True if the click was consumed by the transform session.
		 */
		bool HandleMouseButtonDownEvent(const FPointerEvent& MouseEvent);

		/** 
		 * Returns true if the session is currently transitioning between tools.
		 * 
		 * @return True if switching tools.
		 */
		bool IsSwitchingTools() const { return bIsSwitchingTools; }

		// State Accessors

		/** @return The current virtual pivot managing the center of transformation. */
		TSharedPtr<FVirtualPivotBase> GetPivot() const { return VirtualPivot; }

		/** @return The currently locked axis (if any) for the transformation. */
		EAxisLock GetLockedAxis() const { return LockedAxis; }

		/** @return The Unreal Engine axis list representation of the active lock. */
		EAxisList::Type GetResolvedWidgetAxis() const;

		/** @return The viewport client where the transformation was initiated. */
		FEditorViewportClient* GetActiveViewportClient() const { return ActiveViewportClient; }

		/** @return True if transformations are evaluated in the object's local space. */
		bool IsUsingLocalSpace() const { return bUsingLocalSpace; }

		/** @return True if an axis lock (X/Y/Z) is currently active. */
		bool IsAxisLockActive() const { return bIsAxisLockActive; }

		/** @return The type of objects currently selected (Actors, Components, etc). */
		ESelectionType GetSelectionType() const { return SelectionType; }

		/** @return True if the user is currently typing a numeric value. */
		bool IsNumericInputActive() const
		{
			if (NumericInputProcessor.IsValid())
			{
				return NumericInputProcessor->IsInNumericMode();
			}
			else
			{
				return false;
			}
		}

		/** @return True if the virtual pivot is valid and can be used for calculations. */
		bool HasValidPivot() const
		{
			return VirtualPivot.IsValid() && VirtualPivot->IsValid();
		}

		/** @return The list of currently selected actors being transformed. */
		const TArray<TWeakObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }

		/** @return The virtual mouse position ignoring screen boundaries and wrapping. */
		const FVector2D& GetVirtualMousePos() const { return VirtualMousePosition; }

		/** @return The actual cursor position on the screen, potentially wrapped. */
		const FVector2D& GetWrappedCursorPos() const { return WrappedMousePosition; }

		/** @return The starting position of the mouse when the session began. */
		const FVector2D& GetStartMousePos() const { return StartMousePos; }

		/** @return The total accumulated delta of mouse movement since session start. */
		const FVector2D& GetAccumulatedMouseDelta() const { return AccumulatedMouseDelta; }

		// Selection Type Accessors

		/** @return True if the selection consists of Control Rig elements. */
		bool IsControlRigSelection() const { return SelectionType == ESelectionType::ControlRig; }

		/** @return True if the selection consists of standard level Actors. */
		bool IsActorSelection() const { return SelectionType == ESelectionType::Actors; }

		/** @return The list of selected Control Rig elements being transformed. */
		const TArray<FControlRigElementInfo>& GetSelectedRigElements() const { return SelectedRigElements; }

		// State Setters 

		/** @param bActive Sets whether an axis lock (X/Y/Z) is enforced. */
		void SetAxisLockActive(bool bActive) { bIsAxisLockActive = bActive; }

		/** @param bUsing Sets whether the transformation uses local (true) or world (false) space. */
		void SetUsingLocalSpace(bool bUsing) { bUsingLocalSpace = bUsing; }

		/** @param InAxis The specific axis (or combination) to lock the transformation to. */
		void SetLockedAxis(EAxisLock InAxis) { LockedAxis = InAxis; }

		/** @param InVector Sets the wrapped mouse position on the screen. */
		void SetWrappedMousePos(FVector2D InVector) { WrappedMousePosition = InVector; }

		/** @param InVector Sets the unbounded virtual mouse position. */
		void SetVirtualMousePos(FVector2D InVector) { VirtualMousePosition = InVector; }

		/** @param InVector Sets the initial mouse position at session start. */
		void SetStartMousePos(FVector2D InVector) { StartMousePos = InVector; }

		/** @return Pointer to the numeric input processor handling keyboard digits. */
		FNumericInputProcessor* GetNumericInputProcessor() const { return NumericInputProcessor.Get(); }

	private:
		/** @return True if the editor viewport and selection are valid for transforming. */
		bool ValidateEditorState();

		/** Duplicates the currently selected objects, typically called on session start if requested. */
		void PerformDuplicateSelected();

		/** Sets up the initial mouse cursor states and hides the system cursor. */
		void InitializeMouseState();

		/** Calculates and creates the appropriate virtual pivot based on the selection type. */
		void InitializePivot();

		/** 
		 * Begins an undoable transaction block for the session.
		 * 
		 * @param InTransactionName The localized text to display in the undo history.
		 */
		void InitializeTransaction(const FString& InTransactionName);

		/** Helper to locate the specific blueprint editor associated with our hovered viewport, if any. */
		FBlueprintEditor* GetActiveBlueprintEditor() const;


		// Core Tool Components

		/** Polymorphic instance driving the active logic (Move, Rotate, or Scale). */
		TSharedPtr<FToolBase> CurrentTool;

		/** Handles keystrokes when the user types numbers directly to drive the transform. */
		TUniquePtr<FNumericInputProcessor> NumericInputProcessor;

		/** Calculates the transformation origin based on the active SelectionType and PivotMode. */
		TSharedPtr<FVirtualPivotBase> VirtualPivot;

		/** Manages the Undo/Redo block for the entire session. Cancelled if the user aborts. */
		TUniquePtr<FScopedTransaction> ScopedTransaction;

		/** The viewport where the transform was initiated and where we override input. */
		FEditorViewportClient* ActiveViewportClient = nullptr;


		// Interaction State

		/** Defines whether the user is currently Translating, Rotating, or Scaling. */
		ETransformMode ActiveMode = ETransformMode::None;

		/** Which axis is currently constrained (X, Y, Z, or planar variants). */
		EAxisLock LockedAxis = EAxisLock::All;

		/** Indicates if an explicit axis constraint key (X/Y/Z) was pressed. */
		bool bIsAxisLockActive = false;

		/** True if evaluating transforms relative to the object's local rotation instead of world. */
		bool bUsingLocalSpace = false;

		/** Tracks if Shift is held (cached, though current logic resolves this directly in Tick). */
		bool bIsPrecisionModeHeld = false;

		/** Tracks if Ctrl is held to invert snapping logic (cached, resolved in Tick). */
		bool bIsSnapInvertHeld = false;


		// Mouse & Coordinate Tracking

		/** Position of the mouse on screen when the session first started. */
		FVector2D StartMousePos = FVector2D::ZeroVector;

		/** Mouse position accumulated infinitely, ignoring physical screen boundaries/wrapping. */
		FVector2D VirtualMousePosition = FVector2D::ZeroVector;

		/** The physical, wrapped position of the mouse inside the viewport bounds. */
		FVector2D WrappedMousePosition = FVector2D::ZeroVector;

		/** Anchors used to calculate deltas relative to the start of the click/drag. */
		FVector2D CursorAnchorPoint = FVector2D::ZeroVector;
		FVector2D GlobalCursorAnchor = FVector2D::ZeroVector;

		/** Accumulated total delta since the operation began. */
		FVector2D AccumulatedMouseDelta = FVector2D::ZeroVector;

		/** The mouse location buffered from the Slate event to be processed in the next Tick. */
		FVector2D PendingMousePosition = FVector2D::ZeroVector;

		/** Flag indicating if the mouse moved since the last Tick and needs processing. */
		bool bHasPendingMouseMovement = false;


		// Selection State

		/** Dictates which of the arrays below is actively driving the transform. */
		ESelectionType SelectionType = ESelectionType::None;

		/** Cached list of valid selected actors in the level. */
		TArray<TWeakObjectPtr<AActor>> SelectedActors;

		/** Cached list of valid selected scene components (useful in BP editor). */
		TArray<TWeakObjectPtr<USceneComponent>> SelectedComponents;

		/** Cached list of valid selected Animation Control Rig elements. */
		TArray<FControlRigElementInfo> SelectedRigElements;


		// Session Control Flags

		/** True if we haven't switched tools yet (used for initializing numeric inputs). */
		bool bIsFirstTool = true;

		/** The widget mode of the viewport before any tool was launched. */
		int32 InitialWidgetMode = -1;

		/** True when actively transitioning between Move/Rotate/Scale to pause certain logic. */
		bool bIsSwitchingTools = false;

		/** True when the session ends (Accept/Cancel), flagging it for destruction. */
		bool bHasSessionTerminated = false;

		/** True if the user pressed Shift+D, causing us to spawn clones at the start of the session. */
		bool bStartedWithDuplicate = false;

		/** True if auto-save was active before the session. (Used to force auto-save into waiting whilst tool is active). */
		bool bWasAutoSaveEnabled = true;
	};
}
