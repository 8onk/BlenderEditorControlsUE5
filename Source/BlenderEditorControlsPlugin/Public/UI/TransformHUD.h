// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditorViewport;

namespace BlenderControls
{
	struct FNumericSlotData;

	/** Determines the rotation applied to the custom cursor. */
	enum class ECursorOrient : uint8
	{
		/** No rotation applied (Move/Trackball). */
		None, 
		/** Cursor points from the mouse towards the origin (Scale). */
		AlongLineToOrigin, 
		/** Cursor is rotated 90 degrees from the along-line direction (Rotate). */
		PerpendicularCW, 
	};

	/**
	 * An overlay widget that displays the current transform values, numeric input,
	 * and draws the custom cursor and dashed lines in the viewport.
	 */
	class STransformHUD : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STransformHUD)
			{
			}

		SLATE_END_ARGS()

		/** Initializes the widget layout and sub-widgets. */
		void Construct(const FArguments&);

		/** Sets the primary textual readout (e.g. current translation delta). */
		void SetReadout(const FText& In) const;
		
		/** Sets the text for the numeric input field. */
		void SetNumericEcho(const FString& In) const;

		/** Attaches this HUD to the specified viewport. */
		void Attach(const TSharedPtr<SEditorViewport>& TargetViewport);
		
		/** Updates the text displays and ensures the HUD is attached to the target viewport. */
		void Update(const TSharedPtr<SEditorViewport>& TargetViewport, const FText& Readout, const FString& NumericEcho = TEXT(""));
		
		/** Removes this HUD from the active viewport overlay. */
		void Detach();

		/** Toggles and sets the positions for the dashed guide line (e.g. during Scale/Rotate). */
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
		float GetCappedHudScale() const;
		FSlateFontInfo GetDynamicFont() const;
		FMargin GetDynamicPaddingUE5() const;
		FMargin GetDynamicPaddingLegacy() const;

		TSharedPtr<STextBlock> ReadoutText;
		TSharedPtr<STextBlock> NumericText;

		TWeakPtr<SEditorViewport> AttachedViewport;
		TWeakPtr<SOverlay> AttachedOverlay;
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
