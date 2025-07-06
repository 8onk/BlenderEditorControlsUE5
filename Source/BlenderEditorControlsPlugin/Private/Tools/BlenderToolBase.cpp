#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Selection.h"
#include "Utils/BlenderMathHelpers.h"
#include "DrawDebugHelpers.h"
#include "VectorUtil.h"

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
		SetGrabContextAxisLock(GrabContext, LockedAxis, bIsUsingLocalSpace);
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

		// Refresh object pos to be on new plane
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
			const float LineLength = WORLD_MAX;

			const FVector LineStart = Origin - AxisDir * LineLength;
			const FVector LineEnd = Origin + AxisDir * LineLength;

			const float Lifetime = 0.f; // persistent
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
		ViewLocation = SceneView->ViewLocation;

		Viewport = ViewportClient->Viewport;
		if (!Viewport)
		{
			return;
		}

		FIntPoint MousePosInt;
		Viewport->GetMousePos(MousePosInt);
		FVector2D MousePos = FVector2D(MousePosInt);

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
		GrabContext.HelperPlaneN = -ViewportClient->GetViewRotation().Vector();
		GrabContext.MousePosStart = MousePos;
		GrabContext.MousePosB = GrabContext.MousePosStart + FVector2D(1, 0);
		GrabContext.TotalDelta = FVector::ZeroVector;
		GrabContext.PivotStartPosition = Pivot->GetStartTransform().GetLocation();
		GrabContext.HelperAxisDir = FVector::ZeroVector;

		FVector MousePosBOrigin, MousePosBDirection;
		SceneView->DeprojectFVector2D(GrabContext.MousePosB, MousePosBOrigin, MousePosBDirection);
		FVector MouseIntersectionA = BlenderControls::MathHelper::IntersectHelper(
			GrabContext, StartRayOrigin, StartRayDirection);
		FVector MouseIntersectionB = BlenderControls::MathHelper::IntersectHelper(
			GrabContext, MousePosBOrigin, MousePosBDirection);

		GrabContext.ScreenToWorldScale = FVector::Dist(MouseIntersectionA, MouseIntersectionB);
		GrabContext.ViewForward = ViewForward;
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		CurrentViewportMousePos = CurrentViewportMousePosition;
		if (!Viewport || !ViewportClient || !Pivot)
		{
			return;
		}
		const FIntPoint CurrentMousePosInt = FIntPoint(CurrentViewportMousePosition.X, CurrentViewportMousePosition.Y);
		CurrentMousePosition = FVector2D(CurrentMousePosInt);

		if (bPendingMouseWrap)
		{
			LastMousePosition = CurrentMousePosition;
			bPendingMouseWrap = false;
			return;
		}

		const FVector2D CurrentFrameDelta = CurrentMousePosition - LastMousePosition;
		MouseDelta += CurrentFrameDelta * CurrentPrecisionFactor;
		LastMousePosition = CurrentMousePosition;
	}

	void FBlenderToolBase::OnEnd(bool bApply)
	{
		if (!GEditor || !Pivot)
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

	void FBlenderToolBase::SetGrabContextAxisLock(FGrabContext& GC, EAxisLock AxisLock,
	                                              bool bUseLocalSpace) const
	{
		const FTransform ObjectTransform = Pivot->GetStartTransform();
		const FVector X = bUseLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bUseLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bUseLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

		switch (AxisLock)
		{
		case EAxisLock::All:
			GC.HelperType = FGrabContext::EHelperType::ViewPlane;
			GC.HelperPlaneN = -GC.ViewForward;
			break;

		case EAxisLock::X:
			GC.HelperType = FGrabContext::EHelperType::AxisLine;
			GC.HelperAxisDir = X;
			GC.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Y, Z, GC.ViewForward);
			break;

		case EAxisLock::Y:
			GC.HelperType = FGrabContext::EHelperType::AxisLine;
			GC.HelperAxisDir = Y;
			GC.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(Y, X, Z, GC.ViewForward);
			break;

		case EAxisLock::Z:
			GC.HelperType = FGrabContext::EHelperType::AxisLine;
			GC.HelperAxisDir = Z;
			GC.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(Z, X, Y, GC.ViewForward);
			break;

		case EAxisLock::XY:
			GC.HelperType = FGrabContext::EHelperType::AxisPlane;
			GC.HelperAxisDir = FVector::ZeroVector;
			GC.HelperPlaneN = Z;
			break;

		case EAxisLock::XZ:
			GC.HelperType = FGrabContext::EHelperType::AxisPlane;
			GC.HelperAxisDir = FVector::ZeroVector;
			GC.HelperPlaneN = Y;
			break;

		case EAxisLock::YZ:
			GC.HelperType = FGrabContext::EHelperType::AxisPlane;
			GC.HelperAxisDir = FVector::ZeroVector;
			GC.HelperPlaneN = X;
			break;
		}
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
		SnapOffset = BlenderControls::MathHelper::RoundVectorToInt(SnapOffset);
		SnapOffset *= GridSize;

		return SnapOffset;
	}
} // namespace BlenderControls
