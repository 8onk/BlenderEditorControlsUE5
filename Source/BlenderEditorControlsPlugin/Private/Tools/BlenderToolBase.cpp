#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "Components/LineBatchComponent.h"
#include "Utils/BlenderMathHelpers.h"
#include "DrawDebugHelpers.h"
#include "Tools/SharedPivot.h"

//TODO: make GetSnapOffset abstract
//Draw helper axis in orthographic. 
namespace BlenderControls
{
	FBlenderToolBase::FBlenderToolBase(TSharedPtr<FTransformSession> InSession, ETransformMode InMode, EAxisLock InAxis,
	                                   const FString& InDisplayName)
		: Session(InSession), Mode(InMode), LockedAxis(InAxis), DisplayName(InDisplayName)
	{
	}

	FBlenderToolBase::~FBlenderToolBase()
	{
	}

	void FBlenderToolBase::UpdateAxisLock()
	{
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

		if (bUsingLocalSpace)
		{
			for (const FChildInfo& Child : VirtualPivot->GetChildren())
			{
				if (!Child.Actor) continue;

				if (LockedAxis == EAxisLock::XY)
				{
					DrawAxisLine(EAxisLock::X, &Child);
					DrawAxisLine(EAxisLock::Y, &Child);
				}
				else if (LockedAxis == EAxisLock::XZ)
				{
					DrawAxisLine(EAxisLock::X, &Child);
					DrawAxisLine(EAxisLock::Z, &Child);
				}
				else if (LockedAxis == EAxisLock::YZ)
				{
					DrawAxisLine(EAxisLock::Y, &Child);
					DrawAxisLine(EAxisLock::Z, &Child);
				}
				else if (LockedAxis != EAxisLock::All)
				{
					DrawAxisLine(LockedAxis, &Child);
				}
			}
		}
		else
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
			else if (LockedAxis != EAxisLock::All)
			{
				DrawAxisLine(LockedAxis);
			}
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

	void FBlenderToolBase::DrawAxisLine(const EAxisLock InAxis, const FChildInfo* ChildInfo /*= nullptr*/) const
	{
		if (!CachedBatcher.IsValid()) return;

		FVector Origin;
		FVector AxisDir;

		// Use the provided ChildInfo for local space drawing
		if (ChildInfo && bUsingLocalSpace)
		{
			// Use the SAVED start transform from the FChildInfo struct
			const FTransform& StartTransform = ChildInfo->Transform;

			Origin = StartTransform.GetLocation();
			const FVector WorldAxis = (InAxis == EAxisLock::X)
				                          ? FVector::XAxisVector
				                          : (InAxis == EAxisLock::Y)
				                          ? FVector::YAxisVector
				                          : FVector::ZAxisVector;
			AxisDir = StartTransform.TransformVectorNoScale(WorldAxis);
		}
		else
		{
			// GLOBAL: Fall back to the shared pivot's start transform
			Origin = VirtualPivot->GetStartTransform().GetLocation();
			AxisDir = GetAxisVector(InAxis);
		}

		constexpr float LineLength = 10000.f;
		const FVector LineStart = Origin - AxisDir.GetSafeNormal() * LineLength;
		const FVector LineEnd = Origin + AxisDir.GetSafeNormal() * LineLength;

		const FLinearColor Color = GetAxisColor(InAxis);
		CachedBatcher->DrawLine(LineStart, LineEnd, Color, SDPG_World, 2.0f, 0.f);
		CachedBatcher->MarkRenderStateDirty();
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

		if (bUsingLocalSpace)
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

		VirtualPivot = Session->VirtualPivot;

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
		const FVector2D TestMousePos = FVector2D(MousePosInt);

		UE_LOG(LogTemp, Log, TEXT("Test mouse pos X: %f, Y: %f"), TestMousePos.X, TestMousePos.Y);

		const FVector2D MousePos = Session->StartMousePos;
		UE_LOG(LogTemp, Log, TEXT("Session mouse pos X: %f, Y: %f"), MousePos.X, MousePos.Y);
		FVector StartRayOrigin, StartRayDirection;
		SceneView->DeprojectFVector2D(MousePos, StartRayOrigin, StartRayDirection);

		ViewUp = SceneView->GetViewUp();
		ViewRight = SceneView->GetViewRight();

		if (ViewportClient->IsPerspective())
		{
			ViewForward = ViewportClient->GetViewRotation().Vector();
		}
		else
		{
			switch (ViewportClient->ViewportType)
			{
			case LVT_OrthoXY:
				ViewForward = FVector::UpVector; // +Z, looking down from top
				break;

			case LVT_OrthoNegativeXY:
				ViewForward = -FVector::UpVector; // -Z, looking up from bottom
				break;

			case LVT_OrthoXZ:
				ViewForward = FVector::RightVector; // +Y, looking from front
				break;

			case LVT_OrthoNegativeXZ:
				ViewForward = -FVector::RightVector; // -Y, looking from back
				break;

			case LVT_OrthoYZ:
				ViewForward = FVector::ForwardVector; // +X, looking from side
				break;

			case LVT_OrthoNegativeYZ:
				ViewForward = -FVector::ForwardVector; // -X, looking from other side
				break;

			default:
				ViewForward = FVector::ForwardVector; // Fallback
				break;
			}
		}

		MouseDelta = FVector2D::ZeroVector;
		LastMousePosition = MousePos;
		CurrentMousePosition = MousePos;
		//bPendingMouseWrap = false; Should this be reset here?
		bIsAxisLockActive = Session->bIsAxisLockActive;
		bUsingLocalSpace = Session->bUsingLocalSpace;
		LockedAxis = Session->LockedAxis;

		GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
		GrabContext.HelperPlaneN = -ViewForward;
		GrabContext.StartMousePos = MousePos;
		CurrentViewportMousePos = MousePos;
		const FVector2D MousePosB = GrabContext.StartMousePos + FVector2D(1, 0);
		GrabContext.StartLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();
		GrabContext.HelperAxisDir = FVector::ZeroVector;

		FVector MousePosBOrigin, MousePosBDirection;
		SceneView->DeprojectFVector2D(MousePosB, MousePosBOrigin, MousePosBDirection);
		FVector MouseIntersectionA = MathHelper::IntersectHelper(
			GrabContext, StartRayOrigin, StartRayDirection);
		FVector MouseIntersectionB = MathHelper::IntersectHelper(
			GrabContext, MousePosBOrigin, MousePosBDirection);

		GrabContext.ScreenToWorldScale = FVector::Dist(MouseIntersectionA, MouseIntersectionB);
		GrabContext.ViewForward = ViewForward;

		VirtualMousePosition = CurrentMousePosition;
		SetGrabContextAxisLock(LockedAxis);
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		CurrentViewportMousePos = CurrentViewportMousePosition;
		if (!Viewport || !ViewportClient || !VirtualPivot)
		{
			return;
		}

		const FIntPoint CurrentMousePosInt = FIntPoint(CurrentViewportMousePosition.X,
		                                               CurrentViewportMousePosition.Y);
		CurrentMousePosition = FVector2D(CurrentMousePosInt);

		//THIS works for now, but results in slight drift. Unsure of how to fix this.
		if (bPendingMouseWrap)
		{
			bPendingMouseWrap = false;
			return;
		}

		// UE_LOG(LogHAL, Log, TEXT("Current mouse X: %f, Y: %f"), CurrentMousePosition.X, CurrentMousePosition.Y);
		// UE_LOG(LogHAL, Log, TEXT("Last mouse X: %f, Y: %f"), LastMousePosition.X, LastMousePosition.Y);

		const FVector2D CurrentFrameDelta = CurrentMousePosition - LastMousePosition;

		VirtualMousePosition += CurrentFrameDelta;
		MouseDelta += CurrentFrameDelta * CurrentPrecisionFactor;
		// UE_LOG(LogHAL, Log, TEXT("Current mouse X: %f, Y: %f"), VirtualMousePosition.X, VirtualMousePosition.Y);
		//UE_LOG(LogHAL, Log, TEXT("Mouse Delta X: %f, Y: %f"), MouseDelta.X, MouseDelta.Y);
		LastMousePosition = CurrentMousePosition;

		UE_LOG(LogTemp, Log, TEXT("MouseDelta: X: %f, Y: %f"), MouseDelta.X, MouseDelta.Y);
	}

	void FBlenderToolBase::OnEnd(const bool bApply)
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
			const FVector Delta = VirtualPivot->GetLocation() - VirtualPivot->GetStartTransform().GetLocation();
			const FVector CurrentPivotLocation = Delta + VirtualPivot->GetActiveElement().Transform.GetLocation();
			const FVector StartPivotLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();
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
		bUsingLocalSpace = bLocalSpaceDefault;

		Session->LockedAxis = LockedAxis;
		Session->bUsingLocalSpace = bLocalSpaceDefault;
	}

	void FBlenderToolBase::HandleAxisLock(const EAxisLock AxisPressed)
	{
		if (!bIsAxisLockActive || LockedAxis != AxisPressed)
		{
			StartNewLock(AxisPressed);
		}
		else
		{
			if (bUsingLocalSpace == bLocalSpaceDefault)
			{
				// Second Press
				bUsingLocalSpace = !bLocalSpaceDefault;
			}
			else
			{
				// Third Press
				bIsAxisLockActive = false;
				bUsingLocalSpace = false;
				LockedAxis = EAxisLock::All;
			}
		}

		Session->LockedAxis = LockedAxis;
		Session->bUsingLocalSpace = bUsingLocalSpace;
		Session->bIsAxisLockActive = bIsAxisLockActive;

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

	void FBlenderToolBase::ApplyNumeric(float Value)
	{
		UpdateNumericValue(Value);
		UE_LOG(LogTemp, Log, TEXT("APPLYING NUMMERIC!"));
	}

	void FBlenderToolBase::NotifyMouseWrap()
	{
		bPendingMouseWrap = true;
	}

	void FBlenderToolBase::BeginNumericInput()
	{
		NumericInputSlots = Session->NumericInputSlots;
		CurrentNumericSlotIndex = Session->CurrentNumericSlotIndex;
	}

	void FBlenderToolBase::CycleNumericInputSlot()
	{
		if (LockedAxis == EAxisLock::X || LockedAxis == EAxisLock::Y || LockedAxis == EAxisLock::Z)
		{
			CurrentNumericSlotIndex = (CurrentNumericSlotIndex + 1) % 1;
		}
		else if (LockedAxis == EAxisLock::XY || LockedAxis == EAxisLock::YZ || LockedAxis == EAxisLock::XZ)
		{
			CurrentNumericSlotIndex = (CurrentNumericSlotIndex + 1) % 2;
		}
		else
		{
			CurrentNumericSlotIndex = (CurrentNumericSlotIndex + 1) % 3;
		}

		Session->CurrentNumericSlotIndex = CurrentNumericSlotIndex;
	}

	void FBlenderToolBase::UpdateNumericValue(const float Value)
	{
		UE_LOG(LogTemp, Display, TEXT("%f"), Value);

		switch (CurrentNumericSlotIndex)
		{
		case 0:
			NumericInputSlots.X = Value;
			break;
		case 1:
			NumericInputSlots.Y = Value;
			break;
		case 2:
			NumericInputSlots.Z = Value;
			break;
		default:
			break;
		}

		Session->NumericInputSlots = NumericInputSlots;
	}

	FVector FBlenderToolBase::GetSnapOffset(const FVector OffsetFromStart)
	{
		return FVector::ZeroVector;
	}

	void FBlenderToolBase::SetGrabContextAxisLock(EAxisLock AxisLock)
	{
	}
} // namespace BlenderControls
