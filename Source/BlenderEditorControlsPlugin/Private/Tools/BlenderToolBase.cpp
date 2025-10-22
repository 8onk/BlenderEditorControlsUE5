#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "Components/LineBatchComponent.h" //This is needed, although it's marked as unneeded mistakenly by the IDE. 
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

	bool FBlenderToolBase::InitializeEditorState()
	{
		ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (!ViewportClient)
		{
			return false;
		}

		if (!GEditor)
		{
			return false;
		}

		// Cache initial viewport state
		InitialWidgetMode = ViewportClient->GetWidgetMode();
		ViewportClient->ShowWidget(false);
		ViewportClient->Invalidate();

		// Cache initial editor state
		CachedSelectionColor = GEditor->GetSelectionOutlineColor();
		GEditor->SetSelectionOutlineColor(FLinearColor::White);
		bLocalSpaceDefault = GLevelEditorModeTools().GetCoordSystem() == COORD_Local;

		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			CachedBatcher = World->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent);
		}

		return true;
	}

	void FBlenderToolBase::InitializeTransaction()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		VirtualPivot = Session->VirtualPivot;

		// Start transaction for undo
		ParentTxn = MakeUnique<FScopedTransaction>(FText::FromString(DisplayName));
		for (auto Actor : Session->SelectedActors)
		{
			Actor->Modify();
		}
		VirtualPivot->GetTransformProxy()->BeginTransformEditSequence();
	}

	bool FBlenderToolBase::CacheSceneView()
	{
		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		if (!SceneView)
		{
			return false;
		}

		Viewport = ViewportClient->Viewport;
		if (!Viewport)
		{
			return false;
		}

		ViewLocation = SceneView->ViewLocation;
		return true;
	}

	void FBlenderToolBase::CacheViewVectors()
	{
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
	}

	void FBlenderToolBase::InitializeGrabContext(const FVector2D& InMousePos, const FVector& InRayOrigin,
	                                             const FVector& InRayDirection)
	{
		GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
		GrabContext.HelperPlaneN = -ViewForward;
		GrabContext.StartMousePos = InMousePos;
		GrabContext.StartLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();
		GrabContext.HelperAxisDir = FVector::ZeroVector;
		GrabContext.ViewForward = ViewForward;

		// Calculate ScreenToWorldScale
		const FVector2D MousePosB = GrabContext.StartMousePos + FVector2D(1, 0);
		FVector MousePosBOrigin, MousePosBDirection;
		SceneView->DeprojectFVector2D(MousePosB, MousePosBOrigin, MousePosBDirection);

		FVector MouseIntersectionA = MathHelper::IntersectHelper(
			GrabContext, InRayOrigin, InRayDirection);
		FVector MouseIntersectionB = MathHelper::IntersectHelper(
			GrabContext, MousePosBOrigin, MousePosBDirection);

		GrabContext.ScreenToWorldScale = FVector::Dist(MouseIntersectionA, MouseIntersectionB);
	}

	void FBlenderToolBase::InitializeUI()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		// Setup HUD
		HudWidget = SNew(STransformHUD);
		HudWidget->Attach();
		UpdateHud();
		UpdateToolSettingsForAxisLock();

		// Setup Cursor
		ViewportClient->SetRequiredCursorOverride(false, EMouseCursor::None);
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		HudWidget->SetVirtualCursor(Session->GetWrappedCursorPos());
		Session->VirtualMousePosition = Session->CursorAnchorPoint;
		//Needed since CurrentViewportMousePosition - Session->CursorAnchorPoint; in onactive
	}

	void FBlenderToolBase::RestorePreviousState()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		// Restore axis lock
		SetGrabContextAxisLock(Session->GetLockedAxis());
		if (Session->GetLockedAxis() != EAxisLock::All)
		{
			ClearDrawnAxisLines();
			UpdateAxisLock();
		}

		// Restore numeric mode
		if (Session->IsNumericInputActive())
		{
			ApplyNumeric();
		}
	}

	void FBlenderToolBase::OnBegin()
	{
		checkf(OwningSession.IsValid(), TEXT("OnBegin: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		// 1. Get Viewport, GEditor, cache settings
		if (!InitializeEditorState())
		{
			return; // Bails if no ViewportClient or GEditor
		}

		// 2. Setup Undo/Redo
		InitializeTransaction();

		// 3. Calculate SceneView and base view vectors
		if (!CacheSceneView())
		{
			return;
		}
		CacheViewVectors(); // Sets ViewUp, ViewRight, ViewForward

		const FVector2D MousePos = Session->StartMousePos;
		FVector StartRayOrigin, StartRayDirection;
		SceneView->DeprojectFVector2D(MousePos, StartRayOrigin, StartRayDirection);

		// 5. Set internal tool mouse state
		CurrentMousePosition = MousePos;
		CurrentViewportMousePos = MousePos;

		// 6. Setup the GrabContext for transform calculations
		InitializeGrabContext(MousePos, StartRayOrigin, StartRayDirection);

		// 7. Setup HUD and Cursors
		InitializeUI();

		// 8. Restore state from session (axis locks, numeric input)
		RestorePreviousState();

		bIsToolActive = true;
	}

	void FBlenderToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		UpdateHud();

		if (!Viewport || !ViewportClient || !VirtualPivot || !GetSession().IsValid())
		{
			return;
		}
		CurrentViewportMousePos = CurrentViewportMousePosition;

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
		bIsToolActive = false;
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (!GEditor || !VirtualPivot || !ViewportClient)
		{
			return;
		}

		GEditor->SetSelectionOutlineColor(CachedSelectionColor);
		bPrecisionModeActive = false;
		CurrentPrecisionFactor = 1.0f;
		MouseDelta = FVector2D::ZeroVector;
		CurrentMousePosition = FVector2D::ZeroVector;

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
			GEditor->NoteSelectionChange(/*bNotify=*/true);
			GEditor->RedrawLevelEditingViewports(/*bInvalidateHitProxies:*/true);
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
		UE_LOG(LogTemp, Log, TEXT("LockedAxis set to: %d"), static_cast<int32>(Session->LockedAxis));
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

		// Abort undo-tracking
		if (ParentTxn)
		{
			ParentTxn->Cancel();
			ParentTxn.Reset();
		}

		VirtualPivot->RevertToStartState();
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
} // namespace BlenderControls
