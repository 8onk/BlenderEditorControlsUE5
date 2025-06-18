#pragma once

#include "CoreMinimal.h"

namespace BlenderControls
{
    /** High-level mode enumeration */
    enum class ETransformMode : uint8
    {
        None,
        Translate,
        Rotate,
        Scale
    };
    enum class ETransformAxis : uint8
    {
        X,
        Y,
        Z,
        All
    };
} // namespace BlenderControls