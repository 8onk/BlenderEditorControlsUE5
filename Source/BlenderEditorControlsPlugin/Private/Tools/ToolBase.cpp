#include "Tools/ToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "LevelEditorViewport.h"
#include "TransformSession.h"
#include "BaseGizmos/TransformProxy.h"
#include "Components/LineBatchComponent.h" //This is needed, although it's marked as unneeded mistakenly by the IDE. 
#include "Input/Numeric/NumericInputProcessor.h"
#include "BlenderControlsSettings.h"
#include "Utils/MathHelpers.h"
#include "Tools/SharedPivot.h"
#include "UI/AxisLockGizmoComponent.h"
#include "UI/TransformHUD.h"

namespace BlenderControls
{
	FToolBase::FToolBase(const TSharedRef<FTransformSession>& InSession, ETransformMode InMode,
	                     const FString& InDisplayName)
		: OwningSession(InSession), Mode(InMode), DisplayName(InDisplayName), NumNumericSlots(3)
	{
	}

	void FToolBase::OnBegin()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid())
		{
			return;
		}

		if (!InitializeEditorState())
		{
			return;
		}

		Viewport = ViewportClient->Viewport;
		if (!Viewport)
		{
			return;
		}
		CacheViewVectors();

		const FVector2D MousePos = Session->StartMousePos;
		CurrentMousePosition = MousePos;
		CurrentViewportMousePos = MousePos;

		InitializePivot();
		InitializeGrabContext();
		InitializeUI();
		RestorePreviousState();

		if (Session->NumericInputProcessor.IsValid())
		{
			Session->NumericInputProcessor->OnExitNumericMode.BindSP(AsShared(), &FToolBase::OnExitNumericMode);
		}
	}

	void FToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		if (!bIsToolActive || !GetSession().IsValid() || !ViewportClient || !VirtualPivot.IsValid())
		{
			return;
		}

		CurrentViewportMousePos = CurrentViewportMousePosition;
		HandleMouseMovement(CurrentViewportMousePosition);
	}

	void FToolBase::OnEnd(const bool bApply)
	{
		bIsToolActive = false;

		if (HudWidget.IsValid())
		{
			HudWidget->Detach();
		}

		ClearDrawnAxisLines();

		if (ViewportClient)
		{
			ViewportClient->SetWidgetMode(InitialWidgetMode);
			ViewportClient->ShowWidget(true);
			ViewportClient->SetRequiredCursorOverride(false, EMouseCursor::Default);
			ViewportClient->Invalidate();
		}

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);
		}

		const TSharedPtr<FTransformSession> Session = GetSession();
		if (Session.IsValid() && Session->GetNumericInputProcessor())
		{
			Session->GetNumericInputProcessor()->OnExitNumericMode.Unbind();
		}

		bPrecisionModeActive = false;
		CurrentPrecisionFactor = 1.0f;
		MouseDelta = FVector2D::ZeroVector;
		CurrentMousePosition = FVector2D::ZeroVector;

		if (!Session.IsValid())
		{
			return;
		}

		if (GEditor)
		{
			if (!Session->GetWrappedCursorPos().IsNearlyZero() && Viewport)
			{
				Viewport->SetMouse(static_cast<int32>(Session->GetWrappedCursorPos().X),
				                   static_cast<int32>(Session->GetWrappedCursorPos().Y));
			}

			//Reverts object transform to start transform before transform operation on cancel
			if (!bApply && VirtualPivot.IsValid())
			{
				VirtualPivot->RevertToStartState();
			}

			// Force viewport redraw
			GEditor->NoteSelectionChange(true);
			GEditor->RedrawLevelEditingViewports(true);
		}

		VirtualPivot->GetTransformProxy()->EndTransformEditSequence();
	}

	void FToolBase::UpdateAxisLock()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		UpdateNumActiveSlots();

		Session->GetNumericInputProcessor()->UpdateActiveNumSlots(NumNumericSlots);

		SetGrabContextAxisLock(Session->LockedAxis);
		RedrawAxisLines();

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

	void FToolBase::UpdateNumActiveSlots()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		switch (Session->GetLockedAxis())
		{
		case EAxisLock::X:
		case EAxisLock::Y:
		case EAxisLock::Z:
			NumNumericSlots = 1;
			break;

		case EAxisLock::XY:
		case EAxisLock::XZ:
		case EAxisLock::YZ:
			NumNumericSlots = 2;
			break;

		case EAxisLock::All:
		default:
			NumNumericSlots = 3;
			break;
		}
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

				// Highlight the gizmo for the "Active Element" while dimming others to mimic Blender's 
				// visual feedback when transforming multiple objects in local space.
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

			constexpr float Length = WORLD_MAX;

			if (UAxisLockGizmoComponent* Comp = SpawnAxisGizmo(Origin, AxisDir, Color,
			                                                   GetDefault<UBlenderControlsSettings>()->
			                                                   AxisLineThickness, Length))
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
		const UBlenderControlsSettings* Settings = GetDefault<UBlenderControlsSettings>();

		switch (InAxis)
		{
		case EAxisLock::X:
			return Settings->AxisColorX;
		case EAxisLock::Y:
			return Settings->AxisColorY;
		case EAxisLock::Z:
			return Settings->AxisColorZ;
		default:
			return FLinearColor::White;
		}
	}

	UAxisLockGizmoComponent* FToolBase::SpawnAxisGizmo(const FVector& Origin, const FVector& AxisDir,
	                                                   const FLinearColor& Color, float ThicknessPx,
	                                                   float LineLength) const
	{
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

		InitialWidgetMode = ViewportClient->GetWidgetMode();
		ViewportClient->ShowWidget(false);
		ViewportClient->Invalidate();

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
				ViewForward = FVector::DownVector;
				break;
			case LVT_OrthoNegativeXY:
				ViewForward = FVector::UpVector;
				break;
			case LVT_OrthoXZ:
				ViewForward = FVector::LeftVector;
				break;
			case LVT_OrthoNegativeXZ:
				ViewForward = FVector::RightVector;
				break;
			case LVT_OrthoYZ:
				ViewForward = FVector::ForwardVector;
				break;
			case LVT_OrthoNegativeYZ:
				ViewForward = -FVector::ForwardVector;
				break;
			default:
				ViewForward = FVector::ForwardVector;
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

		GrabContext.ConstraintMode = FGrabContext::EHelperType::ViewPlane;
		GrabContext.PlaneNormal = -ViewForward;
		GrabContext.StartMousePos = Session->StartMousePos;
		GrabContext.StartLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();
		GrabContext.SingleLockAxis = FVector::ZeroVector;
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

		// Deproject two points (current mouse and mouse + 1px) to calculate the world-space size of a single pixel.
		// This allows the tool to maintain consistent drag sensitivity/distance regardless of the user's FOV or distance from the object.
		GrabContext.ScreenToWorldScale = FVector::Dist(MouseIntersectionA, MouseIntersectionB);
	}

	void FToolBase::InitializeUI()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		HudWidget = SNew(STransformHUD);
		HudWidget->Attach();
		UpdateHud();
		UpdateNumActiveSlots();

		ViewportClient->SetRequiredCursorOverride(false, EMouseCursor::None);
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		HudWidget->SetVirtualCursorPos(Session->GetWrappedCursorPos());
		//Needed since CurrentViewportMousePosition - Session->CursorAnchorPoint; in onactive
		Session->VirtualMousePosition = Session->CursorAnchorPoint;
	}

	void FToolBase::RestorePreviousState()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();

		SetGrabContextAxisLock(Session->GetLockedAxis());
		if (Session->GetLockedAxis() != EAxisLock::All)
		{
			ClearDrawnAxisLines();
			UpdateAxisLock();
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
		if (TrueMouseDelta.IsNearlyZero())
		{
			return;
		}

		Session->VirtualMousePosition += TrueMouseDelta;

		// Lock the hardware cursor to a fixed anchor and track a 'VirtualMousePosition' instead.
		// This allows for mouse wrap without drift (mismatch between software cursor and hardware cursor position)
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

			HudWidget->SetVirtualCursorPos(Session->WrappedMousePosition);
		}

		MouseDelta += TrueMouseDelta * CurrentPrecisionFactor;
	}

	void FToolBase::OnSwitch()
	{
		ClearDrawnAxisLines();

		if (HudWidget.IsValid())
		{
			HudWidget->Detach();
		}

		if (ViewportClient)
		{
			ViewportClient->Invalidate();
		}
	}

	void FToolBase::SetPrecisionModeActive(bool bNewPrecisionModeActive)
	{
		if (bNewPrecisionModeActive == bPrecisionModeActive)
		{
			return;
		}

		bPrecisionModeActive = bNewPrecisionModeActive;
		if (bPrecisionModeActive)
		{
			CurrentPrecisionFactor = GetDefault<UBlenderControlsSettings>()->PrecisionScalar;
		}
		else
		{
			CurrentPrecisionFactor = 1.0f;
		}
	}

	void FToolBase::SetSnappingEnabled(bool bNewSnappingEnabled)
	{
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
		const TSharedPtr<FTransformSession> Session = GetSession();

		Session->bIsAxisLockActive = true;
		Session->bUsingLocalSpace = bLocalSpaceDefault;
		Session->LockedAxis = NewAxis;
	}

	void FToolBase::HandleAxisLock(const EAxisLock AxisPressed)
	{
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
		const TSharedPtr<FTransformSession> Session = GetSession();

		if (Session->bIsAxisLockActive && GrabContext.SingleLockAxis != FVector::ZeroVector)
		{
			return true;
		}
		return false;
	}

	void FToolBase::Accept()
	{
		OnEnd(/*bApply=*/true);
	}

	void FToolBase::Cancel()
	{
		if (!GEditor || !VirtualPivot)
		{
			return;
		}

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
} // namespace BlenderControls
