// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once
#include "Components/PrimitiveComponent.h"
#include "AxisLockGizmoComponent.generated.h"

/**
 * A custom component responsible for rendering colored lines in the viewport 
 * to represent the currently locked transformation axes (X, Y, Z, or planar).
 */
UCLASS(ClassGroup=Editor, meta=(BlueprintSpawnableComponent))
class UAxisLockGizmoComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	/** The starting point of the gizmo line in world space. */
	FVector Origin = FVector::ZeroVector;
	
	/** The normalized direction vector of the line. */
	FVector AxisDir = FVector::ForwardVector;
	
	/** The total length of the line in world units. */
	float LineLength = 1000.0f;
	
	/** The primary color of the line. */
	FLinearColor Color = FLinearColor::Red;
	
	/** The thickness of the line drawn on screen. */
	float ThicknessPx = 2.0f;
	
	/** If true, the line will be rendered with a dashed material instead of solid. */
	bool bDashed = false;
	
	/** The base material used to render the line. */
	UMaterialInterface* AxisMaterial = nullptr;
	
	/** The dynamic material instance used to change color and properties at runtime. */
	UMaterialInstanceDynamic* AxisMID = nullptr;
	
	/** The color applied to the dynamic material instance. */
	FLinearColor AxisColor = FLinearColor::Red; // default
	
	/** True if the viewport is currently rendering in wireframe mode. */
	bool bIsWireframeView = false;
	
	/** Updates the color of the gizmo line. */
	void SetAxisColor(const FLinearColor& InColor);

	virtual void OnRegister() override;

	//~ UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	/** Called by Unreal Engine's Renderer to determine the bounding box. */
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
};
