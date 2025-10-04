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
		// Tesselate AB so thickness adapts if depth varies along the line.
		const int32 N = FMath::Max(1, Segments);
		const FVector Dir = (B - A) / float(N);

		UMaterialInterface* Mat = UMaterial::GetDefaultMaterial(MD_Surface);

		FDynamicMeshBuilder MeshBuilder(View.GetFeatureLevel());

		for (int32 i = 0; i < N; ++i)
		{
			const FVector P0 = A + Dir * float(i);
			const FVector P1 = A + Dir * float(i + 1);

			const FVector CamForward = View.GetViewDirection();
			const FVector SegDir = (P1 - P0).GetSafeNormal();
			FVector Right = FVector::CrossProduct(CamForward, SegDir).GetSafeNormal();
			if (Right.IsNearlyZero()) Right = FVector::CrossProduct(FVector::UpVector, SegDir).GetSafeNormal();

			const float wpp0 = WorldPerPixelAt(View, P0);
			const float wpp1 = WorldPerPixelAt(View, P1);
			const float halfW0 = 0.5f * InThicknessPx * wpp0;
			const float halfW1 = 0.5f * InThicknessPx * wpp1;

			const FVector v0 = P0 - Right * halfW0;
			const FVector v1 = P0 + Right * halfW0;
			const FVector v2 = P1 - Right * halfW1;
			const FVector v3 = P1 + Right * halfW1;

			const FVector SegDirW = (P1 - P0).GetSafeNormal();
			FVector RightW = FVector::CrossProduct(View.GetViewDirection(), SegDirW).GetSafeNormal();
			if (RightW.IsNearlyZero()) RightW = FVector::CrossProduct(FVector::UpVector, SegDirW).GetSafeNormal();
			const FVector NormalW = FVector::CrossProduct(SegDirW, RightW).GetSafeNormal();

			const FVector3f TangentX = (FVector3f)SegDirW; // U
			const FVector3f TangentY = (FVector3f)RightW; // V
			const FVector3f TangentZ = (FVector3f)NormalW; // N

			const int32 i0 = MeshBuilder.AddVertex((FVector3f)v0, FVector2f(0, 0), TangentX, TangentY, TangentZ,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i1 = MeshBuilder.AddVertex((FVector3f)v1, FVector2f(1, 0), TangentX, TangentY, TangentZ,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i2 = MeshBuilder.AddVertex((FVector3f)v2, FVector2f(0, 1), TangentX, TangentY, TangentZ,
			                                       FColor(InColor.ToFColor(true)));
			const int32 i3 = MeshBuilder.AddVertex((FVector3f)v3, FVector2f(1, 1), TangentX, TangentY, TangentZ,
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
