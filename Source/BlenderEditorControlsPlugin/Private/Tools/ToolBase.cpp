// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#include "Tools/ToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "TransformSession.h"
#include "Components/LineBatchComponent.h" //This is needed, although it's marked as unneeded mistakenly by the IDE. 
#include "Input/Numeric/NumericInputProcessor.h"
#include "BlenderControlsSettings.h"
#include "BlenderEditorControls.h"
#include "Utils/MathHelpers.h"
#include "Pivots/ControlRigPivot.h"
#include "UI/AxisLockGizmoComponent.h"
#include "UI/TransformHUD.h"
#include "SEditorViewport.h"
#include "Utils/ViewportExposer.h"

namespace BlenderControls
{
	FToolBase::FToolBase(const TSharedRef<FTransformSession>& InSession, ETransformMode InMode,
	                     const FString& InDisplayName)
		: Session(&InSession.Get()), Mode(InMode), DisplayName(InDisplayName), NumNumericSlots(3)
	{
		ViewportClient = Session->GetActiveViewportClient();
		if (!ViewportClient)
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: ViewportClient is null."), __FUNCTION__);
		}
		if (!ViewportClient->Viewport)
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: Viewport is null."), __FUNCTION__);
		}
		Viewport = ViewportClient->Viewport;
	}

	bool FToolBase::OnBegin()
	{
		if (!GEditor)
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: GEditor is null."), __FUNCTION__);
			return false;
		}

		InitializeEditorState();

		if (!CacheViewVectors())
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: CacheViewVectors failed!"), __FUNCTION__);
			return false;
		}

		const FVector2D MousePos = Session->StartMousePos;
		CurrentMousePosition = MousePos;
		CurrentViewportMousePos = MousePos;

		if (!InitializePivot())
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: InitializePivot failed!"), __FUNCTION__);
			return false;
		}
		if (!InitializeGrabContext())
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: InitializeGrabContext failed!"), __FUNCTION__);
			return false;
		}
		if (!InitializeUI())
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: InitializeUI failed!"), __FUNCTION__);
			return false;
		}
		RestoreAxisLock();

		if (Session->NumericInputProcessor.IsValid())
		{
			Session->NumericInputProcessor->OnExitNumericMode.BindSP(AsShared(), &FToolBase::OnExitNumericMode);
		}

		const TSharedPtr<SWindow> SlateWindow = FSlateApplication::Get().FindWidgetWindow(
			ViewportClient->GetEditorViewportWidget().ToSharedRef());
		if (SlateWindow.IsValid() && SlateWindow->GetNativeWindow().IsValid())
		{
			FSlateApplication::Get().GetPlatformApplication()->SetHighPrecisionMouseMode(
				true, SlateWindow->GetNativeWindow());
		}

		// This is necessary to sync blueprint preview in content browser, just like native engine behaviour. The
		// axis argument must be All, for the content browser blueprint preview to sync with the changes. However.
		// for ue 5.8, it must be None, otherwise transformation won't work at all. To make synchronization work for ue 5.8,
		// set EAxisList::None for blueprint viewport (as it does not support internal snapping anyway,
		// and InputWidgetDelta() is bypassed for blueprint viewport anyway).
		// NOTE preview becomes out of sync when undoing a transaction, but this is how the native editor behaves as well.
		if (Session->GetSelectionType() == ESelectionType::SCSTreeNodes)
		{
			ViewportClient->SetCurrentWidgetAxis(EAxisList::All);
		}
		else
		{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 6
			ViewportClient->SetCurrentWidgetAxis(EAxisList::None);
#else
			ViewportClient->SetCurrentWidgetAxis(EAxisList::All);
#endif
		}

		// StartTrackingDueToInput properly configures the engine's internal tracking state.
		// This improves performance considerably  (only if switch widget mode, and do not re-click 
		// the object before transforming it) and configures "pending autosave" behaviour Not sure why this improves performance
		// though. However, for FSCSViewportClient (blueprint viewport), it starts a custom transaction which we do not want.
		// This is handled though by calling FTransformSession::SwitchTool OnSwitch. 
		FViewportClientExposer::CallStartTracking(
			ViewportClient,
			FInputEventState(Viewport, EKeys::LeftMouseButton, IE_Pressed),
			*static_cast<FSceneView*>(nullptr));

		return true;
	}

	void FToolBase::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		if (!bIsToolActive)
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: Tool inactive!"),
			       __FUNCTION__);
			return;
		}
		if (!HudWidget.IsValid())
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: Invalid HudWidget!"),
			       __FUNCTION__);
			return;
		}

		CurrentViewportMousePos = CurrentViewportMousePosition;
		HandleMouseMovement(CurrentViewportMousePosition);
	}

	void FToolBase::OnEnd(const bool bApply)
	{
		bIsToolActive = false;

		if (!HudWidget.IsValid())
		{
			UE_LOG(LogBlenderEditorControls, Error,
			       TEXT("[%hs]: Invalid HudWidget! Likely due to missed OnBegin() call"),
			       __FUNCTION__);
		}
		else
		{
			HudWidget->Detach();
		}

		ClearDrawnAxisLines();

		constexpr bool bIsToolEnding = true;
		SetViewportState(bIsToolEnding);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().GetPlatformApplication()->SetHighPrecisionMouseMode(false, nullptr);
			FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);
		}

		if (Session->GetNumericInputProcessor())
		{
			Session->GetNumericInputProcessor()->OnExitNumericMode.Unbind();
		}

		bPrecisionModeActive = false;
		CurrentPrecisionFactor = 1.0f;
		CurrentMousePosition = FVector2D::ZeroVector;

		if (!Session->GetWrappedCursorPos().IsNearlyZero())
		{
			Viewport->SetMouse(static_cast<int32>(Session->GetWrappedCursorPos().X),
			                   static_cast<int32>(Session->GetWrappedCursorPos().Y));
		}

		if (!bApply && VirtualPivot.IsValid())
		{
			VirtualPivot->RevertTransformToStartState();
		}

		if (GEditor)
		{
			if (!Session->IsControlRigSelection())
			{
				GEditor->NoteSelectionChange(true);
			}
			GEditor->RedrawLevelEditingViewports(true);
		}
		else
		{
			UE_LOG(LogBlenderEditorControls, Error,
			       TEXT("[%hs]: GEditor not valid!"),
			       __FUNCTION__);
		}

		if (VirtualPivot.IsValid())
		{
			VirtualPivot->EndTransformSequence();
		}

		FViewportClientExposer::CallStopTracking(ViewportClient);
		Viewport->Invalidate();
	}

	void FToolBase::InitializeEditorState()
	{
		if (Session->bIsFirstTool)
		{
			Session->InitialWidgetMode = static_cast<int32>(ViewportClient->GetWidgetMode());
		}

		constexpr bool bIsToolEnding = false;
		SetViewportState(bIsToolEnding);
		bLocalSpaceDefault = GLevelEditorModeTools().GetCoordSystem() == COORD_Local;
		GEditor->SetSelectionOutlineColor(FLinearColor::White);
	}

	void FToolBase::SetViewportState(bool bIsToolEnding) const
	{
		const UE::Widget::EWidgetMode InitialMode = static_cast<UE::Widget::EWidgetMode>(Session->InitialWidgetMode);

		ViewportClient->SetWidgetMode(bIsToolEnding ? InitialMode : GetDesiredWidgetMode());
		if (ViewportClient->GetModeTools())
		{
			ViewportClient->GetModeTools()->SetWidgetMode(bIsToolEnding ? InitialMode : GetDesiredWidgetMode());
		}
		ViewportClient->ShowWidget(bIsToolEnding);

		Viewport->CaptureMouse(!bIsToolEnding);
		Viewport->LockMouseToViewport(!bIsToolEnding);
		ViewportClient->SetRequiredCursorOverride(!bIsToolEnding, EMouseCursor::None);
	}

	void FToolBase::UpdateAxisLock()
	{
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
			if (Giz.IsValid())
			{
				Giz->DestroyComponent();
			}
		}

		AxisGizmos.Empty();
	}

	void FToolBase::RedrawAxisLines()
	{
		ClearAxisGizmos();

		// Lambda for adding axis gizmos - works with optional transform info
		auto AddAxisWithTransform = [&](EAxisLock Axis, const FTransform* ElementTransform, bool bIsActiveElement)
		{
			FVector GizmoOriginPoint, GizmoDir;
			// For local space, draw gizmo aligned with the local axis' of the object. 
			if (ElementTransform && Session->bUsingLocalSpace)
			{
				GizmoOriginPoint = ElementTransform->GetLocation();
				const FVector Local =
					(Axis == EAxisLock::X)
						? FVector::XAxisVector
						: (Axis == EAxisLock::Y)
						? FVector::YAxisVector
						: FVector::ZAxisVector;
				GizmoDir = ElementTransform->TransformVectorNoScale(Local);
			}
			// For global space axis lock, use the shared pivot position. 
			else
			{
				GizmoOriginPoint = GetPivotStartLocation();
				GizmoDir = GetAxisVector(Axis);
			}

			FLinearColor Color;
			const FLinearColor BaseColor = GetAxisColor(Axis);

			// Highlight the gizmo for the "Active Element" while dimming others to mimic Blender's 
			// visual feedback when transforming multiple objects in local space.
			if (bIsActiveElement)
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

			if (UAxisLockGizmoComponent* Comp = SpawnAxisGizmo(GizmoOriginPoint, GizmoDir, Color,
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
			if (VirtualPivot.IsValid())
			{
				VirtualPivot->ForEachElementTransform([&](const FTransform& StartTransform, bool bIsActive)
				{
					AddAxisForLock(Session->LockedAxis, &StartTransform, bIsActive);
				});
			}
		}
		else
		{
			// Global space - just add axes at pivot location
			AddAxisForLock(Session->LockedAxis, /*Transform=*/nullptr, /*bIsActive=*/true);
		}
	}

	void FToolBase::ClearAxisGizmos()
	{
		for (auto& Giz : AxisGizmos)
		{
			if (Giz.IsValid())
			{
				Giz->DestroyComponent();
			}
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
		UWorld* World = ViewportClient->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		UAxisLockGizmoComponent* Comp = NewObject<UAxisLockGizmoComponent>(GetTransientPackage());

		const EViewModeIndex ViewMode = ViewportClient->GetViewMode();
		Comp->bIsWireframeView = (ViewMode == VMI_Wireframe || ViewMode == VMI_BrushWireframe) || ViewportClient->
			IsOrtho();

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

	bool FToolBase::InitializePivot()
	{
		VirtualPivot = Session->GetPivot();
		if (VirtualPivot.IsValid())
		{
			VirtualPivot->BeginTransformSequence();
			return true;
		}

		UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: VirtualPivot is invalid!"), __FUNCTION__);
		return false;
	}

	bool FToolBase::CacheViewVectors()
	{
		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		if (!SceneView)
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: CalcSceneView returned null."), __FUNCTION__);
			return false;
		}
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

		return true;
	}

	bool FToolBase::InitializeGrabContext()
	{
		GrabContext.ConstraintMode = FGrabContext::EHelperType::ViewPlane;
		GrabContext.PlaneNormal = -ViewForward;
		GrabContext.StartMousePos = Session->StartMousePos;
		GrabContext.StartLocation = GetActiveElementStartLocation();
		GrabContext.SingleLockAxis = FVector::ZeroVector;
		GrabContext.ViewForward = ViewForward;

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags));

		const FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		if (!SceneView)
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: CalcSceneView returned null."), __FUNCTION__);
			return false;
		}

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

		return true;
	}

	bool FToolBase::InitializeUI()
	{
		HudWidget = SNew(STransformHUD);
		if (!HudWidget.IsValid())
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: HudWidget is invalid!"), __FUNCTION__);
			return false;
		}
		HudWidget->Attach(ViewportClient->GetEditorViewportWidget());
		UpdateHud();
		UpdateNumActiveSlots();

		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		HudWidget->SetVirtualCursorPos(Session->GetWrappedCursorPos());

		if (Session->bIsFirstTool)
		{
			//Needed since CurrentViewportMousePosition - Session->CursorAnchorPoint; in onactive
			Session->VirtualMousePosition = Session->CursorAnchorPoint;
		}

		return true;
	}

	void FToolBase::RestoreAxisLock()
	{
		SetGrabContextAxisLock(Session->GetLockedAxis());
		if (Session->GetLockedAxis() != EAxisLock::All)
		{
			ClearDrawnAxisLines();
			UpdateAxisLock();
		}
	}


	void FToolBase::HandleMouseMovement(const FVector2D& CurrentViewportMousePosition)
	{
		const FVector2D CurrentGlobalPos = FSlateApplication::Get().GetCursorPos();
		const FVector2D CurrentMouseDelta = CurrentGlobalPos - Session->GlobalCursorAnchor;

		if (CurrentMouseDelta.IsNearlyZero())
		{
			return;
		}

		Session->VirtualMousePosition += CurrentMouseDelta;

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
		else
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: HudWidget is invalid!"), __FUNCTION__);
		}

		Session->AccumulatedMouseDelta += CurrentMouseDelta * CurrentPrecisionFactor;
	}

	void FToolBase::OnSwitch()
	{
		if (!HudWidget.IsValid())
		{
			UE_LOG(LogBlenderEditorControls, Error, TEXT("[%hs]: HudWidget is invalid!"), __FUNCTION__);
			return;
		}
		ClearDrawnAxisLines();
		HudWidget->Detach();
		ViewportClient->Invalidate();
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
		Session->bIsAxisLockActive = true;
		Session->bUsingLocalSpace = bLocalSpaceDefault;
		Session->LockedAxis = NewAxis;
	}

	void FToolBase::HandleAxisLock(const EAxisLock AxisPressed)
	{
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
		OnEnd(/*bApply=*/false);
	}

	void FToolBase::UpdateHud()
	{
		if (!Session->NumericInputProcessor.IsValid() || !HudWidget.IsValid())
		{
			return;
		}

		const TUniquePtr<FNumericInputProcessor>& Processor = Session->NumericInputProcessor;

		const TSharedPtr<SEditorViewport> GenericViewportWidget = ViewportClient->GetEditorViewportWidget();

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

	FVector FToolBase::GetActiveElementStartLocation() const
	{
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetActiveElementStartTransform().GetLocation();
		}
		return FVector::ZeroVector;
	}

	FTransform FToolBase::GetActiveElementStartTransform() const
	{
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetActiveElementStartTransform();
		}
		return FTransform::Identity;
	}

	FVector FToolBase::GetActiveElementCurrentLocation() const
	{
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetActiveElementCurrentLocation();
		}
		return FVector::ZeroVector;
	}

	FVector FToolBase::GetPivotStartLocation() const
	{
		if (VirtualPivot.IsValid())
		{
			return VirtualPivot->GetStartLocation();
		}
		return FVector::ZeroVector;
	}
} // namespace BlenderControls
