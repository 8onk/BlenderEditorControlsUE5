#pragma once
#include "BlenderEditorControlsPluginSettings.generated.h"

/**
 * User-editable settings (shows up under
 * Editor Preferences → Plugins → Blender Controls)
 */
UCLASS(config = EditorPerProjectUserSettings, defaultconfig)
class BLENDEREDITORCONTROLSPLUGIN_API UBlenderEditorControlsPluginSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** Plugin starts enabled each editor launch */
    UPROPERTY(EditAnywhere, config, Category = "General")
    bool bEnableOnStartup = true;

    /** Slow-drag multiplier when Shift is held */
    UPROPERTY(EditAnywhere, config, Category = "Controls", meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "0.5"))
    float PrecisionScalar = 0.1f;

    /** Default offset applied in surface-snap mode (Ctrl-drag) */
    UPROPERTY(EditAnywhere, config, Category = "Snapping")
    float SurfaceSnapOffset = 0.f;

    /** Priority passed to Slate when registering the input processor */
    UPROPERTY(EditAnywhere, config, Category = "Advanced", meta = (ClampMin = "-100", ClampMax = "100"))
    int32 SlatePriority = 0;
};
