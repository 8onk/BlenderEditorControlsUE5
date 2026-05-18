#pragma once

#include "CoreMinimal.h"
#include "ControlRig/ControlRigSelectionHelper.h"

namespace BlenderControls
{
	enum class EAxisLock : uint8;
	struct FGrabContext;
	enum class EPivotMode : uint8;

	/**
	 * Manages a group of selected Control Rig elements as a single transformable unit.
	 * Analogous to FSharedPivot for actors, but operates on Control Rig hierarchy elements.
	 */
	class FControlRigPivot
	{
	public:
		explicit FControlRigPivot(const TArray<FControlRigElementInfo>& InSelection, EPivotMode PivotMode);
		~FControlRigPivot() = default;

		const FTransform& GetPivot() const { return PivotTransform; }
		FVector GetStartLocation() const { return StartPivotTransform.GetLocation(); }
		const FTransform& GetStartTransform() const { return StartPivotTransform; }
		
		/** The last element in the selection - used for local-space axis orientation. */
		const FControlRigElementInfo& GetActiveElement() const { return ActiveElement; }
		const TArray<FControlRigElementInfo>& GetElements() const { return Elements; }

		void SetPosition(const FVector& NewPosition);
		
		/** Simple translation - each element moves by Delta in its own local space or world space. */
		void Translate(const FVector& Delta, bool bUsingLocalSpace);
		
		/** Axis-locked translation - uses active element's orientation for local-space constraint. */
		void Translate(bool bUsingLocalSpace, EAxisLock LockedAxis, const FVector& Delta);
		
		void Rotate(FGrabContext GC, float AngleToRotateRad, bool bUsingLocalSpace, EAxisLock LockedAxis);
		void Scale(FVector ScaleMultiplier, bool bUsingLocalSpace);

		void RevertToStartState();

		bool IsValid() const { return Elements.Num() > 0; }

	private:
		void ComputePivotTransform(EPivotMode InPivotMode);
		void ComputeMedianPivot();
		void ComputeActiveElementPivot();

		FTransform PivotTransform = FTransform::Identity;
		FTransform StartPivotTransform = FTransform::Identity;
		TArray<FControlRigElementInfo> Elements;
		FControlRigElementInfo ActiveElement;
	};
} // namespace BlenderControls
