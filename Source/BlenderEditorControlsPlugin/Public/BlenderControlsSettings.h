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
	// Move to "Editor Preferences" window (instead of Project Settings)
	virtual FName GetContainerName() const override { return FName("Editor"); }

	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	// Display Name in the UI
	virtual FText GetSectionText() const override { return FText::FromString("Blender Editor Controls"); }

	// Description in the UI
	virtual FText GetSectionDescription() const override
	{
		return FText::FromString("Configure controls and shortcuts.");
	}

	UFUNCTION()
	bool IsOldVersion() const { return UE_BEFORE_5_6; }

	UFUNCTION()
	bool IsUE57OrLater() const { return UE_5_7_OR_LATER; }

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

	// --- VISUALS (Cursors & HUD) ---
	UPROPERTY(EditAnywhere, config, Category = "Visuals | Axis Colors")
	FLinearColor AxisColorX = FLinearColor(FColor::FromHex(TEXT("9D1E00FF")));

	UPROPERTY(EditAnywhere, config, Category = "Visuals | Axis Colors")
	FLinearColor AxisColorY = FLinearColor(FColor::FromHex(TEXT("5B9400FF")));

	UPROPERTY(EditAnywhere, config, Category = "Visuals | Axis Colors")
	FLinearColor AxisColorZ = FLinearColor(FColor::FromHex(TEXT("004B9BFF")));

	UPROPERTY(EditAnywhere, config, Category = "Visuals | Gizmos", meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float AxisLineThickness = 2.5f;

	/** * Increase this if mouse movement feels too slow. */
	UPROPERTY(EditAnywhere, config, Category = "Interaction", meta = (
		ClampMin = "0.1",
		ClampMax = "10.0",
		EditCondition = "IsUE57OrLater", // Only shows in 5.7 and newer
		EditConditionHides))
	float MouseSensitivity = 2.0f;

	/** Slow-drag multiplier when Shift is held (Precision Mode) */
	UPROPERTY(EditAnywhere, config, Category = "Interaction", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float PrecisionScalar = 0.1f;

	/** Sensitivity for Trackball rotation mode */
	UPROPERTY(EditAnywhere, config, Category = "Interaction", meta = (ClampMin = "0.001", ClampMax = "0.1"))
	float TrackballSensitivity = 0.01f;
};
