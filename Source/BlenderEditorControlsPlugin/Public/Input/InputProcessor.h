// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

// Forward Declarations
class FUICommandList;

namespace BlenderControls
{
	// Forward Declarations
	class FTransformSession;
	enum class ETransformMode : uint8;

	/**
	 * A Slate input pre-processor that captures Blender-style hotkeys.
	 * Its primary role is to create and manage the lifecycle of FTransformSession.
	 * It acts as a "gatekeeper", forwarding input to an active session when one exists.
	 */
	class FInputProcessor : public IInputProcessor,
	                        public TSharedFromThis<FInputProcessor>
	{
	public:
		/** Constructs the input processor and binds it to the plugin's UI commands. */
		explicit FInputProcessor(const TSharedPtr<FUICommandList>& InCommandList);
		~FInputProcessor() = default;

		/** Registers the hotkey bindings (G, R, S, etc.) to their respective functions. */
		void BindCommands();

		//~ IInputProcessor Interface
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent) override;
		virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent) override;
		virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
		virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
		virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
		{
			return false;
		}
		virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent,
		                                            const FPointerEvent* InGestureEvent) override;

	private:
		//~ Session Management
		
		/** @return True if it's appropriate to handle hotkeys (viewport is focused). */
		bool ShouldHandleHotkeys(FSlateApplication& SlateApp) const;
		
		/** @return True if the mouse is currently hovering over any valid editor viewport. */
		bool IsMouseOverAnyViewport() const;
		
		/** @return True if a new transform session can be safely started. */
		bool CanStartTool() const;

		/** 
		 * Initiates a new transformation session.
		 * 
		 * @param Mode The transformation tool to start (Translate, Rotate, Scale).
		 * @param bDuplicateSelection True if the user pressed Shift+D to duplicate before transforming.
		 */
		void OnTransformStart(ETransformMode Mode, bool bDuplicateSelection = false);
		
		/** Callback for the Shift+D hotkey. */
		void DuplicateAndMovePressed();

		//~ Internal State

		/** The command list for binding hotkeys. */
		TSharedPtr<FUICommandList> CommandList;

		/** Pointer to the currently active transformation session, if any. */
		TSharedPtr<FTransformSession> ActiveSession;

		/** Tracks currently held keys to prevent re-triggering on key-repeat events. */
		TSet<FKey> PressedKeys;
	};
}
