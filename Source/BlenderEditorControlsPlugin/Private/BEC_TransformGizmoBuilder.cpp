#include "BEC_TransformGizmoBuilder.h"

UInteractiveGizmo* UBEC_TransformGizmoBuilder::BuildGizmo(const FToolBuilderState& State) const
{
	UBEC_TransformGizmo* NewGizmo = NewObject<UBEC_TransformGizmo>(State.World);
	NewGizmo->Initialize(State.World);  
	return NewGizmo;
}