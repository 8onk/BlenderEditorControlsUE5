#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "Engine/Selection.h"

namespace BlenderControls
{
    FBlenderToolBase::FBlenderToolBase(ETransformMode InMode, ETransformAxis InAxis, const FString &InDisplayName)
        : Mode(InMode), Axis(InAxis), DisplayName(InDisplayName)
    {
        OnBegin();
    }

    FBlenderToolBase::~FBlenderToolBase()
    {
    }

    void FBlenderToolBase::OnBegin()
    {
        if (GEditor)
        {
            CachedSelectionColor = GEditor->GetSelectionOutlineColor();
            GEditor->SetSelectionOutlineColor(FLinearColor::White);
        }

        CaptureSelection();
        Group = MakeShared<FSharedPivot>(SelectedActors);

        // Start transaction for undo
        ParentTxn = MakeUnique<FScopedTransaction>(FText::FromString(DisplayName));
        for (auto Actor : SelectedActors)
        {
            Actor->Modify();
        }
    }

    void FBlenderToolBase::OnEnd(bool bApply)
    {
        if (GEditor)
        {
            GEditor->SetSelectionOutlineColor(CachedSelectionColor);
        }
        SelectedActors.Empty();
    }

    void FBlenderToolBase::Tick(const FVector2D &MouseDelta)
    {
        // Base implementation does nothing
    }

    void FBlenderToolBase::Accept()
    {
        OnEnd(/*bApply=*/true);
        ParentTxn.Reset();
    }

    void FBlenderToolBase::Cancel()
    {
        OnEnd(/*bApply=*/false);
        if (GEditor)
        {
            GEditor->SetSelectionOutlineColor(CachedSelectionColor);
        }

        // Abort undo-tracking
        if (ParentTxn)
        {
            ParentTxn->Cancel();
            ParentTxn.Reset();
        }

        SelectedActors.Empty();
    }

    void FBlenderToolBase::ApplyNumeric(float Value)
    {
        // Base implementation does nothing
    }

    void FBlenderToolBase::CaptureSelection()
    {
        SelectedActors.Empty();

        if (GEditor)
        {
            USelection *ActorSelection = GEditor->GetSelectedActors();
            for (FSelectionIterator It(*ActorSelection); It; ++It)
            {
                if (AActor *Actor = Cast<AActor>(*It))
                {
                    SelectedActors.Add(TWeakObjectPtr<AActor>(Actor));
                    UE_LOG(LogBlenderEditorControls, Log, TEXT("Selected Actor: %s"), *Actor->GetActorLabel());
                }
            }
        }
    }
} // namespace BlenderControls