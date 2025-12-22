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
		static void Initialize();
		static void Shutdown();
		static const ISlateStyle& Get();
		static FName GetStyleSetName();

	private:
		static TSharedPtr<FSlateStyleSet> StyleInstance;

		/** Creates and configures the style set with paths to Content/Resources. */
		static TSharedRef<FSlateStyleSet> Create();
	};
}
