#pragma once

#include "CoreMinimal.h"
#include "Tools/SharedPivot.h"
#include "Misc/Optional.h"
#include "Math/Plane.h"
#include "BlenderEditorControlsPlugin.h"

namespace BlenderControls::Math
{
    /** Converts screen Δ to a world-space translation based on camera vectors */
    FVector ScreenDeltaToWorld(const FVector2D &DeltaPx, float DepthUU);

    /** Ray-plane intersection used by Move & Rotate */
    FVector LinePlaneIntersection(const FVector &RayStart, const FVector &RayDir,
                                  const FVector &PlaneOrigin, const FVector &PlaneNormal);

    /** Compute quaternion that aligns LocalAxis to WorldNormal (surface-snap) */
    FQuat AlignAxisToNormal(const FQuat &CurrentRot, const FVector &LocalAxis, const FVector &TargetNormal);

    /** Draw dashed or solid line helpers (editor-only) */
    void DrawAxisLine(UWorld *, const FVector &Origin, const FVector &Dir, const FLinearColor &, float Life = 1.f);
    void DrawDashedLine(class FCanvas *, const FVector &A, const FVector &B,
                        float Thickness = 2.f, float DashLength = 10.f,
                        const FLinearColor & = FLinearColor::White);

    TOptional<FPlane> MakeDragPlaneFromSelection(TSharedPtr<class BlenderControls::FSharedPivot> Group);

    float ComputeOrthoWidth(const FEditorViewportClient *VC);
} // namespace BlenderControls::Math