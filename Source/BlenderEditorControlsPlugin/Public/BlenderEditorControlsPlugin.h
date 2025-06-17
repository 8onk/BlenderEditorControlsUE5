// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

namespace BlenderControls
{
	class FBlenderEditorControlsPluginModule : public IModuleInterface
	{
	public:
		/** IModuleInterface implementation */
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

		/** Lightweight accessor so other plugins can query whether BC is active */
		static bool IsEnabled();

	private:
		void RegisterStyles();
		void RegisterSettings();
		void RegisterCommands();
		void RegisterInputProcessor();
		void UnregisterInputProcessor();

		/** Persistent objects */
		TSharedPtr<class FBlenderControlsInputProcessor> InputProcessor;
		TSharedPtr<class FUICommandList> CommandList;
	};
} // namespace BlenderControls