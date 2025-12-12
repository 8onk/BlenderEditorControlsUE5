#pragma once
#include "Components/PrimitiveComponent.h"
#include "AxisLockGizmoComponent.generated.h"

UCLASS(ClassGroup=Editor, meta=(BlueprintSpawnableComponent))
class UAxisLockGizmoComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	// editable gizmo inputs:
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

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override
	{
		const FVector SafeDir = AxisDir.IsNearlyZero() ? FVector::ForwardVector : AxisDir.GetSafeNormal();
		const float SafeLen = FMath::Clamp(LineLength, 1.f, 1e7f); // cap to avoid overflow
		const FVector A = Origin - SafeDir * SafeLen;
		const FVector B = Origin + SafeDir * SafeLen;
		const FBox Box(A, B);
		return FBoxSphereBounds(Box).TransformBy(LocalToWorld);
	}
};
