// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBlenderEditorControls, Log, All);

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
		/* ----- internal registration helpers ----- */
		void RegisterStyles();
		void UnregisterStyles();

		void RegisterCommands();
		void UnregisterCommands();

		void RegisterSettings();
		void UnregisterSettings();

		void RegisterMenus();
		void UnregisterMenus();

		void RegisterInputProcessor();
		void UnregisterInputProcessor();

		/* Toolbar delegate */
		void OnTogglePlugin();

		/* Persistent state */
		static inline bool                           bPluginActive = false;
		TSharedPtr<class FUICommandList>             CommandList;
		TSharedPtr<class FBlenderControlsInputProcessor> InputProcessor;
		FDelegateHandle                              ToolMenuOwnerHandle;
	};
} // namespace BlenderControls