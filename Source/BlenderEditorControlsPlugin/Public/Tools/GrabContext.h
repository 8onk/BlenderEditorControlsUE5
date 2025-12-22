#pragma once
#include "CoreMinimal.h"

namespace BlenderControls
{
	struct FGrabContext
	{
		FVector2D StartMousePos;
		FVector   StartLocation;
		FVector   SingleLockAxis;
		FVector   PlaneNormal;
		FVector   PlaneAxisU;
		FVector   PlaneAxisV;
		FVector   ViewForward;
		float     ScreenToWorldScale;

		enum class EHelperType
		{
			ViewPlane,
			AxisLine,
			AxisPlane
		} ConstraintMode;
	};
}
