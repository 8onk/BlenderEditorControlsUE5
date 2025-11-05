#include "UI/TransformHUD.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
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

	void STransformHUD::SetReadout(const FText& In) const
	{
		if (ReadoutText)
		{
			ReadoutText->SetText(In);
		}
	}

	void STransformHUD::SetNumericEcho(const FString& In) const
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
			.ZOrder(0)
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

	void STransformHUD::SetDashState(bool bEnabled, const FVector2D& InOriginPx, const FVector2D& InMousePx)
	{
		bShowDash = bEnabled;
		OriginViewportPx = InOriginPx;
		MouseViewportPx = InMousePx;

		if (!bShowDash)
		{
			bHavePrevLen = false;
			DashPhase = 0.f;
		}
		else
		{
			UpdateDashPhaseForLengthChange();
		}

		Invalidate(EInvalidateWidgetReason::Paint);
	}

	int32 STransformHUD::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                             const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                             int32 LayerId,
	                             const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
		                                   InWidgetStyle, bParentEnabled);

		// Convert absolute/viewport px -> local paint space for this widget
		if (bShowDash)
		{
			const FVector2f A(OriginViewportPx);
			const FVector2f B(MouseViewportPx);

			TArray<FVector2f> Pts;
			Pts.Add(A);
			Pts.Add(B);

			const float Phase = DashPhase; // animate; 0 for static

			FSlateDrawElement::MakeDashedLines(
				OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(), MoveTemp(Pts),
				ESlateDrawEffect::None, FLinearColor::White, DashThickness, DashLengthPx, Phase);
		}

		if (bShowCursor && CursorBrush)
		{
			const FVector2D Size = CursorSize;
			const FVector2D LocalP = VirtualCursorViewportPx; // anchor under mouse
			const FVector2D Pos = LocalP - CursorHotspot;

			// Dashed line endpoints in viewport px
			const FVector2D A = OriginViewportPx; // pivot/object
			const FVector2D B = MouseViewportPx; // mouse/virtual cursor
			const FVector2D V = (B - A);

			static float LastAngle = 0.f;
			float BaseAngle = (V.SizeSquared() > KINDA_SMALL_NUMBER)
				                  ? FMath::Atan2(V.Y, V.X) // radians (Y-down)
				                  : LastAngle;

			// Decide final angle based on tool mode
			float Angle = BaseAngle;
			switch (CursorOrient)
			{
			case ECursorOrient::None:
				Angle = 0.f;
				break;

			case ECursorOrient::AlongLineToMouse:
				Angle = BaseAngle;
				break;

			case ECursorOrient::AlongLineToOrigin:
				Angle = BaseAngle + PI;
				break;

			case ECursorOrient::PerpendicularCW:
				Angle = BaseAngle + HALF_PI;
				break;

			case ECursorOrient::PerpendicularCCW:
				Angle = BaseAngle - HALF_PI;
				break;
			}
			LastAngle = BaseAngle;

			const FPaintGeometry PG = AllottedGeometry.ToPaintGeometry(
				Size, FSlateLayoutTransform(Pos));

			FSlateDrawElement::MakeRotatedBox(
				OutDrawElements,
				++LayerId,
				PG,
				CursorBrush,
				ESlateDrawEffect::None,
				Angle,
				CursorHotspot,
				FSlateDrawElement::RelativeToElement
			);
		}

		return LayerId;
	}

	void STransformHUD::UpdateDashPhaseForLengthChange()
	{
		const float curLen = (MouseViewportPx - OriginViewportPx).Size();

		if (!bHavePrevLen)
		{
			PrevLen = curLen;
			bHavePrevLen = true;
			return;
		}

		const float dL = curLen - PrevLen; // pixels along the line (+grow, -shrink)

		if (FMath::Abs(dL) > 0.01f) // dead-zone
		{
			// Slide the pattern opposite to the length change, so ticks "move"
			DashPhase -= dL;

			// Keep phase within one period [0, 2*DashLengthPx)
			const float Period = 2.f * DashLengthPx;
			DashPhase = FMath::Fmod(DashPhase, Period);
			if (DashPhase < 0.f) DashPhase += Period;

			PrevLen = curLen;
		}
	}
}
