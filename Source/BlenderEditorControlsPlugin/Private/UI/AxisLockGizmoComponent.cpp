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
		  , Color(Comp->AxisColor)
		  , ThicknessPx(Comp->ThicknessPx)
	{
		const ERHIFeatureLevel::Type FL = GetScene().GetFeatureLevel();

		DrawMaterialFromComponent =
			Comp->AxisMID
				? (UMaterialInterface*)Comp->AxisMID
				: (Comp->AxisMaterial ? Comp->AxisMaterial : nullptr);

		FallbackMatPersp = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/BlenderEditorControlsPlugin/Materials/M_AxisRibbon_Translucent.M_AxisRibbon_Translucent"));
		FallbackMatOrtho = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/BlenderEditorControlsPlugin/Materials/M_AxisRibbon_Opaque.M_AxisRibbon_Opaque"));

		FMaterialRelevance R;
		if (DrawMaterialFromComponent) { R |= DrawMaterialFromComponent->GetRelevance_Concurrent(FL); }
		if (FallbackMatPersp) { R |= FallbackMatPersp->GetRelevance_Concurrent(FL); }
		if (FallbackMatOrtho) { R |= FallbackMatOrtho->GetRelevance_Concurrent(FL); }
		MaterialRelevance = R;

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

			/* Choose different material for perspective vs orthographic.
			Using same material results in wierd disconnected line segments in
			orthographic view.
			*/
			UMaterialInterface* ActiveMat =
				DrawMaterialFromComponent
					? DrawMaterialFromComponent
					: (View->IsPerspectiveProjection()
						   ? FallbackMatPersp
						   : FallbackMatOrtho);

			if (!ActiveMat) { ActiveMat = UMaterial::GetDefaultMaterial(MD_Surface); }

			const FMaterialRenderProxy* BaseProxy = ActiveMat->GetRenderProxy();
			auto* OneFrameColored = new FColoredMaterialRenderProxy(BaseProxy, Color, FName("LineColor"));
			Collector.RegisterOneFrameMaterialProxy(OneFrameColored);

			BuildRibbonQuads(*View, ViewIndex, A, B, ThicknessPx, Color, 32, Collector,
			                 /*PDI*/ Collector.GetPDI(ViewIndex),
			                 /*Proxy*/ OneFrameColored);
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
	UMaterialInterface* DrawMaterial = nullptr;
	FMaterialRelevance MaterialRelevance;
	UMaterialInterface* DrawMaterialFromComponent = nullptr;
	UMaterialInterface* FallbackMatPersp = nullptr;
	UMaterialInterface* FallbackMatOrtho = nullptr;

	static float WorldPerPixelAt(const FSceneView& View, const FVector& WorldPos)
	{
		if (View.IsPerspectiveProjection())
		{
			const FVector ViewPos = View.ViewMatrices.GetViewMatrix().TransformPosition(WorldPos);
			const float depth = FMath::Abs(ViewPos.Z);
			const float HFovDeg = View.FOV;
			const float HFovRad = FMath::DegreesToRadians(HFovDeg);
			const float viewWidthPx = float(View.UnscaledViewRect.Width());
			const float screenWidthWorld = 2.f * depth * FMath::Tan(HFovRad * 0.5f);
			return screenWidthWorld / viewWidthPx;
		}
		else
		{
			const float orthoWidthWorld = View.ViewMatrices.GetProjectionMatrix().M[0][0] == 0
				                              ? 1.f
				                              : (View.ViewMatrices.GetInvProjectionMatrix().TransformFVector4(
						                              FVector4(1, 0, 1, 1)).X
					                              - View.ViewMatrices.GetInvProjectionMatrix().TransformFVector4(
						                              FVector4(-1, 0, 1, 1)).X) * 0.5f;

			const float viewWidthPx = float(View.UnscaledViewRect.Width());
			return orthoWidthWorld / viewWidthPx;
		}
	}

	void BuildRibbonQuads(
		const FSceneView& View,
		int32 ViewIndex,
		const FVector& A,
		const FVector& B,
		float InThicknessPx,
		const FLinearColor& InColor,
		int Segments,
		FMeshElementCollector& Collector,
		FPrimitiveDrawInterface* PDI,
		const FMaterialRenderProxy* Proxy) const
	{
		const float midWPP = WorldPerPixelAt(View, (A + B) * 0.5f);
		const float approxPxLen = (B - A).Length() / FMath::Max(1e-3f, midWPP);
		const int32 N = FMath::Clamp(int32(approxPxLen / 20.f), 8, 256);
		const FVector Dir = (B - A) / float(N);

		FDynamicMeshBuilder MeshBuilder(View.GetFeatureLevel());

		for (int32 i = 0; i < N; ++i)
		{
			const FVector P0 = A + Dir * float(i);
			const FVector P1 = A + Dir * float(i + 1);

			const FVector CamPos = View.ViewMatrices.GetViewOrigin();

			// Per-vertex world-per-pixel
			const float wpp0 = WorldPerPixelAt(View, P0);
			const float wpp1 = WorldPerPixelAt(View, P1);
			const float halfW0 = 0.5f * InThicknessPx * wpp0;
			const float halfW1 = 0.5f * InThicknessPx * wpp1;

			// Segment direction (world)
			const FVector SegDir = (P1 - P0).GetSafeNormal();

			// Per-end view rays and “billboard right” vectors
			const FVector V0 = (P0 - CamPos).GetSafeNormal();
			const FVector V1 = (P1 - CamPos).GetSafeNormal();

			FVector Right0 = FVector::CrossProduct(V0, SegDir).GetSafeNormal();
			FVector Right1 = FVector::CrossProduct(V1, SegDir).GetSafeNormal();

			// Robust fallback if degenerate
			if (Right0.IsNearlyZero()) Right0 = FVector::CrossProduct(FVector::UpVector, SegDir).GetSafeNormal();
			if (Right1.IsNearlyZero()) Right1 = FVector::CrossProduct(FVector::UpVector, SegDir).GetSafeNormal();

			// Build the quad with per-end widths & rights
			const FVector v0 = P0 - Right0 * halfW0;
			const FVector v1 = P0 + Right0 * halfW0;
			const FVector v2 = P1 - Right1 * halfW1;
			const FVector v3 = P1 + Right1 * halfW1;

			// Tangents per-end (optional but good)
			const FVector3f TangentX0 = (FVector3f)SegDir;
			const FVector3f TangentY0 = (FVector3f)Right0;
			const FVector3f Normal0 = (FVector3f)FVector::CrossProduct(SegDir, Right0).GetSafeNormal();

			const FVector3f TangentX1 = (FVector3f)SegDir;
			const FVector3f TangentY1 = (FVector3f)Right1;
			const FVector3f Normal1 = (FVector3f)FVector::CrossProduct(SegDir, Right1).GetSafeNormal();

			const int32 i0 = MeshBuilder.AddVertex((FVector3f)v0, FVector2f(0, 0), TangentX0, TangentY0, Normal0,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i1 = MeshBuilder.AddVertex((FVector3f)v1, FVector2f(1, 0), TangentX0, TangentY0, Normal0,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i2 = MeshBuilder.AddVertex((FVector3f)v2, FVector2f(0, 1), TangentX1, TangentY1, Normal1,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i3 = MeshBuilder.AddVertex((FVector3f)v3, FVector2f(1, 1), TangentX1, TangentY1, Normal1,
			                                       FColor(InColor.ToFColor(true)));

			MeshBuilder.AddTriangle(i0, i2, i1);
			MeshBuilder.AddTriangle(i1, i2, i3);
		}

		const FMatrix LocalToWorldMatrix = FMatrix::Identity;

		MeshBuilder.GetMesh(
			LocalToWorldMatrix,
			Proxy,
			SDPG_Foreground,
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
	if (AxisMID)
	{
		AxisMID->SetVectorParameterValue(TEXT("LineColor"), AxisColor);
	}
}

void UAxisLockGizmoComponent::OnRegister()
{
	Super::OnRegister();
}

FPrimitiveSceneProxy* UAxisLockGizmoComponent::CreateSceneProxy()
{
	return new FAxisLockSceneProxy(this);
}
