#include "ControlRig/ControlRigSelectionHelper.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "ControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "Rigs/RigHierarchyElements.h"
#include "EditMode/ControlRigEditMode.h" // Path may vary by UE version

DEFINE_LOG_CATEGORY_STATIC(LogControlRigHelper, Log, All);

namespace BlenderControls
{
	FControlRigEditMode* FControlRigSelectionHelper::GetControlRigEditMode()
	{
		if (!GEditor)
		{
			return nullptr;
		}

		FEditorModeTools& ModeTools = GLevelEditorModeTools();
		FEdMode* ActiveMode = ModeTools.GetActiveMode(FControlRigEditMode::ModeName);
		
		return ActiveMode ? static_cast<FControlRigEditMode*>(ActiveMode) : nullptr;
	}

	bool FControlRigSelectionHelper::IsControlRigEditModeActive()
	{
		return GetControlRigEditMode() != nullptr;
	}

	int32 FControlRigSelectionHelper::GetSelectedRigElementCount()
	{
		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode)
		{
			return 0;
		}

		TMap<UControlRig*, TArray<FRigElementKey>> SelectedControls;
		EditMode->GetAllSelectedControls(SelectedControls);
		
		int32 TotalCount = 0;
		for (const auto& Pair : SelectedControls)
		{
			TotalCount += Pair.Value.Num();
		}
		return TotalCount;
	}

	bool FControlRigSelectionHelper::HasSelectedRigElements()
	{
		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode)
		{
			return false;
		}

		UControlRig* ControlRig = GetActiveControlRig();
		if (ControlRig)
		{
			// ValidControlTypeMask filters to Control-type elements only (excludes bones, nulls, etc.)
			return EditMode->AreRigElementsSelected(FControlRigEditMode::ValidControlTypeMask(), ControlRig);
		}

		return false;
	}

	bool FControlRigSelectionHelper::GetSelectedRigElements(TArray<FControlRigElementInfo>& OutElements)
	{
		OutElements.Empty();

		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode)
		{
			return false;
		}

		TMap<UControlRig*, TArray<FRigElementKey>> SelectedControls;
		EditMode->GetAllSelectedControls(SelectedControls);
		
		for (const auto& Pair : SelectedControls)
		{
			UControlRig* ControlRig = Pair.Key;
			if (!ControlRig)
			{
				continue;
			}

			URigHierarchy* Hierarchy = ControlRig->GetHierarchy();
			if (!Hierarchy)
			{
				continue;
			}

			// Control Rig transforms are in rig-local space; we need world space for tools
			FTransform HostingActorTransform = FTransform::Identity;
			if (AActor* HostingActor = ControlRig->GetHostingActor())
			{
				HostingActorTransform = HostingActor->GetActorTransform();
			}

			for (const FRigElementKey& Key : Pair.Value)
			{
				FControlRigElementInfo ElementInfo;
				ElementInfo.ElementKey = Key;
				ElementInfo.OwningControlRig = ControlRig;
				
				const FTransform RigSpaceTransform = Hierarchy->GetGlobalTransform(Key);
				ElementInfo.StartTransform = RigSpaceTransform * HostingActorTransform;
				ElementInfo.StartRotation = ElementInfo.StartTransform.GetRotation();
				
				OutElements.Add(ElementInfo);
			}
		}

		return OutElements.Num() > 0;
	}

	UControlRig* FControlRigSelectionHelper::GetActiveControlRig()
	{
		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode)
		{
			return nullptr;
		}

		// Multiple Control Rigs can be active simultaneously; we use the first one
		TArrayView<TWeakObjectPtr<UControlRig>> ControlRigs = EditMode->GetControlRigs();
		
		if (ControlRigs.Num() > 0 && ControlRigs[0].IsValid())
		{
			return ControlRigs[0].Get();
		}

		return nullptr;
	}

	URigHierarchy* FControlRigSelectionHelper::GetRigHierarchy()
	{
		UControlRig* ControlRig = GetActiveControlRig();
		if (ControlRig)
		{
			return ControlRig->GetHierarchy();
		}
		return nullptr;
	}

	void FControlRigSelectionHelper::SetElementGlobalTransform(
		const FRigElementKey& ElementKey,
		const FTransform& NewTransform,
		bool bInitial)
	{
		UControlRig* ControlRig = GetActiveControlRig();
		if (!ControlRig)
		{
			UE_LOG(LogControlRigHelper, Warning, TEXT("SetElementGlobalTransform: No Control Rig available"));
			return;
		}

		URigHierarchy* Hierarchy = ControlRig->GetHierarchy();
		if (!Hierarchy)
		{
			UE_LOG(LogControlRigHelper, Warning, TEXT("SetElementGlobalTransform: No hierarchy available"));
			return;
		}

		FTransform HostingActorTransform = FTransform::Identity;
		if (AActor* HostingActor = ControlRig->GetHostingActor())
		{
			HostingActorTransform = HostingActor->GetActorTransform();
		}
		const FTransform RigSpaceTransform = NewTransform.GetRelativeTransform(HostingActorTransform);

		if (ElementKey.Type == ERigElementType::Control)
		{
			// Configure the modification context
			// KeyMask = AllTransform ensures location, rotation, and scale are all considered
			FRigControlModifiedContext Context;
			Context.SetKey = EControlRigSetKey::DoNotCare;
			Context.KeyMask = (uint32)EControlRigContextChannelToKey::AllTransform;
			
			const bool bNotify = true;
			const bool bSetupUndo = false; // Undo handled by FScopedTransaction in calling code
			const bool bPrintPythonCommands = false;
			// bFixEulerFlips helps with rotation continuity for EulerTransform controls
			const bool bFixEulerFlips = true;
			
			ControlRig->SetControlGlobalTransform(
				ElementKey.Name,
				RigSpaceTransform,
				bNotify,
				Context,
				bSetupUndo,
				bPrintPythonCommands,
				bFixEulerFlips
			);
		}
		else
		{
			const bool bAffectChildren = true;
			const bool bPropagate = true;
			Hierarchy->SetGlobalTransform(ElementKey, RigSpaceTransform, bInitial, bAffectChildren, bPropagate);
		}
	}

	FTransform FControlRigSelectionHelper::GetElementGlobalTransform(const FRigElementKey& ElementKey)
	{
		UControlRig* ControlRig = GetActiveControlRig();
		if (!ControlRig)
		{
			UE_LOG(LogControlRigHelper, Warning, TEXT("GetElementGlobalTransform: No Control Rig available"));
			return FTransform::Identity;
		}

		URigHierarchy* Hierarchy = ControlRig->GetHierarchy();
		if (!Hierarchy)
		{
			UE_LOG(LogControlRigHelper, Warning, TEXT("GetElementGlobalTransform: No hierarchy available"));
			return FTransform::Identity;
		}

		const FTransform RigSpaceTransform = Hierarchy->GetGlobalTransform(ElementKey);
		FTransform HostingActorTransform = FTransform::Identity;
		if (AActor* HostingActor = ControlRig->GetHostingActor())
		{
			HostingActorTransform = HostingActor->GetActorTransform();
		}
		return RigSpaceTransform * HostingActorTransform;
	}

	FVector FControlRigSelectionHelper::ComputeMedianPivotLocation()
	{
		TArray<FControlRigElementInfo> Elements;
		if (!GetSelectedRigElements(Elements) || Elements.Num() == 0)
		{
			return FVector::ZeroVector;
		}

		FVector Accum = FVector::ZeroVector;
		for (const FControlRigElementInfo& Element : Elements)
		{
			Accum += Element.StartTransform.GetLocation();
		}

		return Accum / Elements.Num();
	}

	void FControlRigSelectionHelper::BeginTransaction(const FText& TransactionName)
	{
		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode)
		{
			return;
		}

		// Mark the Control Rig as modified to participate in the undo transaction
		UControlRig* ControlRig = GetActiveControlRig();
		if (ControlRig)
		{
			ControlRig->Modify();
		}
	}

	void FControlRigSelectionHelper::EndTransaction(bool bApply)
	{
		// FScopedTransaction in calling code handles commit/cancel
	}
} // namespace BlenderControls
