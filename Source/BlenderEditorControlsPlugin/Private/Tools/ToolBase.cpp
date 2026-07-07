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
#include "ControlRig/ControlRigPivot.h"
#include "UI/AxisLockGizmoComponent.h"
#include "UI/TransformHUD.h"
#include "SEditorViewport.h"

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

			ViewportClient->TrackingStarted(FInputEventState(Viewport, EKeys::LeftMouseButton, IE_Pressed), true,
			                                false);
	}

	void FToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		//#TODO HAS VALID PIVOT INTERNAL BREAKS SCSEDITOR (MOUSE MOVEMENT IS SKIPPED FOR IT). Do Something about it
		if (!bIsToolActive || !GetSession().IsValid() || !ViewportClient)
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

			// Reverts object transform to start transform before transform operation on cancel
			if (!bApply)
			{
				RevertElementsToStartState();
			}

			// Force viewport redraw
			GEditor->NoteSelectionChange(true);
			GEditor->RedrawLevelEditingViewports(true);
		}

		// End transform proxy sequence for actor-based transforms
		if (VirtualPivot.IsValid() && VirtualPivot->GetTransformProxy())
		{
			VirtualPivot->GetTransformProxy()->EndTransformEditSequence();
		}

		ViewportClient->TrackingStopped();
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

		// Lambda for adding axis gizmos - works with optional transform info
		auto AddAxisWithTransform = [&](EAxisLock Axis, const FTransform* ElementTransform, bool bIsActiveElement)
		{
			FVector Origin, AxisDir;
			if (ElementTransform && Session->bUsingLocalSpace)
			{
				Origin = ElementTransform->GetLocation();
				const FVector Local =
					(Axis == EAxisLock::X)
						? FVector::XAxisVector
						: (Axis == EAxisLock::Y)
						? FVector::YAxisVector
						: FVector::ZAxisVector;
				AxisDir = ElementTransform->TransformVectorNoScale(Local);
			}
			else
			{
				Origin = GetPivotStartLocation();
				AxisDir = GetAxisVector(Axis);
			}

			FLinearColor Color;
			const FLinearColor BaseColor = GetAxisColor(Axis);

			// Highlight the gizmo for the "Active Element" while dimming others to mimic Blender's 
			// visual feedback when transforming multiple objects in local space.
			if (bIsActiveElement || !Session->IsUsingLocalSpace())
			{
				Color = BaseColor * 2.0f;
				Color.A = 1.0f;
			}
			else
			{
				Color = BaseColor * 0.3f;
				Color.A = 0.7f;
			}

			constexpr float Length = WORLD_MAX;

			if (UAxisLockGizmoComponent* Comp = SpawnAxisGizmo(Origin, AxisDir, Color,
			                                                   GetDefault<UBlenderControlsSettings>()->
			                                                   AxisLineThickness, Length))
			{
				AxisGizmos.Add(Comp);
			}
		};

		auto AddAxisForLock = [&](EAxisLock LockedAxis, const FTransform* Transform, bool bIsActive)
		{
			switch (LockedAxis)
			{
			case EAxisLock::XY:
				AddAxisWithTransform(EAxisLock::X, Transform, bIsActive);
				AddAxisWithTransform(EAxisLock::Y, Transform, bIsActive);
				break;
			case EAxisLock::XZ:
				AddAxisWithTransform(EAxisLock::X, Transform, bIsActive);
				AddAxisWithTransform(EAxisLock::Z, Transform, bIsActive);
				break;
			case EAxisLock::YZ:
				AddAxisWithTransform(EAxisLock::Y, Transform, bIsActive);
				AddAxisWithTransform(EAxisLock::Z, Transform, bIsActive);
				break;
			case EAxisLock::All:
				break;
			default:
				AddAxisWithTransform(LockedAxis, Transform, bIsActive);
				break;
			}
		};

		if (Session->IsUsingLocalSpace())
		{
			if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
			{
				// Control Rig elements
				const FControlRigElementInfo& ActiveElement = ControlRigVirtualPivot->GetActiveElement();
				for (const FControlRigElementInfo& Element : ControlRigVirtualPivot->GetElements())
				{
					if (!Element.IsValid()) continue;
					const bool bIsActive = (Element.ElementKey == ActiveElement.ElementKey);
					AddAxisForLock(Session->LockedAxis, &Element.StartTransform, bIsActive);
				}
			}
			else if (VirtualPivot.IsValid())
			{
				// Actor elements
				const FChildInfo& ActiveElement = VirtualPivot->GetActiveElement();
				for (const FChildInfo& Child : VirtualPivot->GetChildren())
				{
					if (!Child.Actor) continue;
					const bool bIsActive = (Child.Actor == ActiveElement.Actor);
					AddAxisForLock(Session->LockedAxis, &Child.Transform, bIsActive);
				}
			}
		}
		else
		{
			// Global space - just add axes at pivot location
			AddAxisForLock(Session->LockedAxis, nullptr, true);
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

		UWorld* World = ViewportClient->GetWorld();
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
			const FTransform ActiveTransform = GetActiveElementStartTransform();
			AxisVector = ActiveTransform.TransformVectorNoScale(AxisVector);
		}
		return AxisVector.GetSafeNormal();
	}

	bool FToolBase::InitializeEditorState()
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		ViewportClient = Session->GetActiveViewportClient();
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

		if (Session->IsControlRigSelection())
		{
			ControlRigVirtualPivot = Session->GetControlRigPivot();
			// Control Rig doesn't use TransformProxy - transforms are applied directly through the hierarchy
		}
		else
		{
			VirtualPivot = Session->GetPivot();
			if (VirtualPivot.IsValid() && VirtualPivot->GetTransformProxy())
			{
				VirtualPivot->GetTransformProxy()->BeginTransformEditSequence();
			}
		}
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
		// if (!HasValidPivotInternal())
		// {
		// 	return;
		// }

		const TSharedPtr<FTransformSession> Session = GetSession();
		if (!Session.IsValid())
		{
			return;
		}

		GrabContext.ConstraintMode = FGrabContext::EHelperType::ViewPlane;
		GrabContext.PlaneNormal = -ViewForward;
		GrabContext.StartMousePos = Session->StartMousePos;
		GrabContext.StartLocation = GetActiveElementStartLocation();
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
		HudWidget->Attach(ViewportClient->GetEditorViewportWidget());
		UpdateHud();
		UpdateNumActiveSlots();

		ViewportClient->SetRequiredCursorOverride(false, EMouseCursor::None);
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		HudWidget->SetVirtualCursorPos(Session->GetWrappedCursorPos());
		
		if (Session->bIsFirstTool)
		{
			//Needed since CurrentViewportMousePosition - Session->CursorAnchorPoint; in onactive
			Session->VirtualMousePosition = Session->CursorAnchorPoint;
		}
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

		const FVector2D CurrentGlobalPos = FSlateApplication::Get().GetCursorPos();
		const FVector2D TrueMouseDelta = CurrentGlobalPos - Session->GlobalCursorAnchor;

		// UE_LOG(LogTemp, Warning, TEXT("[HandleMouseMovement] GlobalPos: %s | GlobalAnchor: %s | TrueDelta: %s"),
		//        *CurrentGlobalPos.ToString(),
		//        *Session->GlobalCursorAnchor.ToString(),
		//        *TrueMouseDelta.ToString());

		if (TrueMouseDelta.IsNearlyZero())
		{
			return;
		}

		Session->VirtualMousePosition += TrueMouseDelta;

		// Force the hardware cursor back globally to perfectly lock it
		FSlateApplication::Get().GetPlatformApplication()->Cursor->SetPosition(
			static_cast<int32>(Session->GlobalCursorAnchor.X),
			static_cast<int32>(Session->GlobalCursorAnchor.Y));

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
		if (!GEditor || !HasValidPivotInternal())
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

		TSharedPtr<SEditorViewport> GenericViewportWidget = ViewportClient->GetEditorViewportWidget();

		if (Processor->IsInNumericMode())
		{
			HudWidget->Update(GenericViewportWidget, GetNumericHudText());
		}
		else
		{
			HudWidget->Update(GenericViewportWidget, GetLiveHudText());
		}
	}

	// --- Pivot Helper Method Implementations ---

	bool FToolBase::IsControlRigMode() const
	{
		const TSharedPtr<FTransformSession> Session = GetSession();
		return Session.IsValid() && Session->IsControlRigSelection();
	}

	bool FToolBase::HasValidPivotInternal() const
	{
		return true;
		// if (IsControlRigMode())
		// {
		// 	return ControlRigVirtualPivot.IsValid() && ControlRigVirtualPivot->IsValid();
		// }
		// return VirtualPivot.IsValid();
	}

	FVector FToolBase::GetActiveElementStartLocation() const
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			return ControlRigVirtualPivot->GetActiveElement().StartTransform.GetLocation();
		}
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetActiveElement().Transform.GetLocation();
		}
		return FVector::ZeroVector;
	}

	FTransform FToolBase::GetActiveElementStartTransform() const
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			return ControlRigVirtualPivot->GetActiveElement().StartTransform;
		}
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetActiveElement().Transform;
		}
		return FTransform::Identity;
	}

	FVector FToolBase::GetActiveElementCurrentLocation() const
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			// For Control Rig, get the current transform from the hierarchy
			const FControlRigElementInfo& Element = ControlRigVirtualPivot->GetActiveElement();
			return FControlRigSelectionHelper::GetElementGlobalTransform(Element.ElementKey).GetLocation();
		}
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetActiveElement().Actor->GetActorLocation();
		}
		return FVector::ZeroVector;
	}

	FVector FToolBase::GetPivotStartLocation() const
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			return ControlRigVirtualPivot->GetStartLocation();
		}
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetStartTransform().GetLocation();
		}
		return FVector::ZeroVector;
	}

	void FToolBase::TranslateElements(const FVector& Delta, bool bUsingLocalSpace)
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			ControlRigVirtualPivot->Translate(Delta, bUsingLocalSpace);
		}
		else if (VirtualPivot.IsValid())
		{
			VirtualPivot->Translate(Delta, bUsingLocalSpace);
		}
	}

	void FToolBase::TranslateElements(bool bUsingLocalSpace, EAxisLock LockedAxis, const FVector& Delta)
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			ControlRigVirtualPivot->Translate(bUsingLocalSpace, LockedAxis, Delta);
		}
		else if (VirtualPivot.IsValid())
		{
			VirtualPivot->Translate(bUsingLocalSpace, LockedAxis, Delta);
		}
	}

	void FToolBase::RotateElements(float AngleRad, bool bUsingLocalSpace, EAxisLock LockedAxis)
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			ControlRigVirtualPivot->Rotate(GrabContext, AngleRad, bUsingLocalSpace, LockedAxis);
		}
		else if (VirtualPivot.IsValid())
		{
			VirtualPivot->Rotate(GrabContext, AngleRad, bUsingLocalSpace, LockedAxis);
		}
	}

	void FToolBase::ScaleElements(const FVector& ScaleMultiplier, bool bUsingLocalSpace)
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			ControlRigVirtualPivot->Scale(ScaleMultiplier, bUsingLocalSpace);
		}
		else if (VirtualPivot.IsValid())
		{
			VirtualPivot->Scale(ScaleMultiplier, bUsingLocalSpace);
		}
	}

	void FToolBase::RevertElementsToStartState()
	{
		if (IsControlRigMode() && ControlRigVirtualPivot.IsValid())
		{
			ControlRigVirtualPivot->RevertToStartState();
		}
		else if (VirtualPivot.IsValid())
		{
			VirtualPivot->RevertToStartState();
		}
	}
} // namespace BlenderControls
