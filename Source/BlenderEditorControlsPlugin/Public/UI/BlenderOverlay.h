#pragma once

#include "SceneViewExtension.h"

namespace BlenderControls
{
    /**
     * Lightweight SceneViewExtension that draws axis guides & numeric overlay.
     * Registered only while CurrentTool is valid.
     */
    class FBlenderOverlay : public FSceneViewExtensionBase
    {
    public:
        FBlenderOverlay(const FAutoRegister &AutoReg) : FSceneViewExtensionBase(AutoReg) {}

        virtual void Draw(const FSceneViewFamily &ViewFamily,
                          FViewport *,
                          FPrimitiveDrawInterface *PDI) override;

        /** Bound by InputProcessor each time a tool starts */
        void SetContext(TWeakPtr<FBlenderToolBase> InTool) { CurrentTool = InTool; }

    private:
        TWeakPtr<FBlenderToolBase> CurrentTool;
    };
} // namespace BlenderControls
