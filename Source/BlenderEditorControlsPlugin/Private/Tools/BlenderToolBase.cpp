#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Selection.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
	FBlenderToolBase::FBlenderToolBase(ETransformMode InMode, ETransformAxis InAxis, const FString& InDisplayName)
		: Mode(InMode), Axis(InAxis), DisplayName(InDisplayName)
	{
	}

	FBlenderToolBase::~FBlenderToolBase()
	{
	}

	void FBlenderToolBase::UpdateDragPlane()
	{
		if (!Pivot.IsValid() || !SceneView)
		{
			return;
		}

		FlushDrawnAxisLines();

		FVector PlaneNormal = FVector::ZeroVector;

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		SceneView = ViewportClient->CalcSceneView(&ViewFamily);

		if (!bIsAxisLockActive || EnumHasAllFlags(Axis, ETransformAxis::All))
		{
			PlaneNormal = -SceneView->ViewRotation.Vector();
		}
		else if (EnumHasAllFlags(Axis, ETransformAxis::X | ETransformAxis::Y))
		{
			DrawAxisLine(ETransformAxis::X);
			DrawAxisLine(ETransformAxis::Y);
			PlaneNormal = GetAxisVector(ETransformAxis::Z);
		}
		else if (EnumHasAllFlags(Axis, ETransformAxis::X | ETransformAxis::Z))
		{
			DrawAxisLine(ETransformAxis::X);
			DrawAxisLine(ETransformAxis::Z);
			PlaneNormal = GetAxisVector(ETransformAxis::Y);
		}
		else if (EnumHasAllFlags(Axis, ETransformAxis::Y | ETransformAxis::Z))
		{
			DrawAxisLine(ETransformAxis::Y);
			DrawAxisLine(ETransformAxis::Z);
			PlaneNormal = GetAxisVector(ETransformAxis::X);
		}
		else if (EnumHasAnyFlags(Axis, ETransformAxis::X))
		{
			FVector AxisDir = GetAxisVector(ETransformAxis::X);
			FVector ViewDir = SceneView->GetViewDirection().GetSafeNormal();
			DrawAxisLine(ETransformAxis::X);
			PlaneNormal = FVector::CrossProduct(AxisDir, ViewDir).GetSafeNormal();
		}
		else if (EnumHasAnyFlags(Axis, ETransformAxis::Y))
		{
			FVector AxisDir = GetAxisVector(ETransformAxis::Y);
			FVector ViewDir = SceneView->GetViewDirection().GetSafeNormal();
			DrawAxisLine(ETransformAxis::Y);
			PlaneNormal = FVector::CrossProduct(AxisDir, ViewDir).GetSafeNormal();
		}
		else if (EnumHasAnyFlags(Axis, ETransformAxis::Z))
		{
			FVector AxisDir = GetAxisVector(ETransformAxis::Z);
			FVector ViewDir = SceneView->GetViewDirection().GetSafeNormal();
			DrawAxisLine(ETransformAxis::Z);
			PlaneNormal = FVector::CrossProduct(AxisDir, ViewDir).GetSafeNormal();
		}
		else
		{
			PlaneNormal = -SceneView->ViewRotation.Vector();
		}

		const FVector PlaneOrigin = Pivot->GetStartTransform().GetLocation();
		DragPlane = FPlane(PlaneOrigin, PlaneNormal);

		// OnAxisLockRecalculated(CurrentViewportMousePos);
	}

	void FBlenderToolBase::FlushDrawnAxisLines() const
	{
		if (CachedBatcher.IsValid())
		{
			CachedBatcher->Flush(); // Removes all current batched lines
			CachedBatcher->MarkRenderStateDirty();
		}
	}

	FLinearColor FBlenderToolBase::GetAxisColor(ETransformAxis InAxis)
	{
		switch (InAxis)
		{
		case ETransformAxis::X:
			return FLinearColor::Red;
		case ETransformAxis::Y:
			return FLinearColor::Green;
		case ETransformAxis::Z:
			return FLinearColor::Blue;
		default:
			return FLinearColor::White;
		}
	}

	void FBlenderToolBase::DrawAxisLine(ETransformAxis InAxis) const
	{
		if (CachedBatcher.IsValid())
		{
			const FVector AxisDir = GetAxisVector(InAxis);
			const FVector Origin = Pivot->GetStartTransform().GetLocation();
			const float LineLength = 10000;

			const FVector LineStart = Origin - AxisDir * LineLength;
			const FVector LineEnd = Origin + AxisDir * LineLength;

			const float Lifetime = 0.f; // persistent
			const FLinearColor Color = GetAxisColor(InAxis);
			const float Thickness = CalculateDynamicThickness(Origin);

			CachedBatcher->DrawLine(LineStart, LineEnd, Color, SDPG_World, Thickness, Lifetime);
			CachedBatcher->MarkRenderStateDirty();
		}
	}

	float FBlenderToolBase::CalculateDynamicThickness(const FVector& Origin) const
	{
		if (!SceneView)
		{
			return FallbackLineThickness;
		}

		const FVector CameraLocation = SceneView->ViewLocation;
		const float Distance = FVector::Dist(CameraLocation, Origin);

		// Linear scaling: thickness grows with distance
		float Scaled = FallbackLineThickness * (Distance / ReferenceDistance);

		// Clamp to reasonable bounds
		return FMath::Clamp(Scaled, MinLineThickness, MaxLineThickness);
	}

	FVector FBlenderToolBase::GetAxisVector(ETransformAxis InAxis) const
	{
		FVector AxisVector =
			(InAxis == ETransformAxis::X)
				? FVector::XAxisVector
				: (InAxis == ETransformAxis::Y)
				? FVector::YAxisVector
				: (InAxis == ETransformAxis::Z)
				? FVector::ZAxisVector
				: FVector::ZeroVector;

		if (bIsUsingLocalSpace)
		{
			AxisVector = Pivot->GetPivot().TransformVectorNoScale(AxisVector);
		}
		return AxisVector.GetSafeNormal();
	}

	void FBlenderToolBase::OnBegin()
	{
		ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (!ViewportClient)
		{
			return;
		}

		if (!GEditor)
		{
			return;
		}
		CachedSelectionColor = GEditor->GetSelectionOutlineColor();
		GEditor->SetSelectionOutlineColor(FLinearColor::White);
		bLocalSpaceDefault = (GLevelEditorModeTools().GetCoordSystem() == COORD_Local);

		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			CachedBatcher = World->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent);
		}

		CaptureSelection();
		Pivot = MakeShared<FSharedPivot>(SelectedActors);
		InitialWidgetMode = GLevelEditorModeTools().GetWidgetMode();
		GLevelEditorModeTools().SetWidgetMode(UE::Widget::WM_None);

		// Start transaction for undo
		ParentTxn = MakeUnique<FScopedTransaction>(FText::FromString(DisplayName));
		for (auto Actor : SelectedActors)
		{
			Actor->Modify();
		}
		Pivot->GetTransformProxy()->BeginTransformEditSequence();

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		if (!SceneView)
		{
			return;
		}

		Viewport = ViewportClient->Viewport;
		if (!Viewport)
		{
			return;
		}

		FIntPoint MousePosInt;
		Viewport->GetMousePos(MousePosInt);
		FVector2D MousePos = FVector2D(MousePosInt);

		FVector WorldOrigin, WorldDirection;
		SceneView->DeprojectFVector2D(MousePos, WorldOrigin, WorldDirection);

		UpdateDragPlane();

		PreviousPlaneIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                         WorldOrigin + (WorldDirection *
			                                                         BIG_NUMBER),
		                                                         DragPlane);
		NewPivotPosition = Pivot->GetStartTransform().GetLocation();
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		CurrentViewportMousePos = CurrentViewportMousePosition;
		if (!Viewport || !ViewportClient || !Pivot)
		{
			return;
		}
		const FIntPoint CurrentMousePosInt = FIntPoint(CurrentViewportMousePosition.X, CurrentViewportMousePosition.Y);
		const FVector2D CurrentMousePos = FVector2D(CurrentMousePosInt);

		FVector WorldOrigin, WorldDirection;
		FSceneViewFamilyContext TempViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		SceneView = ViewportClient->CalcSceneView(&TempViewFamily);
		if (SceneView)
		{
			SceneView->DeprojectFVector2D(CurrentMousePos, WorldOrigin, WorldDirection);
		}

		CurrentPlaneIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                        WorldOrigin + (WorldDirection * BIG_NUMBER), DragPlane);
	}

	void FBlenderToolBase::OnEnd(bool bApply)
	{
		if (!GEditor || !Pivot)
		{
			return;
		}

		GEditor->SetSelectionOutlineColor(CachedSelectionColor);
		SelectedActors.Empty();
		Pivot->GetTransformProxy()->EndTransformEditSequence();

		if (FEditorModeTools* ModeTools = &GLevelEditorModeTools())
		{
			ModeTools->SetWidgetMode(InitialWidgetMode);
		}

		if (GEditor)
		{
			FVector NewPivot = bApply ? Pivot->GetPivot().GetLocation() : Pivot->GetStartTransform().GetLocation();
			GEditor->SetPivot(NewPivot, false, true, false);
		}

		FlushDrawnAxisLines();
	}

	void FBlenderToolBase::StartNewLock(const ETransformAxis NewAxis)
	{
		Axis = NewAxis;
		bIsAxisLockActive = true;
		bIsUsingLocalSpace = bLocalSpaceDefault;
	}

	void FBlenderToolBase::HandleAxisLock(const ETransformAxis AxisPressed)
	{
		if (!bIsAxisLockActive || Axis != AxisPressed)
		{
			StartNewLock(AxisPressed);
		}
		else
		{
			// Are we currently in the default space? (This was set on the first press)
			if (bIsUsingLocalSpace == bLocalSpaceDefault)
			{
				// Second Press: We were in the default space, so switch to the alternate one.
				bIsUsingLocalSpace = !bLocalSpaceDefault;
			}
			else
			{
				// Third Press: We were in the alternate space, so cycle is complete. Unlock.
				bIsAxisLockActive = false;
				Axis = ETransformAxis::All;
			}
		}

		UpdateDragPlane();
		OnActive(CurrentViewportMousePos);
	}

	void FBlenderToolBase::Accept()
	{
		OnEnd(/*bApply=*/true);

		if (ParentTxn)
		{
			ParentTxn.Reset();
		}
	}

	void FBlenderToolBase::Cancel()
	{
		if (!GEditor || !Pivot)
		{
			return;
		}

		OnEnd(/*bApply=*/false);
		GEditor->SetSelectionOutlineColor(CachedSelectionColor);

		// Reset pivot to start location
		Pivot->GetTransformProxy()->SetTransform(Pivot->GetStartTransform());

		// Abort undo-tracking
		if (ParentTxn)
		{
			ParentTxn->Cancel();
			ParentTxn.Reset();
		}

		SelectedActors.Empty();
	}

	void FBlenderToolBase::OnAxisLockRecalculated(const FVector2D& CurrentViewportMousePosition)
	{
		OnActive(CurrentViewportMousePosition);
	}

	void FBlenderToolBase::ApplyNumeric(float Value)
	{
		// Base implementation does nothing
	}

	void FBlenderToolBase::CaptureSelection()
	{
		SelectedActors.Empty();

		if (GEditor)
		{
			USelection* ActorSelection = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*ActorSelection); It; ++It)
			{
				if (AActor* Actor = Cast<AActor>(*It))
				{
					SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
				}
			}
		}
	}

	FVector FBlenderToolBase::GetSnapOffset(const FVector OffsetFromStart)
	{
		if (!GEditor)
		{
			return FVector::ZeroVector;
		}

		float GridSize = GEditor->GetGridSize();
		FVector SnapOffset = OffsetFromStart / GridSize;
		SnapOffset = BlenderControls::Math::RoundVectorToInt(SnapOffset);
		SnapOffset *= GridSize;

		return SnapOffset;
	}
} // namespace BlenderControls
