#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "BlenderEditorControlsEnums.h"
#include "UI/BlenderOverlay.h"

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

			if (bIsReciprocal)
			{
				if (!FMath::IsNearlyZero(Total))
				{
					Total = 1.0 / Total;
				}
				else
				{
					Total = 0.0;
				}
			}

			// Apply negation
			if (bIsNegated)
			{
				Total *= -1.0;
			}

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
	};

	class FBlenderControlsInputProcessor : public IInputProcessor,
	                                       public TSharedFromThis<FBlenderControlsInputProcessor>
	{
	public:
		explicit FBlenderControlsInputProcessor(TSharedPtr<FUICommandList> InCommandList);
		~FBlenderControlsInputProcessor();

		void BindCommands();

		/* Public toggle – called by toolbar button / settings */
		void SetActive(bool bEnable) { bActive = bEnable; }

		/** IInputProcessor overrides */
		virtual void Tick(const float DeltaTime, FSlateApplication&, TSharedRef<ICursor>) override;
		virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent&) override;
		virtual bool HandleKeyUpEvent(FSlateApplication&, const FKeyEvent&) override;
		virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent&) override;
		virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent&) override;
		virtual bool HandleMouseButtonUpEvent(FSlateApplication&, const FPointerEvent&) override;
		virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent,
		                                            const FPointerEvent* InGestureEvent) override;

	private:
		/* Helpers */
		void BeginTool(ETransformMode Mode);
		void EndTool(bool bApply);
		void CaptureSelection() const;
		static bool TryMapKeyToNumericChar(const FKey& Key, TCHAR& OutChar);
		bool ShouldHandleToolHotkeys(FSlateApplication& SlateApp) const;
		bool IsMouseOverLevelViewport() const;

		/* Input handlers for activating transform tools */
		void TranslatePressed() { BeginTool(ETransformMode::Translate); }
		void RotatePressed() { BeginTool(ETransformMode::Rotate); }
		void ScalePressed() { BeginTool(ETransformMode::Scale); }

		void DuplicateAndMovePressed();

		TSharedPtr<FTransformSession> CurrentSession;
		bool bActive = true;
		bool bNumericInput = false;
		FString NumericBuffer;
		FVector2D StartMousePos = FVector2D::ZeroVector;
		TSharedPtr<FBlenderToolBase> CurrentTool;
		ETransformMode ActiveMode;
		TSharedPtr<FUICommandList> CommandList;
		TSharedPtr<FBlenderOverlay> Overlay;
		TSharedPtr<SWidget> SoftwareCursorWidget;
		TSet<FKey> PressedKeys;
	};
} // namespace BlenderControls
