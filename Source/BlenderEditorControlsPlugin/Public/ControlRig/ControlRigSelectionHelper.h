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
		FRigElementKey ElementKey;
		TWeakObjectPtr<UControlRig> OwningControlRig;

		/** World-space transform captured when the operation began */
		FTransform StartTransform;
		FQuat StartRotation;

		/** 
		 * The exact local value of the control captured when the operation began. Necessary to revert transforms on cancel,
		 * as using world-space transform values seem to be order-dependant (Parent must be restored first, then child etc.) 
		 */
		FRigControlValue StartLocalValue;

		bool IsValid() const { return OwningControlRig.IsValid() && ElementKey.IsValid(); }
	};

	/**
	 * Helper class for detecting and working with Control Rig selections in the Level Editor.
	 * Provides an abstraction over FControlRigEditMode for transform operations.
	 */
	class FControlRigSelectionHelper
	{
	public:
		static bool IsControlRigEditModeActive();
		static int32 GetSelectedRigElementCount();
		static bool HasSelectedRigElements();

		/**
		 * Populates OutElements with all selected Control Rig elements and their cached transforms.
		 * @return True if any elements were found.
		 */
		static bool GetSelectedRigElements(TArray<FControlRigElementInfo>& OutElements);

		static UControlRig* GetActiveControlRig();
		static URigHierarchy* GetRigHierarchy();

		/**
		 * Sets the world-space transform of a rig element. Handles the rig-space conversion
		 * internally and uses the appropriate API for Control vs non-Control elements.
		 */
		static void SetElementGlobalTransform(
			const FRigElementKey& ElementKey,
			const FTransform& NewTransform,
			bool bInitial = false);

		/**
		 * Sets the local control value of a rig element. Useful for reverting safely.
		 */
		static void SetElementLocalValue(
			const FRigElementKey& ElementKey,
			const FRigControlValue& LocalValue);

		/** Returns the world-space transform of the element. */
		static FTransform GetElementGlobalTransform(const FRigElementKey& ElementKey);

		/**
		 * Marks the Control Rig as modified for undo. Call within an FScopedTransaction.
		 */
		static void BeginTransaction(const FText& TransactionName);
		static void EndTransaction(bool bApply);

	private:
		static FControlRigEditMode* GetControlRigEditMode();
	};
} // namespace BlenderControls
