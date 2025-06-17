#pragma once

#include "UObject/WeakObjectPtr.h"

namespace BlenderControls
{
    class FBlenderToolBase : public TSharedFromThis<FBlenderToolBase>
    {
    public:
        FBlenderToolBase(ETransformMode InMode,
                         ETransformAxis InAxis,
                         const FString &InDisplayName);
        virtual ~FBlenderToolBase();

        /** Per-frame update from input-processor */
        virtual void Tick(const FVector2D &MouseDelta) {}

        virtual void Accept();
        virtual void Cancel();

        /** Axis helpers */
        void SetAxis(ETransformAxis NewAxis) { Axis = NewAxis; }
        ETransformAxis GetAxis() const { return Axis; }

        /** Numeric entry apply */
        virtual void ApplyNumeric(float Value) {}

    protected:
        /** Child tools call this to populate Selected & prepare undo */
        void CaptureSelection();

        /** Implemented in derived classes */
        virtual void OnBegin() = 0;
        virtual void OnEnd(bool bApply) = 0;
        virtual void HandleDelta(const FVector2D &MouseDelta) = 0;

        /* Transaction utilities */
        TUniquePtr<struct FScopedTransaction> ParentTxn;

        /* Common data */
        ETransformMode Mode;
        ETransformAxis Axis;
        FString DisplayName;
        TArray<TWeakObjectPtr<AActor>> SelectedActors;
        TMap<TWeakObjectPtr<AActor>, FTransform> OriginalTransforms;
    };
} // namespace BlenderControls
