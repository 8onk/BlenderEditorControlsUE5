// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBlenderEditorControls, Log, All);

namespace BlenderControls
{
	/**
	 * This module acts as the entry point for the plugin, responsible for 
	 * initializing the command system and registering (and unregistering) the Slate input processor
	 * that captures hotkeys on engine start and shutdown
	 */
	class FBlenderEditorControlsPluginModule : public IModuleInterface
	{
	public:
		/** 
		 * Initializes the plugin by mapping Slate UI commands, injecting our high-priority 
		 * input pre-processor, and checking if the first-time welcome window should be shown.
		 */
		virtual void StartupModule() override;

		/** Cleans up the input pre-processor and all registered UI commands. */
		virtual void ShutdownModule() override;

	private:
		/** Initializes and registers the plugin's bindable Slate UI commands. */
		void RegisterCommands();
		void UnregisterCommands();

		/** Instantiates our FInputProcessor at a high priority (100) to intercept inputs before other editor systems. */
		void RegisterInputProcessor();
		void UnregisterInputProcessor();

		/** Hooked to editor initialization to display the one-time welcome popup. 
		 * 
		 * @param InTime the time it took for editor to start. 
		 */
		void OnEditorInitialized(double InTime);

		/** 
		 * Ensures the welcome window is parented to the editor's main root window once created.
		 * 
		 * @param InRootWindow The newly created root window for the editor.
		 * @param bIsNewProjectWindow True if it's a new project (required by Unreal delegate, unused).
		 */
		void OnMainFrameCreationFinished(TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow);

		/** 
		 * Spawns the welcome widget and updates the local config so it isn't shown again.
		 * 
		 * @param ParentWindow The parent window to attach the welcome popup to.
		 */
		void ShowWelcomeWindow(TSharedPtr<SWindow> ParentWindow);

		/** Stores and manages the plugin's bindable actions. */
		TSharedPtr<FUICommandList> CommandList;

		/** Custom Slate pre-processor that intercepts Blender-style hotkeys globally. */
		TSharedPtr<class FInputProcessor> InputProcessor;
	};
} // namespace BlenderControls
