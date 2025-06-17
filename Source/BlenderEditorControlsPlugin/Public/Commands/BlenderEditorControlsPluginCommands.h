#pragma once
#include "Framework/Commands/Commands.h"
#include "Styling/SlateStyle.h"

namespace BlenderControls
{
    class FBlenderEditorControlsPluginCommands : public TCommands<FBlenderEditorControlsPluginCommands>
    {
    public:
        FBlenderEditorControlsPluginCommands();

        /** TCommands interface */
        virtual void RegisterCommands() override;

        TSharedPtr<FUICommandInfo> CommandTogglePlugin;

        TSharedPtr<FUICommandInfo> CommandTranslate;
        TSharedPtr<FUICommandInfo> CommandRotate;
        TSharedPtr<FUICommandInfo> CommandScale;

        TSharedPtr<FUICommandInfo> CommandAxisX;
        TSharedPtr<FUICommandInfo> CommandAxisY;
        TSharedPtr<FUICommandInfo> CommandAxisZ;

        TSharedPtr<FUICommandInfo> CommandAccept;
        TSharedPtr<FUICommandInfo> CommandCancel;
    };
} // namespace BlenderControls
