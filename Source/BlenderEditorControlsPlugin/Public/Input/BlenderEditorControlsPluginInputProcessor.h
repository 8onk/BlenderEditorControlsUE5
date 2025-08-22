#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "BlenderEditorControlsEnums.h"
#include "UI/BlenderOverlay.h"

namespace BlenderControls
{
	class FSharedPivot;

	struct FTransformSession
	{
		TSharedPtr<FSharedPivot> VirtualPivot;
		EAxisLock LockedAxis = EAxisLock::All;
		FVector2D StartMousePos = FVector2D::ZeroVector;
		TArray<TWeakObjectPtr<AActor>> SelectedActors;

		bool bUsingLocalSpace = false;
		bool bIsAxisLockActive = false;
		
		FString NumericBuffer;
		FVector NumericInputSlots = FVector::ZeroVector;
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
		void UpdateAxis(EAxisLock Axis);
		void FeedNumeric(const TCHAR Digit);
		void FlushNumeric();
		void WrapMouse(const FVector2D& CurrentViewportMousePosition) const;
		void CaptureSelection() const;
		static bool TryMapKeyToNumericChar(const FKey& Key, TCHAR& OutChar);
		// void ShowSoftwareGrabCursor();

		/* Input handlers for activating transform tools */
		void TranslatePressed() { BeginTool(ETransformMode::Translate); }
		void RotatePressed() { BeginTool(ETransformMode::Rotate); }
		void ScalePressed() { BeginTool(ETransformMode::Scale); }

		TSharedPtr<FTransformSession> CurrentSession;
		bool bActive = true;
		bool bNumericInput = false;
		FString NumericBuffer;
		FVector2D StartMousePos = FVector2D::ZeroVector;
		TSharedPtr<class FBlenderToolBase> CurrentTool;
		ETransformMode ActiveMode;
		TSharedPtr<FUICommandList> CommandList;
		TSharedPtr<FBlenderOverlay> Overlay;
		TSharedPtr<SWidget> SoftwareCursorWidget;
		TSet<FKey> PressedKeys;
	};
} // namespace BlenderControls
