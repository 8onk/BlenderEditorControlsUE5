#include "Tools/SharedPivot.h"
#include "LevelEditorViewport.h"

namespace BlenderControls
{
    FSharedPivot::FSharedPivot(const TArray<TWeakObjectPtr<AActor>> &Selection)
    {
        TransformProxy = NewObject<UTransformProxy>();
        
        if(!TransformProxy){
            return;
        }

        for (auto &APtr : Selection)
        {
            if (APtr.IsValid())
            {
                AActor *A = APtr.Get();

                // Uses bounding box centre for now only, maybe expand to be able to choose.
                FVector Origin, Extent;
                A->GetActorBounds(false, Origin, Extent);

                Children.Add({A, A->GetActorTransform(), FVector::ZeroVector});
                TransformProxy->AddComponent(A->GetRootComponent(), true);
                Pivot += Origin; // use bounds center
            }
        }

        if (Children.Num())
        {
            Pivot /= Children.Num();
        }
        for (FChildInfo &Child : Children)
        {
            Child.Offset = Child.Actor->GetActorLocation() - Pivot;
        } 
    }

    void FSharedPivot::MoveBy(const FVector &Delta)
    {
        if(!TransformProxy){
            return;
        }

        // Access the active editor viewport client
        auto LevelEditorViewportClient = GCurrentLevelEditingViewportClient;
        if (!LevelEditorViewportClient)
        {
            return;
        }

        FViewport* Viewport = GEditor->GetActiveViewport();
        if (!Viewport)
        {
            return;
        }

        FViewportClient* ViewportClient = Viewport->GetClient();
        if (!ViewportClient)
        {
            return;
        }

        FEditorViewportClient* EditorViewportClient = static_cast<FEditorViewportClient*>(ViewportClient);
        if (!EditorViewportClient)
        {
            return;
        }

        EditorViewportClient->SetCurrentWidgetAxis(EAxisList::X);

        // Apply movement using InputWidgetDelta
        FVector DragDelta = Delta;
        FRotator RotDelta = FRotator::ZeroRotator;
        FVector ScaleDelta = FVector::ZeroVector;

        LevelEditorViewportClient->InputWidgetDelta(
            GEditor->GetActiveViewport(),
            EAxisList::X,  
            DragDelta,
            RotDelta,
            ScaleDelta
        );
        
        // Update internal pivot state (if needed for your custom logic)
        Pivot += Delta;
    }
} // namespace BlenderControls