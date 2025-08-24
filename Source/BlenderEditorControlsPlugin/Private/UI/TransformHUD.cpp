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

	FString STransformHUD::FormatOneField(const FString& Label, const FNumericSlotData& SlotData,
	                                      bool bIsSlotBeingEdited, const FString& Unit)
	{
		// --- Case 1: This slot is currently being edited ---
		if (bIsSlotBeingEdited)
		{
			// Subcase 1a: The user is in additive mode (e.g., typing "5" after "50")
			if (SlotData.IsAdditiveMode())
			{
				const float Base = SlotData.CommittedValue.Get(0.0f);
				const float Additive = SlotData.LiveValue.Get(0.0f);
				const float Total = SlotData.GetTotal();

				FString AdditiveString;
				FString Operator;

				// Handle the negative formatting for the additive part
				if (Additive < 0.0f)
				{
					Operator = TEXT("-");
					AdditiveString = FString::Printf(TEXT("(%s%s)"), *ToTrimmed3(FMath::Abs(Additive)), *Unit);
				}
				else
				{
					Operator = TEXT("+");
					AdditiveString = FString::Printf(TEXT("%s%s"), *ToTrimmed3(Additive), *Unit);
				}

				// This builds the string from your image: "Dx: [50cm + 5cm|] = 55cm"
				return FString::Printf(TEXT("%s: [%s %s %s %s|] = %s %s"),
				                       *Label,
				                       *ToTrimmed3(Base), *Unit,
				                       *Operator,
				                       *AdditiveString,
				                       *ToTrimmed3(Total), *Unit
				);
			}
			// Subcase 1b: The user is typing a new value from scratch
			else
			{
				const float Total = SlotData.GetTotal();
				const FString Num = ToTrimmed3(Total);

				// This is your original active format: "Dx: [12.3|] = 12.3 cm"
				return FString::Printf(TEXT("%s: [%s|] = %s%s"), *Label, *Num, *Num, *Unit);
			}
		}

		return FString::Printf(TEXT("%s: %s %s"), *Label, *ToTrimmed3(SlotData.GetTotal()), *Unit);
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
