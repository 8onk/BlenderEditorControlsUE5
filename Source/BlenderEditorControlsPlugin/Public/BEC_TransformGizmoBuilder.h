#pragma once
#include "CoreMinimal.h"
#include "BaseGizmos/CombinedTransformGizmo.h"
#include "BEC_TransformGizmo.h"
#include "BEC_TransformGizmoBuilder.generated.h" 

UCLASS()
class BLENDEREDITORCONTROLSPLUGIN_API UBEC_TransformGizmoBuilder : public UCombinedTransformGizmoBuilder
{
	GENERATED_BODY()
public:
	virtual UInteractiveGizmo* BuildGizmo(const FToolBuilderState& State) const override;
};
