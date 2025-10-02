// AxisLockGizmoComponent.cpp
#include "UI/AxisLockGizmoComponent.h"
#include "PrimitiveSceneProxy.h"
#include "DynamicMeshBuilder.h"
#include "Materials/MaterialRenderProxy.h"

class FAxisLockSceneProxy : public FPrimitiveSceneProxy
{
public:
	FAxisLockSceneProxy(const UAxisLockGizmoComponent* Comp)
		: FPrimitiveSceneProxy(Comp)
		  , Origin(Comp->Origin)
		  , AxisDir(Comp->AxisDir.GetSafeNormal())
		  , LineLength(Comp->LineLength)
		  , ThicknessPx(Comp->ThicknessPx)
		  , Color(Comp->AxisColor)
	{
		const ERHIFeatureLevel::Type FL = GetScene().GetFeatureLevel();

		// Use engine base material for relevance (unlit opaque)
		UMaterialInterface* BaseMat = GEngine->LevelColorationUnlitMaterial;
		MaterialRelevance = BaseMat->GetRelevance_Concurrent(FL);

		bWillEverBeLit = false;
	}

	virtual SIZE_T GetTypeHash() const override { return 0u; }

	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector& Collector) const override
	{
		const FVector A = Origin - AxisDir * LineLength;
		const FVector B = Origin + AxisDir * LineLength;

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
		{
			if ((VisibilityMap & (1 << ViewIndex)) == 0) continue;
			const FSceneView* View = Views[ViewIndex];
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			// Build camera-facing ribbon along AB with screen-constant width.
			BuildRibbonQuads(*View, ViewIndex, A, B, ThicknessPx, Color, /*Segments*/ 32, Collector, PDI, /*Proxy*/
			                 MaterialProxy);
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance R;
		R.bDrawRelevance = IsShown(View);
		R.bDynamicRelevance = true;
		R.bRenderInMainPass = true;
		MaterialRelevance.SetPrimitiveViewRelevance(R);
		return R;
	}

	virtual uint32 GetMemoryFootprint() const override { return sizeof(*this) + GetAllocatedSize(); }
	uint32 GetAllocatedSize() const { return 0; }

private:
	FVector Origin;
	FVector AxisDir;
	float LineLength;
	FLinearColor Color;
	float ThicknessPx = 2.0f;
	bool bDashed;
	const FMaterialRenderProxy* MaterialProxy = nullptr;
	FMaterialRelevance MaterialRelevance;

	// Compute world-units-per-pixel at a point for this view (persp & ortho).
	static float WorldPerPixelAt(const FSceneView& View, const FVector& WorldPos)
	{
		if (View.IsPerspectiveProjection())
		{
			const FVector ViewPos = View.ViewMatrices.GetViewMatrix().TransformPosition(WorldPos);
			const float depth = FMath::Abs(ViewPos.Z);
			const float HFovDeg = View.FOV; // UE exposes FOV here
			const float HFovRad = FMath::DegreesToRadians(HFovDeg);
			const float viewWidthPx = float(View.UnscaledViewRect.Width());
			const float screenWidthWorld = 2.f * depth * FMath::Tan(HFovRad * 0.5f);
			return screenWidthWorld / viewWidthPx;
		}
		else
		{
			// Orthographic width in world units across the view rect:
			const float orthoWidthWorld = View.ViewMatrices.GetProjectionMatrix().M[0][0] == 0
				                              ? 1.f
				                              : (View.ViewMatrices.GetInvProjectionMatrix().TransformFVector4(
						                              FVector4(1, 0, 1, 1)).X
					                              - View.ViewMatrices.GetInvProjectionMatrix().TransformFVector4(
						                              FVector4(-1, 0, 1, 1)).X) * 0.5f;

			// More robust: UE exposes View.WorldToScreenScale/UnscaledViewRect; using rect width is fine:
			const float viewWidthPx = float(View.UnscaledViewRect.Width());
			return orthoWidthWorld / viewWidthPx;
		}
	}

	static void BuildRibbonQuads(
		const FSceneView& View,
		int32 ViewIndex,
		const FVector& A,
		const FVector& B,
		float ThicknessPx,
		const FLinearColor& Color,
		int Segments,
		FMeshElementCollector& Collector,
		FPrimitiveDrawInterface* PDI,
		const FMaterialRenderProxy* Proxy)
	{
		// Tesselate AB so thickness adapts if depth varies along the line.
		const int32 N = FMath::Max(1, Segments);
		const FVector Dir = (B - A) / float(N);

		UMaterialInterface* Mat = UMaterial::GetDefaultMaterial(MD_Surface);

		FDynamicMeshBuilder MeshBuilder(View.GetFeatureLevel());

		for (int32 i = 0; i < N; ++i)
		{
			const FVector P0 = A + Dir * float(i);
			const FVector P1 = A + Dir * float(i + 1);

			// Camera-facing right vector = normalize( ViewDir x SegmentDir )
			// View forward (world) can be approximated from view matrix:
			const FVector CamForward = View.GetViewDirection(); // world forward (towards scene)
			const FVector SegDir = (P1 - P0).GetSafeNormal();
			FVector Right = FVector::CrossProduct(CamForward, SegDir).GetSafeNormal();
			// If nearly parallel, fall back to another basis:
			if (Right.IsNearlyZero()) Right = FVector::CrossProduct(FVector::UpVector, SegDir).GetSafeNormal();

			const float wpp0 = WorldPerPixelAt(View, P0);
			const float wpp1 = WorldPerPixelAt(View, P1);
			const float halfW0 = 0.5f * ThicknessPx * wpp0;
			const float halfW1 = 0.5f * ThicknessPx * wpp1;

			const FVector v0 = P0 - Right * halfW0;
			const FVector v1 = P0 + Right * halfW0;
			const FVector v2 = P1 - Right * halfW1;
			const FVector v3 = P1 + Right * halfW1;

			const FVector SegDirW = (P1 - P0).GetSafeNormal(); // along the line (U)
			FVector RightW = FVector::CrossProduct(View.GetViewDirection(), SegDirW).GetSafeNormal();
			if (RightW.IsNearlyZero()) RightW = FVector::CrossProduct(FVector::UpVector, SegDirW).GetSafeNormal();
			const FVector NormalW = FVector::CrossProduct(SegDirW, RightW).GetSafeNormal(); // VxU = N

			// Convert to float-precision for FDynamicMeshBuilder:
			const FVector3f TangentX = (FVector3f)SegDirW; // U
			const FVector3f TangentY = (FVector3f)RightW; // V
			const FVector3f TangentZ = (FVector3f)NormalW; // N

			// Your quad verts (world, double-precision)
			// v0,v1,v2,v3 are FVector (double) from your earlier code

			// AddVertex wants FVector3f/FVector2f/FVector3f/FVector3f/FVector3f/FColor
			const int32 i0 = MeshBuilder.AddVertex((FVector3f)v0, FVector2f(0, 0), TangentX, TangentY, TangentZ,
			                                       FColor(Color.ToFColor(true)));
			const int32 i1 = MeshBuilder.AddVertex((FVector3f)v1, FVector2f(1, 0), TangentX, TangentY, TangentZ,
			                                       FColor(Color.ToFColor(true)));
			const int32 i2 = MeshBuilder.AddVertex((FVector3f)v2, FVector2f(0, 1), TangentX, TangentY, TangentZ,
			                                       FColor(Color.ToFColor(true)));
			const int32 i3 = MeshBuilder.AddVertex((FVector3f)v3, FVector2f(1, 1), TangentX, TangentY, TangentZ,
			                                       FColor(Color.ToFColor(true)));

			MeshBuilder.AddTriangle(i0, i2, i1);
			MeshBuilder.AddTriangle(i1, i2, i3);
		}

		// Draw
		const FMatrix LocalToWorld = FMatrix::Identity;

		UMaterialInterface* BaseMat = GEngine->LevelColorationUnlitMaterial;

		// 2) Wrap with a per-draw colored proxy
		const FMaterialRenderProxy* BaseProxy = BaseMat->GetRenderProxy();
		auto* OneFrameColored = new FColoredMaterialRenderProxy(BaseProxy, Color, NAME_Color);
		Collector.RegisterOneFrameMaterialProxy(OneFrameColored);

		MeshBuilder.GetMesh(
			LocalToWorld,
			OneFrameColored,
			SDPG_Foreground, // or SDPG_Foreground if you want it drawn later (still depth-tested)
			/*bDisableBackfaceCulling*/ true,
			/*bReceivesDecals*/ false,
			ViewIndex,
			Collector
		);
	}
};

void UAxisLockGizmoComponent::SetAxisColor(const FLinearColor& InColor)
{
	AxisColor = InColor;
	if (AxisMID) // each component should have its own MID
	{
		AxisMID->SetVectorParameterValue(TEXT("LineColor"), AxisColor);
		// No need to rebuild the proxy for a MID param change, but harmless if you do:
		// MarkRenderStateDirty();
	}

	UE_LOG(LogTemp, Log, TEXT("Color = %s"), *AxisColor.ToString());
}

void UAxisLockGizmoComponent::OnRegister()
{
	Super::OnRegister();

	if (!AxisMaterial)
	{
		AxisMaterial = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/Materials/M_AxisRibbon.M_AxisRibbon"));
	}

	if (AxisMaterial && !AxisMID)
	{
		AxisMID = UMaterialInstanceDynamic::Create(AxisMaterial, this);
	}
	if (AxisMID)
	{
		AxisMID->SetVectorParameterValue(TEXT("LineColor"), AxisColor);
	}
}

FPrimitiveSceneProxy* UAxisLockGizmoComponent::CreateSceneProxy()
{
	return new FAxisLockSceneProxy(this);
}
