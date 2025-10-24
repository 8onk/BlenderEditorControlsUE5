#pragma once

namespace BlenderControls
{
    class FBlenderControlsStyle
    {
    public:
        static void Initialize();
        static void Shutdown();
        static const ISlateStyle& Get();
        static FName GetStyleSetName();

    private:
        static TSharedPtr<FSlateStyleSet> StyleInstance;
        static TSharedRef<FSlateStyleSet> Create();
    };
}
