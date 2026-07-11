// Copyright Epic Games, Inc. All Rights Reserved.

#include "SWelcomeWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SHyperlink.h"

namespace WelcomeText
{
	const FText WindowTitle = FText::FromString("BlenderEditorControls");
	const FText Header = FText::FromString("Thank you for installing!");
	const FText Body1 = FText::FromString("Blender Editor Controls plugin provides seamless shortcuts and workflows inspired by Blender directly inside Unreal Engine.\n\nYou can always customize these bindings by going to ");
	const FText LinkText = FText::FromString("Edit > Editor Preferences");
	const FString LinkUrl = TEXT("https://google.com");
	const FText Body2 = FText::FromString(" > Keyboard Shortcuts.");
	const FText Button = FText::FromString("Get Started");
}

void SWelcomeWindow::Construct(const FArguments& InArgs)
{
	SWindow::Construct(SWindow::FArguments()
	                   .Title(WelcomeText::WindowTitle)
	                   .AutoCenter(EAutoCenter::PrimaryWorkArea)
	                   .ClientSize(FVector2D(800, 450))
	                   .SupportsMaximize(false)
	                   .SupportsMinimize(false)
	                   .SizingRule(ESizingRule::FixedSize)
	);

	SetContent(
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
				.Text(WelcomeText::Header)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.VAlign(VAlign_Top)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(WelcomeText::Body1)
					.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SHyperlink)
						.Text(WelcomeText::LinkText)
						.OnNavigate_Lambda([]()
						{
							FPlatformProcess::LaunchURL(*WelcomeText::LinkUrl, nullptr, nullptr);
						})
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(WelcomeText::Body2)
					]
				]
			]

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
	);
}
