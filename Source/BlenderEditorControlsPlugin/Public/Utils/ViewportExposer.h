// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

#include "EditorViewportClient.h"

namespace BlenderControls
{
	/** 
	 * Exposes protected FEditorViewportClient members and methods by subclassing it.
	 * Used to manually invoke native tracking functions or set state that is otherwise 
	 * protected in the engine API.
	 */
	class FViewportClientExposer : public FEditorViewportClient
	{
	public:
		using FEditorViewportClient::FEditorViewportClient;

		/** 
		 * Manually triggers the engine's tracking start routine.
		 * 
		 * @note We intentionally pass an empty (dereferenced nullptr) reference for the View parameter.
		 *       This is completely safe in C++ because StartTrackingDueToInput does not actually 
		 *       use or access the View parameter internally. This trick allows us to bypass the 
		 *       expensive operation of creating a full FSceneView context just to satisfy the signature.
		 */
		static void CallStartTracking(FEditorViewportClient* Client, const FInputEventState& InputState, FSceneView& View)
		{
			static_cast<FViewportClientExposer*>(Client)->StartTrackingDueToInput(InputState, View);
		}

		/** 
		 * Forcibly overrides the internal tracking state of the viewport.
		 * Mitigates desync issues and suppresses unwanted auto-saves.
		 */
		static void SetViewportState(FEditorViewportClient* Client, bool bInTracking, bool bAxisControlledByDrag)
		{
			// Cast the client to our exposer so we can write to the protected variables
			static_cast<FViewportClientExposer*>(Client)->bIsTracking = bInTracking;
			static_cast<FViewportClientExposer*>(Client)->bWidgetAxisControlledByDrag = bAxisControlledByDrag;
		}

		/** 
		 * Manually triggers the engine's tracking stop routine.
		 * This properly cleans up the MouseDeltaTracker and resets the widget axis visuals.
		 */
		static void CallStopTracking(FEditorViewportClient* Client)
		{
			static_cast<FViewportClientExposer*>(Client)->StopTracking();
		}
	};
}
