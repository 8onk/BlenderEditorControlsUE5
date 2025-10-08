#pragma once
#include "CoreMinimal.h"
#include "BlenderEditorControlsEnums.h"

namespace BlenderControls
{
	class FSharedPivot;

	struct FNumericSlotData
	{
		FString Label;
		ESlotState SlotState = ESlotState::Pristine;
		TOptional<double> CommittedValue;
		TOptional<double> LiveValue;
		bool bIsNegated = false;
		bool bIsReciprocal = false;
		FString Display = "";

		double GetTotal() const
		{
			double Total = CommittedValue.Get(0.0) + LiveValue.Get(0.0);
			if (bIsReciprocal && !FMath::IsNearlyZero(Total))
				Total = 1.0 / Total;
			if (bIsNegated)
				Total *= -1.0;
			return Total;
		}

		// Convert SlotState to string
		static const TCHAR* SlotStateToString(ESlotState State)
		{
			switch (State)
			{
			case ESlotState::Pristine: return TEXT("Pristine");
			case ESlotState::FirstEdit: return TEXT("FirstEdit");
			case ESlotState::Committed: return TEXT("Committed");
			case ESlotState::Additive: return TEXT("Additive");
			case ESlotState::InvalidInput: return TEXT("InvalidInput");
			default: return TEXT("Unknown");
			}
		}

		// Debug print
		void Print() const
		{
			const FString CommittedStr = CommittedValue.IsSet()
				                             ? FString::SanitizeFloat(*CommittedValue)
				                             : TEXT("None");
			const FString LiveStr = LiveValue.IsSet() ? FString::SanitizeFloat(*LiveValue) : TEXT("None");

			UE_LOG(LogTemp, Log, TEXT("FNumericSlotData { State=%s, Committed=%s, Live=%s, Display=\"%s\", Total=%f }"),
			       SlotStateToString(SlotState),
			       *CommittedStr,
			       *LiveStr,
			       *Display,
			       GetTotal());
		}
	};

	struct FTransformSession
	{
		TSharedPtr<FSharedPivot> VirtualPivot;
		EAxisLock LockedAxis = EAxisLock::All;
		FVector2D StartMousePos = FVector2D::ZeroVector;
		TArray<TWeakObjectPtr<AActor>> SelectedActors;

		bool bUsingLocalSpace = false;
		bool bIsAxisLockActive = false;

		FString NumericBuffer;
		FNumericSlotData NumericSlots[3];
		int32 CurrentNumericSlotIndex = 0;
		bool bIsNumericInputActive = false;

		FVector2D WrappedCursorPosition = FVector2D::ZeroVector;
		FVector2D VirtualMousePosition = FVector2D::ZeroVector;
		FVector2D CursorAnchorPoint = FVector2D::ZeroVector;
	};
}
