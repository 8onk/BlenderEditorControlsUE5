#pragma once
#include "ToolBase.h"
#include "Enums.h"

namespace BlenderControls
{
	/**
	 * Implementation of the Blender-style 'Grab' (Translate) operation.
	 * Handles freeform screen-space movement, single-axis locking, and plane-locking.
	 */
	class FMoveTool final : public FToolBase
	{
	public:
		FMoveTool(const TSharedRef<FTransformSession>& InSession);

		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value) override;

		virtual bool OnBegin() override;
		virtual FText GetNumericHudText() const override;

	protected:
		virtual UE::Widget::EWidgetMode GetDesiredWidgetMode() const override { return UE::Widget::WM_Translate; }
		virtual FText GetLiveHudText() const override;
		FText BuildFreeformHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		                           const FText& MagText) const;
		FText BuildSingleAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		                             const FText& MagText) const;
		FText BuildDualAxisHudText(const FVector& LiveDelta, const FNumberFormattingOptions& NumFmt,
		                           const FText& MagText) const;

	private:
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;
		FVector GetSnapOffset(const FVector LiveDelta) const;
		void ApplyTranslationInternal(const FVector& TotalDelta, bool bIsNumeric);

		FVector StartActiveLocation;
		FVector StartVirtualPivotLocation;
		FVector ActiveToPivotOffset;
	};
} // namespace BlenderControls
