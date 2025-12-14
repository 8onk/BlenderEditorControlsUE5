#pragma once
#include "BlenderControlsSettings.generated.h"

/**
 * User-editable settings (shows up under Editor Preferences → Plugins → Blender3D Editor Controls)
 */
UCLASS(config = EditorSettings, defaultconfig)
class BLENDEREDITORCONTROLSPLUGIN_API UBlenderControlsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	//--- SETUP ---
	// Move to "Editor Preferences" window (instead of Project Settings)
	virtual FName GetContainerName() const override { return FName("Editor"); }

	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	// Display Name in the UI
	virtual FText GetSectionText() const override { return FText::FromString("Blender3D Editor Controls"); }

	// Description in the UI
	virtual FText GetSectionDescription() const override
	{
		return FText::FromString("Configure controls and shortcuts.");
	}

	//--- SETTINGS ---
	UPROPERTY(EditAnywhere, config, Category = "Visuals")
	FVector2D CursorSize = FVector2D(32, 32);

	// --- VISUALS (Cursors & HUD) ---
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Colors")
	FLinearColor AxisColorX = FLinearColor::Red;

	UPROPERTY(EditAnywhere, config, Category = "Visuals | Colors")
	FLinearColor AxisColorY = FLinearColor::Green;

	UPROPERTY(EditAnywhere, config, Category = "Visuals | Colors")
	FLinearColor AxisColorZ = FLinearColor::Blue;

	UPROPERTY(EditAnywhere, config, Category = "Visuals | Gizmos", meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float AxisLineThickness = 2.5f;

	/** Slow-drag multiplier when Shift is held (Precision Mode) */
	UPROPERTY(EditAnywhere, config, Category = "Interaction", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float PrecisionScalar = 0.1f;

	/** Sensitivity for Trackball rotation mode */
	UPROPERTY(EditAnywhere, config, Category = "Interaction", meta = (ClampMin = "0.001", ClampMax = "0.1"))
	float TrackballSensitivity = 0.01f;
};
