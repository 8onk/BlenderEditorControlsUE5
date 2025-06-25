#include "Utils/BlenderMathHelpers.h"
#include "Utils/BlenderMathHelpers.h"
#include "Math/Vector.h"
#include "Kismet/KismetMathLibrary.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "Components/LineBatchComponent.h"
#include "Editor.h"
#include "Kismet/GameplayStatics.h"
#include "LevelEditorViewport.h"

namespace BlenderControls::Math
{
	void GetMousePosToViewportPos(const FVector2D &DesktopPos, FVector2D &OutViewportPos)
	{
		FViewport *Viewport = GEditor->GetActiveViewport();
		if (!Viewport)
			return;

		const FIntPoint DesktopInt(static_cast<int32>(DesktopPos.X),
								   static_cast<int32>(DesktopPos.Y));

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