// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlenderEditorControls.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "BlenderControlsCommands.h"
#include "Input/InputProcessor.h"
#include "Editor.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IMainFrameModule.h"
DEFINE_LOG_CATEGORY(LogBlenderEditorControls);

namespace BlenderControls
{
	void FBlenderEditorControlsPluginModule::StartupModule()
	{
		UE_LOG(LogBlenderEditorControls, Log, TEXT("BlenderEditorControlsPlugin: StartupModule"));

		RegisterCommands();
		RegisterInputProcessor();

		if (!IsRunningCommandlet())
		{
			FEditorDelegates::OnEditorInitialized.
				AddRaw(this, &FBlenderEditorControlsPluginModule::OnEditorInitialized);
		}

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
		bool bHasSeenWelcome = false;

		if (GConfig)
		{
			GConfig->GetBool(TEXT("BlenderControlsPlugin"), TEXT("bHasSeenWelcome"), bHasSeenWelcome,
			                 GEditorPerProjectIni);
		}

		// [DEBUG] TEMPORARY: Force popup to always show on launch for testing.
		bHasSeenWelcome = false;

		if (!bHasSeenWelcome)
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
		TSharedRef<SWindow> WelcomeWindow = SNew(SWindow)
			.Title(FText::FromString("BlenderEditorControls"))
			.AutoCenter(EAutoCenter::PrimaryWorkArea)
			.ClientSize(FVector2D(800, 450))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.SizingRule(ESizingRule::FixedSize);

		WelcomeWindow->SetContent(
			SNew(SBorder)
			.Padding(FMargin(20.0f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 15)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Thank you for installing!"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Text(FText::FromString(
						"Blender Controls provides seamless shortcuts and workflows inspired by Blender directly inside Unreal Engine.\n\nYou can always customize these bindings by going to Edit > Editor Preferences > Keyboard Shortcuts."))
					.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0, 20, 0, 0)
				[
					SNew(SButton)
					.ContentPadding(FMargin(15, 5))
					.Text(FText::FromString("Get Started"))
					.OnClicked_Lambda([WelcomeWindow]() -> FReply
					{
						WelcomeWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		);

		if (ParentWindow.IsValid())
		{
			FSlateApplication::Get().AddModalWindow(WelcomeWindow, ParentWindow, false);
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
