#include "UI/TransformHUD.h"

#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"

class FLevelEditorModule;

void STransformHUD::Construct(const FArguments&)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(10.f, 4.f))
		.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(ReadoutText, STextBlock)
				.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
				.ColorAndOpacity(FStyleColors::Foreground)
				.ShadowOffset(FVector2D(1, 1))
			]
			// + SOverlay::Slot()
			// .VAlign(VAlign_Bottom)
			// .HAlign(HAlign_Right)
			// .Padding(FMargin(0, 0, 2, 0))
			// [
			// 	SAssignNew(NumericText, STextBlock)
			// 	.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
			// 	.ColorAndOpacity(FStyleColors::AccentBlue)
			// ]
		]
	];
}

void STransformHUD::SetReadout(const FText& In)
{
	if (ReadoutText)
	{
		ReadoutText->SetText(In);
	}
}

void STransformHUD::SetNumericEcho(const FString& In)
{
	if (NumericText)
	{
		NumericText->SetText(FText::FromString(In));
	}
}

TSharedPtr<SLevelViewport> STransformHUD::GetActiveLevelViewportWidget()
{
	FLevelEditorModule& LevelEditorModule =
		FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedPtr<SLevelViewport> Viewport = LevelEditorModule.GetFirstActiveLevelViewport();

	//fallback via ILevelEditor instance
	if (!Viewport.IsValid())
	{
		if (const TSharedPtr<ILevelEditor> LE = LevelEditorModule.GetFirstLevelEditor())
		{
			Viewport = LE->GetActiveViewportInterface();
		}
	}
	return Viewport;
}

void STransformHUD::Attach()
{
	// Find the currently active Level Viewport (focused one)
	TSharedPtr<SLevelViewport> ActiveViewport = GetActiveLevelViewportWidget();
	if (!ActiveViewport.IsValid())
	{
		return;
	}

	if (AttachedViewport.Pin() == ActiveViewport && OverlayWrapper.IsValid())
	{
		return;
	}

	Detach();

	OverlayWrapper =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Fill)
		[
			SNew(SBox)
			.Visibility(EVisibility::HitTestInvisible) // pass clicks through to the viewport
			[
				SharedThis(this) // insert our HUD widget
			]
		];

	ActiveViewport->AddOverlayWidget(OverlayWrapper.ToSharedRef());
	AttachedViewport = ActiveViewport;
}

void STransformHUD::Update(const FText& Readout, const FString& NumericEcho)
{
	// Ensure we're attached to whatever viewport is active *right now*
	{
		TSharedPtr<SLevelViewport> Pinned = AttachedViewport.Pin();
		TSharedPtr<SLevelViewport> Active = GetActiveLevelViewportWidget();
		if (!Pinned.IsValid() || Pinned != Active || !OverlayWrapper.IsValid())
		{
			Attach();
		}
	}

	// Update texts
	SetReadout(Readout);

	if (NumericText)
	{
		if (NumericEcho.Len() > 0)
		{
			NumericText->SetText(FText::FromString(NumericEcho));
			NumericText->SetVisibility(EVisibility::Visible);
		}
		else
		{
			// Hide the numeric echo when empty (keeps layout clean)
			NumericText->SetText(FText::GetEmpty());
			NumericText->SetVisibility(EVisibility::Collapsed);
		}
	}
}

void STransformHUD::Detach()
{
	if (TSharedPtr<SLevelViewport> VP = AttachedViewport.Pin())
	{
		if (OverlayWrapper.IsValid())
		{
			VP->RemoveOverlayWidget(OverlayWrapper.ToSharedRef());
		}
	}

	OverlayWrapper.Reset();
	AttachedViewport.Reset();
}

FString STransformHUD::FormatOneField(const TCHAR* Label, const TOptional<double>& ValueOpt, bool bNumericMode, int SlotBeingModified,
                                      int32 ValueIndex,
                                      const TCHAR* Unit)
{
	const bool bIsActive = bNumericMode && (ValueIndex == SlotBeingModified);
	
	if (bIsActive)
	{
		if (!ValueOpt.IsSet())
		{
			return FString::Printf(TEXT("%s: |NONE|"), Label);
		}
		else
		{
			const FString Num = ToTrimmed3(ValueOpt.GetValue());
			return FString::Printf(TEXT("%s: [%s|] = %s %s"), Label, *Num, *Num, Unit);
		}
	}

	if (!ValueOpt.IsSet())
	{
		return FString::Printf(TEXT("%s: NONE"), Label);
	}
	else
	{
		return FString::Printf(TEXT("%s: %s %s"), Label, *ToTrimmed3(ValueOpt.GetValue()), Unit);
	}
}

FString STransformHUD::FormatMagnitude(float Magnitude, const TCHAR* Unit)
{
	return ToTrimmed3(Magnitude) + TEXT(" ") + Unit;
}

FString STransformHUD::ToTrimmed3(float InValue)
{
	if (FMath::IsNearlyZero(InValue))
	{
		InValue = 0.f;
	}
	FNumberFormattingOptions Opts;
	Opts.MinimumFractionalDigits = 0; // don’t force trailing zeros
	Opts.MaximumFractionalDigits = 3; // cap at 3 decimals
	return FText::AsNumber(InValue, &Opts).ToString();
}
