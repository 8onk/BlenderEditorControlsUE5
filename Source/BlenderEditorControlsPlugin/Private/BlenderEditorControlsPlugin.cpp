// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlenderEditorControlsPlugin.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlenderEditorControls, Log, All);

#define LOCTEXT_NAMESPACE "FBlenderEditorControlsPluginModule"

void FBlenderEditorControlsPluginModule::StartupModule()
{
    UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: StartupModule"));

    // GrabProcessor = MakeShared<FGrabInputProcessor>();
    // FSlateApplication::Get().RegisterInputPreProcessor(GrabProcessor);
}

void FBlenderEditorControlsPluginModule::ShutdownModule()
{
    UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: ShutdownModule"));
    /*if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UnregisterInputPreProcessor(GrabProcessor);
    }
    GrabProcessor.Reset();
    */
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlenderEditorControlsPluginModule, BlenderEditorControlsPlugin)