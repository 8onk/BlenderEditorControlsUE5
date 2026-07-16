// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlenderEditorControls.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "BlenderControlsCommands.h"
#include "Input/InputProcessor.h"
#include "Editor.h"
#include "Misc/ConfigCacheIni.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "SWelcomeWindow.h"
DEFINE_LOG_CATEGORY(LogBlenderEditorControls);

namespace BlenderControls
{
	void FBlenderEditorControlsPluginModule::StartupModule()
	{
		UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: StartupModule"));

		RegisterCommands();
		RegisterInputProcessor();

		// [DEBUG] TEMPORARY: Register console command to show the popup at will
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("BlenderControls.ShowWelcome"),
			TEXT("Shows the welcome popup for testing"),
			FConsoleCommandDelegate::CreateRaw(this, &FBlenderEditorControlsPluginModule::OnEditorInitialized, 0.0)
		);
	}

	void FBlenderEditorControlsPluginModule::ShutdownModule()
	{
		UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: ShutdownModule"));

		// [DEBUG] TEMPORARY
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("BlenderControls.ShowWelcome"));

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

	void FBlenderEditorControlsPluginModule::OnEditorInitialized(double InTime)
	{
		IMainFrameModule* MainFrame = FModuleManager::GetModulePtr<IMainFrameModule>("MainFrame");

		if (MainFrame && MainFrame->GetParentWindow().IsValid())
		{
			ShowWelcomeWindow(MainFrame->GetParentWindow());
		}
		else if (MainFrame)
		{
			MainFrame->OnMainFrameCreationFinished().AddRaw(
				this, &FBlenderEditorControlsPluginModule::OnMainFrameCreationFinished);
		}
		else
		{
			IMainFrameModule& LoadedMainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			LoadedMainFrame.OnMainFrameCreationFinished().AddRaw(
				this, &FBlenderEditorControlsPluginModule::OnMainFrameCreationFinished);
		}
	}

	void FBlenderEditorControlsPluginModule::OnMainFrameCreationFinished(
		TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow)
	{
		IMainFrameModule* MainFrame = FModuleManager::GetModulePtr<IMainFrameModule>("MainFrame");
		if (MainFrame)
		{
			MainFrame->OnMainFrameCreationFinished().RemoveAll(this);
		}

		ShowWelcomeWindow(InRootWindow);
	}

	void FBlenderEditorControlsPluginModule::ShowWelcomeWindow(TSharedPtr<SWindow> ParentWindow)
	{
		TSharedRef<SWelcomeWindow> WelcomeWindow = SNew(SWelcomeWindow);

		if (ParentWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(WelcomeWindow, ParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(WelcomeWindow);
		}

		if (GConfig)
		{
			GConfig->SetBool(TEXT("BlenderControlsPlugin"), TEXT("bHasSeenWelcome"), true, GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
	}
} // namespace BlenderControls

IMPLEMENT_MODULE(BlenderControls::FBlenderEditorControlsPluginModule, BlenderEditorControlsPlugin)
