// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#include "UI/AxisLockGizmoComponent.h"
#include "PrimitiveSceneProxy.h"
#include "DynamicMeshBuilder.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 2
#include "Materials/MaterialInstanceDynamic.h"
#else
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 1
#include "Materials/MaterialRenderProxy.h"
#include "MaterialDomain.h"
#endif

//FAxisLockSceneProxy doesn't need to be visible to other classes.
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
		  , bIsWireframeView(Comp->bIsWireframeView)
	{
		const ERHIFeatureLevel::Type FL = GetScene().GetFeatureLevel();

		DrawMaterialFromComponent =
			Comp->AxisMID
				? (UMaterialInterface*)Comp->AxisMID
				: (Comp->AxisMaterial ? Comp->AxisMaterial : nullptr);

		FallbackMatPersp = LoadObject<UMaterialInterface>(
			nullptr, TEXT(
				"/BlenderEditorControlsPlugin/BlenderEditorControls/Materials/M_AxisRibbon_Translucent.M_AxisRibbon_Translucent"));
		FallbackMatOrtho = LoadObject<UMaterialInterface>(
			nullptr, TEXT(
				"/BlenderEditorControlsPlugin/BlenderEditorControls/Materials/M_AxisRibbon_Opaque.M_AxisRibbon_Opaque"));

		FMaterialRelevance R;
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		if (DrawMaterialFromComponent) { R |= DrawMaterialFromComponent->GetRelevance_Concurrent(FL); }
		if (FallbackMatPersp) { R |= FallbackMatPersp->GetRelevance_Concurrent(FL); }
		if (FallbackMatOrtho) { R |= FallbackMatOrtho->GetRelevance_Concurrent(FL); }
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
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
			if ((VisibilityMap & (1 << ViewIndex)) == 0)
			{
				continue;
			}
			const FSceneView* View = Views[ViewIndex];

			/* Choose different material for perspective vs orthographic.
				Using same material results in disconnected line segments in
				orthographic view due to grid lines. 
			*/
			UMaterialInterface* ActiveMat =
				DrawMaterialFromComponent
					? DrawMaterialFromComponent
					: (bIsWireframeView
						   ? FallbackMatOrtho
						   : FallbackMatPersp);

			if (!ActiveMat) { ActiveMat = UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface); }

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
	bool bIsWireframeView;
	FMaterialRelevance MaterialRelevance;
	UMaterialInterface* DrawMaterialFromComponent = nullptr;
	UMaterialInterface* FallbackMatPersp = nullptr;
	UMaterialInterface* FallbackMatOrtho = nullptr;

	static float WorldPerPixelAt(const FSceneView& View, const FVector& WorldPos)
	{
		const float ViewWidthPx = float(View.UnscaledViewRect.Width());

#if ENGINE_MAJOR_VERSION >= 6 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
		const FMatrix& ProjMatrix = View.ViewMatrices.GetViewToClip();
#else
		const FMatrix& ProjMatrix = View.ViewMatrices.GetProjectionMatrix();
#endif

		if (View.IsPerspectiveProjection())
		{
			// Calculate depth in View Space
#if ENGINE_MAJOR_VERSION >= 6 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
			const FVector ViewPos = View.ViewMatrices.GetWorldToView().TransformPosition(WorldPos);
#else
			const FVector ViewPos = View.ViewMatrices.GetViewMatrix().TransformPosition(WorldPos);
#endif
			const float Depth = FMath::Abs(ViewPos.Z);

			// M[0][0] is the scaling factor for the X-axis in the Projection Matrix.
			// Relationship: 1.0 (NDC Edge) = (ViewX * M00) / Depth
			// Therefore, ViewX (Half Width at Depth) = Depth / M00
			// Total World Width at Depth = (2 * Depth) / M00
			const float M00 = ProjMatrix.M[0][0];

			// Protect against division by zero just in case
			const float ScreenWidthWorld = (M00 > 0.0f) ? (2.f * Depth) / M00 : 1.0f;

			return ScreenWidthWorld / ViewWidthPx;
		}
		else // Orthographic
		{
			const float OrthoWidthWorld = (ProjMatrix.M[0][0] == 0)
				                              ? 1.f
				                              : 2.f / FMath::Abs(ProjMatrix.M[0][0]); // 2.0 / ScaleX gives world width

			return OrthoWidthWorld / ViewWidthPx;
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
		const FVector ViewForward = View.GetViewDirection();

		for (int32 i = 0; i < N; ++i)
		{
			const FVector P0 = A + Dir * float(i);
			const FVector P1 = A + Dir * float(i + 1);

			// Per-vertex world-per-pixel
			const float wpp0 = WorldPerPixelAt(View, P0);
			const float wpp1 = WorldPerPixelAt(View, P1);
			const float halfW0 = 0.5f * InThicknessPx * wpp0;
			const float halfW1 = 0.5f * InThicknessPx * wpp1;

			// Segment direction (world)
			const FVector SegDir = (P1 - P0).GetSafeNormal();

			FVector V0, V1;

			if (View.IsPerspectiveProjection())
			{
				const FVector CamPos = View.ViewMatrices.GetViewOrigin();
				V0 = (P0 - CamPos).GetSafeNormal();
				V1 = (P1 - CamPos).GetSafeNormal();
			}
			else
			{
				V0 = ViewForward;
				V1 = ViewForward;
			}

			FVector Right0 = FVector::CrossProduct(V0, SegDir).GetSafeNormal();
			FVector Right1 = FVector::CrossProduct(V1, SegDir).GetSafeNormal();

			// Robust fallback if degenerate
			if (Right0.IsNearlyZero())
			{
				Right0 = FVector::CrossProduct(FVector::UpVector, SegDir).GetSafeNormal();
			}
			if (Right1.IsNearlyZero())
			{
				Right1 = FVector::CrossProduct(FVector::UpVector, SegDir).GetSafeNormal();
			}
			// Build the quad with per-end widths & rights
			const FVector v0 = P0 - Right0 * halfW0;
			const FVector v1 = P0 + Right0 * halfW0;
			const FVector v2 = P1 - Right1 * halfW1;
			const FVector v3 = P1 + Right1 * halfW1;

			// Tangents per-end (optional but good)
			const FVector3f TangentX0 = FVector3f(SegDir);
			const FVector3f TangentY0 = FVector3f(Right0);
			const FVector3f Normal0 = FVector3f(FVector::CrossProduct(SegDir, Right0).GetSafeNormal());

			const FVector3f TangentX1 = FVector3f(SegDir);
			const FVector3f TangentY1 = FVector3f(Right1);
			const FVector3f Normal1 = FVector3f(FVector::CrossProduct(SegDir, Right1).GetSafeNormal());

			const int32 i0 = MeshBuilder.AddVertex(FVector3f(v0), FVector2f(0, 0), TangentX0, TangentY0, Normal0,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i1 = MeshBuilder.AddVertex(FVector3f(v1), FVector2f(1, 0), TangentX0, TangentY0, Normal0,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i2 = MeshBuilder.AddVertex(FVector3f(v2), FVector2f(0, 1), TangentX1, TangentY1, Normal1,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i3 = MeshBuilder.AddVertex(FVector3f(v3), FVector2f(1, 1), TangentX1, TangentY1, Normal1,
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

FBoxSphereBounds UAxisLockGizmoComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVector SafeDir = AxisDir.IsNearlyZero() ? FVector::ForwardVector : AxisDir.GetSafeNormal();
	const float SafeLen = FMath::Clamp(LineLength, 1.f, 1e7f); // cap to avoid overflow
	const FVector A = Origin - SafeDir * SafeLen;
	const FVector B = Origin + SafeDir * SafeLen;
	const FBox Box(A, B);
	return FBoxSphereBounds(Box).TransformBy(LocalToWorld);
}
