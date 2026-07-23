// Copyright 2026 Axiom Toolworks. All Rights Reserved.

#pragma once

namespace BlenderControls
{
	/**
	 * Manages the Slate visual assets for the plugin (Icons, Cursors, and Brushes).
	 * Implements a Singleton pattern to provide global access to the plugin's Style Set.
	 */
	class FStyle
	{
	public:
		/** Instantiates the global style set and registers it with the Slate Style Registry. */
		static void Initialize();
		
		/** Unregisters the style set from the Slate Style Registry and destroys the instance. */
		static void Shutdown();
		
		/** 
		 * Retrieves the active style set instance. 
		 * 
		 * @return The global ISlateStyle used by the plugin.
		 */
		static const ISlateStyle& Get();
		
		/** 
		 * Retrieves the internal Slate identifier for this style set.
		 * 
		 * @return The FName identifier ("BlenderEditorControlsStyle").
		 */
		static FName GetStyleSetName();

	private:
		/** The singleton instance holding the plugin's brushes, fonts, and cursors. */
		static TSharedPtr<FSlateStyleSet> StyleInstance;

		/** Creates and configures the style set with paths to Content/Resources. */
		static TSharedRef<FSlateStyleSet> Create();
	};
}
