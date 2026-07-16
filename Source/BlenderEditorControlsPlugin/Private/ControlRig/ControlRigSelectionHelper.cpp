#include "ControlRig/ControlRigSelectionHelper.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "ControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "Rigs/RigHierarchyElements.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
#include "ControlRigEditor/Private/EditMode/ControlRigEditMode.h"
#else
#include "EditMode/ControlRigEditMode.h"
#endif


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

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
		// Bypass LNK2001 by using the hardcoded string since ModeName is unexported
		FEdMode* ActiveMode = ModeTools.GetActiveMode(TEXT("EditMode.ControlRig"));
#else
		FEdMode* ActiveMode = ModeTools.GetActiveMode(FControlRigEditMode::ModeName);
#endif

		return ActiveMode ? static_cast<FControlRigEditMode*>(ActiveMode) : nullptr;
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
	UControlRig* FControlRigSelectionHelper::GetActiveControlRig()
	{
		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode)
		{
			return nullptr;
		}

		// Bypass LNK2019: FControlRigEditMode is unexported; we cannot call GetControlRig().
		// Workaround: Find the active UControlRig currently in memory.
		UControlRig* FallbackRig = nullptr;
		for (TObjectIterator<UControlRig> It; It; ++It)
		{
			UControlRig* Rig = *It;
			if (IsValid(Rig) && !Rig->HasAnyFlags(RF_ClassDefaultObject | RF_Transient))
			{
				// The active rig being manipulated in the editor will have an active selection
				if (Rig->CurrentControlSelection().Num() > 0)
				{
					return Rig;
				}

				// Store a fallback in case there is no selection yet
				if (Rig->GetWorld() && (Rig->GetWorld()->WorldType == EWorldType::Editor || Rig->GetWorld()->WorldType
					== EWorldType::PIE))
				{
					FallbackRig = Rig;
				}
			}
		}
		return FallbackRig;
	}
#endif

	bool FControlRigSelectionHelper::IsControlRigEditModeActive()
	{
		return GetControlRigEditMode() != nullptr;
	}

	int32 FControlRigSelectionHelper::GetSelectedRigElementCount()
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
		const UControlRig* ControlRig = GetActiveControlRig();
		return ControlRig ? ControlRig->CurrentControlSelection().Num() : 0;
#else
		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode) return 0;

		TMap<UControlRig*, TArray<FRigElementKey>> SelectedControls;
		EditMode->GetAllSelectedControls(SelectedControls);

		int32 TotalCount = 0;
		for (const auto& Pair : SelectedControls)
		{
			TotalCount += Pair.Value.Num();
		}
		return TotalCount;
#endif
	}

	bool FControlRigSelectionHelper::HasSelectedRigElements()
	{
		return GetSelectedRigElementCount() > 0;
	}

	bool FControlRigSelectionHelper::GetSelectedRigElements(TArray<FControlRigElementInfo>& OutElements)
	{
		OutElements.Empty();

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
		UControlRig* ControlRig = GetActiveControlRig();
		if (!ControlRig)
		{
			return false;
		}

		URigHierarchy* Hierarchy = ControlRig->GetHierarchy();
		if (!Hierarchy)
		{
			return false;
		}

		FTransform HostingActorTransform = FTransform::Identity;
		if (TSharedPtr<IControlRigObjectBinding> ObjectBinding = ControlRig->GetObjectBinding())
		{
			if (AActor* BoundActor = ObjectBinding->GetHostingActor())
			{
				HostingActorTransform = BoundActor->GetActorTransform();
			}
		}
		if (HostingActorTransform.Equals(FTransform::Identity))
		{
			if (AActor* HostingActor = ControlRig->GetTypedOuter<AActor>())
			{
				HostingActorTransform = HostingActor->GetActorTransform();
			}
		}

		TArray<FName> SelectedNames = ControlRig->CurrentControlSelection();
		for (const FName& Name : SelectedNames)
		{
			FRigElementKey Key(Name, ERigElementType::Control);
			if (const FRigControlElement* ControlElement = Hierarchy->Find<FRigControlElement>(Key))
			{
				ERigControlType ControlType = ControlElement->Settings.ControlType;
				if (ControlType == ERigControlType::Transform || ControlType == ERigControlType::TransformNoScale ||
					ControlType == ERigControlType::EulerTransform || ControlType == ERigControlType::Position ||
					ControlType == ERigControlType::Rotator)
				{
					FControlRigElementInfo ElementInfo;
					ElementInfo.ElementKey = Key;
					ElementInfo.OwningControlRig = ControlRig;

					const FTransform RigSpaceTransform = Hierarchy->GetGlobalTransform(ElementInfo.ElementKey);
					ElementInfo.StartTransform = RigSpaceTransform * HostingActorTransform;
					ElementInfo.StartRotation = ElementInfo.StartTransform.GetRotation();
					ElementInfo.StartLocalValue = Hierarchy->GetControlValue(Key, ERigControlValueType::Current);

					OutElements.Add(ElementInfo);
				}
			}
		}
#else
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

			FTransform HostingActorTransform = FTransform::Identity;
			if (TSharedPtr<IControlRigObjectBinding> ObjectBinding = ControlRig->GetObjectBinding())
			{
				if (AActor* BoundActor = ObjectBinding->GetHostingActor())
				{
					HostingActorTransform = BoundActor->GetActorTransform();
				}
			}
			if (HostingActorTransform.Equals(FTransform::Identity))
			{
				if (const AActor* HostingActor = ControlRig->GetTypedOuter<AActor>())
				{
					HostingActorTransform = HostingActor->GetActorTransform();
				}
			}

			for (const FRigElementKey& Key : Pair.Value)
			{
				if (const FRigControlElement* ControlElement = Hierarchy->Find<FRigControlElement>(Key))
				{
					// Filter selection to prevent attributes from changing on transformation 
					ERigControlType ControlType = ControlElement->Settings.ControlType;
					if (ControlType == ERigControlType::Transform || ControlType == ERigControlType::TransformNoScale ||
						ControlType == ERigControlType::EulerTransform || ControlType == ERigControlType::Position ||
						ControlType == ERigControlType::Rotator)
					{
						FControlRigElementInfo ElementInfo;
						ElementInfo.ElementKey = Key;
						ElementInfo.OwningControlRig = ControlRig;

						const FTransform RigSpaceTransform = Hierarchy->GetGlobalTransform(Key);
						ElementInfo.StartTransform = RigSpaceTransform * HostingActorTransform;
						ElementInfo.StartRotation = ElementInfo.StartTransform.GetRotation();
						ElementInfo.StartLocalValue = Hierarchy->GetControlValue(Key, ERigControlValueType::Current);

						OutElements.Add(ElementInfo);
					}
				}
			}
		}
#endif

		return OutElements.Num() > 0;
	}


	void FControlRigSelectionHelper::SetElementGlobalTransform(
		UControlRig* ControlRig,
		const FRigElementKey& ElementKey,
		const FTransform& NewTransform,
		bool bInitial)
	{
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
		if (TSharedPtr<IControlRigObjectBinding> ObjectBinding = ControlRig->GetObjectBinding())
		{
			if (AActor* BoundActor = ObjectBinding->GetHostingActor())
			{
				HostingActorTransform = BoundActor->GetActorTransform();
			}
		}
		if (HostingActorTransform.Equals(FTransform::Identity))
		{
			if (const AActor* HostingActor = ControlRig->GetTypedOuter<AActor>())
			{
				HostingActorTransform = HostingActor->GetActorTransform();
			}
		}
		const FTransform RigSpaceTransform = NewTransform.GetRelativeTransform(HostingActorTransform);

		if (ElementKey.Type == ERigElementType::Control)
		{
			// Configure the modification context
			// KeyMask = AllTransform ensures location, rotation, and scale are all considered
			FRigControlModifiedContext Context;
			Context.SetKey = EControlRigSetKey::DoNotCare;
			Context.KeyMask = static_cast<uint32>(EControlRigContextChannelToKey::AllTransform);

			// Has to be true for undo transactions to work (bSetupUndo seems to have no impact)
			constexpr bool bNotify = true;
			constexpr bool bSetupUndo = false; // Undo handled by FScopedTransaction in calling code
			constexpr bool bPrintPythonCommands = false;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
			ControlRig->SetControlGlobalTransform(
				ElementKey.Name,
				RigSpaceTransform,
				bNotify,
				Context,
				bSetupUndo,
				bPrintPythonCommands
			);
#else
			// bFixEulerFlips helps with rotation continuity for EulerTransform controls
			constexpr bool bFixEulerFlips = true;

			ControlRig->SetControlGlobalTransform(
				ElementKey.Name,
				RigSpaceTransform,
				bNotify,
				Context,
				bSetupUndo,
				bPrintPythonCommands,
				bFixEulerFlips
			);
#endif
		}
		else
		{
			constexpr bool bAffectChildren = true;
			constexpr bool bPropagate = true;
			Hierarchy->SetGlobalTransform(ElementKey, RigSpaceTransform, bInitial, bAffectChildren, bPropagate);
		}
	}

	void FControlRigSelectionHelper::SetElementLocalValue(
		UControlRig* ControlRig,
		const FRigElementKey& ElementKey,
		const FRigControlValue& LocalValue)
	{
		if (!ControlRig) return;

		URigHierarchy* Hierarchy = ControlRig->GetHierarchy();
		if (!Hierarchy) return;

		if (ElementKey.Type == ERigElementType::Control)
		{
			FRigControlModifiedContext Context;
			Context.SetKey = EControlRigSetKey::DoNotCare;
			Context.KeyMask = static_cast<uint32>(EControlRigContextChannelToKey::AllTransform);

			constexpr bool bNotify = true;
			constexpr bool bSetupUndo = false;
			constexpr bool bPrintPythonCommands = false;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
			ControlRig->SetControlValue(
				ElementKey.Name,
				LocalValue,
				bNotify,
				Context,
				bSetupUndo,
				bPrintPythonCommands
			);
#else
			constexpr bool bFixEulerFlips = true;
			ControlRig->SetControlValue(
				ElementKey.Name,
				LocalValue,
				bNotify,
				Context,
				bSetupUndo,
				bPrintPythonCommands,
				bFixEulerFlips
			);
#endif
		}
	}

	FTransform FControlRigSelectionHelper::GetElementGlobalTransform(UControlRig* ControlRig,
	                                                                 const FRigElementKey& ElementKey)
	{
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
		if (TSharedPtr<IControlRigObjectBinding> ObjectBinding = ControlRig->GetObjectBinding())
		{
			if (AActor* BoundActor = ObjectBinding->GetHostingActor())
			{
				HostingActorTransform = BoundActor->GetActorTransform();
			}
		}
		if (HostingActorTransform.Equals(FTransform::Identity))
		{
			if (const AActor* HostingActor = ControlRig->GetTypedOuter<AActor>())
			{
				HostingActorTransform = HostingActor->GetActorTransform();
			}
		}
		return RigSpaceTransform * HostingActorTransform;
	}

	void FControlRigSelectionHelper::BeginTransaction(const TArray<FControlRigElementInfo>& ElementsToSelect,
	                                                  const FText& TransactionName)
	{
		FControlRigEditMode* EditMode = GetControlRigEditMode();
		if (!EditMode)
		{
			return;
		}

		// Mark the Control Rigs as modified to participate in the undo transaction
		TSet<UControlRig*> RigsToModify;
		for (const FControlRigElementInfo& Element : ElementsToSelect)
		{
			if (UControlRig* ControlRig = Element.OwningControlRig.Get())
			{
				RigsToModify.Add(ControlRig);
			}
		}

		for (UControlRig* Rig : RigsToModify)
		{
			Rig->Modify();
		}
	}

	void FControlRigSelectionHelper::EndTransaction(bool bApply)
	{
		// FScopedTransaction in calling code handles commit/cancel
	}


} // namespace BlenderControls
