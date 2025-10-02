#pragma once
#include "Components/PrimitiveComponent.h"
#include "AxisLockGizmoComponent.generated.h"

UCLASS(ClassGroup=Editor, meta=(BlueprintSpawnableComponent))
class UAxisLockGizmoComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	// editable gizmo inputs:
	UPROPERTY(EditAnywhere, Category="Gizmo")
	FVector Origin = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category="Gizmo")
	FVector AxisDir = FVector::ForwardVector;
	UPROPERTY(EditAnywhere, Category="Gizmo")
	float LineLength = 1000.0f;
	UPROPERTY(EditAnywhere, Category="Gizmo")
	FLinearColor Color = FLinearColor::Red;
	UPROPERTY(EditAnywhere, Category="Gizmo")
	float ThicknessPx = 2.0f;
	UPROPERTY(EditAnywhere, Category="Gizmo")
	bool bDashed = false;
	UPROPERTY()
	UMaterialInterface* AxisMaterial = nullptr;
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* AxisMID = nullptr;
	UPROPERTY(EditAnywhere, Category="Gizmo")
	FLinearColor AxisColor = FLinearColor::Red; // default
	UFUNCTION()
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
