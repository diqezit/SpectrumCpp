// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines centralized constants for the user interface layout.
// It ensures a consistent look and feel across all UI panels and widgets,
// and simplifies the process of tuning the UI's appearance.
//
// Defines the UILayout namespace, which serves as the single source of truth
// for all UI metrics and styling constants. It ensures visual consistency
// and simplifies global UI adjustments by centralizing layout data.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_UI_LAYOUT_H
#define SPECTRUM_CPP_UI_LAYOUT_H

#include "Common.h"

namespace Spectrum {
	namespace UILayout {

		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
		// General Metrics & Spacing
		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

		constexpr float kPadding = 15.0f;
		constexpr float kGroupSpacing = 20.0f;
		constexpr float kWidgetSpacing = 12.0f;

		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
		// Control Panel Metrics
		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

		constexpr float kControlPanelWidth = 220.0f;
		constexpr float kControlPanelHeight = 220.0f;
		constexpr float kNavWidgetHeight = 35.0f;
		constexpr float kNavButtonWidth = 40.0f;
		constexpr float kStandaloneButtonHeight = 30.0f;

		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
		// Audio Settings Panel Metrics
		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

		constexpr float kAudioPanelWidth = 340.0f;
		constexpr float kAudioPanelHeight = 200.0f;
		constexpr float kAudioPanelTitleHeight = 40.0f;

		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
		// Shared Widget Metrics
		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

		constexpr float kSliderHeight = 15.0f;
		constexpr float kSliderLabelYOffset = -18.0f;

		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
		// Animation & Styling Constants
		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

		constexpr float kAnimationSpeed = 8.0f;
		constexpr float kToggleButtonWidth = 20.0f;
		constexpr Color kSeparatorColor = { 1.0f, 1.0f, 1.0f, 0.1f };

	} // namespace UILayout
} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_LAYOUT_H