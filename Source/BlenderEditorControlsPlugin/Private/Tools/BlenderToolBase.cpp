#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Selection.h"
#include "Utils/BlenderMathHelpers.h"
#include "DrawDebugHelpers.h"
#include "Misc/OutputDeviceDebug.h"

namespace BlenderControls
{
	FBlenderToolBase::FBlenderToolBase(ETransformMode InMode, EAxisLock InAxis, const FString& InDisplayName)
		: Mode(InMode), LockedAxis(InAxis), DisplayName(InDisplayName)
	{
	}

	FBlenderToolBase::~FBlenderToolBase()
	{
	}

	void FBlenderToolBase::UpdateAxisLock()
	{
		if (!ViewportClient)
		{
			return;
		}

		GrabContext.Pivot = Pivot->GetStartTransform().GetLocation();

		FVector ViewDirection;
		if (ViewportClient->IsPerspective())
		{
			ViewDirection = ViewportClient->GetViewRotation().Vector();
		}
		else
		{
			switch (ViewportClient->ViewportType)
			{
			case LVT_OrthoXY:
				ViewDirection = FVector::UpVector;
				break; // Top view
			case LVT_OrthoXZ:
				ViewDirection = FVector::RightVector;
				break; // Front view
			case LVT_OrthoYZ:
				ViewDirection = FVector::ForwardVector;
				break; // Side view
			case LVT_OrthoNegativeXY:
				ViewDirection = -FVector::UpVector;
				break;
			case LVT_OrthoNegativeXZ:
				ViewDirection = -FVector::RightVector;
				break;
			case LVT_OrthoNegativeYZ:
				ViewDirection = -FVector::ForwardVector;
				break;
			default:
				ViewDirection = FVector::ForwardVector;
				break;
			}
		}

		FlushDrawnAxisLines();
		switch (LockedAxis)
		{
		case EAxisLock::All:
			GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
			GrabContext.HelperPlaneN = -ViewDirection;
			break;

		case EAxisLock::X:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = FVector::XAxisVector;
			DrawAxisLine(EAxisLock::X);
			break;

		case EAxisLock::Y:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = FVector::YAxisVector;
			DrawAxisLine(EAxisLock::Y);
			break;

		case EAxisLock::Z:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = FVector::ZAxisVector;
			DrawAxisLine(EAxisLock::Z);
			break;

		case EAxisLock::YZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperPlaneN = FVector::XAxisVector;
			DrawAxisLine(EAxisLock::Y);
			DrawAxisLine(EAxisLock::Z);
			break;

		case EAxisLock::XZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperPlaneN = FVector::YAxisVector;
			DrawAxisLine(EAxisLock::X);
			DrawAxisLine(EAxisLock::Z);
			break;

		case EAxisLock::XY:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperPlaneN = FVector::ZAxisVector;
			DrawAxisLine(EAxisLock::X);
			DrawAxisLine(EAxisLock::Y);
			break;
		}
		
		//Update object pos to be on new plane
		OnActive(CurrentViewportMousePos);
	}

	void FBlenderToolBase::FlushDrawnAxisLines() const
	{
		if (CachedBatcher.IsValid())
		{
			CachedBatcher->Flush(); // Removes all current batched lines
			CachedBatcher->MarkRenderStateDirty();
		}
	}

	FLinearColor FBlenderToolBase::GetAxisColor(EAxisLock InAxis)
	{
		switch (InAxis)
		{
		case EAxisLock::X:
			return FLinearColor::Red;
		case EAxisLock::Y:
			return FLinearColor::Green;
		case EAxisLock::Z:
			return FLinearColor::Blue;
		default:
			return FLinearColor::White;
		}
	}

	void FBlenderToolBase::DrawAxisLine(EAxisLock InAxis) const
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

	FVector FBlenderToolBase::GetAxisVector(EAxisLock InAxis) const
	{
		FVector AxisVector =
			(InAxis == EAxisLock::X)
				? FVector::XAxisVector
				: (InAxis == EAxisLock::Y)
				? FVector::YAxisVector
				: (InAxis == EAxisLock::Z)
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

		UE_LOG(LogTemp, Log, TEXT("OnBegin: CurrentMousePos X=%.2f Y=%.2f"), MousePos.X, MousePos.Y);

		SceneView->DeprojectFVector2D(MousePos, CurrentRayOrigin, CurrentRayDirection);

		GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
		GrabContext.HelperPlaneN = -ViewportClient->GetViewRotation().Vector();
		GrabContext.PivotStartPos = Pivot->GetStartTransform().GetLocation();

		//GrabContext.StartHit = BlenderControls::Math::IntersectHelper(GrabContext, WorldOrigin, WorldDirection);
		GrabContext.TotalDelta = FVector::ZeroVector;
		GrabContext.DeltaAnchor = FVector::ZeroVector;
		
		GrabContext.MousePosA = MousePos;
		GrabContext.MousePosB = GrabContext.MousePosA + FVector2D(1, 0);

		FVector MousePosBOrigin, MousePosBDirection;
		SceneView->DeprojectFVector2D(GrabContext.MousePosB, MousePosBOrigin, MousePosBDirection);
		FVector MouseIntersectionA = BlenderControls::Math::IntersectHelper(GrabContext, CurrentRayOrigin, CurrentRayDirection);
		FVector MouseIntersectionB = BlenderControls::Math::IntersectHelper(
			GrabContext, MousePosBOrigin, MousePosBDirection);

		GrabContext.ScreenToWorldScale = FVector::Dist(MouseIntersectionA, MouseIntersectionB);
		ViewUp = SceneView->GetViewUp();
		ViewRight = SceneView->GetViewRight();
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

		FSceneViewFamilyContext TempViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		SceneView = ViewportClient->CalcSceneView(&TempViewFamily);
		if (SceneView)
		{
			SceneView->DeprojectFVector2D(CurrentMousePos, CurrentRayOrigin, CurrentRayDirection);
		}

		MouseDelta = CurrentMousePos - GrabContext.MousePosA;
	}

	void FBlenderToolBase::OnEnd(bool bApply)
	{
		if (!GEditor || !Pivot)
		{
			return;
		}

		GEditor->SetSelectionOutlineColor(CachedSelectionColor);
		LockedAxis = EAxisLock::All;
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

	void FBlenderToolBase::SetPrecisionModeActive(bool bNewPrecisionModeActive)
	{
		// On shift held down
		if (bNewPrecisionModeActive && !bPrecisionModeActive)
		{
			GrabContext.DeltaAnchor = GrabContext.TotalDelta;
			GrabContext.ShiftStartHit = CurrentHit;
			CurrentPrecisionFactor = PrecisionFactor;
			bWasPrecisionModeActive = false;
		}

		// On shift released
		if (!bNewPrecisionModeActive && bPrecisionModeActive)
		{
			const FVector NewCurrentHit = BlenderControls::Math::IntersectHelper(
				GrabContext, CurrentRayOrigin, CurrentRayDirection);
			//GrabContext.StartHit = NewCurrentHit;
			GrabContext.DeltaAnchor = GrabContext.TotalDelta;
			CurrentPrecisionFactor = 1.0f;
		}

		bPrecisionModeActive = bNewPrecisionModeActive;
	}

	void FBlenderToolBase::StartNewLock(const EAxisLock NewAxis)
	{
		LockedAxis = NewAxis;
		bIsAxisLockActive = true;
		bIsUsingLocalSpace = bLocalSpaceDefault;
	}

	void FBlenderToolBase::HandleAxisLock(const EAxisLock AxisPressed)
	{
		if (!bIsAxisLockActive || LockedAxis != AxisPressed)
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
				LockedAxis = EAxisLock::All;
			}
		}

		UpdateAxisLock();
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
