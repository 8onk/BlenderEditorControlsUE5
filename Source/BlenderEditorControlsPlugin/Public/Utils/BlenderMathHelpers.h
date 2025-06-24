#pragma once

#include "CoreMinimal.h"
#include "Tools/SharedPivot.h"
#include "Misc/Optional.h"
#include "Math/Plane.h"
#include "BlenderEditorControlsPlugin.h"

namespace BlenderControls::Math
{
	void GetMousePosToViewportPos(const FVector2D& DesktopPos, FVector2D& OutViewportPos);
} // namespace BlenderControls::Math
