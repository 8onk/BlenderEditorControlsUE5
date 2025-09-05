#include "UI/TransformHUD.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"

namespace BlenderControls
{
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

	FString STransformHUD::FormatOneField(const FString& Label, const FNumericSlotData& SlotData, const FString& Unit,
	                                      bool bIsActiveSlot)
	{
		FString ValueString; // This will hold the core value to be displayed.

		// 1. CALCULATE STAGE: Determine the base value string, assuming the slot is NOT active.
		// This switch has no knowledge of bIsActiveSlot.
		switch (SlotData.SlotState)
		{
		case ESlotState::Pristine:
			ValueString = TEXT("NONE");
			break;

		case ESlotState::Committed:
			ValueString = FString::Printf(TEXT("%s %s"), *ToTrimmed3(SlotData.CommittedValue.Get(0.0)), *Unit);
			break;

		case ESlotState::FirstEdit:
			{
				const double LiveValue = FCString::Atod(*SlotData.Display);
				ValueString = FString::Printf(TEXT("%s %s"), *ToTrimmed3(LiveValue), *Unit);
			}
			break;

		case ESlotState::Additive:
			{
				const double Total = SlotData.CommittedValue.Get(0.0) + FCString::Atod(*SlotData.Display);
				ValueString = FString::Printf(TEXT("%s %s"), *ToTrimmed3(Total), *Unit);
			}
			break;

		case ESlotState::InvalidInput:
			ValueString = TEXT("INVALID");
			break;

		default:
			return TEXT("ERROR");
		}

		if (bIsActiveSlot)
		{
			// A second, smaller switch handles the specific "active" formatting.
			switch (SlotData.SlotState)
			{
			case ESlotState::Pristine:
				ValueString = TEXT("|NONE|");
				break;

			case ESlotState::Committed:
				// For Committed, we show the value ready to be edited additively.
				ValueString = FString::Printf(TEXT("[%s|] = %s"), *ValueString, *ValueString);
				break;

			case ESlotState::FirstEdit:
				// Here, ValueString already holds the "= Result" part. We just prepend the input part.
				ValueString = FString::Printf(TEXT("[%s|] = %s"), *SlotData.Display, *ValueString);
				break;

			case ESlotState::Additive:
				// ValueString holds the total. We prepend the additive input format.
				{
					const FString CommittedString = FString::Printf(
						TEXT("%s %s"), *ToTrimmed3(SlotData.CommittedValue.Get(0.0)), *Unit);
					ValueString = FString::Printf(TEXT("[%s %s|] = %s"), *CommittedString, *SlotData.Display,
					                              *ValueString);
				}
				break;

			case ESlotState::InvalidInput:
				ValueString = FString::Printf(TEXT("[%s|] = INVALID"), *SlotData.Display);
				break;

				// Note: The 'Live' state has no special active formatting, so we don't need a case for it here.
			}
		}

		// Finally, combine the label and the final (potentially decorated) value string.
		return FString::Printf(TEXT("%s: %s"), *Label, *ValueString);
	}

	FString STransformHUD::FormatMagnitude(float Magnitude, const TCHAR* Unit)
	{
		return ToTrimmed3(Magnitude) + TEXT(" ") + Unit;
	}

	FString STransformHUD::ToTrimmed3(double InValue)
	{
		if (FMath::IsNearlyZero(InValue))
		{
			InValue = 0.f;
		}
		FNumberFormattingOptions Opts;
		Opts.MinimumFractionalDigits = 0;
		Opts.MaximumFractionalDigits = 3;
		return FText::AsNumber(InValue, &Opts).ToString();
	}
}
