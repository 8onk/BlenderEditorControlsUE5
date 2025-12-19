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
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

	private:
		void RegisterCommands();
		void UnregisterCommands();

		void RegisterInputProcessor();
		void UnregisterInputProcessor();

		//Bindable keybindings implemented by the plugin
		TSharedPtr<FUICommandList> CommandList;
		TSharedPtr<class FInputProcessor> InputProcessor;
	};
} // namespace BlenderControls
