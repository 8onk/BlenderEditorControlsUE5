#pragma once
#include "ToolBase.h"
#include "Enums.h"

namespace BlenderControls
{
	class FMoveTool final : public FToolBase
	{
	public:
		FMoveTool(const TSharedRef<FTransformSession>& InSession);

		/* FToolBase */
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value) override;
		virtual void UpdateHud() override;

		virtual void OnBegin() override;
		virtual FText GetNumericHudText() const override;

	protected:
		virtual FText GetLiveHudText() const override;
		FText BuildFreeformHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		                           const FText& MagText) const;
		FText BuildSingleAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		                             const FText& MagText) const;
		FText BuildDualAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		                           const FText& MagText) const;

	private:
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;
		virtual FVector GetSnapOffset(const FVector OffsetFromStart) override;

		FVector StartActiveLocation;
		FVector StartVirtualPivotLocation;
		FVector ActiveToPivotOffset;
	};
} // namespace BlenderControls
