#pragma once

#include "CoreMinimal.h"
#include "BlenderEditorControlsEnums.h"
#include "Tools/SharedPivot.h"
#include "Misc/Optional.h"
#include "Math/Plane.h"
#include "BlenderEditorControlsPlugin.h"

namespace BlenderControls::Math
{
	void GetMousePosToViewportPos(const FVector2D &DesktopPos, FVector2D &OutViewportPos);

	/** Rounds all components of a vector to their nearest integer values */
	FVector RoundVectorToInt(const FVector &InVector);
	FVector GetAxisVector(ETransformAxis InAxis); 
} // namespace BlenderControls::Math
