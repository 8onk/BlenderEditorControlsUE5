// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlenderEditorControls.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "BlenderControlsCommands.h"
#include "Input/InputProcessor.h"

DEFINE_LOG_CATEGORY(LogBlenderEditorControls);

namespace BlenderControls
{
	void FBlenderEditorControlsPluginModule::StartupModule()
	{
		UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: StartupModule"));

		RegisterCommands();
		RegisterInputProcessor();
	}

	void FBlenderEditorControlsPluginModule::ShutdownModule()
	{
		UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: ShutdownModule"));

		UnregisterInputProcessor();
		UnregisterCommands();
	}

	void FBlenderEditorControlsPluginModule::RegisterCommands()
	{
		FBlenderControlsCommands::Register();
		CommandList = MakeShared<FUICommandList>();
	}

	void FBlenderEditorControlsPluginModule::UnregisterCommands()
	{
		CommandList.Reset();
		FBlenderControlsCommands::Unregister();
	}

	void FBlenderEditorControlsPluginModule::RegisterInputProcessor()
	{
		InputProcessor = MakeShared<FInputProcessor>(CommandList);
		InputProcessor->BindCommands();
		//Ensure that this input-processor receives input first (amongst all input-processors). 
		constexpr int32 Priority = 100;
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, Priority);
	}

	void FBlenderEditorControlsPluginModule::UnregisterInputProcessor()
	{
		if (FSlateApplication::IsInitialized() && InputProcessor.IsValid())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
		}
		InputProcessor.Reset();
	}
} // namespace BlenderControls

IMPLEMENT_MODULE(BlenderControls::FBlenderEditorControlsPluginModule, BlenderEditorControlsPlugin)
