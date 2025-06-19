#include "Utils/BlenderMathHelpers.h"
#include "Utils/BlenderMathHelpers.h"
#include "Math/Vector.h"
#include "Kismet/KismetMathLibrary.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "Components/LineBatchComponent.h"

namespace BlenderControls::Math
{
    FVector ScreenDeltaToWorld(const FVector2D &ScreenDelta, const FViewportCameraTransform &CamXForm)
    {
        return FVector::ZeroVector; // tmp stub
    }

    FVector LinePlaneIntersection(const FVector &RayStart, const FVector &RayDir,
                                  const FVector &PlaneOrigin, const FVector &PlaneNormal)
    {
        return FVector::ZeroVector; // tmp stub
    }

    FQuat AlignAxisToNormal(const FQuat &CurrentRot, const FVector &LocalAxis, const FVector &TargetNormal)
    {
        return FQuat::Identity; // tmp stub
    }

    void DrawAxisLine(UWorld *, const FVector &Origin, const FVector &Dir, const FLinearColor &, float Life)
    {
        // tmp stub
    }

    void DrawDashedLine(class FCanvas *, const FVector &A, const FVector &B,
                        float Thickness, float DashLength, const FLinearColor &)
    {
        // tmp stub
    }
}