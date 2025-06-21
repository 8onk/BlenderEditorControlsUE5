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
    FVector ScreenDeltaToWorld(const FVector2D &DeltaPx, float DepthUU)
    {
        FViewport *Viewport = GEditor ? GEditor->GetActiveViewport() : nullptr;
        if (!Viewport)
        {
            return FVector::ZeroVector;
        }

        FEditorViewportClient *VC = static_cast<FEditorViewportClient *>(Viewport->GetClient());
        if (!VC)
        {
            return FVector::ZeroVector;
        }

        const FIntPoint Size = Viewport->GetSizeXY();
        if (Size.X == 0 || Size.Y == 0)
        {
            return FVector::ZeroVector;
        }

        float UnitsPerPixel = 0.f;

        if (VC->IsOrtho())
        {
            float OrthoWidth = ComputeOrthoWidth(VC);
            UnitsPerPixel = OrthoWidth / static_cast<float>(Size.X);
        }
        else
        {
            const float FovRad = FMath::DegreesToRadians(VC->ViewFOV);
            UnitsPerPixel = 2.f * FMath::Tan(FovRad * 0.5f) * DepthUU / static_cast<float>(Size.Y);
        }

        FVector Right = VC->GetViewRotation().RotateVector(FVector::RightVector);
        FVector Up = VC->GetViewRotation().RotateVector(FVector::UpVector);

        return (Right * DeltaPx.X + Up * -DeltaPx.Y) * UnitsPerPixel;
    }

    FVector LinePlaneIntersection(const FVector &RayStart, const FVector &RayDir,
                                  const FVector &PlaneOrigin, const FVector &PlaneNormal)
    {
        const FVector N = PlaneNormal.GetSafeNormal();
        const float Den = FVector::DotProduct(RayDir, N);

        if (FMath::IsNearlyZero(Den))
        {
            return RayStart; // parallel → give back start
        }

        const float T = FVector::DotProduct(PlaneOrigin - RayStart, N) / Den;
        return RayStart + RayDir * T;
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

    TOptional<FPlane> MakeDragPlaneFromSelection(TSharedPtr<BlenderControls::FSharedPivot> Group)
    {
        FVector Pivot = Group->GetPivot();

        // 2 – camera basis
        FLevelEditorViewportClient *VC = GCurrentLevelEditingViewportClient;
        if (!VC)
        {
            return TOptional<FPlane>();
        }

        const FVector CamFwd = VC->GetViewRotation().Vector();

        // 3 – plane (point, normal)
        return FPlane(/*point=*/Pivot, /*normal=*/CamFwd);
    }

    float ComputeOrthoWidth(const FEditorViewportClient *VC)
    {
        if (VC->GetViewMode() == LVT_OrthoXY ||
            VC->GetViewMode() == LVT_OrthoXZ ||
            VC->GetViewMode() == LVT_OrthoYZ)
        {
            const float HalfWidth = VC->GetOrthoZoom();
            const float FullWidth = HalfWidth * 2.0f;
            UE_LOG(LogBlenderEditorControls, Log, TEXT("Ortho Viewport Width: %f"), FullWidth);
        }

        return 0.f;
    }
}