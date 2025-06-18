#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "Misc/EngineVersionComparison.h"

namespace BlenderControls
{
     class FBlenderToolBase;
    /**
     * Lightweight SceneViewExtension that draws axis guides & numeric overlay.
     * Registered only while CurrentTool is valid.
     */
    class FBlenderOverlay : public FSceneViewExtensionBase
    {
    public:
        FBlenderOverlay(const FAutoRegister &AutoReg) : FSceneViewExtensionBase(AutoReg) {}

    // Old signature (UE 5.0 to 5.5)
    #if UE_VERSION_OLDER_THAN(5, 6, 0)
            // Old signature that takes a Canvas.
            virtual void Draw(const FSceneViewFamily &ViewFamily,
                            class FCanvas &Canvas) override;
    #else // 5.6+
        // New entry-point that fires on the render thread.
            virtual void PostRenderViewFamily_RenderThread(
                FRDGBuilder &GraphBuilder,
                FSceneViewFamily &InViewFamily) override;
    #endif

        /** Bound by InputProcessor each time a tool starts */
        void SetContext(TWeakPtr<FBlenderToolBase> InTool) { CurrentTool = InTool; }

    private:
        TWeakPtr<FBlenderToolBase> CurrentTool;
    };
} // namespace BlenderControls
