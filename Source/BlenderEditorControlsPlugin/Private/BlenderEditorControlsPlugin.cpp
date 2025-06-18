// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlenderEditorControlsPlugin.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "Commands/BlenderEditorControlsPluginCommands.h"
#include "Input/BlenderEditorControlsPluginInputProcessor.h"

DEFINE_LOG_CATEGORY(LogBlenderEditorControls);

#define LOCTEXT_NAMESPACE "FBlenderEditorControlsPluginModule"

namespace BlenderControls
{
    void FBlenderEditorControlsPluginModule::StartupModule()
    {
        UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: StartupModule"));

        RegisterStyles();         // icons, brushes
        RegisterCommands();       // UI_COMMANDs
        RegisterSettings();       // Preferences panel
        RegisterMenus();          // Toolbar toggle button
        RegisterInputProcessor(); // Input processor
    }

    void FBlenderEditorControlsPluginModule::ShutdownModule()
    {
        UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: ShutdownModule"));

        UnregisterInputProcessor();
        UnregisterMenus();
        UnregisterSettings();
        UnregisterCommands();
        UnregisterStyles();
    }

    void FBlenderEditorControlsPluginModule::RegisterStyles()
    {
    
    }
    
    void FBlenderEditorControlsPluginModule::UnregisterStyles()
    {
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

    void FBlenderEditorControlsPluginModule::RegisterSettings()
    {
    }
    void FBlenderEditorControlsPluginModule::UnregisterSettings()
    {
    }

    void FBlenderEditorControlsPluginModule::RegisterMenus()
    {
    }
    void FBlenderEditorControlsPluginModule::UnregisterMenus()
    {
    }

    void FBlenderEditorControlsPluginModule::RegisterInputProcessor()
    {
        InputProcessor = MakeShared<FBlenderControlsInputProcessor>(CommandList);
        InputProcessor->BindCommands();
        FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
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