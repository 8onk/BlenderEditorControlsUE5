#pragma once

#include "Framework/Application/IInputProcessor.h"

namespace BlenderControls
{
    /** High-level mode enumeration */
    enum class ETransformMode : uint8
    {
        None,
        Translate,
        Rotate,
        Scale
    };
    enum class ETransformAxis : uint8
    {
        None,
        X,
        Y,
        Z,
        All
    };

    class FBlenderControlsInputProcessor : public IInputProcessor, public TSharedFromThis<FBlenderControlsInputProcessor>
    {
    public:
        explicit FBlenderControlsInputProcessor(TSharedPtr<FUICommandList> InCommandList);
        ~FBlenderControlsInputProcessor();

        /* Public toggle – called by toolbar button / settings */
        void SetActive(bool bEnable);

        /** IInputProcessor overrides */
        virtual void Tick(const float DeltaTime, FSlateApplication &, TSharedRef<ICursor>) override;
        virtual bool HandleKeyDownEvent(FSlateApplication &, const FKeyEvent &) override;
        virtual bool HandleKeyUpEvent(FSlateApplication &, const FKeyEvent &) override;
        virtual bool HandleMouseMoveEvent(FSlateApplication &, const FPointerEvent &) override;
        virtual bool HandleMouseButtonDownEvent(FSlateApplication &, const FPointerEvent &) override;
        virtual bool HandleMouseButtonUpEvent(FSlateApplication &, const FPointerEvent &) override;

    private:
        /* Helpers */
        void BeginTool(ETransformMode Mode);
        void EndTool(bool bApply);
        void UpdateAxis(ETransformAxis Axis);
        void FeedNumeric(const TCHAR Digit);
        void FlushNumeric();

        /* State */
        bool bActive = false;
        bool bNumericInput = false;
        FString NumericBuffer;
        FVector2D LastMousePos = FVector2D::ZeroVector;
        TSharedPtr<class FBlenderToolBase> CurrentTool;
        TWeakPtr<FUICommandList> CommandList;
    };
} // namespace BlenderControls
