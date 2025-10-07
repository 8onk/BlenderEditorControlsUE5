#include "Tools/RotateTool.h"
#include "LevelEditorViewport.h"
#include "Style/BlenderControlsStyle.h"
#include "Tools/SharedPivot.h"
#include "UI/TransformHUD.h"
#include "Utils/BlenderMathHelpers.h"

namespace BlenderControls
{
    FRotateTool::FRotateTool(TSharedPtr<FTransformSession> InSession, EAxisLock InAxis)
        : FBlenderToolBase(InSession, ETransformMode::Rotate, InAxis, TEXT("Rotate"))
    {
    }

    void FRotateTool::OnBegin()
    {
        FBlenderToolBase::OnBegin();
        bTrackballModeEnabled = false;

        FVector RayOrigin, RayDirection;
        SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);

        StartPivotTransform = VirtualPivot->GetStartTransform();
        PivotStartPosition = VirtualPivot->GetStartTransform().GetLocation();

        SceneView->WorldToPixel(PivotStartPosition, PivotViewportPosition);
        StartDragVector = CurrentMousePosition - PivotViewportPosition;
        LastDragVector = StartDragVector;

        ViewportClient->SetWidgetMode(UE::Widget::WM_Rotate);
        ViewportClient->Invalidate();
        AccumulatedAngleRad = 0.0f;
        TrackballMouseDelta = FVector2D::ZeroVector;
        AngleToApplyRad = 0.0f;

        CursorBrush = BlenderEditorControls::FBlenderControlsStyle::Get().GetBrush(
            TEXT("BlenderEditorControls.Cursors.DoubleArrow"));
        HudWidget->SetCursorBrush(CursorBrush);
        HudWidget->SetCursorSize(FVector2D(24, 24));
        HudWidget->SetCursorHotspot(FVector2D(12, 12));
        HudWidget->SetCursorOrientation(ECursorOrient::PerpendicularCW);
    }

    void FRotateTool::OnActive(const FVector2D& CurrentViewportMousePosition)
    {
        FBlenderToolBase::OnActive(CurrentViewportMousePosition);

        if (!GEditor)
        {
            return;
        }

        if (bTrackballModeEnabled)
        {
            constexpr float MouseDeltaSensitivity = 0.01f;
            TrackballMouseDelta = FVector2D(MouseDelta.X, MouseDelta.Y) * MouseDeltaSensitivity;

            if (bSnappingEnabled)
            {
                //Can use .Yaw or .Pitch or .Roll since the snapping value is the same for all.
                const float SnapAngleDeg = GEditor->GetRotGridSize().Yaw;
                const float SnapAngleRad = FMath::DegreesToRadians(SnapAngleDeg);
                const float SnapIncrement = SnapAngleRad;

                TrackballMouseDelta.X = FMath::GridSnap(TrackballMouseDelta.X, SnapIncrement);
                TrackballMouseDelta.Y = FMath::GridSnap(TrackballMouseDelta.Y, SnapIncrement);
            }

            const FVector RotationAxis = (-ViewUp * TrackballMouseDelta.X) + (-ViewRight * TrackballMouseDelta.Y);
            const float RotationAngle = RotationAxis.Length();
            const FQuat TargetRotation = FQuat(RotationAxis.GetSafeNormal(), RotationAngle);

            FTransform NewTransform = StartPivotTransform;
            const FQuat FinalRotation = TargetRotation * StartPivotTransform.GetRotation();
            NewTransform.SetRotation(FinalRotation);
            NewTransform.NormalizeRotation();
            VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
        }
        else
        {
            const FVector2D CurrentDragVector = VirtualMousePosition - PivotViewportPosition;
            float AngleDeltaRad = MathHelper::GetSignedAngle2D(LastDragVector, CurrentDragVector);
            AccumulatedAngleRad += AngleDeltaRad * CurrentPrecisionFactor;

            const FVector PivotPosition = VirtualPivot->GetTransformProxy()->GetTransform().GetLocation();
            const FVector ViewToPivot = PivotPosition - ViewLocation;

            const FVector RotationAxis = GrabContext.HelperAxisDir;
            //if lock‐axis is “backwards” relative to the camera, flip the sign
            float SignedAccum = AccumulatedAngleRad;
            if (FVector::DotProduct(ViewToPivot, RotationAxis) < 0)
            {
                SignedAccum = -AccumulatedAngleRad;
            }

            AngleToApplyRad = SignedAccum;
            if (bSnappingEnabled)
            {
                const float SnapAngleDeg = GEditor->GetRotGridSize().Yaw;
                const float TotalAngleDeg = FMath::RadiansToDegrees(SignedAccum);
                const float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
                AngleToApplyRad = FMath::DegreesToRadians(SnappedAngleDeg);
            }

            VirtualPivot->Rotate(GrabContext, AngleToApplyRad, bUsingLocalSpace, LockedAxis);

            LastDragVector = CurrentDragVector;
        }

        if (HudWidget.IsValid() && !bTrackballModeEnabled)
        {
            HudWidget->SetDashState(true, PivotViewportPosition, VirtualMousePosition);
        }
        else
        {
            HudWidget->SetDashState(false, PivotViewportPosition, VirtualMousePosition);
        }

        UpdateHud();
    }


    void FRotateTool::ApplyNumeric(const double Value)
    {
        FBlenderToolBase::ApplyNumeric(Value);

        if (bTrackballModeEnabled)
        {
            const float Slot1 = Session->NumericSlots[0].GetTotal();
            const float Slot2 = Session->NumericSlots[1].GetTotal();

            const float AngleXRad = FMath::DegreesToRadians(Slot2);
            const float AngleYRad = FMath::DegreesToRadians(Slot1);

            const FVector RotationAxis = (-ViewUp * AngleXRad) + (-ViewRight * AngleYRad);
            const float RotationAngle = RotationAxis.Length();

            // Check for zero rotation to avoid issues with GetSafeNormal()
            if (FMath::IsNearlyZero(RotationAngle))
            {
                UpdateHud();
                return;
            }

            const FQuat TargetRotation = FQuat(RotationAxis.GetSafeNormal(), RotationAngle);

            FTransform NewTransform = StartPivotTransform;
            const FQuat FinalRotation = TargetRotation * StartPivotTransform.GetRotation();
            NewTransform.SetRotation(FinalRotation);
            NewTransform.NormalizeRotation();
            VirtualPivot->GetTransformProxy()->SetTransform(NewTransform);
            UpdateHud();
        }
        else
        {
            const double TotalAngleDegrees = Session->NumericSlots[0].GetTotal();
            const double RadiansToRotate = FMath::DegreesToRadians(TotalAngleDegrees);
            VirtualPivot->Rotate(GrabContext, RadiansToRotate, bUsingLocalSpace, LockedAxis);
            UpdateHud();
        }
    }

    void FRotateTool::UpdateHud()
    {
        if (!VirtualPivot || !HudWidget.IsValid())
        {
            return;
        }

        const double LiveAngleDeg = FMath::RadiansToDegrees(AngleToApplyRad) * -1;
        static const TCHAR* Unit = TEXT("\u00B0"); // Degree symbol
        static const TCHAR* Sep = TEXT("\u2003"); // EM SPACE
        const FString Space = bUsingLocalSpace ? TEXT("local") : TEXT("global");
        const bool bNumeric = Session->bIsNumericInputActive;

        enum class EValueSlot : int32
        {
            Angle = 0,
            TrackballX = 0,
            TrackballY = 1,
        };

        struct FHudFieldData
        {
            FString Label;
            double LiveValue;
            EValueSlot SlotIndex;
        };

        TArray<FHudFieldData> HudFields;
        FString Suffix;
        FString Header;

        if (bTrackballModeEnabled)
        {
            const double LiveAngleY = FMath::RadiansToDegrees(TrackballMouseDelta.X);
            const double LiveAngleX = FMath::RadiansToDegrees(TrackballMouseDelta.Y);

            const int32 SlotX = static_cast<int32>(EValueSlot::TrackballX);
            const int32 SlotY = static_cast<int32>(EValueSlot::TrackballY);

            // This is a small helper function to format a single trackball value correctly.
            // It handles all the required states: live, active numeric, and inactive numeric ("None").
            auto FormatTrackballValue = [&](int32 SlotIndex, double LiveValue) -> FString
            {
                if (bNumeric)
                {
                    const FNumericSlotData& SlotData = Session->NumericSlots[SlotIndex];
                    if (Session->CurrentNumericSlotIndex == SlotIndex)
                    {
                        return HudWidget->FormatOneField(TEXT(""), SlotData, Unit, true);
                    }

                    if (SlotData.SlotState == ESlotState::Pristine)
                    {
                        return TEXT("NONE");
                    }

                    return FString::Printf(TEXT("%.2f%s"), SlotData.GetTotal(), Unit);
                }

                return FString::Printf(TEXT("%.2f"), LiveValue);
            };

            const FString FormattedX = FormatTrackballValue(SlotX, LiveAngleX);
            const FString FormattedY = FormatTrackballValue(SlotY, LiveAngleY);

            HudString = FString::Printf(TEXT("Trackball: %s %s"), *FormattedX, *FormattedY);
            HudWidget->Update(FText::FromString(HudString));
            return;
        }

        FString AxisString;
        switch (LockedAxis)
        {
        case EAxisLock::All:
            HudFields.Add({TEXT("Rotation"), LiveAngleDeg, EValueSlot::Angle});
            break;
        case EAxisLock::X:
            AxisString = TEXT("X");
            break;
        case EAxisLock::Y:
            AxisString = TEXT("Y");
            break;
        case EAxisLock::Z:
            AxisString = TEXT("Z");
            break;
        case EAxisLock::XY:
            AxisString = TEXT("Z");
            break;
        case EAxisLock::XZ:
            AxisString = TEXT("Y");
            break;
        case EAxisLock::YZ:
            AxisString = TEXT("X");
            break;
        }

        if (!AxisString.IsEmpty())
        {
            HudFields.Add({TEXT("Rotation"), LiveAngleDeg, EValueSlot::Angle});
            Suffix = FString::Printf(TEXT("along %s %s"), *Space, *AxisString);
        }

        TArray<FString> FormattedFields;
        for (const FHudFieldData& FieldData : HudFields)
        {
            const int32 SlotIndexInt = static_cast<int32>(FieldData.SlotIndex);
            FNumericSlotData DataToFormat;

            if (bNumeric)
            {
                DataToFormat = Session->NumericSlots[SlotIndexInt];
            }
            else
            {
                DataToFormat.SlotState = ESlotState::Committed;
                DataToFormat.CommittedValue = FieldData.LiveValue;
            }

            const bool bIsActive = bNumeric && (SlotIndexInt == Session->CurrentNumericSlotIndex);

            FormattedFields.Add(HudWidget->FormatOneField(
                FieldData.Label,
                DataToFormat,
                Unit,
                bIsActive
            ));
        }

        const FString FieldsString = FString::Join(FormattedFields, Sep);

        HudString = FString::Printf(TEXT("%s%s %s"), *Header, *FieldsString, *Suffix).TrimEnd();
        HudWidget->Update(FText::FromString(HudString));

        // TEMPORARY LOG
        if (bNumeric)
        {
            for (int i = 0; i < 3; ++i)
            {
                Session->NumericSlots[i].Print();
                UE_LOG(LogTemp, Log, TEXT("NEW LINE    "));
            }
        }
    }

    void FRotateTool::OnEnd(const bool bApply)
    {
        FBlenderToolBase::OnEnd(bApply);
    }

    void FRotateTool::HandleAxisLock(const EAxisLock AxisPressed)
    {
        if (bTrackballModeEnabled)
        {
            return;
        }

        FBlenderToolBase::HandleAxisLock(AxisPressed);
    }

    void FRotateTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
    {
        const FTransform ObjectTransform = VirtualPivot->GetStartTransform();
        const FVector X = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
        const FVector Y = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
        const FVector Z = bUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

        switch (AxisLock)
        {
        case EAxisLock::X: GrabContext.HelperAxisDir = X;
            break;
        case EAxisLock::Y: GrabContext.HelperAxisDir = Y;
            break;
        case EAxisLock::Z: GrabContext.HelperAxisDir = Z;
            break;

        case EAxisLock::XY: GrabContext.HelperAxisDir = Z;
            break;
        case EAxisLock::YZ: GrabContext.HelperAxisDir = X;
            break;
        case EAxisLock::XZ: GrabContext.HelperAxisDir = Y;
            break;

        case EAxisLock::All: GrabContext.HelperAxisDir = GrabContext.ViewForward;
            break;
        }
    }

    FVector FRotateTool::GetSnapOffset(const FVector OffsetFromStart)
    {
        return FVector::ZeroVector;
        // FVector SnapOffset = OffsetFromStart / RotationGridSize;
        // SnapOffset = MathHelper::RoundVect
        // orToInt(SnapOffset);
        // SnapOffset *= RotationGridSize;
        //
        // return SnapOffset;
    }

    void FRotateTool::SetTrackballRotationMode(const bool bEnabled)
    {
        if (!bTrackballModeEnabled)
        {
            PreviousAxisLock = LockedAxis;
            LockedAxis = EAxisLock::All;
        }
        else
        {
            LockedAxis = PreviousAxisLock;
        }
        Session->LockedAxis = LockedAxis;
        bTrackballModeEnabled = bEnabled;
        CursorBrush = BlenderEditorControls::FBlenderControlsStyle::Get().GetBrush(
            TEXT("BlenderEditorControls.Cursors.Trackball"));
        HudWidget->SetCursorBrush(CursorBrush);
        HudWidget->SetCursorOrientation(ECursorOrient::None);
        UpdateAxisLock();
    }

    bool FRotateTool::GetTrackballRotationMode()
    {
        return bTrackballModeEnabled;
    }

    void FRotateTool::UpdateToolSettingsForAxisLock()
    {
        if (bTrackballModeEnabled)
        {
            NumNumericSlots = 2;
        }
        else
        {
            NumNumericSlots = 1;
        }
    }

    FString FRotateTool::GetFormattedValueForEditing(const FNumericSlotData& Slot) const
    {
        if (Slot.CommittedValue.IsSet())
        {
            // Degree symbol
            static const TCHAR* Unit = TEXT("\u00B0");
            const FString ValueString = FString::Printf(TEXT("%g"), Slot.CommittedValue.Get(0.0));

            return FString::Printf(TEXT("%s%s"), *ValueString, Unit);
        }
        return FString();
    }
} // namespace BlenderControls
