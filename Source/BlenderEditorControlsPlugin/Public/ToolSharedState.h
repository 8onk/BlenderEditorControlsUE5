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
		bool bUsingDegrees = false;
		FString Display = "";

		double GetTotal()
		{
			double Total = CommittedValue.Get(0.0) + LiveValue.Get(0.0);
			if (bIsReciprocal && !FMath::IsNearlyZero(Total))
			{
				Total = 1.0 / Total;
			}
			if (bIsNegated)
			{
				Total *= -1.0;
			}

			return Total;
		}

		static double DegreesToRadians(double Degrees)
		{
			return Degrees * (PI / 180.0f);
		}

		static double RadiansToDegrees(double Radians)
		{
			return Radians * (180.0 / PI);
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
		void Print()
		{
			const FString CommittedStr = CommittedValue.IsSet()
				                             ? FString::SanitizeFloat(*CommittedValue)
				                             : TEXT("None");

			const FString LiveStr = LiveValue.IsSet()
				                        ? FString::SanitizeFloat(*LiveValue)
				                        : TEXT("None");

			const double TotalValue = GetTotal();

			UE_LOG(LogTemp, Log, TEXT("---- FNumericSlotData ----"));
			UE_LOG(LogTemp, Log, TEXT("Label: %s"), *Label);
			UE_LOG(LogTemp, Log, TEXT("State: %s"), SlotStateToString(SlotState));
			UE_LOG(LogTemp, Log, TEXT("CommittedValue: %s"), *CommittedStr);
			UE_LOG(LogTemp, Log, TEXT("LiveValue: %s"), *LiveStr);
			UE_LOG(LogTemp, Log, TEXT("Display: \"%s\""), *Display);
			UE_LOG(LogTemp, Log, TEXT("bUsingDegrees:     %s"), bUsingDegrees ? TEXT("true") : TEXT("false"));
			UE_LOG(LogTemp, Log, TEXT("bIsNegated:        %s"), bIsNegated ? TEXT("true") : TEXT("false"));
			UE_LOG(LogTemp, Log, TEXT("bIsReciprocal:     %s"), bIsReciprocal ? TEXT("true") : TEXT("false"));
			UE_LOG(LogTemp, Log, TEXT("Total: %f"), TotalValue);
			UE_LOG(LogTemp, Log, TEXT("---------------------------"));
		}
	};

	// struct FTransformSession
	// {
	// 	TSharedPtr<FSharedPivot> VirtualPivot;
	// 	EAxisLock LockedAxis = EAxisLock::All;
	// 	FVector2D StartMousePos = FVector2D::ZeroVector;
	// 	TArray<TWeakObjectPtr<AActor>> SelectedActors;
	//
	// 	bool bUsingLocalSpace = false;
	// 	bool bIsAxisLockActive = false;
	//
	// 	FString NumericBuffer;
	// 	FNumericSlotData NumericSlots[3];
	// 	int32 CurrentNumericSlotIndex = 0;
	// 	bool bIsNumericInputActive = false;
	// 	ETransformMode PreviouslyActiveMode = ETransformMode::None;
	//
	// 	FVector2D WrappedMousePosition = FVector2D::ZeroVector;
	// 	FVector2D VirtualMousePosition = FVector2D::ZeroVector;
	// 	FVector2D CursorAnchorPoint = FVector2D::ZeroVector;
	// };
}
