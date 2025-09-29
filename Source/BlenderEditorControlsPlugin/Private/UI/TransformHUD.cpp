#include "UI/TransformHUD.h"
#include <devicetopology.h>
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Input/BlenderEditorControlsPluginInputProcessor.h"
#include "Slate/SceneViewport.h"
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
		if (SlotData.SlotState == ESlotState::Pristine && !bIsActiveSlot)
		{
			if (Label.IsEmpty())
			{
				return TEXT("NONE");
			}
			return FString::Printf(TEXT("%s: NONE"), *Label);
		}

		FString ResultString;
		if (SlotData.SlotState == ESlotState::InvalidInput)
		{
			ResultString = TEXT("INVALID");
		}
		else
		{
			const double TotalValue = SlotData.GetTotal();
			ResultString = FString::Printf(TEXT("%s %s"), *ToTrimmed3(TotalValue), *Unit);
		}

		if (!bIsActiveSlot)
		{
			if (Label.IsEmpty())
			{
				return ResultString;
			}
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

				if (Label.IsEmpty())
				{
					return FinalValueString;
				}

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

		//Combine everything into the final string
		FString FinalValueString = FString::Printf(TEXT("[%s] = %s"), *InputString, *ResultString);
		if (Label.IsEmpty())
		{
			return FinalValueString;
		}
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

	void STransformHUD::SetDashState(bool bEnabled, const FVector2D& InOriginAbsPx, const FVector2D& InMouseAbsPx)
	{
		bShowDash = bEnabled;
		OriginAbsPx = InOriginAbsPx;
		MouseAbsPx = InMouseAbsPx;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	int32 STransformHUD::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                             const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                             int32 LayerId,
	                             const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
		                                   InWidgetStyle, bParentEnabled);

		if (!bShowDash) return LayerId;

		// Convert absolute/viewport px -> local paint space for this widget
		const FVector2f A(OriginAbsPx);
		const FVector2f B(MouseAbsPx);

		TArray<FVector2f> Pts;
		Pts.Add(A);
		Pts.Add(B);

		const float Phase = DashPhase; // animate; 0 for static

		FSlateDrawElement::MakeDashedLines(
			OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(), MoveTemp(Pts),
			ESlateDrawEffect::None, FLinearColor::White, DashThickness, DashLengthPx, Phase);

		return LayerId;
	}

	void STransformHUD::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		// // advance the “marching ants” phase
		// DashPhase = FMath::Fmod(DashPhase + 60.f * InDeltaTime, DashLengthPx * 2.f);
		//
		// // if the dashed line is visible, ask Slate to repaint
		// if (bShowDash)
		// {
		// 	Invalidate(EInvalidateWidgetReason::Paint);
		// }
	}
}
