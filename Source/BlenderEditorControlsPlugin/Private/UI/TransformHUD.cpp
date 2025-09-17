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
		// 1. First, determine the final resulting value string (e.g., "= -9.0 cm")
		FString ResultString;
		if (SlotData.SlotState == ESlotState::InvalidInput)
		{
			ResultString = TEXT("INVALID");
		}
		else
		{
			// Use the GetTotal() function which correctly handles negation
			const double TotalValue = SlotData.GetTotal();
			ResultString = FString::Printf(TEXT("%s %s"), *ToTrimmed3(TotalValue), *Unit);
		}

		// If the slot isn't being actively edited, we're done. Just show the result.
		if (!bIsActiveSlot)
		{
			return FString::Printf(TEXT("%s: %s"), *Label, *ResultString);
		}

		// 2. Build the live "input" part of the string (the content inside "[...]")
		FString InputString;
		switch (SlotData.SlotState)
		{
		case ESlotState::FirstEdit:
		case ESlotState::InvalidInput:
			InputString = FString::Printf(TEXT("%s|"), *SlotData.Display);
			break;
		case ESlotState::Additive:
			InputString = FString::Printf(TEXT("%s %s%s|"), *ToTrimmed3(SlotData.CommittedValue.Get(0.0)), *Unit,
			                              *SlotData.Display);
			break;
		case ESlotState::Committed:
			InputString = FString::Printf(TEXT("%s %s|"), *ToTrimmed3(SlotData.CommittedValue.Get(0.0)), *Unit);
			break;
		case ESlotState::Pristine:
		default:
			{
				FString FinalValueString = TEXT("|NONE|");
				return FString::Printf(TEXT("%s: %s"), *Label, *FinalValueString);
			}
		}

		if (SlotData.bIsReciprocal)
		{
			InputString = FString::Printf(TEXT("1/(%s)"), *InputString);
		}

		if (SlotData.bIsNegated)
		{
			InputString = FString::Printf(TEXT("-(%s)"), *InputString);
		}

		// 4. Combine everything into the final string
		FString FinalValueString = FString::Printf(TEXT("[%s] = %s"), *InputString, *ResultString);
		return FString::Printf(TEXT("%s: %s"), *Label, *FinalValueString);
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
