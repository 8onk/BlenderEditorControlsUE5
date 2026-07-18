#include "UI/TransformHUD.h"
#include "BlenderControlsSettings.h"
#include "SEditorViewport.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Slate/SceneViewport.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 2
#include "Brushes/SlateRoundedBoxBrush.h"
#endif


namespace BlenderControls
{
	float STransformHUD::GetCappedHudScale() const
	{
		const float RequestedScale = GetDefault<UBlenderControlsSettings>()->HudScale;

		float ViewportWidth = 1920.f;
		if (AttachedViewport.IsValid())
		{
			TSharedPtr<FSceneViewport> SceneVP = AttachedViewport.Pin()->GetSceneViewport();
			if (SceneVP.IsValid())
			{
				ViewportWidth = SceneVP->GetSizeXY().X;
			}
		}

		const float MarginLeft = GetDefault<UBlenderControlsSettings>()->MarginLeft;
		constexpr float BaseMaxTextWidth = 500.f;
		const float AvailableSpace = ViewportWidth - MarginLeft - 20.f;

		const float MaxAllowedScale = FMath::Max(0.1f, AvailableSpace / BaseMaxTextWidth);

		return FMath::Min(RequestedScale, MaxAllowedScale);
	}

	FSlateFontInfo STransformHUD::GetDynamicFont() const
	{
		const float Scale = GetCappedHudScale();
		FSlateFontInfo FontInfo = FAppStyle::Get().GetFontStyle("NormalFont");
		FontInfo.Size = FMath::RoundToInt(FontInfo.Size * Scale);
		return FontInfo;
	}

	FMargin STransformHUD::GetDynamicPaddingUE5() const
	{
		const float Scale = GetCappedHudScale();
		return FMargin(10.f * Scale, 4.f * Scale);
	}

	FMargin STransformHUD::GetDynamicPaddingLegacy() const
	{
		const float Scale = GetCappedHudScale();
		return FMargin(12.f * Scale, 5.5f * Scale);
	}

	void STransformHUD::Construct(const FArguments&)
	{
		//UE5.6 introduced a new UI layout for level viewport
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		ChildSlot
		[
			SNew(SBorder)
			.Padding(this, &STransformHUD::GetDynamicPaddingUE5)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(ReadoutText, STextBlock)
					.Font(this, &STransformHUD::GetDynamicFont)
					.ColorAndOpacity(FStyleColors::Foreground)
					.ShadowOffset(FVector2D(1, 1))
				]
			]
		];
#else
		static FSlateRoundedBoxBrush PillBrush(
			FLinearColor(0.02f, 0.02f, 0.02f, 0.75f), // Background Color
			5.0f, // Radius (Half of height = Pill)
			FLinearColor(0.1f, 0.1f, 0.1f, 1.0f), // Outline Color
			1.0f // Outline Width
		);

		ChildSlot
		[
			SNew(SBorder)
			// Point to the static brush created above
			             .BorderImage(&PillBrush)
			// Remove the manual background color override since the brush handles it
			//.BorderBackgroundColor(...) 
			             .Padding(this, &STransformHUD::GetDynamicPaddingLegacy) // Increased side padding for pill look
			             .HAlign(HAlign_Center)
			             .VAlign(VAlign_Center)
			[
				SAssignNew(ReadoutText, STextBlock)
				.Font(this, &STransformHUD::GetDynamicFont)
				.ColorAndOpacity(FLinearColor::White)
			]
		];
#endif
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

	static TSharedPtr<SOverlay> FindViewportOverlay(TSharedPtr<SWidget> RootWidget)
	{
		if (!RootWidget.IsValid())
		{
			return nullptr;
		}
		TArray<TSharedPtr<SWidget>> Queue;
		Queue.Add(RootWidget);

		while (Queue.Num() > 0)
		{
			TSharedPtr<SWidget> Current = Queue[0];
			Queue.RemoveAt(0);

			if (Current->GetTypeAsString() == TEXT("SOverlay"))
			{
				return StaticCastSharedPtr<SOverlay>(Current);
			}

			FChildren* Children = Current->GetChildren();
			if (Children)
			{
				for (int32 i = 0; i < Children->Num(); ++i)
				{
					Queue.Add(Children->GetChildAt(i));
				}
			}
		}

		return nullptr;
	}

	void STransformHUD::Attach(TSharedPtr<SEditorViewport> TargetViewport)
	{
		if (!TargetViewport.IsValid())
		{
			return;
		}

		if (AttachedViewport.Pin() == TargetViewport && OverlayWrapper.IsValid())
		{
			return;
		}

		Detach();

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
		FIntPoint ViewportSize(1920, 1080); // Default fallback

		if (FViewport* Viewport = GEditor->GetActiveViewport())
		{
			ViewportSize = Viewport->GetSizeXY();
		}

		const UBlenderControlsSettings* Settings = GetDefault<UBlenderControlsSettings>();

		//NOTE Only runs at start first time
		ClampedLeft = FMath::Clamp(Settings->MarginLeft, 0.f, ViewportSize.X);
		ClampedTop = FMath::Clamp(Settings->MarginTop, 0.f, ViewportSize.Y);
#endif
		OverlayWrapper =
			SNew(SOverlay)
			+ SOverlay::Slot()
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 2
			.ZOrder(0)
#endif
			.VAlign(VAlign_Top)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
			.HAlign(HAlign_Fill)
#else
.HAlign(HAlign_Left)
				.Padding(Settings->MarginLeft, Settings->MarginTop, 0.f, 0.f)
#endif
			[
				SNew(SBox)
				.Visibility(EVisibility::HitTestInvisible) // pass clicks through to the viewport
				[
					SharedThis(this) // insert our HUD widget
				]
			];

		TSharedPtr<SOverlay> HackedOverlay = FindViewportOverlay(TargetViewport);
		if (HackedOverlay.IsValid())
		{
			HackedOverlay->AddSlot()
			[
				OverlayWrapper.ToSharedRef()
			];
			AttachedViewport = TargetViewport;
		}
	}

	void STransformHUD::Update(TSharedPtr<SEditorViewport> TargetViewport, const FText& Readout, const FString& NumericEcho)
	{
		// Ensure we're attached to whatever viewport is active right now
		{
			TSharedPtr<SEditorViewport> Pinned = AttachedViewport.Pin();
			if (!Pinned.IsValid() || Pinned != TargetViewport || !OverlayWrapper.IsValid())
			{
				Attach(TargetViewport);
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
				NumericText->SetText(FText::GetEmpty());
				NumericText->SetVisibility(EVisibility::Collapsed);
			}
		}
	}

	void STransformHUD::Detach()
	{
		if (TSharedPtr<SEditorViewport> VP = AttachedViewport.Pin())
		{
			if (OverlayWrapper.IsValid())
			{
				TSharedPtr<SOverlay> HackedOverlay = FindViewportOverlay(VP);
				if (HackedOverlay.IsValid())
				{
					HackedOverlay->RemoveSlot(OverlayWrapper.ToSharedRef());
				}
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

		Invalidate(EInvalidateWidgetReason::Paint);
	}

	int32 STransformHUD::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                             const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                             int32 LayerId,
	                             const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
		                                   InWidgetStyle, bParentEnabled);

		const FVector2f LineStart(MouseViewportPx - FVector2D(ClampedLeft, ClampedTop));
		const FVector2f LineEnd(OriginViewportPx - FVector2D(ClampedLeft, ClampedTop));
		FVector2f LineVector = (LineEnd - LineStart);

		if (bShowDash)
		{
			const float DashScale = GetDefault<UBlenderControlsSettings>()->DashLineScale;
			const float ScaledDashThickness = DashThickness * DashScale;
			constexpr float BaseDashScale = 7.5f;
			const float ScaledDashLength = BaseDashScale * DashScale;
			constexpr float GapRatio = 0.3f;
			const float ScaledDashDistance = ScaledDashLength * GapRatio;
			float NumTotalLines = LineVector.Size() / (ScaledDashLength + ScaledDashDistance);
			FVector2f Direction = LineVector.GetSafeNormal();
			int NumFittingLines = FMath::CeilToFloat(NumTotalLines);

			FVector2f DashStart = LineStart;
			FVector2f DashEnd = LineStart + LineVector.GetSafeNormal() * ScaledDashLength;
			for (int i = 0; i < NumFittingLines; ++i)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
				TArray<FVector2D> Pts;
				Pts.Add(FVector2D(DashStart));
				Pts.Add(FVector2D(DashEnd));
#else
				TArray<FVector2f> Pts;
				Pts.Add(DashStart);
				Pts.Add(DashEnd);
#endif


				//Using anti alias in some older UE versions makes dashes render joined together. 
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
				const bool bUseAA = false;
#else
				const bool bUseAA = true;
#endif

				FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Pts,
				                             ESlateDrawEffect::None, FLinearColor::White, bUseAA, ScaledDashThickness);

				DashStart = DashEnd + (Direction * ScaledDashDistance);
				float DistToOrigin = (LineEnd - DashStart).Size();
				if (DistToOrigin > ScaledDashLength)
				{
					DashEnd = DashStart + (Direction * ScaledDashLength);
				}
				else
				{
					DashEnd = LineEnd;
				}
			}
		}

		if (bShowCursor && CursorBrush)
		{
			const FVector2D VirtualCursorViewportPos = VirtualCursorViewportPx;
			FVector2D Pos = VirtualCursorViewportPos - CursorHotspot;
			Pos -= FVector2D(ClampedLeft, ClampedTop);

			static float LastAngle = 0.f;
			float BaseAngle = (LineVector.SizeSquared() > KINDA_SMALL_NUMBER)
				                  ? FMath::Atan2(LineVector.Y, LineVector.X) // radians (Y-down)
				                  : LastAngle;

			// Decide cursor angle based on tool mode
			float CursorAngle = BaseAngle;
			switch (CursorOrient)
			{
			case ECursorOrient::None:
				CursorAngle = 0.f;
				break;

			case ECursorOrient::AlongLineToOrigin:
				CursorAngle = BaseAngle + PI;
				break;

			case ECursorOrient::PerpendicularCW:
				CursorAngle = BaseAngle + HALF_PI;
				break;
			}
			LastAngle = BaseAngle;

			const FPaintGeometry PG = AllottedGeometry.ToPaintGeometry(
				CursorSize, FSlateLayoutTransform(Pos));

			FSlateDrawElement::MakeRotatedBox(
				OutDrawElements,
				++LayerId,
				PG,
				CursorBrush,
				ESlateDrawEffect::None,
				CursorAngle,
				CursorHotspot,
				FSlateDrawElement::RelativeToElement
			);
		}

		return LayerId;
	}
}
