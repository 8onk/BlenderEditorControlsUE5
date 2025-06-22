#include "Tools/BlenderToolBase.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
    FBlenderToolBase::FBlenderToolBase(ETransformMode InMode, ETransformAxis InAxis, const FString &InDisplayName)
        : Mode(InMode), Axis(InAxis), DisplayName(InDisplayName)
    {
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
        InitialWidgetMode = GLevelEditorModeTools().GetWidgetMode();
        GLevelEditorModeTools().SetWidgetMode(UE::Widget::WM_None);

        // Start transaction for undo
        ParentTxn = MakeUnique<FScopedTransaction>(FText::FromString(DisplayName));
        for (auto Actor : SelectedActors)
        {
            Actor->Modify();
        }

        Group->GetTransformProxy()->BeginTransformEditSequence();
    }

    void FBlenderToolBase::OnEnd(bool bApply)
    {
        if (GEditor)
        {
            GEditor->SetSelectionOutlineColor(CachedSelectionColor);
        }
        SelectedActors.Empty();

        Group->GetTransformProxy()->EndTransformEditSequence();
        GLevelEditorModeTools().SetWidgetMode(InitialWidgetMode);
    }

    // void FBlenderToolBase::Tick(const FVector2D &MouseDelta)
    // {
    //     // Base implementation does nothing
    // }

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
                }
            }
        }
    }
} // namespace BlenderControls