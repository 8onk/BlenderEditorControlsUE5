// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once
#include "BlenderControlsSettings.generated.h"

/**
 * User-editable settings (shows up under Editor Preferences → Plugins → Blender Editor Controls)
 */
UCLASS(config = EditorSettings, defaultconfig)
class BLENDEREDITORCONTROLSPLUGIN_API UBlenderControlsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 
	 * Determines the top-level settings container for this config object.
	 * 
	 * @return Always returns "Editor" to place these settings in the Editor Preferences (instead of Project Settings).
	 */
	virtual FName GetContainerName() const override { return FName("Editor"); }

	/** 
	 * Determines the category under which these settings will appear.
	 * 
	 * @return Returns "Plugins" so it sits alongside other plugin configurations.
	 */
	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	/** Explicitly define the section name for navigation */
	virtual FName GetSectionName() const override { return FName("BlenderEditorControls"); }

	/** 
	 * Provides the display name for the section in the settings UI sidebar.
	 * 
	 * @return The localized text "Blender Editor Controls".
	 */
	virtual FText GetSectionText() const override { return FText::FromString("Blender Editor Controls"); }

	/** 
	 * Provides the tooltip description for the section in the settings UI.
	 * 
	 * @return The localized text explaining what this settings page configures.
	 */
	virtual FText GetSectionDescription() const override
	{
		return FText::FromString("Configure controls and shortcuts.");
	}

	/** 
	 * Helper function used by EditCondition metadata to show/hide legacy UI properties.
	 * 
	 * @return True if the engine version is older than 5.6.
	 */
	UFUNCTION()
	static bool IsOldVersion() { return UE_BEFORE_5_6; }

	/** Hidden helper to drive UI visibility based on engine version */
	UPROPERTY(Transient)
	bool bIsOldVersion = UE_BEFORE_5_6;

	/** Overlay margin from the left edge of the viewport (in pixels). */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Overlay", meta = (
		ClampMin = "0",
		EditCondition = "IsOldVersion()",
		EditConditionHides))
	float MarginLeft = 5.f;

	/** Overlay margin from the top edge of the viewport (in pixels). */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Overlay", meta = (
		ClampMin = "0",
		EditCondition = "IsOldVersion()",
		EditConditionHides))
	float MarginTop = 40.f;

	/** --- VISUALS (Cursors & HUD) --- */
	
	/** The display color used for the X-Axis in gizmos and lines. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Axis Colors")
	FLinearColor AxisColorX = FLinearColor(FColor::FromHex(TEXT("FA3500FF")));

	/** The display color used for the Y-Axis in gizmos and lines. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Axis Colors")
	FLinearColor AxisColorY = FLinearColor(FColor::FromHex(TEXT("9BF700FF")));

	/** The display color used for the Z-Axis in gizmos and lines. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Axis Colors")
	FLinearColor AxisColorZ = FLinearColor(FColor::FromHex(TEXT("007BF6FF")));

	/** Base thickness of axis lines drawn in the viewport during transform. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Gizmos", meta = (ClampMin = "0.5", ClampMax = "6.0"))
	float AxisLineThickness = 4.f;

	/** Visual scale multiplier for dashed lines drawn during constraint operations. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Gizmos", meta = (ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.5", UIMax = "2.0"))
	float DashLineScale = 1.0f;

	/** Scale multiplier for the heads-up display elements (like the numeric input box). */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | HUD", meta = (ClampMin = "1", ClampMax = "3.0", UIMin = "1", UIMax = "2.0"))
	float HudScale = 1.0f;

	/** Scale multiplier for the custom mouse cursor used during transformations. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Cursors", meta = (ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.5", UIMax = "2.0"))
	float CustomCursorScale = 1.0f;

	/** Slow-drag multiplier when Shift is held (Precision Mode) */
	UPROPERTY(EditAnywhere, config, Category = "Interaction", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float PrecisionScalar = 0.1f;

	/** Sensitivity for Trackball rotation mode */
	UPROPERTY(EditAnywhere, config, Category = "Interaction", meta = (ClampMin = "0.001", ClampMax = "0.1"))
	float TrackballSensitivity = 0.01f;
};
