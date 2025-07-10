#include "Tools/RotateTool.h"

#include "BaseGizmos/GizmoMath.h"
#include "Utils/BlenderMathHelpers.h"
//TODO fix the rotation behaving oddly when mouse wraps around
//TODO draw the rotation gizmo handle
//TODO cancelling resets the rotation. Make the shared pivot logic be more robust. 

namespace BlenderControls
{
	FRotateTool::FRotateTool(EAxisLock InAxis)
		: FBlenderToolBase(ETransformMode::Rotate, InAxis, TEXT("Rotate"))
	{
	}

	void FRotateTool::OnBegin()
	{
		FBlenderToolBase::OnBegin();

		FVector RayOrigin, RayDirection;
		SceneView->DeprojectFVector2D(CurrentMousePosition, RayOrigin, RayDirection);

		StartPivotTransform = Pivot->GetStartTransform();
		PivotStartPos = Pivot->GetStartTransform().GetLocation();

		SceneView->WorldToPixel(PivotStartPos, PivotViewportLocation);

		StartDragVector = CurrentMousePosition - PivotViewportLocation;
	}

	void FRotateTool::OnActive(const FVector2D& CurrentViewportMousePosition)
	{
		FBlenderToolBase::OnActive(CurrentViewportMousePosition);
		//TEMPORARY
		bTrackballModeEnabled = true;

		if (bTrackballModeEnabled)
		{
			const FVector SphereCenter = Pivot->GetStartTransform().GetLocation();
			//RADIUS should be 3D distance from the pivot point to the corner of the bounding box that is furthest away. 
			constexpr double SphereRadius = 100.0;

			if (!SceneView)
			{
				UE_LOG(LogBlenderEditorControls, Warning, TEXT("No scene view found! RotateTool.ccp::OnActive"));
				return;
			}
			FVector StartRayOrigin, StartRayDirection;
			SceneView->DeprojectFVector2D(GrabContext.StartMousePos, StartRayOrigin, StartRayDirection);

			bool bStartHit;
			FVector ArcStartPoint;
			GizmoMath::RaySphereIntersection(SphereCenter, SphereRadius, StartRayOrigin, StartRayDirection, bStartHit,
			                                 ArcStartPoint);

			FVector CurrentRayOrigin, CurrentRayDirection;
			SceneView->DeprojectFVector2D(CurrentMousePosition, CurrentRayOrigin, CurrentRayDirection);
			bool bCurrentHit;
			FVector ArcEndPoint;
			GizmoMath::RaySphereIntersection(SphereCenter, SphereRadius, CurrentRayOrigin, CurrentRayDirection,
			                                 bCurrentHit,
			                                 ArcEndPoint);

			if (bStartHit && bCurrentHit)
			{
				FVector VecStartFromCenter = ArcStartPoint - SphereCenter;
				FVector VecEndFromCenter = ArcEndPoint - SphereCenter;
				FVector RotationAxis = FVector::CrossProduct(VecStartFromCenter, VecEndFromCenter).GetSafeNormal();
				float TotalRotationAngle = MathHelper::GetSignedAngle3D(VecStartFromCenter, VecEndFromCenter);
				//
				// // Grab the editor world
				// UWorld* World = nullptr;
				// if (GEditor)
				// {
				// 	World = GEditor->GetEditorWorldContext().World();
				// }
				// if (World)
				// {
				// 	// 1) Draw the sphere outline
				// 	DrawDebugSphere(
				// 		World,
				// 		SphereCenter,
				// 		SphereRadius,
				// 		32, // segments
				// 		FColor::Emerald, // color
				// 		false, // persistent (auto‐remove)
				// 		0.1f, // lifetime
				// 		0, // depth priority
				// 		1.0f // line thickness
				// 	);
				//
				// 	// 2) Draw the two radius vectors
				// 	DrawDebugLine(
				// 		World,
				// 		SphereCenter,
				// 		ArcStartPoint,
				// 		FColor::Blue,
				// 		false, // not persistent
				// 		0.1f, // life
				// 		0,
				// 		2.0f // thickness
				// 	);
				// 	DrawDebugLine(
				// 		World,
				// 		SphereCenter,
				// 		ArcEndPoint,
				// 		FColor::Red,
				// 		false,
				// 		0.1f,
				// 		0,
				// 		2.0f
				// 	);
				//
				// 	// 3) Mark the actual hit points
				// 	DrawDebugPoint(
				// 		World,
				// 		ArcStartPoint,
				// 		8.0f,
				// 		FColor::Blue,
				// 		false,
				// 		0.1f
				// 	);
				// 	DrawDebugPoint(
				// 		World,
				// 		ArcEndPoint,
				// 		8.0f,
				// 		FColor::Red,
				// 		false,
				// 		0.1f
				// 	);
				//
				// 	// 4) Draw the rotation axis
				// 	DrawDebugDirectionalArrow(
				// 		World,
				// 		SphereCenter,
				// 		SphereCenter + RotationAxis * SphereRadius,
				// 		40.0f, // arrow size
				// 		FColor::Yellow,
				// 		false,
				// 		0.1f,
				// 		0,
				// 		2.5f // shaft thickness
				// 	);
				// }
				
				if (!GEditor)
				{
					return;
				}

				//REFACTOR INTO A SNAP OFFSET FUNCTION
				// FRotator RotationGridSize = GEditor->GetRotGridSize();
				// float SnapAngleDeg = RotationGridSize.Yaw;
				// float TotalAngleDeg = FMath::RadiansToDegrees(TotalRotationAngle);
				// float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
				// float SnappedAngleRad = FMath::DegreesToRadians(SnappedAngleDeg);
				// float DegreesToRotate = bSnappingEnabled ? SnappedAngleRad : TotalRotationAngle;
				const FQuat TargetRotation = FQuat(RotationAxis, TotalRotationAngle);

				UE_LOG(LogBlenderEditorControls, Log, TEXT("Degrees to rotate: %f"), TotalRotationAngle);

				FTransform NewTransform = StartPivotTransform;
				NewTransform.ConcatenateRotation(TargetRotation);
				Pivot->GetTransformProxy()->SetTransform(NewTransform);
			}
		}
		else
		{
			const FVector2D CurrentDragVector = CurrentMousePosition - PivotViewportLocation;

			const FVector RotationAxis = GrabContext.HelperAxisDir;
			const float ViewAlignmentWithAxis = FVector::DotProduct(GrabContext.ViewForward, RotationAxis);
			float TotalAngleRadians = MathHelper::GetSignedAngle2D(StartDragVector, CurrentDragVector);
			const bool bIsRotationAxisFacingCamera = ViewAlignmentWithAxis > 0;
			if (!bIsRotationAxisFacingCamera)
			{
				TotalAngleRadians = -TotalAngleRadians;
			}

			if (!GEditor)
			{
				return;
			}

			//REFACTOR INTO A SNAP OFFSET FUNCTION
			FRotator RotationGridSize = GEditor->GetRotGridSize();
			float SnapAngleDeg = RotationGridSize.Yaw;
			float TotalAngleDeg = FMath::RadiansToDegrees(TotalAngleRadians);
			float SnappedAngleDeg = FMath::GridSnap(TotalAngleDeg, SnapAngleDeg);
			float SnappedAngleRad = FMath::DegreesToRadians(SnappedAngleDeg);
			float DegreesToRotate = bSnappingEnabled ? SnappedAngleRad : TotalAngleRadians;
			const FQuat TargetRotation = FQuat(RotationAxis, DegreesToRotate);

			FTransform NewTransform = StartPivotTransform;
			NewTransform.ConcatenateRotation(TargetRotation);
			Pivot->GetTransformProxy()->SetTransform(NewTransform);
		}
	}

	void FRotateTool::ApplyNumeric(float Value)
	{
	}

	void FRotateTool::OnEnd(bool bApply)
	{
		FBlenderToolBase::OnEnd(bApply);
	}

	void FRotateTool::SetGrabContextAxisLock(const EAxisLock AxisLock)
	{
		const FTransform ObjectTransform = Pivot->GetStartTransform();
		const FVector X = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::X) : FVector::XAxisVector;
		const FVector Y = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Y) : FVector::YAxisVector;
		const FVector Z = bIsUsingLocalSpace ? ObjectTransform.GetUnitAxis(EAxis::Z) : FVector::ZAxisVector;

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
		// SnapOffset = MathHelper::RoundVectorToInt(SnapOffset);
		// SnapOffset *= RotationGridSize;
		//
		// return SnapOffset;
	}
} // namespace BlenderControls
