#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
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
		if (!Group.IsValid() || !SceneView)
		{
			return;
		}

		const FVector PlaneOrigin = Group->GetPivot().GetLocation();
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
			UE_LOG(LogTemp, Log, TEXT("No axis locked - Free Transform"));
		}
		else if (EnumHasAllFlags(Axis, ETransformAxis::X | ETransformAxis::Y))
		{
			PlaneNormal = GetAxisVector(ETransformAxis::Z);
		}
		else if (EnumHasAllFlags(Axis, ETransformAxis::X | ETransformAxis::Z))
		{
			PlaneNormal = GetAxisVector(ETransformAxis::Y);
		}
		else if (EnumHasAllFlags(Axis, ETransformAxis::Y | ETransformAxis::Z))
		{
			PlaneNormal = GetAxisVector(ETransformAxis::X);
		}
		else if (EnumHasAnyFlags(Axis, ETransformAxis::X))
		{
			FVector AxisDir = GetAxisVector(ETransformAxis::X);
			FVector ViewDir = SceneView->GetViewDirection().GetSafeNormal();
			PlaneNormal = FVector::CrossProduct(AxisDir, ViewDir).GetSafeNormal();
		}
		else if (EnumHasAnyFlags(Axis, ETransformAxis::Y))
		{
			FVector AxisDir = GetAxisVector(ETransformAxis::Y);
			FVector ViewDir = SceneView->GetViewDirection().GetSafeNormal();
			PlaneNormal = FVector::CrossProduct(AxisDir, ViewDir).GetSafeNormal();
		}
		else if (EnumHasAnyFlags(Axis, ETransformAxis::Z))
		{
			FVector AxisDir = GetAxisVector(ETransformAxis::Z);
			FVector ViewDir = SceneView->GetViewDirection().GetSafeNormal();
			PlaneNormal = FVector::CrossProduct(AxisDir, ViewDir).GetSafeNormal();
		}
		else
		{
			PlaneNormal = -SceneView->ViewRotation.Vector();
		}

		DragPlane = FPlane(PlaneOrigin, PlaneNormal);
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
			AxisVector = Group->GetPivot().TransformVectorNoScale(AxisVector);
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

		CaptureSelection();
		Group = MakeShared<FSharedPivot>(SelectedActors);
		InitialWidgetMode = GLevelEditorModeTools().GetWidgetMode();
		GLevelEditorModeTools().SetWidgetMode(UE::Widget::WM_None);

		// Start transaction for undo
		ParentTxn = MakeUnique<FScopedTransaction>(FText::FromString(DisplayName));
		for (auto Actor : SelectedActors)
		{
			Actor->Modify();
		}
		Group->GetTransformProxy()->BeginTransformEditSequence();

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

		PreviousIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                         WorldOrigin + (WorldDirection *
			                                                         BIG_NUMBER),
		                                                         DragPlane);
		FloatingOrigin = Group->GetStartTransform().GetLocation();
		GrabStartIntersectionPoint = PreviousIntersectionPoint;
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		if (!Viewport || !ViewportClient || !Group)
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

		CurrentIntersectionPoint = FMath::LinePlaneIntersection(WorldOrigin,
		                                                        WorldOrigin + (WorldDirection * BIG_NUMBER), DragPlane);

		// UE_LOG(LogTemp, Log, TEXT("Previous Intersection: X=%.2f Y=%.2f Z=%.2f"), PreviousIntersectionPoint.X,
		//        PreviousIntersectionPoint.Y, PreviousIntersectionPoint.Z);
		// UE_LOG(LogTemp, Log, TEXT("Current Intersection: X=%.2f Y=%.2f Z=%.2f"), CurrentIntersectionPoint.X,
		//        CurrentIntersectionPoint.Y, CurrentIntersectionPoint.Z);
	}

	void FBlenderToolBase::OnEnd(bool bApply)
	{
		if (!GEditor || !Group)
		{
			return;
		}

		GEditor->SetSelectionOutlineColor(CachedSelectionColor);
		SelectedActors.Empty();
		Group->GetTransformProxy()->EndTransformEditSequence();

		if (FEditorModeTools* ModeTools = &GLevelEditorModeTools())
		{
			ModeTools->SetWidgetMode(InitialWidgetMode);
		}

		if (GEditor)
		{
			FVector NewPivot = bApply ? Group->GetPivot().GetLocation() : Group->GetStartTransform().GetLocation();
			GEditor->SetPivot(NewPivot, false, true, false);
		}
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
		UE_LOG(LogTemp, Log, TEXT("Current Axis: %d"), static_cast<uint8>(Axis));
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
		if (!GEditor || !Group)
		{
			return;
		}

		OnEnd(/*bApply=*/false);
		GEditor->SetSelectionOutlineColor(CachedSelectionColor);

		// Reset pivot to start location
		Group->GetTransformProxy()->SetTransform(Group->GetStartTransform());

		// Abort undo-tracking
		if (ParentTxn)
		{
			ParentTxn->Cancel();
			ParentTxn.Reset();
		}

		SelectedActors.Empty();
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
