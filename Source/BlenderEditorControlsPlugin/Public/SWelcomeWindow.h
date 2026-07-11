// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"

/**
 * Custom window widget that displays a welcome greeting when the plugin is installed.
 */
class SWelcomeWindow : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SWelcomeWindow) {}
	SLATE_END_ARGS()

	/** Constructs the window and its interior layout */
	void Construct(const FArguments& InArgs);
};
