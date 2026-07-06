#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditorViewport;

namespace BlenderControls
{
	struct FNumericSlotData;

	enum class ECursorOrient : uint8
	{
		None, // no rotation (Move/Trackball)
		AlongLineToOrigin, // along B->A  (mouse -> origin)
		PerpendicularCW, // +90° from along-line
	};

	class STransformHUD : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STransformHUD)
			{
			}

		SLATE_END_ARGS()

		void Construct(const FArguments&);

		void SetReadout(const FText& In) const;
		void SetNumericEcho(const FString& In) const;

		void Attach(TSharedPtr<SEditorViewport> TargetViewport);
		void Update(TSharedPtr<SEditorViewport> TargetViewport, const FText& Readout, const FString& NumericEcho = TEXT(""));
		void Detach();

		void SetDashState(bool bEnabled, const FVector2D& InOriginPx,
		                  const FVector2D& InMousePx);

		void SetLineEndpoints(const FVector2D& InOriginPx, const FVector2D& InMousePx)
		{
			OriginViewportPx = InOriginPx;
			MouseViewportPx = InMousePx;
		}

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

		void SetVirtualCursorPos(const FVector2D& InViewportPx)
		{
			VirtualCursorViewportPx = InViewportPx;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetCursorOrientation(ECursorOrient InMode) { CursorOrient = InMode; }

	protected:
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		                      const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	private:
		TSharedPtr<STextBlock> ReadoutText;
		TSharedPtr<STextBlock> NumericText;

		TWeakPtr<SEditorViewport> AttachedViewport;
		TSharedPtr<SWidget> OverlayWrapper;

		bool bShowDash = false;
		FVector2D OriginViewportPx = FVector2D::ZeroVector;
		FVector2D MouseViewportPx = FVector2D::ZeroVector;

		float DashThickness = 2.5f;
		float ClampedLeft;
		float ClampedTop;

		bool bShowCursor = true;
		const FSlateBrush* CursorBrush = nullptr;
		FVector2D CursorSize = FVector2D(32, 32);
		FVector2D CursorHotspot = FVector2D(16, 16); // offset of "tip" inside the image
		FVector2D VirtualCursorViewportPx = FVector2D::ZeroVector;
		ECursorOrient CursorOrient = ECursorOrient::None;
	};
}
