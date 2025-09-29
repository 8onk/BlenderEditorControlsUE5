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
		void SetDashState(bool bEnabled, const FVector2D& InOriginWS,
		                  const FVector2D& InMouseSS);

		FString FormatOneField(
			const FString& Label,
			const FNumericSlotData& SlotData,
			const FString& Unit,
			bool bIsActiveSlot);

		FString FormatMagnitude(float Magnitude, const TCHAR* Unit);

		static FString ToTrimmed3(double InValue);

	protected:
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		                      const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	private:
		TSharedPtr<class STextBlock> ReadoutText;
		TSharedPtr<class STextBlock> NumericText;

		TWeakPtr<SLevelViewport> AttachedViewport;
		TSharedPtr<SWidget> OverlayWrapper;

		TSharedPtr<SLevelViewport> GetActiveLevelViewportWidget();

		bool bShowDash = false;
		FVector2D OriginAbsPx = FVector2D::ZeroVector; // absolute/viewport px
		FVector2D MouseAbsPx = FVector2D::ZeroVector; // absolute/viewport px
		float DashPhase = 0.f; // animated offset
	};
}
