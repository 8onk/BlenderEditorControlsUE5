#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "BlenderEditorControlsEnums.h"
#include "ToolSharedState.h"
#include "Tools/BlenderToolBase.h"

namespace BlenderControls
{
	class FSharedPivot;

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

		bool MatchesCommandKeyIgnoringModifiers(
			const FKeyEvent& KeyEvent,
			const TSharedPtr<FUICommandInfo>& CmdInfo);


		/* Input handlers for activating transform tools */
		void TranslatePressed() { BeginTool(ETransformMode::Translate); }
		void RotatePressed() { BeginTool(ETransformMode::Rotate); }
		void ScalePressed() { BeginTool(ETransformMode::Scale); }

		bool IsToolActive() const { return bActive && CurrentTool.IsValid(); }
		bool IsRotateToolActive() const { return IsToolActive() && ActiveMode == ETransformMode::Rotate; }
		bool IsNumericActive() const { return IsToolActive() && Session->bIsNumericInputActive; }

		// Accept / Cancel
		void AcceptPressed() { EndTool(true); }
		void CancelPressed() { EndTool(false); }

		// Axis locks
		void AxisXPressed() { HandleAxisKey(EKeys::X); }
		void AxisYPressed() { HandleAxisKey(EKeys::Y); }
		void AxisZPressed() { HandleAxisKey(EKeys::Z); }

		void HandleAxisKey(FKey Key) const;

		// Numeric helpers
		void NumericBackspacePressed() const { CurrentTool->HandleBackspace(); }
		void NumericToggleNegationPressed() { CurrentTool->ToggleNegation(); }
		void NumericToggleReciprocalPressed() { CurrentTool->ToggleReciprocal(); }
		void NumericCycleSlotPressed() { CurrentTool->CycleNumericInputSlot(); }

		// Modifiers
		void ToggleTrackballPressed()
		{
			if (IsRotateToolActive()) CurrentTool->SetTrackballRotationMode(!CurrentTool->GetTrackballRotationMode());
		}

		void PrecisionPressed() { if (CurrentTool.IsValid()) CurrentTool->SetPrecisionModeActive(true); }

		void SnapInvertPressed()
		{
			if (CurrentTool.IsValid()) CurrentTool->SetSnappingEnabled(!CurrentTool->IsSnappingEnabled());
		}

		void DuplicateAndMovePressed();

		TSharedPtr<FTransformSession> Session;
		bool bActive = true;
		bool bNumericInput = false;
		FString NumericBuffer;
		FVector2D StartMousePos = FVector2D::ZeroVector;
		TSharedPtr<FBlenderToolBase> CurrentTool;
		ETransformMode ActiveMode;
		TSharedPtr<FUICommandList> CommandList;
		TSharedPtr<SWidget> SoftwareCursorWidget;
		TSet<FKey> PressedKeys;
	};
} // namespace BlenderControls
