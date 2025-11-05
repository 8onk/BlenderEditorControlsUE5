#include "Tools/ToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "BaseGizmos/TransformProxy.h"
#include "Components/LineBatchComponent.h" //This is needed, although it's marked as unneeded mistakenly by the IDE. 
#include "Input/Numeric/NumericInputProcessor.h"
#include "Utils/MathHelpers.h"
#include "Tools/SharedPivot.h"
#include "UI/AxisLockGizmoComponent.h"
#include "UI/TransformHUD.h"

//TODO: Mouse wrapping offsets the cursor slightly, due to the virtual cursor origin.
//TODO: do something about the GetSnapOffset abstract function
namespace BlenderControls
{
	FToolBase::FToolBase(const TSharedRef<FTransformSession>& InSession, ETransformMode InMode,
	                     const FString& InDisplayName)
		: OwningSession(InSession), Mode(InMode), DisplayName(InDisplayName), NumNumericSlots(3)
	{
	}

	void FToolBase::UpdateAxisLock()
	{
		checkf(OwningSession.IsValid(), TEXT("UpdateAxisLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (Session->GetNumericInputProcessor()->IsInNumericMode())
		{
			int32 NewSlotCount;
			switch (Session->LockedAxis)
			{
			case EAxisLock::X:
			case EAxisLock::Y:
			case EAxisLock::Z:
				NewSlotCount = 1;
				break;
			case EAxisLock::XY:
			case EAxisLock::XZ:
			case EAxisLock::YZ:
				NewSlotCount = 2;
				break;
			case EAxisLock::All:
			default:
				NewSlotCount = 3;
				break;
			}
			Session->GetNumericInputProcessor()->UpdateActiveNumSlots(NewSlotCount);
		}

		UpdateToolSettingsForAxisLock();
		SetGrabContextAxisLock(Session->LockedAxis);
		RedrawAxisLines();

		// Refresh
		if (Session->IsNumericInputActive())
		{
			ApplyNumeric();
		}
		else
		{
			OnActive(CurrentViewportMousePos);
		}
		UpdateHud();
	}

	void FToolBase::ClearDrawnAxisLines()
	{
		for (auto& Giz : AxisGizmos)
		{
			if (Giz.IsValid()) Giz->DestroyComponent();
		}
		AxisGizmos.Empty();
	}

	void FToolBase::RedrawAxisLines()
	{
		checkf(OwningSession.IsValid(), TEXT("RedrawAxisLines: Session was invalid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		ClearAxisGizmos();

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

	void FToolBase::ClearAxisGizmos()
	{
		for (auto& Giz : AxisGizmos)
		{
			if (Giz.IsValid()) Giz->DestroyComponent();
		}
		AxisGizmos.Empty();
	}
	
	void FToolBase::OnExitNumericMode()
	{
		OnActive(CurrentViewportMousePos);
	}

	FLinearColor FToolBase::GetAxisColor(EAxisLock InAxis)
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

	UAxisLockGizmoComponent* FToolBase::SpawnAxisGizmo(const FVector& Origin, const FVector& AxisDir,
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

	FVector FToolBase::GetAxisVector(const EAxisLock InAxis) const
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

	bool FToolBase::InitializeEditorState()
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

		return true;
	}

	void FToolBase::InitializePivot()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		VirtualPivot = Session->VirtualPivot;
		VirtualPivot->GetTransformProxy()->BeginTransformEditSequence();
	}

	void FToolBase::CacheViewVectors()
	{
		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		ViewUp = SceneView->GetViewUp();
		ViewRight = SceneView->GetViewRight();
		ViewLocation = SceneView->ViewLocation;

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

	void FToolBase::InitializeGrabContext()
	{
		if (!VirtualPivot.IsValid())
		{
			return;
		}

		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid())
		{
			return;
		}

		GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
		GrabContext.HelperPlaneN = -ViewForward;
		GrabContext.StartMousePos = Session->StartMousePos;
		GrabContext.StartLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();
		GrabContext.HelperAxisDir = FVector::ZeroVector;
		GrabContext.ViewForward = ViewForward;

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);

		FVector StartRayOrigin, StartRayDirection;
		SceneView->DeprojectFVector2D(Session->StartMousePos, StartRayOrigin, StartRayDirection);

		const FVector2D MousePosB = GrabContext.StartMousePos + FVector2D(1, 0);
		FVector MousePosBOrigin, MousePosBDirection;
		SceneView->DeprojectFVector2D(MousePosB, MousePosBOrigin, MousePosBDirection);

		FVector MouseIntersectionA = MathHelper::IntersectHelper(
			GrabContext, StartRayOrigin, StartRayDirection);
		FVector MouseIntersectionB = MathHelper::IntersectHelper(
			GrabContext, MousePosBOrigin, MousePosBDirection);

		GrabContext.ScreenToWorldScale = FVector::Dist(MouseIntersectionA, MouseIntersectionB);
	}

	void FToolBase::InitializeUI()
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
		HudWidget->SetVirtualCursorPos(Session->GetWrappedCursorPos());
		//Needed since CurrentViewportMousePosition - Session->CursorAnchorPoint; in onactive
		Session->VirtualMousePosition = Session->CursorAnchorPoint;
	}

	void FToolBase::RestorePreviousState()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		// Restore axis lock
		SetGrabContextAxisLock(Session->GetLockedAxis());
		if (Session->GetLockedAxis() != EAxisLock::All)
		{
			ClearDrawnAxisLines();
			UpdateAxisLock();
		}
		//
		// // Restore numeric mode
		// if (Session->IsNumericInputActive())
		// {
		// 	ApplyNumeric();
		// }
	}

	void FToolBase::OnBegin()
	{
		checkf(OwningSession.IsValid(), TEXT("OnBegin: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		// 1. Get Viewport, GEditor, cache settings
		if (!InitializeEditorState())
		{
			return; // Bails if no ViewportClient or GEditor
		}

		// 3. Calculate SceneView and base view vectors
		Viewport = ViewportClient->Viewport;
		if (!Viewport)
		{
			return;
		}
		CacheViewVectors(); // Sets ViewUp, ViewRight, ViewForward

		const FVector2D MousePos = Session->StartMousePos;

		// 5. Set internal tool mouse state
		CurrentMousePosition = MousePos;
		CurrentViewportMousePos = MousePos;

		InitializePivot();

		// 6. Setup the GrabContext for transform calculations
		InitializeGrabContext();

		// 7. Setup HUD and Cursors
		InitializeUI();

		// 8. Restore state from session (axis locks, numeric input)
		RestorePreviousState();

		if (Session->NumericInputProcessor.IsValid())
		{
			Session->NumericInputProcessor->OnExitNumericMode.BindSP(AsShared(), &FToolBase::OnExitNumericMode);
		}
	}

	void FToolBase::HandleMouseMovement(const FVector2D& CurrentViewportMousePosition)
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid())
		{
			return;
		}

		const FVector2D TrueMouseDelta = CurrentViewportMousePosition - Session->CursorAnchorPoint;
		// If the delta is zero, do nothing to avoid drift from the SetMouse call itself.
		if (TrueMouseDelta.IsNearlyZero())
		{
			return;
		}

		Session->VirtualMousePosition += TrueMouseDelta;
		Viewport->SetMouse(static_cast<int32>(Session->CursorAnchorPoint.X),
		                   static_cast<int32>(Session->CursorAnchorPoint.Y));

		//Software cursor is part of HudWidget
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

			HudWidget->SetVirtualCursorPos(Session->WrappedMousePosition);
		}

		MouseDelta += TrueMouseDelta * CurrentPrecisionFactor;
	}

	void FToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		if (!bIsToolActive) return;

		if (!Viewport || !ViewportClient || !VirtualPivot) return;

		CurrentViewportMousePos = CurrentViewportMousePosition;
		HandleMouseMovement(CurrentViewportMousePosition);
	}

	void FToolBase::OnEnd(const bool bApply)
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

		if (!bApply)
		{
			VirtualPivot->RevertToStartState();
		}
		VirtualPivot->GetTransformProxy()->EndTransformEditSequence();

		//Force the editor to redraw gizmos so that they are up to date 
		if (GEditor)
		{
			GEditor->NoteSelectionChange(/*bNotify=*/true);
			GEditor->RedrawLevelEditingViewports(/*bInvalidateHitProxies:*/true);
		}

		if (Session->NumericInputProcessor.IsValid())
		{
			Session->NumericInputProcessor->OnExitNumericMode.Unbind();
		}
	}

	void FToolBase::SetPrecisionModeActive(bool bNewPrecisionModeActive)
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

	void FToolBase::SetSnappingEnabled(bool bNewSnappingEnabled)
	{
		checkf(OwningSession.IsValid(), TEXT("SetSnappingEnabled: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (bSnappingEnabled == bNewSnappingEnabled)
		{
			return;
		}

		bSnappingEnabled = bNewSnappingEnabled;

		if (!Session->IsNumericInputActive())
		{
			OnActive(CurrentMousePosition);
		}
	}

	void FToolBase::StartNewLock(const EAxisLock NewAxis) const
	{
		checkf(OwningSession.IsValid(), TEXT("StartNewLock: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		Session->bIsAxisLockActive = true;
		Session->bUsingLocalSpace = bLocalSpaceDefault;
		Session->LockedAxis = NewAxis;
	}

	void FToolBase::HandleAxisLock(const EAxisLock AxisPressed)
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

	bool FToolBase::IsSingleAxisLocked() const
	{
		checkf(OwningSession.IsValid(), TEXT("IsSingleAxisLocked: Session must be valid for %s"), *DisplayName);
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (Session->bIsAxisLockActive && GrabContext.HelperAxisDir != FVector::ZeroVector)
		{
			return true;
		}
		return false;
	}

	void FToolBase::Accept()
	{
		// if (ParentTxn)
		// {
		// 	ParentTxn.Reset();
		// }

		OnEnd(/*bApply=*/true);
	}

	void FToolBase::Cancel()
	{
		if (!GEditor || !VirtualPivot)
		{
			return;
		}

		GEditor->SetSelectionOutlineColor(CachedSelectionColor);
		OnEnd(/*bApply=*/false);
	}

	void FToolBase::UpdateHud()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid() || !Session->NumericInputProcessor.IsValid() || !HudWidget.IsValid())
		{
			return;
		}

		const TUniquePtr<FNumericInputProcessor>& Processor = Session->NumericInputProcessor;

		if (Processor->IsInNumericMode())
		{
			HudWidget->Update(GetNumericHudText());
		}
		else
		{
			HudWidget->Update(GetLiveHudText());
		}
	}

	FVector FToolBase::GetSnapOffset(const FVector OffsetFromStart)
	{
		return FVector::ZeroVector;
	}
} // namespace BlenderControls
