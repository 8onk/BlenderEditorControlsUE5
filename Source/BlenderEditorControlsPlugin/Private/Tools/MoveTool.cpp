#include "Tools/MoveTool.h"
#include "LevelEditorViewport.h"
#include "Tools/SharedPivot.h"
#include "UI/TransformHUD.h"
#include "Utils/BlenderMathHelpers.h"
//TODO Cleanup on active?
//TODO adding numeric values and displaying them correctly, also the - and "/" for reciprocal. 

namespace BlenderControls
{
	FMoveTool::FMoveTool(TSharedPtr<FTransformSession> InSession, EAxisLock InAxis)
		: FBlenderToolBase(InSession, ETransformMode::Translate, InAxis, TEXT("Move"))
	{
	}

	void FMoveTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();

		ViewportClient->SetWidgetMode(UE::Widget::WM_Translate);
		ViewportClient->Invalidate();
	}

	FString FMoveTool::GetFormattedValueForEditing(const FNumericSlotData& Slot) const
	{
		if (Slot.CommittedValue.IsSet())
		{
			static const TCHAR* Unit = TEXT("cm");
			const FString ValueString = FString::Printf(TEXT("%g"), Slot.CommittedValue.Get(0.0));

			return FString::Printf(TEXT("%s %s"), *ValueString, Unit);
		}
		return FString();
	}

	void FMoveTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);

		constexpr float ParallelCos = 0.990f;
		float CosAngle = SMALL_NUMBER;
		//If HelperAxisDir is not 0 then we are in single axis lock. 
		if (!GrabContext.HelperAxisDir.IsNearlyZero())
		{
			CosAngle = FMath::Abs(FVector::DotProduct(ViewForward.GetSafeNormal(),
			                                          GrabContext.HelperAxisDir.GetSafeNormal()));
		}

		FVector FinalTotalDelta;
		if (CosAngle <= ParallelCos)
		{
			const FVector UnconstrainedMouseDelta3d = (ViewRight * MouseDelta.X * GrabContext.ScreenToWorldScale) +
				(-ViewUp * MouseDelta.Y * GrabContext.ScreenToWorldScale);

			const FVector ActiveObjectLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();

			//Intersect ghost pos to get the blender "feel", NOT the current mouse pos. 
			const FVector GhostPos = ActiveObjectLocation + UnconstrainedMouseDelta3d;

			FVector RayOrigin, RayDir;
			if (ViewportClient->IsPerspective())
			{
				RayOrigin = ViewLocation;
				RayDir = (GhostPos - RayOrigin).GetSafeNormal();
				const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
				FinalTotalDelta = FinalHit - ActiveObjectLocation;
			}
			else
			{
				RayOrigin = GhostPos;
				RayDir = ViewForward.GetSafeNormal();

				if (GrabContext.HelperType == FGrabContext::EHelperType::AxisPlane)
				{
					const float PlaneViewDot = FVector::DotProduct(RayDir, GrabContext.HelperPlaneN);

					//If view forward and plane normal vectors are parallel
					if (FMath::IsNearlyZero(PlaneViewDot, KINDA_SMALL_NUMBER))
					{
						const FVector VisibleAxis = MathHelper::SelectMostPerpendicularAxis(
							GrabContext.PlaneAxisU, GrabContext.PlaneAxisV,
							ViewForward.GetSafeNormal());

						FinalTotalDelta = UnconstrainedMouseDelta3d.ProjectOnToNormal(VisibleAxis);
					}
					else
					{
						const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
						FinalTotalDelta = FinalHit - ActiveObjectLocation;
					}
				}
				else
				{
					const FVector FinalHit = MathHelper::IntersectHelper(GrabContext, RayOrigin, RayDir);
					FinalTotalDelta = FinalHit - ActiveObjectLocation;
				}
			}
		}
		else
		{
			const FVector2D ScreenUpVector(0.0f, -1.0f);
			const float ScreenSpaceFactor = FVector2D::DotProduct(MouseDelta, ScreenUpVector);
			const float ScaledFactor = FMath::Sign(ScreenSpaceFactor) * FMath::Square(ScreenSpaceFactor) * 0.1f;

			FinalTotalDelta = GrabContext.HelperAxisDir * ScaledFactor;
		}

		FVector LiveDelta = FinalTotalDelta;
		if (bSnappingEnabled)
		{
			LiveDelta = GetSnapOffset(LiveDelta);
		}

		VirtualPivot->Translate(bUsingLocalSpace, LockedAxis, LiveDelta);
	}

	void FMoveTool::ApplyNumeric(double Value)
	{
		FBlenderToolBase::ApplyNumeric(Value);

		FVector Delta = FVector::ZeroVector;
		const double Slot1 = NumericSlots[0].GetTotal();
		const double Slot2 = NumericSlots[1].GetTotal();
		const double Slot3 = NumericSlots[2].GetTotal();

		switch (LockedAxis)
		{
		case EAxisLock::All:
			Delta = FVector(Slot1, Slot2, Slot3);
			break;

		case EAxisLock::X:
			Delta.X = Slot1;
			break;
		case EAxisLock::Y:
			Delta.Y = Slot1;
			break;
		case EAxisLock::Z:
			Delta.Z = Slot1;
			break;

		case EAxisLock::XY:
			Delta.X = Slot1;
			Delta.Y = Slot2;
			break;
		case EAxisLock::XZ:
			Delta.X = Slot1;
			Delta.Z = Slot2;
			break;
		case EAxisLock::YZ:
			Delta.Y = Slot1;
			Delta.Z = Slot2;
			break;
		}

		VirtualPivot->Translate(Delta, bUsingLocalSpace);
		UpdateHud();
	}

	void FMoveTool::UpdateHud()
	{
		const FVector CurrentLocation = VirtualPivot->GetActiveElement().Actor->GetActorLocation();
		const FVector StartLocation = VirtualPivot->GetActiveElement().Transform.GetLocation();
		const FVector LiveDeltaCM = CurrentLocation - StartLocation;

		static const TCHAR* Unit = TEXT("cm");
		static const TCHAR* Sep = TEXT("\u2003"); // EM SPACE
		const FString Space = bUsingLocalSpace ? TEXT("local") : TEXT("global");
		const bool bNumeric = Session->bIsNumericInputActive;

		enum class EValueSlot : int32
		{
			X = 0,
			Y = 1,
			Z = 2
		};

		struct FHudFieldData
		{
			FString Label;
			double LiveValue;
			EValueSlot SlotIndex;
		};

		TArray<FHudFieldData> HudFields;
		FString Suffix;
		double SignedMag = 0.0;

		switch (LockedAxis)
		{
		case EAxisLock::All:
			HudFields.Add({TEXT("Dx"), LiveDeltaCM.X, EValueSlot::X});
			HudFields.Add({TEXT("Dy"), LiveDeltaCM.Y, EValueSlot::Y});
			HudFields.Add({TEXT("Dz"), LiveDeltaCM.Z, EValueSlot::Z});
			SignedMag = LiveDeltaCM.Size();
			break;

		case EAxisLock::X:
			{
				HudFields.Add({
					TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::X)), EValueSlot::X
				});
				Suffix = FString::Printf(TEXT("along %s X"), *Space);

				const double Comp = FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::X));
				SignedMag = Comp;
			}
			break;

		case EAxisLock::Y:
			{
				HudFields.Add({
					TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Y)), EValueSlot::X
				});
				Suffix = FString::Printf(TEXT("along %s Y"), *Space);

				const double Comp = FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Y));
				SignedMag = Comp;
			}
			break;

		case EAxisLock::Z:
			{
				HudFields.Add({
					TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Z)), EValueSlot::X
				});
				Suffix = FString::Printf(TEXT("along %s Z"), *Space);

				const double Comp = FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Z));
				SignedMag = Comp;
			}
			break;

		case EAxisLock::XY:
			HudFields.Add({TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::X)), EValueSlot::X});
			HudFields.Add({TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Y)), EValueSlot::Y});
			Suffix = FString::Printf(TEXT("locking %s Z"), *Space);
			SignedMag = LiveDeltaCM.Size();
			break;

		case EAxisLock::XZ:
			HudFields.Add({TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::X)), EValueSlot::X});
			HudFields.Add({
				TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Z)), EValueSlot::Y
			});
			Suffix = FString::Printf(TEXT("locking %s Y"), *Space);
			SignedMag = LiveDeltaCM.Size();
			break;

		case EAxisLock::YZ:
			HudFields.Add({
				TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Y)), EValueSlot::X
			});
			HudFields.Add({
				TEXT("D"), FVector::DotProduct(LiveDeltaCM, GetAxisVector(EAxisLock::Z)), EValueSlot::Y
			});
			Suffix = FString::Printf(TEXT("locking %s X"), *Space);
			SignedMag = LiveDeltaCM.Size();
			break;
		}

		//format string
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
		const FString MagString = FString::Printf(TEXT(" (%s)"), *HudWidget->FormatMagnitude(SignedMag, Unit));

		HudString = FString::Printf(TEXT("%s%s %s"), *FieldsString, *MagString, *Suffix).TrimEnd();

		HudWidget->Update(FText::FromString(HudString));

		//TEMPORARY LOG
		if (bNumeric)
		{
			for (int i = 0; i < 3; ++i)
			{
				Session->NumericSlots[i].Print();
				UE_LOG(LogTemp, Log, TEXT("NEW LINE	"));
			}
		}
	}

	void FMoveTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FMoveTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		if (!VirtualPivot)
		{
			return;
		}
		const FTransform ActiveObjectTransform = VirtualPivot->GetActiveElement().Transform;
		const FVector X = bUsingLocalSpace ? ActiveObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bUsingLocalSpace ? ActiveObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bUsingLocalSpace ? ActiveObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

		switch (AxisLock)
		{
		case EAxisLock::All:
			GrabContext.HelperType = FGrabContext::EHelperType::ViewPlane;
			GrabContext.HelperPlaneN = -GrabContext.ViewForward;
			break;

		case EAxisLock::X:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = X;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(Y, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Y:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Y;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Z, GrabContext.ViewForward);
			break;

		case EAxisLock::Z:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisLine;
			GrabContext.HelperAxisDir = Z;
			GrabContext.HelperPlaneN = MathHelper::SelectMostParallelPlaneNormal(X, Y, GrabContext.ViewForward);
			break;

		case EAxisLock::XY:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Z;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Y;
			break;

		case EAxisLock::XZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = Y;
			GrabContext.PlaneAxisU = X;
			GrabContext.PlaneAxisV = Z;
			break;

		case EAxisLock::YZ:
			GrabContext.HelperType = FGrabContext::EHelperType::AxisPlane;
			GrabContext.HelperAxisDir = FVector::ZeroVector;
			GrabContext.HelperPlaneN = X;
			GrabContext.PlaneAxisU = Y;
			GrabContext.PlaneAxisV = Z;
			break;
		}
	}

	FVector FMoveTool::GetSnapOffset(const FVector OffsetFromStart)
	{
		if (!GEditor)
		{
			return FVector::ZeroVector;
		}

		const float GridSize = GEditor->GetGridSize();
		FVector SnapOffset;
		if (LockedAxis == EAxisLock::X || LockedAxis == EAxisLock::Y || LockedAxis == EAxisLock::Z)
		{
			const FVector SnapAxis = GrabContext.HelperAxisDir;
			const float DistanceAlongAxis = FVector::DotProduct(OffsetFromStart, SnapAxis);
			const float SnappedDistance = FMath::GridSnap(DistanceAlongAxis, GridSize);
			SnapOffset = SnapAxis * SnappedDistance;
		}
		else if (LockedAxis == EAxisLock::XY || LockedAxis == EAxisLock::XZ || LockedAxis == EAxisLock::YZ)
		{
			const float DistanceAlongU = FVector::DotProduct(OffsetFromStart, GrabContext.PlaneAxisU);
			const float DistanceAlongV = FVector::DotProduct(OffsetFromStart, GrabContext.PlaneAxisV);

			const float SnappedDistanceU = FMath::GridSnap(DistanceAlongU, GridSize);
			const float SnappedDistanceV = FMath::GridSnap(DistanceAlongV, GridSize);

			SnapOffset = (GrabContext.PlaneAxisU * SnappedDistanceU) + (GrabContext.PlaneAxisV * SnappedDistanceV);
		}
		else
		{
			SnapOffset = OffsetFromStart / GridSize;
			SnapOffset = MathHelper::RoundVectorToInt(SnapOffset);
			SnapOffset *= GridSize;
		}

		return SnapOffset;
	}
}
