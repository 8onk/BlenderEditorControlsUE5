#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Selection.h"
#include "Utils/BlenderMathHelpers.h"
#include "DrawDebugHelpers.h"

//TODO: make GetSnapOffset abstract
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
		//
		// FVector ViewDirection;
		// if (ViewportClient->IsPerspective())
		// {
		// 	ViewDirection = ViewportClient->GetViewRotation().Vector();
		// }
		// else
		// {
		// 	switch (ViewportClient->ViewportType)
		// 	{
		// 	case LVT_OrthoXY:
		// 		ViewDirection = FVector::UpVector;
		// 		break; // Top view
		// 	case LVT_OrthoXZ:
		// 		ViewDirection = FVector::RightVector;
		// 		break; // Front view
		// 	case LVT_OrthoYZ:
		// 		ViewDirection = FVector::ForwardVector;
		// 		break; // Side view
		// 	case LVT_OrthoNegativeXY:
		// 		ViewDirection = -FVector::UpVector;
		// 		break;
		// 	case LVT_OrthoNegativeXZ:
		// 		ViewDirection = -FVector::RightVector;
		// 		break;
		// 	case LVT_OrthoNegativeYZ:
		// 		ViewDirection = -FVector::ForwardVector;
		// 		break;
		// 	default:
		// 		ViewDirection = FVector::ForwardVector;
		// 		break;
		// 	}
		// }

		SetGrabContextAxisLock(LockedAxis);
		RedrawAxisLines();

		// Refresh
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

	void FBlenderToolBase::RedrawAxisLines() const
	{
		FlushDrawnAxisLines();
		if (LockedAxis == EAxisLock::XY || LockedAxis == EAxisLock::XZ || LockedAxis == EAxisLock::YZ)
		{
			if (LockedAxis == EAxisLock::XY)
			{
				DrawAxisLine(EAxisLock::X);
				DrawAxisLine(EAxisLock::Y);
			}
			else if (LockedAxis == EAxisLock::XZ)
			{
				DrawAxisLine(EAxisLock::X);
				DrawAxisLine(EAxisLock::Z);
			}
			else if (LockedAxis == EAxisLock::YZ)
			{
				DrawAxisLine(EAxisLock::Y);
				DrawAxisLine(EAxisLock::Z);
			}
		}
		else if (LockedAxis != EAxisLock::All)
		{
			DrawAxisLine(LockedAxis);
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

	void FBlenderToolBase::DrawAxisLine(const EAxisLock InAxis) const
	{
		if (CachedBatcher.IsValid())
		{
			const FVector AxisDir = GetAxisVector(InAxis);
			const FVector Origin = VirtualPivot->GetStartTransform().GetLocation();
			constexpr float LineLength = WORLD_MAX;

			const FVector LineStart = Origin - AxisDir * LineLength;
			const FVector LineEnd = Origin + AxisDir * LineLength;

			constexpr float Lifetime = 0.f; // persistent
			const FLinearColor Color = GetAxisColor(InAxis);

			CachedBatcher->DrawLine(LineStart, LineEnd, Color, SDPG_World, 2.0f, Lifetime);
			CachedBatcher->MarkRenderStateDirty();
		}
	}

	float FBlenderToolBase::CalculateDynamicThickness(const FVector& Origin) const
	{
		return 0.f;
		// if (!SceneView)
		// {
		// 	return FallbackLineThickness;
		// }
		//
		// const FVector CameraLocation = SceneView->ViewLocation;
		// const float Distance = FVector::Dist(CameraLocation, Origin);
		//
		// // Linear scaling: thickness grows with distance
		// float Scaled = FallbackLineThickness * (Distance / ReferenceDistance);
		//
		// // Clamp to reasonable bounds
		// return FMath::Clamp(Scaled, MinLineThickness, MaxLineThickness);
	}

	FVector FBlenderToolBase::GetAxisVector(const EAxisLock InAxis) const
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
			AxisVector = VirtualPivot->GetStartTransform().TransformVectorNoScale(AxisVector);
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
		InitialWidgetMode = ViewportClient->GetWidgetMode();
		ViewportClient->ShowWidget(false);
		ViewportClient->Invalidate();

		if (!GEditor)
		{
			return;
		}
		CachedSelectionColor = GEditor->GetSelectionOutlineColor();
		GEditor->SetSelectionOutlineColor(FLinearColor::White);
		bLocalSpaceDefault = GLevelEditorModeTools().GetCoordSystem() == COORD_Local;

		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			CachedBatcher = World->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent);
		}

		CaptureSelection();
		constexpr EPivotMode PivotMode = EPivotMode::MedianPoint;
		VirtualPivot = MakeShared<FSharedPivot>(SelectedActors, PivotMode);

		// Start transaction for undo
		ParentTxn = MakeUnique<FScopedTransaction>(FText::FromString(DisplayName));
		for (auto Actor : SelectedActors)
		{
			Actor->Modify();
		}
		VirtualPivot->GetTransformProxy()->BeginTransformEditSequence();

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
		ViewLocation = SceneView->ViewLocation;

		Viewport = ViewportClient->Viewport;
		if (!Viewport)
		{
			return;
		}

		FIntPoint MousePosInt;
		Viewport->GetMousePos(MousePosInt);
		const FVector2D MousePos = FVector2D(MousePosInt);

		FVector StartRayOrigin, StartRayDirection;
		SceneView->DeprojectFVector2D(MousePos, StartRayOrigin, StartRayDirection);

		ViewUp = SceneView->GetViewUp();
		ViewRight = SceneView->GetViewRight();
		ViewForward = ViewportClient->GetViewRotation().Vector();
		MouseDelta = FVector2D::ZeroVector;
		LastMousePosition = MousePos;
		CurrentMousePosition = MousePos;
		bPendingMouseWrap = false;
		bIsAxisLockActive = false;
		bIsUsingLocalSpace = bLocalSpaceDefault;
		LockedAxis = EAxisLock::All;

		GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
		GrabContext.HelperPlaneN = -ViewForward;
		GrabContext.StartMousePos = MousePos;
		CurrentViewportMousePos = MousePos;
		FVector2D MousePosB = GrabContext.StartMousePos + FVector2D(1, 0);
		GrabContext.StartLocation = VirtualPivot->GetStartTransform().GetLocation();
		GrabContext.HelperAxisDir = FVector::ZeroVector;

		FVector MousePosBOrigin, MousePosBDirection;
		SceneView->DeprojectFVector2D(MousePosB, MousePosBOrigin, MousePosBDirection);
		FVector MouseIntersectionA = BlenderControls::MathHelper::IntersectHelper(
			GrabContext, StartRayOrigin, StartRayDirection);
		FVector MouseIntersectionB = BlenderControls::MathHelper::IntersectHelper(
			GrabContext, MousePosBOrigin, MousePosBDirection);

		GrabContext.ScreenToWorldScale = FVector::Dist(MouseIntersectionA, MouseIntersectionB);
		GrabContext.ViewForward = ViewForward;

		UnscaledMouseDelta = MousePos;

		SetGrabContextAxisLock(LockedAxis);
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		CurrentViewportMousePos = CurrentViewportMousePosition;
		if (!Viewport || !ViewportClient || !VirtualPivot)
		{
			return;
		}
		const FIntPoint CurrentMousePosInt = FIntPoint(CurrentViewportMousePosition.X, CurrentViewportMousePosition.Y);
		CurrentMousePosition = FVector2D(CurrentMousePosInt);

		if (bPendingMouseWrap)
		{
			OnMouseWrap();
			bPendingMouseWrap = false;
			return;
		}

		const FVector2D CurrentFrameDelta = CurrentMousePosition - LastMousePosition;
		UnscaledMouseDelta += CurrentFrameDelta;
		MouseDelta += CurrentFrameDelta * CurrentPrecisionFactor;
		LastMousePosition = CurrentMousePosition;
	}

	void FBlenderToolBase::OnEnd(bool bApply)
	{
		if (!GEditor || !VirtualPivot || !ViewportClient)
		{
			return;
		}

		GEditor->SetSelectionOutlineColor(CachedSelectionColor);
		LockedAxis = EAxisLock::All;
		bPrecisionModeActive = false;
		CurrentPrecisionFactor = 1.0f;
		MouseDelta = FVector2D::ZeroVector;
		CurrentMousePosition = FVector2D::ZeroVector;
		LastMousePosition = FVector2D::ZeroVector;

		SelectedActors.Empty();
		VirtualPivot->GetTransformProxy()->EndTransformEditSequence();

		if (GEditor)
		{
			const FVector CurrentPivotLocation = VirtualPivot->GetCurrentLocation();
			const FVector StartPivotLocation = VirtualPivot->GetStartTransform().GetLocation();
			const FVector NewPivotPosition = bApply ? CurrentPivotLocation : StartPivotLocation;

			constexpr bool bSnapPivotToGrid = false;
			constexpr bool bIgnoreAxis = true;
			constexpr bool bAssignPivotToActors = false;
			GEditor->SetPivot(NewPivotPosition, bSnapPivotToGrid, bIgnoreAxis, bAssignPivotToActors);
		}

		ViewportClient->SetWidgetMode(InitialWidgetMode);
		constexpr bool bShowWidget = true;
		ViewportClient->ShowWidget(bShowWidget);
		ViewportClient->Invalidate();

		FlushDrawnAxisLines();
	}

	void FBlenderToolBase::SetPrecisionModeActive(bool bNewPrecisionModeActive)
	{
		// Only proceed if the state is actually changing.
		if (bNewPrecisionModeActive == bPrecisionModeActive)
		{
			return;
		}

		bPrecisionModeActive = bNewPrecisionModeActive;
		if (bPrecisionModeActive)
		{
			CurrentPrecisionFactor = PrecisionFactor;
		}
		else
		{
			CurrentPrecisionFactor = 1.0f;
		}
	}

	void FBlenderToolBase::SetTrackballRotationMode(const bool bEnabled)
	{
	}

	bool FBlenderToolBase::GetTrackballRotationMode()
	{
		return false;
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
			if (bIsUsingLocalSpace == bLocalSpaceDefault)
			{
				// Second Press
				bIsUsingLocalSpace = !bLocalSpaceDefault;
			}
			else
			{
				// Third Press
				bIsAxisLockActive = false;
				LockedAxis = EAxisLock::All;
			}
		}

		UpdateAxisLock();
	}

	bool FBlenderToolBase::IsSingleAxisLocked() const
	{
		if (bIsAxisLockActive && GrabContext.HelperAxisDir != FVector::ZeroVector)
		{
			return true;
		}
		return false;
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
		if (!GEditor || !VirtualPivot)
		{
			return;
		}

		OnEnd(/*bApply=*/false);
		GEditor->SetSelectionOutlineColor(CachedSelectionColor);

		// Reset pivot to start location
		VirtualPivot->GetTransformProxy()->SetTransform(VirtualPivot->GetStartTransform());

		// Abort undo-tracking
		if (ParentTxn)
		{
			ParentTxn->Cancel();
			ParentTxn.Reset();
		}

		SelectedActors.Empty();
	}

	void FBlenderToolBase::OnMouseWrap()
	{
		LastMousePosition = CurrentMousePosition;
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
		return FVector::ZeroVector;
	}

	void FBlenderToolBase::SetGrabContextAxisLock(EAxisLock AxisLock)
	{
	}
} // namespace BlenderControls
