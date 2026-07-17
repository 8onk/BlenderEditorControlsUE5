#pragma once
#include "Tools/ToolBase.h"

namespace BlenderControls
{
	/**
	 * Implementation of the Blender-style 'Scale' operation.
	 */
	class FScaleTool final : public FToolBase
	{
	public:
		explicit FScaleTool(const TSharedRef<FTransformSession>& InSession);

		//~ FToolBase Interface
		virtual void OnActive(const FVector2D& CurrentViewportMousePosition) override;
		virtual void ApplyNumeric(double Value) override;
		virtual bool OnBegin() override;
		virtual void OnEnd(bool bApply) override;
		virtual void Tick() override;

	private:
		FVector PivotStartPosition;
		FVector2D PivotViewportPosition;
		FVector StartScale;
		FVector2D InitialMousePosition;
		float InitialMouseToPivotDistance;
		float CurrentMouseToPivotDistance;
		float ScaleFactor = 1.0f;
		FVector CurrentScaleMultiplier = FVector::OneVector;

		//~ FToolBase Interface
		virtual void SetGrabContextAxisLock(EAxisLock AxisLock) override;

	protected:
		//~ FToolBase Interface
		virtual UE::Widget::EWidgetMode GetDesiredWidgetMode() const override { return UE::Widget::WM_Scale; }
		virtual FText GetNumericHudText() const override;
		virtual FText GetLiveHudText() const override;
		
		FText BuildFreeformHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const;
		FText BuildSingleAxisHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const;
		FText BuildDualAxisHudText(const FVector& LiveScale, const FNumberFormattingOptions& NumFmt) const;
	};
} // namespace BlenderControls
