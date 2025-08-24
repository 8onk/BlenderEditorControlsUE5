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

		FString FormatOneField(
			const FString& Label,
			const FNumericSlotData& SlotData,
			bool bIsSlotBeingEdited,
			const FString& Unit);

		FString FormatMagnitude(float Magnitude, const TCHAR* Unit);

		static FString ToTrimmed3(double InValue);

	private:
		TSharedPtr<class STextBlock> ReadoutText;
		TSharedPtr<class STextBlock> NumericText;

		TWeakPtr<SLevelViewport> AttachedViewport;
		TSharedPtr<SWidget> OverlayWrapper;

		TSharedPtr<SLevelViewport> GetActiveLevelViewportWidget();
	};
}
