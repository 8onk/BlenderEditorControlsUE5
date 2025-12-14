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

	private:
		void RegisterCommands();
		void UnregisterCommands();

		void RegisterInputProcessor();
		void UnregisterInputProcessor();

		// /* Toolbar delegate */
		// void OnTogglePlugin();

		/* Persistent state */
		static inline bool bPluginActive = false;
		TSharedPtr<FUICommandList> CommandList;
		TSharedPtr<class FInputProcessor> InputProcessor;
		FDelegateHandle ToolMenuOwnerHandle;
	};
} // namespace BlenderControls
