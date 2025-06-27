#include "Utils/BlenderMathHelpers.h"
#include "Math/Vector.h"
#include "Engine/Engine.h"
#include "Editor.h"

namespace BlenderControls::Math
{
	void GetMousePosToViewportPos(const FVector2D &ScreenSpacePos, FVector2D &OutViewportPos)
	{
		FViewport *Viewport = GEditor->GetActiveViewport();
		if (!Viewport)
			return;

		const FIntPoint DesktopInt(static_cast<int32>(ScreenSpacePos.X),
								   static_cast<int32>(ScreenSpacePos.Y));

		const FVector2D Normalized = Viewport->VirtualDesktopPixelToViewport(DesktopInt);
		OutViewportPos = Normalized * FVector2D(Viewport->GetSizeXY());
	}

	FVector RoundVectorToInt(const FVector &InVector)
	{
		return FVector(
			FMath::RoundToInt(InVector.X),
			FMath::RoundToInt(InVector.Y),
			FMath::RoundToInt(InVector.Z));
	}
}