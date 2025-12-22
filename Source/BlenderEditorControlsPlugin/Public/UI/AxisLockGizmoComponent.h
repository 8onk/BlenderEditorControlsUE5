#pragma once
#include "Components/PrimitiveComponent.h"
#include "AxisLockGizmoComponent.generated.h"

UCLASS(ClassGroup=Editor, meta=(BlueprintSpawnableComponent))
class UAxisLockGizmoComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	FVector Origin = FVector::ZeroVector;
	FVector AxisDir = FVector::ForwardVector;
	float LineLength = 1000.0f;
	FLinearColor Color = FLinearColor::Red;
	float ThicknessPx = 2.0f;
	bool bDashed = false;
	UMaterialInterface* AxisMaterial = nullptr;
	UMaterialInstanceDynamic* AxisMID = nullptr;
	FLinearColor AxisColor = FLinearColor::Red; // default
	void SetAxisColor(const FLinearColor& InColor);

	virtual void OnRegister() override;

	// UPrimitiveComponent
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	//Called by Unreal Engine's Renderer. 
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
};
