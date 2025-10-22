#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "Components/LineBatchComponent.h" //This is needed, although it's marked as unneeded mistakenly. 
#include "Utils/BlenderMathHelpers.h"
#include "Tools/SharedPivot.h"
#include "UI/AxisLockGizmoComponent.h"
#include "UI/TransformHUD.h"

//TODO: make GetSnapOffset abstract
namespace BlenderControls
{
	FBlenderToolBase::FBlenderToolBase(const TSharedRef<FTransformSession>& InSession, ETransformMode InMode,
	                                   const FString& InDisplayName)
		: OwningSession(InSession), Mode(InMode), DisplayName(InDisplayName), NumNumericSlots(3)
	{
	}

	FBlenderToolBase::~FBlenderToolBase()
	{
		UE_LOG(LogTemp, Warning, TEXT("~FBlenderToolBase"));
	}

	void FBlenderToolBase::UpdateAxisLock()
	{
		checkf(OwningSession.IsValid(), TEXT("UpdateAxisLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		UpdateToolSettingsForAxisLock();
		SetGrabContextAxisLock(Session->LockedAxis);
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
		checkf(OwningSession.IsValid(), TEXT("RedrawAxisLines: Session was invalid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		// Clear any previous gizmos first
		for (auto& Giz : AxisGizmos)
		{
			if (Giz.IsValid()) Giz->DestroyComponent();
		}
		AxisGizmos.Empty();

		auto AddAxis = [&](EAxisLock Axis, const FChildInfo* ChildInfo)
		{
			FVector Origin, AxisDir;
			if (ChildInfo && Session->bUsingLocalSpace)
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

				if (bIsActive || !Session->IsUsingLocalSpace())
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

		if (Session->IsUsingLocalSpace())
		{
			for (const FChildInfo& Child : VirtualPivot->GetChildren())
			{
				if (!Child.Actor) continue;
				switch (Session->LockedAxis)
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
					AddAxis(Session->LockedAxis, &Child);
					break;
				}
			}
		}
		else
		{
			switch (Session->LockedAxis)
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
				AddAxis(Session->LockedAxis, nullptr);
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
	
	UAxisLockGizmoComponent* FBlenderToolBase::SpawnAxisGizmo(const FVector& Origin, const FVector& AxisDir,
	                                                          const FLinearColor& Color, float ThicknessPx,
	                                                          float LineLength) const
	{
		checkf(OwningSession.IsValid(), TEXT("SpawnAxisGizmo: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

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
		checkf(OwningSession.IsValid(), TEXT("GetAxisVector: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		FVector AxisVector =
			(InAxis == EAxisLock::X)
				? FVector::XAxisVector
				: (InAxis == EAxisLock::Y)
				? FVector::YAxisVector
				: (InAxis == EAxisLock::Z)
				? FVector::ZAxisVector
				: FVector::ZeroVector;

		if (Session->IsUsingLocalSpace())
		{
			AxisVector = VirtualPivot->GetActiveElement().Transform.TransformVectorNoScale(AxisVector);
		}
		return AxisVector.GetSafeNormal();
	}

	void FBlenderToolBase::OnBegin()
	{
		checkf(OwningSession.IsValid(), TEXT("OnBegin: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

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
		for (auto Actor : Session->SelectedActors)
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

		FVector StartRayOrigin, StartRayDirection;
		const FVector2D MousePos = Session->StartMousePos;
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
		CurrentMousePosition = MousePos;

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

		SetGrabContextAxisLock(Session->LockedAxis);
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
		}

		//Ensures the hardware cursor never resurfaces while tool is active (works regardless)
		ViewportClient->SetRequiredCursorOverride(false, EMouseCursor::None);
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		HudWidget->SetVirtualCursor(Session->WrappedMousePosition);
		Session->VirtualMousePosition = Session->CursorAnchorPoint;
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		UpdateHud();

		if (!Viewport || !ViewportClient || !VirtualPivot || !GetSession().IsValid())
		{
			return;
		}

		const TSharedPtr<FTransformSession> Session = GetSession();
		const FVector2D TrueMouseDelta = CurrentViewportMousePosition - Session->CursorAnchorPoint;

		// If the delta is zero, do nothing to avoid drift from the SetMouse call itself.
		if (TrueMouseDelta.IsNearlyZero())
		{
			return;
		}

		Session->VirtualMousePosition += TrueMouseDelta;

		Viewport->SetMouse(static_cast<int32>(Session->CursorAnchorPoint.X),
		                   static_cast<int32>(Session->CursorAnchorPoint.Y));

		if (HudWidget.IsValid())
		{
			const FVector2D TotalDelta = Session->VirtualMousePosition - Session->CursorAnchorPoint;
			const FVector2D LogicalCursorPosition = Session->CursorAnchorPoint + TotalDelta;

			const FVector2D ViewportSize = Viewport->GetSizeXY();

			Session->WrappedMousePosition.X = FMath::Fmod(LogicalCursorPosition.X, ViewportSize.X);
			Session->WrappedMousePosition.Y = FMath::Fmod(LogicalCursorPosition.Y, ViewportSize.Y);

			if (Session->WrappedMousePosition.X < 0)
			{
				Session->WrappedMousePosition.X += ViewportSize.X;
			}
			if (Session->WrappedMousePosition.Y < 0)
			{
				Session->WrappedMousePosition.Y += ViewportSize.Y;
			}

			HudWidget->SetVirtualCursor(Session->WrappedMousePosition);
		}

		MouseDelta += TrueMouseDelta * CurrentPrecisionFactor;
	}

	void FBlenderToolBase::OnEnd(const bool bApply)
	{
		if (!GetSession().IsValid())
		{
			return;
		}
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (!GEditor || !VirtualPivot || !ViewportClient)
		{
			return;
		}

		GEditor->SetSelectionOutlineColor(CachedSelectionColor);
		Session->LockedAxis = EAxisLock::All;
		bPrecisionModeActive = false;
		CurrentPrecisionFactor = 1.0f;
		MouseDelta = FVector2D::ZeroVector;
		CurrentMousePosition = FVector2D::ZeroVector;

		Session->SelectedActors.Empty();
		VirtualPivot->GetTransformProxy()->EndTransformEditSequence();

		ViewportClient->SetWidgetMode(InitialWidgetMode);
		constexpr bool bShowWidget = true;
		ViewportClient->ShowWidget(bShowWidget);
		HudWidget->Detach();
		ViewportClient->Invalidate();

		ClearDrawnAxisLines();
		if (!Session->WrappedMousePosition.IsNearlyZero())
		{
			Viewport->SetMouse(static_cast<int32>(Session->WrappedMousePosition.X),
			                   static_cast<int32>(Session->WrappedMousePosition.Y));
		}
		ViewportClient->SetRequiredCursorOverride(false, EMouseCursor::Default);
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);

		if (GEditor)
		{
			//Force the editor to redraw gizmos so that they are up to date 
			GEditor->NoteSelectionChange(true);
			GEditor->RedrawLevelEditingViewports(true);
		}
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
		checkf(OwningSession.IsValid(), TEXT("SetSnappingEnabled: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

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

	void FBlenderToolBase::StartNewLock(const EAxisLock NewAxis) const
	{
		checkf(OwningSession.IsValid(), TEXT("StartNewLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		Session->bIsAxisLockActive = true;
		Session->bUsingLocalSpace = bLocalSpaceDefault;
		Session->LockedAxis = NewAxis;
	}

	void FBlenderToolBase::HandleAxisLock(const EAxisLock AxisPressed)
	{
		checkf(OwningSession.IsValid(), TEXT("HandleAxisLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (!Session->IsAxisLockActive() || Session->LockedAxis != AxisPressed)
		{
			StartNewLock(AxisPressed);
		}
		else
		{
			// Second Press
			if (Session->IsUsingLocalSpace() == bLocalSpaceDefault)
			{
				Session->bUsingLocalSpace = !bLocalSpaceDefault;
			}
			// Third Press
			else
			{
				Session->bIsAxisLockActive = false;
				Session->bUsingLocalSpace = false;
				Session->LockedAxis = EAxisLock::All;
			}
		}

		UpdateAxisLock();
	}

	bool FBlenderToolBase::IsSingleAxisLocked() const
	{
		checkf(OwningSession.IsValid(), TEXT("IsSingleAxisLocked: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (Session->bIsAxisLockActive && GrabContext.HelperAxisDir != FVector::ZeroVector)
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

		OnEnd(/*bApply=*/true);
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

		OnEnd(/*bApply=*/false);
	}

	void FBlenderToolBase::UpdateHud()
	{
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


	//NUMERIC MODE
	//
	// void FBlenderToolBase::BeginNumericMode()
	// {
	// 	Session->bIsNumericInputActive = true;
	//
	// 	if (Mode == ETransformMode::Rotate)
	// 	{
	// 		for (auto& Slot : Session->NumericSlots)
	// 		{
	// 			Slot.bUsingDegrees = true;
	// 		}
	// 	}
	//
	// 	ApplyNumeric();
	// }
	//
	// void FBlenderToolBase::CycleNumericInputSlot()
	// {
	// 	FNumericSlotData& CurrentSlot = Session->NumericSlots[Session->CurrentNumericSlotIndex];
	// 	CurrentSlot.CommittedValue = CurrentSlot.GetTotal();
	// 	if (!CurrentSlot.Display.IsEmpty())
	// 	{
	// 		if (CurrentSlot.LiveValue.IsSet())
	// 		{
	// 			CurrentSlot.LiveValue.Reset();
	// 		}
	//
	// 		CurrentSlot.Display.Empty();
	// 		CurrentSlot.SlotState = ESlotState::Committed;
	// 	}
	// 	CurrentSlot.bIsReciprocal = false;
	// 	CurrentSlot.bIsNegated = false;
	//
	// 	if (NumNumericSlots > 0)
	// 	{
	// 		Session->CurrentNumericSlotIndex = (Session->CurrentNumericSlotIndex + 1) % NumNumericSlots;
	// 	}
	//
	// 	UpdateHud();
	//
	// 	//DEBUG
	// 	for (int32 i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
	// 	{
	// 		Session->NumericSlots[i].Print();
	// 	}
	// }
	//
	// void FBlenderToolBase::ExitNumericMode()
	// {
	// 	if (!Session.IsValid())
	// 	{
	// 		return;
	// 	}
	//
	// 	Session->bIsNumericInputActive = false;
	// 	Session->NumericBuffer.Empty();
	// 	Session->CurrentNumericSlotIndex = 0;
	//
	// 	for (int i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
	// 	{
	// 		Session->NumericSlots[i] = FNumericSlotData();
	// 	}
	//
	// 	OnActive(CurrentViewportMousePos);
	// 	UpdateHud();
	// }
	//
	// void FBlenderToolBase::ClearLiveNumericValue()
	// {
	// 	FNumericSlotData& Slot = Session->NumericSlots[Session->CurrentNumericSlotIndex];
	//
	// 	Slot.LiveValue.Reset();
	//
	// 	Session->NumericSlots[Session->CurrentNumericSlotIndex] = Slot;
	// 	UpdateHud();
	// }
	//
	// void FBlenderToolBase::UpdateActiveNumericSlot(const TCHAR Character)
	// {
	// 	FNumericSlotData& Slot = Session->NumericSlots[Session->CurrentNumericSlotIndex];
	//
	// 	switch (Slot.SlotState)
	// 	{
	// 	case ESlotState::Pristine:
	// 		Slot.SlotState = ESlotState::FirstEdit;
	// 		break;
	// 	case ESlotState::Committed:
	// 		Slot.SlotState = ESlotState::Additive;
	// 		break;
	// 	default:
	// 		break;
	// 	}
	//
	// 	Slot.Display.AppendChar(Character);
	//
	// 	double CurrentLiveValue = 0.0;
	// 	if (FDefaultValueHelper::ParseDouble(Slot.Display, CurrentLiveValue))
	// 	{
	// 		if (Slot.GetTotal() < 0.0f)
	// 		{
	// 			Slot.LiveValue = -CurrentLiveValue;
	// 		}
	// 		else
	// 		{
	// 			Slot.LiveValue = CurrentLiveValue;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		Slot.SlotState = ESlotState::InvalidInput;
	// 	}
	//
	// 	Session->NumericSlots[Session->CurrentNumericSlotIndex] = Slot;
	// 	ApplyNumeric();
	// 	UpdateHud();
	//
	// 	for (int32 i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
	// 	{
	// 		Session->NumericSlots[i].Print();
	// 	}
	// }
	//
	// void FBlenderToolBase::HandleBackspace()
	// {
	// 	FNumericSlotData& Slot = Session->NumericSlots[Session->CurrentNumericSlotIndex];
	//
	// 	if (!Slot.Display.IsEmpty())
	// 	{
	// 		Slot.Display.RemoveAt(Slot.Display.Len() - 1);
	// 	}
	// 	else
	// 	{
	// 		switch (Slot.SlotState)
	// 		{
	// 		case ESlotState::FirstEdit:
	// 			{
	// 				bool bOtherSlotsHaveValues = false;
	// 				for (int32 i = 0; i < 3; ++i)
	// 				{
	// 					if (i == Session->CurrentNumericSlotIndex) continue;
	//
	// 					if (Session->NumericSlots[i].SlotState != ESlotState::Pristine)
	// 					{
	// 						bOtherSlotsHaveValues = true;
	// 						break;
	// 					}
	// 				}
	//
	// 				if (bOtherSlotsHaveValues)
	// 				{
	// 					Slot.SlotState = ESlotState::Pristine;
	// 				}
	// 				else
	// 				{
	// 					ExitNumericMode();
	// 					return;
	// 				}
	// 				break;
	// 			}
	//
	// 		case ESlotState::Pristine:
	// 			ExitNumericMode();
	// 			return;
	//
	// 		case ESlotState::Additive:
	// 			Slot.SlotState = ESlotState::Committed;
	// 		case ESlotState::Committed:
	// 			{
	// 				const FString DisplayString = GetFormattedValueForEditing(Slot);
	// 				if (!DisplayString.IsEmpty())
	// 				{
	// 					Slot.Display = DisplayString;
	// 					Slot.Display.LeftChopInline(1);
	// 					Slot.CommittedValue.Reset();
	// 					Slot.SlotState = ESlotState::FirstEdit;
	// 				}
	// 			}
	// 			break;
	// 		}
	// 	}
	//
	// 	double CurrentLiveValue = 0.0;
	//
	// 	if (!Slot.Display.IsEmpty() && FDefaultValueHelper::ParseDouble(Slot.Display, CurrentLiveValue))
	// 	{
	// 		Slot.LiveValue = CurrentLiveValue;
	//
	// 		if (Slot.SlotState == ESlotState::InvalidInput)
	// 		{
	// 			Slot.SlotState = ESlotState::FirstEdit;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		if (!Slot.Display.IsEmpty())
	// 		{
	// 			Slot.SlotState = ESlotState::InvalidInput;
	// 		}
	// 		else
	// 		{
	// 			Slot.LiveValue.Reset();
	// 		}
	// 	}
	//
	// 	Session->NumericSlots[Session->CurrentNumericSlotIndex] = Slot;
	// 	ApplyNumeric();
	//
	// 	for (int32 i = 0; i < UE_ARRAY_COUNT(Session->NumericSlots); ++i)
	// 	{
	// 		Session->NumericSlots[i].Print();
	// 	}
	// }
	//
	// void FBlenderToolBase::ToggleNegation()
	// {
	// 	if (!Session.IsValid() || !Session->bIsNumericInputActive)
	// 	{
	// 		return;
	// 	}
	//
	// 	FNumericSlotData& Slot = Session->NumericSlots[Session->CurrentNumericSlotIndex];
	// 	Slot.bIsNegated = !Slot.bIsNegated;
	// 	Session->NumericSlots[Session->CurrentNumericSlotIndex] = Slot;
	// 	ApplyNumeric();
	// 	UpdateHud();
	// }
	//
	// void FBlenderToolBase::ToggleReciprocal()
	// {
	// 	if (!Session.IsValid() || !Session->bIsNumericInputActive)
	// 	{
	// 		return;
	// 	}
	//
	// 	FNumericSlotData& Slot = Session->NumericSlots[Session->CurrentNumericSlotIndex];
	// 	Slot.bIsReciprocal = !Slot.bIsReciprocal;
	// 	Session->NumericSlots[Session->CurrentNumericSlotIndex] = Slot;
	// 	ApplyNumeric();
	// 	UpdateHud();
	// }
} // namespace BlenderControls
