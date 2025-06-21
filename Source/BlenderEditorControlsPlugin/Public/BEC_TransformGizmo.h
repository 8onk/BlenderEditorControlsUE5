#pragma once
#include "CoreMinimal.h"
#include "EditorGizmos/TransformGizmo.h"
#include "BEC_TransformGizmo.generated.h"

UCLASS()
class BLENDEREDITORCONTROLSPLUGIN_API UBEC_TransformGizmo : public UTransformGizmo
{
	GENERATED_BODY()

protected:
	// Called once the gizmo is fully constructed
	virtual void Setup() override;

	// Example: amplify X-axis drags by ×2
	virtual void OnClickDragTranslateAxis(const FInputDeviceRay& DragPos) override;
};
