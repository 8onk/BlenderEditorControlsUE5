#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "Components/LineBatchComponent.h"
#include "Utils/BlenderMathHelpers.h"
#include "DrawDebugHelpers.h"
#include "Misc/DefaultValueHelper.h"
#include "Tools/SharedPivot.h"
#include "UI/AxisLockGizmoComponent.h"
#include "UI/TransformHUD.h"

//TODO: make GetSnapOffset abstract
//Holding control should immediately snap even without moving mouse?
//Draw helper axis in orthographic.
//TODO ROTATION WHEN SWITCHING FROM SCALE/MOVE ROTATES AROUND WRONG PIVOT?
namespace BlenderControls
{
	FBlenderToolBase::FBlenderToolBase(TSharedPtr<FTransformSession> InSession, ETransformMode InMode, EAxisLock InAxis,
	                                   const FString& InDisplayName)
		: Session(InSession), Mode(InMode), LockedAxis(InAxis), DisplayName(InDisplayName), NumNumericSlots(3)
	{
	}

	FBlenderToolBase::~FBlenderToolBase()
	{
	}

	void FBlenderToolBase::UpdateAxisLock()
	{
		UpdateToolSettingsForAxisLock();
		SetGrabContextAxisLock(LockedAxis);
		RedrawAxisLines();

		// Refresh
		if (Session->bIsNumericInputActive)
		{
			ApplyNumeric();
		}
		else
		{
			OnActive(CurrentViewportMousePos);
		}
		UpdateHud();
	}

	void FBlenderToolBase::ClearDrawnAxisLines()
	{
		for (auto& Giz : AxisGizmos)
		{
			if (Giz.IsValid()) Giz->DestroyComponent();
		}
		AxisGizmos.Empty();
	}

	void FBlenderToolBase::RedrawAxisLines()
	{
		// Clear any previous gizmos first
		for (auto& Giz : AxisGizmos)
		{
			if (Giz.IsValid()) Giz->DestroyComponent();
		}
		AxisGizmos.Empty();

		auto AddAxis = [&](EAxisLock Axis, const FChildInfo* ChildInfo)
		{
			FVector Origin, AxisDir;
			if (ChildInfo && bUsingLocalSpace)
			{
				const FTransform& T = ChildInfo->Transform;
				Origin = T.GetLocation();
				const FVector Local =
					(Axis == EAxisLock::X)
						? FVector::XAxisVector
						: (Axis == EAxisLock::Y)
						? FVector::YAxisVector
						: FVector::ZAxisVector;
				AxisDir = T.TransformVectorNoScale(Local);
			}
			else
			{
				Origin = VirtualPivot->GetStartTransform().GetLocation();
				AxisDir = GetAxisVector(Axis);
			}

			FLinearColor Color;
			const FLinearColor BaseColor = GetAxisColor(Axis);
			if (VirtualPivot)
			{
				const FChildInfo& Active = VirtualPivot->GetActiveElement();
				const bool bIsActive =
					(ChildInfo && ChildInfo->Actor && ChildInfo->Actor == Active.Actor);

				if (bIsActive || !bUsingLocalSpace)
				{
					Color = BaseColor * 2.0f;
					Color.A = 1.0f;
				}
				else
				{
					Color = BaseColor * 0.3f;
					Color.A = 0.7f;
				}
			}

			constexpr float ThicknessPx = 2.5f;
			constexpr float Length = WORLD_MAX;

			if (UAxisLockGizmoComponent* Comp = SpawnAxisGizmo(Origin, AxisDir, Color, ThicknessPx, Length))
			{
				AxisGizmos.Add(Comp);
			}
		};

		if (bUsingLocalSpace)
		{
			for (const FChildInfo& Child : VirtualPivot->GetChildren())
			{
				if (!Child.Actor) continue;
				switch (LockedAxis)
				{
				case EAxisLock::XY:
					AddAxis(EAxisLock::X, &Child);
					AddAxis(EAxisLock::Y, &Child);
					break;
				case EAxisLock::XZ:
					AddAxis(EAxisLock::X, &Child);
					AddAxis(EAxisLock::Z, &Child);
					break;
				case EAxisLock::YZ:
					AddAxis(EAxisLock::Y, &Child);
					AddAxis(EAxisLock::Z, &Child);
					break;
				case EAxisLock::All: break;
				default:
					AddAxis(LockedAxis, &Child);
					break;
				}
			}
		}
		else
		{
			switch (LockedAxis)
			{
			case EAxisLock::XY:
				AddAxis(EAxisLock::X, nullptr);
				AddAxis(EAxisLock::Y, nullptr);
				break;
			case EAxisLock::XZ:
				AddAxis(EAxisLock::X, nullptr);
				AddAxis(EAxisLock::Z, nullptr);
				break;
			case EAxisLock::YZ:
				AddAxis(EAxisLock::Y, nullptr);
				AddAxis(EAxisLock::Z, nullptr);
				break;
			case EAxisLock::All: break;
			default:
				AddAxis(LockedAxis, nullptr);
				break;
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

	UAxisLockGizmoComponent* FBlenderToolBase::SpawnAxisGizmo(const FVector& Origin, const FVector& AxisDir,
	                                                          const FLinearColor& Color, float ThicknessPx,
	                                                          float LineLength) const
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World) return nullptr;

		UAxisLockGizmoComponent* Comp =
			NewObject<UAxisLockGizmoComponent>(GetTransientPackage());

		Comp->SetMobility(EComponentMobility::Movable);
		Comp->bHiddenInGame = false;
		Comp->SetCastShadow(false);

		Comp->Origin = Origin;
		Comp->AxisDir = AxisDir;
		Comp->AxisColor = Color;
		Comp->ThicknessPx = ThicknessPx;
		Comp->LineLength = LineLength;

		Comp->RegisterComponentWithWorld(World);
		Comp->SetAxisColor(Color);

		return Comp;
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
			AxisVector = VirtualPivot->GetActiveElement().Transform.TransformVectorNoScale(AxisVector);
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
		const FVector2D MousePos = Session->StartMousePos;
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
		CurrentNumericSlotIndex = Session->CurrentNumericSlotIndex;

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

		HudWidget = SNew(STransformHUD);
		HudWidget->Attach();
		UpdateHud();
		UpdateToolSettingsForAxisLock();

		if (Session->LockedAxis != EAxisLock::All)
		{
			ClearDrawnAxisLines();
			UpdateAxisLock();
		}

		if (Session->bIsNumericInputActive)
		{
			ApplyNumeric();
			//TEMPORARY DEBUG
			// for (int i = 0; i < 3; ++i)
			// {
			// 	Session->NumericSlots[i].Print();
			// 	UE_LOG(LogTemp, Log, TEXT("NEW LINE	"));
			// }
		}
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		UpdateHud();

		if (!Viewport || !ViewportClient || !VirtualPivot || Session->bIsNumericInputActive)
		{
			return;
		}

		CurrentViewportMousePos = CurrentViewportMousePosition;
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

		//UE_LOG(LogTemp, Log, TEXT("MouseDelta: X: %f, Y: %f"), MouseDelta.X, MouseDelta.Y);
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
			const FVector Delta = VirtualPivot->GetStartLocation() - VirtualPivot->GetStartTransform().GetLocation();
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
		HudWidget->Detach();
		ViewportClient->Invalidate();

		ClearDrawnAxisLines();
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

	void FBlenderToolBase::SetSnappingEnabled(bool bNewSnappingEnabled)
	{
		if (bSnappingEnabled == bNewSnappingEnabled)
		{
			return;
		}

		bSnappingEnabled = bNewSnappingEnabled;

		if (!Session->bIsNumericInputActive)
		{
			OnActive(CurrentMousePosition);
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

	void FBlenderToolBase::NotifyMouseWrap()
	{
		bPendingMouseWrap = true;
	}

	FVector FBlenderToolBase::GetSnapOffset(const FVector OffsetFromStart)
	{
		return FVector::ZeroVector;
	}

	FString FBlenderToolBase::GetFormattedValueForEditing(const FNumericSlotData& Slot) const
	{
		if (Slot.CommittedValue.IsSet())
		{
			return FString::Printf(TEXT("%g"), Slot.CommittedValue.Get(0.0));
		}
		return FString();
	}

	void FBlenderToolBase::BeginNumericMode()
	{
		Session->bIsNumericInputActive = true;
		CurrentNumericSlotIndex = Session->CurrentNumericSlotIndex;
		ApplyNumeric();
	}

	void FBlenderToolBase::CycleNumericInputSlot()
	{
		FNumericSlotData& CurrentSlot = Session->NumericSlots[CurrentNumericSlotIndex];
		CurrentSlot.CommittedValue = CurrentSlot.GetTotal();
		if (!CurrentSlot.Display.IsEmpty())
		{
			if (CurrentSlot.LiveValue.IsSet())
			{
				CurrentSlot.LiveValue.Reset();
			}

			CurrentSlot.SlotState = ESlotState::Committed;
			CurrentSlot.Display.Empty();
		}
		CurrentSlot.bIsReciprocal = false;
		CurrentSlot.bIsNegated = false;

		if (NumNumericSlots > 0)
		{
			CurrentNumericSlotIndex = (CurrentNumericSlotIndex + 1) % NumNumericSlots;
		}

		Session->CurrentNumericSlotIndex = CurrentNumericSlotIndex;
		UpdateHud();
	}

	void FBlenderToolBase::ExitNumericMode()
	{
		if (!Session.IsValid())
		{
			return;
		}

		Session->bIsNumericInputActive = false;
		Session->NumericBuffer.Empty();
		Session->CurrentNumericSlotIndex = 0;
		for (int i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
		{
			Session->NumericSlots[i] = FNumericSlotData();
		}

		CurrentNumericSlotIndex = 0;
		for (int i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
		{
			Session->NumericSlots[i] = FNumericSlotData();
		}

		OnActive(CurrentViewportMousePos);
		UpdateHud();
	}

	void FBlenderToolBase::ClearLiveNumericValue()
	{
		FNumericSlotData& Slot = Session->NumericSlots[CurrentNumericSlotIndex];

		Slot.LiveValue.Reset();

		Session->NumericSlots[CurrentNumericSlotIndex] = Slot;
		UpdateHud();
	}

	void FBlenderToolBase::UpdateActiveNumericSlot(const TCHAR Character)
	{
		FNumericSlotData& Slot = Session->NumericSlots[CurrentNumericSlotIndex];

		switch (Slot.SlotState)
		{
		case ESlotState::Pristine:
			Slot.SlotState = ESlotState::FirstEdit;
			FirstEditedSlotIndex = CurrentNumericSlotIndex;
			break;
		case ESlotState::Committed:
			Slot.SlotState = ESlotState::Additive;
			break;
		default:
			break;
		}

		Slot.Display.AppendChar(Character);

		double CurrentLiveValue = 0.0;
		if (FDefaultValueHelper::ParseDouble(Slot.Display, CurrentLiveValue))
		{
			if (Slot.GetTotal() < 0.0f)
			{
				Slot.LiveValue = -CurrentLiveValue;
			}
			else
			{
				Slot.LiveValue = CurrentLiveValue;
			}
		}
		else
		{
			Slot.SlotState = ESlotState::InvalidInput;
		}

		Session->NumericSlots[CurrentNumericSlotIndex] = Slot;
		ApplyNumeric();
		UpdateHud();
	}

	void FBlenderToolBase::HandleBackspace()
	{
		FNumericSlotData& Slot = Session->NumericSlots[CurrentNumericSlotIndex];

		if (!Slot.Display.IsEmpty())
		{
			Slot.Display.RemoveAt(Slot.Display.Len() - 1);
		}
		else
		{
			switch (Slot.SlotState)
			{
			case ESlotState::FirstEdit:
				{
					bool bOtherSlotsHaveValues = false;
					for (int32 i = 0; i < 3; ++i)
					{
						if (i == CurrentNumericSlotIndex) continue;

						if (Session->NumericSlots[i].SlotState != ESlotState::Pristine)
						{
							bOtherSlotsHaveValues = true;
							break;
						}
					}

					if (bOtherSlotsHaveValues)
					{
						Slot.SlotState = ESlotState::Pristine;
					}
					else
					{
						ExitNumericMode();
						return;
					}
					break;
				}

			case ESlotState::Pristine:
				ExitNumericMode();
				return;

			case ESlotState::Additive:
				Slot.SlotState = ESlotState::Committed;
			case ESlotState::Committed:
				{
					const FString DisplayString = GetFormattedValueForEditing(Slot);
					if (!DisplayString.IsEmpty())
					{
						Slot.Display = DisplayString;
						Slot.Display.LeftChopInline(1);
						Slot.CommittedValue.Reset();
						Slot.SlotState = ESlotState::FirstEdit;
					}
				}
				break;
			}
		}

		double CurrentLiveValue = 0.0;

		if (!Slot.Display.IsEmpty() && FDefaultValueHelper::ParseDouble(Slot.Display, CurrentLiveValue))
		{
			Slot.LiveValue = CurrentLiveValue;

			if (Slot.SlotState == ESlotState::InvalidInput)
			{
				Slot.SlotState = ESlotState::FirstEdit;
			}
		}
		else
		{
			if (!Slot.Display.IsEmpty())
			{
				Slot.SlotState = ESlotState::InvalidInput;
			}
			else
			{
				Slot.LiveValue.Reset();
			}
		}

		Session->NumericSlots[CurrentNumericSlotIndex] = Slot;
		ApplyNumeric();
	}

	void FBlenderToolBase::ToggleNegation()
	{
		if (!Session.IsValid() || !Session->bIsNumericInputActive)
		{
			return;
		}

		FNumericSlotData& Slot = Session->NumericSlots[CurrentNumericSlotIndex];
		Slot.bIsNegated = !Slot.bIsNegated;
		Session->NumericSlots[CurrentNumericSlotIndex] = Slot;
		ApplyNumeric();
		UpdateHud();
	}

	void FBlenderToolBase::ToggleReciprocal()
	{
		if (!Session.IsValid() || !Session->bIsNumericInputActive)
		{
			return;
		}

		FNumericSlotData& Slot = Session->NumericSlots[CurrentNumericSlotIndex];
		Slot.bIsReciprocal = !Slot.bIsReciprocal;
		Session->NumericSlots[CurrentNumericSlotIndex] = Slot;
		ApplyNumeric();
		UpdateHud();
	}
} // namespace BlenderControls
