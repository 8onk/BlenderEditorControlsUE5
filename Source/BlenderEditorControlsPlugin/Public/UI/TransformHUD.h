#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SLevelViewport;

namespace BlenderControls
{
	struct FNumericSlotData;

	class STransformHUD : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STransformHUD)
			{
			}

		SLATE_END_ARGS()

		void Construct(const FArguments&);

		void SetReadout(const FText& In);
		void SetNumericEcho(const FString& In);

		void Attach();
		void Update(const FText& Readout, const FString& NumericEcho = TEXT(""));
		void Detach();

		void SetDashState(bool bEnabled, const FVector2D& InOriginPx,
		                  const FVector2D& InMousePx);

		void SetLineEndpoints(const FVector2D& InOriginPx, const FVector2D& InMousePx)
		{
			OriginViewportPx = InOriginPx;
			MouseViewportPx = InMousePx;
		}

		FString FormatOneField(
			const FString& Label,
			const FNumericSlotData& SlotData,
			const FString& Unit,
			bool bIsActiveSlot);

		FString FormatMagnitude(float Magnitude, const TCHAR* Unit);

		static FString ToTrimmed3(double InValue);

		void SetCursorBrush(const FSlateBrush* InBrush)
		{
			CursorBrush = InBrush;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetCursorSize(const FVector2D& InSize)
		{
			CursorSize = InSize;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetCursorHotspot(const FVector2D& InHot)
		{
			CursorHotspot = InHot;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetCursorVisible(bool bInVisible)
		{
			bShowCursor = bInVisible;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetVirtualCursor(const FVector2D& InViewportPx)
		{
			VirtualCursorViewportPx = InViewportPx;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

	protected:
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		                      const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	private:
		void UpdateDashPhaseForLengthChange();

		TSharedPtr<STextBlock> ReadoutText;
		TSharedPtr<STextBlock> NumericText;

		TWeakPtr<SLevelViewport> AttachedViewport;
		TSharedPtr<SWidget> OverlayWrapper;

		TSharedPtr<SLevelViewport> GetActiveLevelViewportWidget();

		bool bShowDash = false;
		FVector2D OriginViewportPx = FVector2D::ZeroVector;
		FVector2D MouseViewportPx = FVector2D::ZeroVector;

		float DashPhase = 0.f; // passed as DashScreenOffset
		float DashLengthPx = 4.0f; // ON length; gap = ON; period = 2*DashLengthPx
		float DashThickness = 1.5f;

		float PrevLen = 0.f; // for dL
		bool bHavePrevLen = false;
		float LenEpsilon = 0.75f; // px dead-zone (tune or inline)

		bool bShowCursor = true;
		const FSlateBrush* CursorBrush = nullptr;
		FVector2D CursorSize = FVector2D(24, 24);
		FVector2D CursorHotspot = FVector2D(0, 0); // offset of "tip" inside the image
		FVector2D VirtualCursorViewportPx = FVector2D::ZeroVector;
	};
}
