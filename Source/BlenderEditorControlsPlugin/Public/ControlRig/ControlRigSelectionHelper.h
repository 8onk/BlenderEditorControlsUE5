// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Rigs/RigHierarchyDefines.h"

class UControlRig;
class URigHierarchy;
class FControlRigEditMode;

namespace BlenderControls
{
	/**
	 * Stores information about a selected Control Rig element (bone/control/null).
	 * Caches the element's initial state at the start of a transform operation.
	 */
	struct FControlRigElementInfo
	{
		/** Unique identifier for the bone/control in the rig hierarchy. */
		FRigElementKey ElementKey;
		
		/** Pointer to the specific Control Rig asset this element belongs to. */
		TWeakObjectPtr<UControlRig> OwningControlRig;

		/** World-space transform captured when the operation began */
		FTransform StartTransform;
		
		/** World-space rotation captured when the operation began. */
		FQuat StartRotation;

		/** 
		 * The exact local value of the control captured when the operation began. Necessary to revert transforms on cancel,
		 * as using world-space transform values seem to be order-dependant (Parent must be restored first, then child etc.) 
		 */
		FRigControlValue StartLocalValue;

		/** @return True if both the rig and the element key are still valid. */
		bool IsValid() const { return OwningControlRig.IsValid() && ElementKey.IsValid(); }
	};

	/**
	 * Helper class for detecting and working with Control Rig selections in the Level Editor.
	 * Provides an abstraction over FControlRigEditMode for transform operations.
	 */
	class FControlRigSelectionHelper
	{
	public:
		/** @return True if the Animation Mode (Control Rig) is currently active in the Level Editor. */
		static bool IsControlRigEditModeActive();
		
		/** @return The total number of individually selected bones/controls in the active rig. */
		static int32 GetSelectedRigElementCount();
		
		/** @return True if at least one Control Rig element is selected. */
		static bool HasSelectedRigElements();

		/**
		 * Populates OutElements with all selected Control Rig elements and their cached transforms.
		 * 
		 * @param OutElements The array to populate with the active selection.
		 * @return True if any elements were found.
		 */
		static bool GetSelectedRigElements(TArray<FControlRigElementInfo>& OutElements);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
		/** @return The primary Control Rig currently being interacted with in Animation Mode (Legacy UE5.3). */
		static UControlRig* GetActiveControlRig();
#endif

		/**
		 * Sets the world-space transform of a rig element. Handles the rig-space conversion
		 * internally and uses the appropriate API for Control vs non-Control elements.
		 * 
		 * @param ControlRig The rig owning the element.
		 * @param ElementKey The identifier of the element to modify.
		 * @param NewTransform The target world-space transform.
		 * @param bInitial True if this is the first setup tick (determines event broadcasting logic).
		 */
		static void SetElementGlobalTransform(
			UControlRig* ControlRig,
			const FRigElementKey& ElementKey,
			const FTransform& NewTransform,
			bool bInitial = false);

		/**
		 * Sets the local control value of a rig element. Useful for reverting safely.
		 * 
		 * @param ControlRig The rig owning the element.
		 * @param ElementKey The identifier of the element to modify.
		 * @param LocalValue The raw local value to revert to.
		 */
		static void SetElementLocalValue(
			UControlRig* ControlRig,
			const FRigElementKey& ElementKey,
			const FRigControlValue& LocalValue);

		/** 
		 * Returns the world-space transform of the element. 
		 * 
		 * @param ControlRig The rig owning the element.
		 * @param ElementKey The identifier of the element to query.
		 * @return The world-space transform.
		 */
		static FTransform GetElementGlobalTransform(UControlRig* ControlRig, const FRigElementKey& ElementKey);

		/**
		 * Marks the Control Rig as modified for undo. Call within an FScopedTransaction.
		 * 
		 * @param ElementsToSelect The elements involved in the transaction.
		 * @param TransactionName The description of the undo step.
		 */
		static void BeginTransaction(const TArray<FControlRigElementInfo>& ElementsToSelect, const FText& TransactionName);
		
		/** 
		 * Finalizes the ongoing Control Rig transaction. 
		 * 
		 * @param bApply True if confirming changes, False to discard.
		 */
		static void EndTransaction(bool bApply);

	private:
		/** @return The internal pointer to the FControlRigEditMode instance. */
		static FControlRigEditMode* GetControlRigEditMode();
	};
} // namespace BlenderControls
