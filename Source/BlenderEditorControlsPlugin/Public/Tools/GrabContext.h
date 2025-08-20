#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	struct FGrabContext
	{
		FVector2D StartMousePos;
		FVector   StartLocation;
		FVector   HelperAxisDir;
		FVector   HelperPlaneN;
		FVector   PlaneAxisU;
		FVector   PlaneAxisV;
		FVector   ViewForward;
		float     ScreenToWorldScale;

		enum class EHelperType
		{
			ViewPlane,
			AxisLine,
			AxisPlane
		} HelperType;
	};
}
