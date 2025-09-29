#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SLevelViewport;

namespace BlenderControls
{
	enum class ECursorPolicy : uint8
	{
		AlongLine, // Scale tool: cursor points along dashed line
		Perpendicular, // Rotate tool: 90° to the line
		Default
	};

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
		void SetCursorPolicy(ECursorPolicy InPolicy) { CursorPolicy = InPolicy; }

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

	protected:
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		                      const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
		virtual FCursorReply
		OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	private:
		void UpdateDashPhaseForLengthChange();
		static EMouseCursor::Type PickNearestCursor_Aligned(const FVector2D& Dir);

		TSharedPtr<class STextBlock> ReadoutText;
		TSharedPtr<class STextBlock> NumericText;

		TWeakPtr<SLevelViewport> AttachedViewport;
		TSharedPtr<SWidget> OverlayWrapper;

		TSharedPtr<SLevelViewport> GetActiveLevelViewportWidget();

		bool bShowDash = false;
		ECursorPolicy CursorPolicy = ECursorPolicy::Default;
		FVector2D OriginViewportPx = FVector2D::ZeroVector;
		FVector2D MouseViewportPx = FVector2D::ZeroVector;

		float DashPhase = 0.f; // passed as DashScreenOffset
		float DashLengthPx = 4.0f; // ON length; gap = ON; period = 2*DashLengthPx
		float DashThickness = 1.5f;

		float PrevLen = 0.f; // for dL
		bool bHavePrevLen = false;
		float LenEpsilon = 0.75f; // px dead-zone (tune or inline)
	};
}
