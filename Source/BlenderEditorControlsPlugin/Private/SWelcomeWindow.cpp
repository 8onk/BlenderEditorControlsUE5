// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#include "SWelcomeWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SHyperlink.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"

namespace WelcomeText
{
	const FText WindowTitle = FText::FromString("BlenderEditorControls Plugin Overview");
	const FText Header = FText::FromString("Thank you for installing Blender Editor Controls!");
	const FText GettingStartedHeader = FText::FromString("Getting Started");
	const FText Body1 = FText::FromString(
		"You can customize the keyboard shortcuts by searching for \"Blender Editor Controls\" in:");
	const FText BindingsLinkText = FText::FromString("Edit > Editor Preferences > General > Keyboard Shortcuts");

	const FText Body2 = FText::FromString("You can configure the plugin settings by going to:");
	const FText SettingsLinkText = FText::FromString("Edit > Editor Preferences > Plugins > Blender Editor Controls");
	const FText Button = FText::FromString("Get Started");

	const FText FeaturesHeader = FText::FromString("Features");

	const FText SupportHeader = FText::FromString("Feedback & Support");
	const FText SupportBody1 = FText::FromString("If you experience any issues or have suggestions, feel free to contribute to the ");
	const FText SupportBody2 = FText::FromString("If you have the time, please consider leaving a ");
	const FText SupportBodyReview = FText::FromString("review");
	const FText SupportBody3 = FText::FromString(" on Fab to help support this free tool!");

	const FText GithubLinkText = FText::FromString("GitHub repository.");
	const FString GithubUrl = TEXT("https://github.com/jefimh/BlenderControlsUEPlugin");
}

static TSharedRef<SWidget> MakeFeatureRow(const FString& BoldPart, const FString& NormalPart)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(10, 4, 6, 4))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("•")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0, 4, 4, 4))
		[
			SNew(STextBlock) 
			.Text(FText::FromString(BoldPart))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(FMargin(0, 4, 0, 4))
		[
			SNew(STextBlock)
			.Text(FText::FromString(NormalPart))
			.AutoWrapText(true)
		];
}

void SWelcomeWindow::Construct(const FArguments& InArgs)
{
	SWindow::Construct(SWindow::FArguments()
	                   .Title(WelcomeText::WindowTitle)
	                   .AutoCenter(EAutoCenter::PrimaryWorkArea)
	                   .SupportsMaximize(false)
	                   .SupportsMinimize(false)
	                   .SizingRule(ESizingRule::Autosized)
	);

	SetContent(
		SNew(SBox)
		.WidthOverride(850)
		[
			SNew(SBorder)
			.Padding(FMargin(20.0f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				// --- Welcome Header Section ---
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 15)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(WelcomeText::Header)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				]

				// --- Getting Started Section ---
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 0)
				[
					SNew(SBorder)
					.Padding(FMargin(10.0f))
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 10)
						[
							SNew(STextBlock)
							.Text(WelcomeText::GettingStartedHeader)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 5)
						[
							SNew(STextBlock)
							.Text(WelcomeText::Body1)
							.AutoWrapText(true)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						.Padding(0, 0, 0, 10)
						[
							SNew(SHyperlink)
							.Text(WelcomeText::BindingsLinkText)
							.OnNavigate_Lambda([]()
							{
								FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(
									FName("Editor"), FName("General"), FName("InputBindings"));
							})
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 5)
						[
							SNew(STextBlock)
							.Text(WelcomeText::Body2)
							.AutoWrapText(true)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						[
							SNew(SHyperlink)
							.Text(WelcomeText::SettingsLinkText)
							.OnNavigate_Lambda([]()
							{
								FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(
									FName("Editor"), FName("Plugins"), FName("BlenderEditorControls"));
							})
						]
					]
				]

				// --- Feedback & Support Section ---
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 10, 0, 0)
				[
					SNew(SBorder)
					.Padding(FMargin(10.0f))
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 5)
						[
							SNew(STextBlock)
							.Text(WelcomeText::SupportHeader)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 5)
						[
							SNew(SWrapBox)
							.UseAllottedSize(true)

							+ SWrapBox::Slot()
							[
								SNew(STextBlock)
								.Text(WelcomeText::SupportBody1)
							]

							+ SWrapBox::Slot()
							[
								SNew(SHyperlink)
								.Text(WelcomeText::GithubLinkText)
								.OnNavigate_Lambda([]()
								{
									FPlatformProcess::LaunchURL(*WelcomeText::GithubUrl, nullptr, nullptr);
								})
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 10)
						[
							SNew(SWrapBox)
							.UseAllottedSize(true)

							+ SWrapBox::Slot()
							[
								SNew(STextBlock)
								.Text(WelcomeText::SupportBody2)
							]

							+ SWrapBox::Slot()
							[
								SNew(STextBlock)
								.Text(WelcomeText::SupportBodyReview)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]

							+ SWrapBox::Slot()
							[
								SNew(STextBlock)
								.Text(WelcomeText::SupportBody3)
							]
						]

					]
				]

				// --- Features List Section ---
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.VAlign(VAlign_Top)
				.Padding(0, 10, 0, 0)
				[
					SNew(SBorder)
					.Padding(FMargin(10.0f))
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 10)
						[
							SNew(STextBlock)
							.Text(WelcomeText::FeaturesHeader)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("G / R / S Keys: "),
								TEXT(
									"Classic Blender shortcuts for Translate, Rotate, and Scale (including Arcball rotation)."))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Single Axis Constraint: "),
								TEXT(
									"Tap X, Y, or Z during an operation to restrict transformations to that axis. Press again respective axis "
									"to cycle from local to global or global to local depending on what the editor's was set to when "
									"starting the tool."))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Dual Axis Constraint: "),
								TEXT(
									"Press Shift + X / Y / Z to lock to dual axis mode. Double press to cycle between local/global transform mode"))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Numeric Input: "),
								TEXT(
									"Type numbers directly for exact coordinates (supports Tab to cycle slots, Backspace, Subtract to negate, and Divide)."))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Snapping Features: "),
								TEXT(
									"Conforms to native editor snapping settings and snap inversion (hold Ctrl)."))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Precision Mode: "),
								TEXT("Hold Shift for precision mode (slows down the transform operation)."))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Duplicate & Move: "),
								TEXT("Press Shift + D to duplicate selection and begin transforming it immediately."))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Multi-Context Support: "),
								TEXT(
									"Works in Actor Viewports (including orthographic), Blueprint Viewports, and Control Rig animation mode."))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeFeatureRow(
								TEXT("Undo & Redo: "),
								TEXT("Supports native editor Ctrl+Z and Ctrl+Y (undo and redo)."))
						]
					]
				]
				// --- Footer Button ---
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0, 20, 0, 0)
				[
					SNew(SButton)
					.ContentPadding(FMargin(15, 5))
					.Text(WelcomeText::Button)
					.OnClicked_Lambda([this]() -> FReply
					{
						RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		]
	);
}
