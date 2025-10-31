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
	 * Its primary role is to create and manage the lifecycle of a FTransformSession.
	 * It acts as a "gatekeeper", forwarding input to an active session when one exists.
	 */
	class FInputProcessor : public IInputProcessor,
	                                       public TSharedFromThis<FInputProcessor>
	{
	public:
		explicit FInputProcessor(TSharedPtr<FUICommandList> InCommandList);
		~FInputProcessor() = default;

		void BindCommands();

		/** IInputProcessor overrides */
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent) override;
		virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent) override;
		virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
		virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

		// The following input handlers are not used by the session but are required to override.
		virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
		{
			return false;
		}

		virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent,
		                                            const FPointerEvent* InGestureEvent) override;

	private:
		/** Checks if it's appropriate to handle hotkeys (e.g., viewport is focused). */
		bool ShouldHandleHotkeys(FSlateApplication& SlateApp) const;
		bool IsMouseOverLevelViewport() const;
		
		bool CanStartTool() const;

		// --- Command Handlers for Starting a Session ---
		void OnTransformStart(ETransformMode Mode, bool bDuplicateSelection = false);
		void DuplicateAndMovePressed();

		/** The command list for binding hotkeys. */
		TSharedPtr<FUICommandList> CommandList;

		/** The currently active transform session. This is the single source of truth for state. */
		TSharedPtr<FTransformSession> ActiveSession;

		/** Tracks currently held keys to prevent re-triggering on key-repeat events. */
		TSet<FKey> PressedKeys;
	};
}
