#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "BlenderEditorControlsEnums.h"
#include "UI/BlenderOverlay.h"
#include "BlenderEditorControlsPlugin.h"

namespace BlenderControls
{

    class FBlenderControlsInputProcessor : public IInputProcessor, public TSharedFromThis<FBlenderControlsInputProcessor>
    {
    public:
        explicit FBlenderControlsInputProcessor(TSharedPtr<FUICommandList> InCommandList);
        ~FBlenderControlsInputProcessor();

        void BindCommands();
        void AttachToLevelViewport();

        /* Public toggle – called by toolbar button / settings */
        void SetActive(bool bEnable) { bActive = bEnable; }

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

        bool IsLevelViewportFocused(FSlateApplication &SlateApp) const;

        /* Input handlers for activating transform tools */
        void TranslatePressed() { BeginTool(ETransformMode::Translate); }
        void RotatePressed() { BeginTool(ETransformMode::Rotate); }
        void ScalePressed() { BeginTool(ETransformMode::Scale); }

        /* State */
        bool bActive = false;
        bool bNumericInput = false;
        FString NumericBuffer;
        FVector2D LastMousePos = FVector2D::ZeroVector;
        TSharedPtr<class FBlenderToolBase> CurrentTool;
        ETransformMode ActiveMode;
        TSharedPtr<FUICommandList> CommandList;
        TSharedPtr<FBlenderOverlay> Overlay;
    };
} // namespace BlenderControls
