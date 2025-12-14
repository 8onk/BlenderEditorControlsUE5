// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlenderEditorControlsPlugin.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Input/InputProcessor.h"

DEFINE_LOG_CATEGORY(LogBlenderEditorControls);

#define LOCTEXT_NAMESPACE "FBlenderEditorControlsPluginModule"

namespace BlenderControls
{
	void FBlenderEditorControlsPluginModule::StartupModule()
	{
		UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: StartupModule"));
		
		RegisterCommands(); // UI_COMMANDs
		RegisterInputProcessor(); // Input processor
	}

	void FBlenderEditorControlsPluginModule::ShutdownModule()
	{
		UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: ShutdownModule"));

		UnregisterInputProcessor();
		UnregisterCommands();
	}

	void FBlenderEditorControlsPluginModule::RegisterCommands()
	{
		FBlenderEditorControlsPluginCommands::Register();
		CommandList = MakeShared<FUICommandList>();
	}

	void FBlenderEditorControlsPluginModule::UnregisterCommands()
	{
		CommandList.Reset();
		FBlenderEditorControlsPluginCommands::Unregister();
	}

	void FBlenderEditorControlsPluginModule::RegisterInputProcessor()
	{
		InputProcessor = MakeShared<FInputProcessor>(CommandList);
		InputProcessor->BindCommands();
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(BlenderControls::FBlenderEditorControlsPluginModule, BlenderEditorControlsPlugin)
