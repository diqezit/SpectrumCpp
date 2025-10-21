#ifndef SPECTRUM_CPP_UI_LAYOUT_H
#define SPECTRUM_CPP_UI_LAYOUT_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines centralized UI layout constants and metrics.
// 
// This namespace serves as the single source of truth for all UI dimensions,
// spacing, and styling constants. It ensures visual consistency across the
// application and simplifies global UI adjustments.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"

namespace Spectrum
{
    namespace UILayout
    {
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // General Metrics & Spacing
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        constexpr float kPadding = 15.0f;
        constexpr float kGroupSpacing = 20.0f;
        constexpr float kWidgetSpacing = 12.0f;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Control Panel Metrics
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        constexpr float kControlPanelWidth = 220.0f;

        // Navigation controls (3 items: Renderer, Quality, Spectrum Scale)
        constexpr float kNavWidgetHeight = 35.0f;
        constexpr float kNavButtonWidth = 40.0f;
        constexpr size_t kNavControlCount = 3;

        // Action buttons (3 items: Audio Settings, Toggle Overlay, Toggle Capture)
        constexpr float kStandaloneButtonHeight = 30.0f;
        constexpr size_t kActionButtonCount = 3;

        // Calculate total height:
        // = Top Padding
        // + (Nav Controls * Height) + (Nav Controls - 1) * Widget Spacing
        // + Group Spacing (separator)
        // + (Action Buttons * Height) + (Action Buttons - 1) * Widget Spacing
        // + Bottom Padding
        constexpr float kControlPanelHeight =
            kPadding +                                                          // Top padding: 15
            (kNavControlCount * kNavWidgetHeight) +                             // Nav controls: 3 * 35 = 105
            ((kNavControlCount - 1) * kWidgetSpacing) +                         // Nav spacing: 2 * 12 = 24
            kGroupSpacing +                                                     // Group separator: 20
            (kActionButtonCount * kStandaloneButtonHeight) +                    // Action buttons: 3 * 30 = 90
            ((kActionButtonCount - 1) * kWidgetSpacing) +                       // Button spacing: 2 * 12 = 24
            kPadding;                                                           // Bottom padding: 15
        // TOTAL: 293

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Audio Settings Panel Metrics
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        constexpr float kAudioPanelWidth = 340.0f;
        constexpr float kAudioPanelTitleHeight = 50.0f;
        constexpr float kCloseButtonSize = 25.0f;
        constexpr float kCloseButtonPadding = 10.0f;

        // Slider components (3 items: Amplification, Smoothing, Bar Count)
        constexpr float kSliderHeight = 15.0f;
        constexpr float kSliderLabelYOffset = -18.0f;
        constexpr float kSliderLabelSpacing = 20.0f;  // Space above each slider for label
        constexpr size_t kSliderCount = 3;

        // Calculate total height:
        // = Top Padding
        // + Title Height
        // + (Slider Count * (Label Spacing + Slider Height))
        // + ((Slider Count - 1) * Group Spacing) between sliders
        // + Bottom Padding
        constexpr float kAudioPanelHeight =
            kPadding +                                                          // Top padding: 15
            kAudioPanelTitleHeight +                                            // Title: 50
            (kSliderCount * (kSliderLabelSpacing + kSliderHeight)) +            // Sliders: 3 * 35 = 105
            ((kSliderCount - 1) * kGroupSpacing) +                              // Slider spacing: 2 * 20 = 40
            kPadding;                                                           // Bottom padding: 15
        // TOTAL: 225

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Animation & Styling Constants
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        constexpr float kAnimationSpeed = 8.0f;
        constexpr float kToggleButtonWidth = 20.0f;
        constexpr float kToggleButtonHeight = 40.0f;
        constexpr Color kSeparatorColor = { 1.0f, 1.0f, 1.0f, 0.1f };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Control Panel Layout Helper Functions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // Calculate Y position for navigation control by index (0-based)
        constexpr float GetNavControlY(size_t index) noexcept
        {
            return kPadding + index * (kNavWidgetHeight + kWidgetSpacing);
        }

        // Calculate Y position for separator line
        constexpr float GetSeparatorY() noexcept
        {
            return kPadding +
                (kNavControlCount * kNavWidgetHeight) +
                ((kNavControlCount - 1) * kWidgetSpacing) +
                (kGroupSpacing / 2.0f);
        }

        // Calculate Y position for action buttons by index (0-based)
        constexpr float GetActionButtonY(size_t index) noexcept
        {
            return kPadding +
                (kNavControlCount * kNavWidgetHeight) +
                ((kNavControlCount - 1) * kWidgetSpacing) +
                kGroupSpacing +
                index * (kStandaloneButtonHeight + kWidgetSpacing);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Audio Settings Panel Layout Helper Functions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // Calculate Y position for slider by index (0-based), relative to panel top
        constexpr float GetSliderYOffset(size_t index) noexcept
        {
            return kPadding +
                kAudioPanelTitleHeight +
                kSliderLabelSpacing +
                index * (kSliderLabelSpacing + kSliderHeight + kGroupSpacing);
        }

        // Calculate slider width based on panel width
        constexpr float GetSliderWidth() noexcept
        {
            return kAudioPanelWidth - 2 * kPadding;
        }

        // Calculate close button X position (relative to panel right edge)
        constexpr float GetCloseButtonXOffset() noexcept
        {
            return kCloseButtonSize + kCloseButtonPadding;
        }

        // Calculate close button Y position (relative to panel top)
        constexpr float GetCloseButtonYOffset() noexcept
        {
            return kCloseButtonPadding;
        }

    } // namespace UILayout

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_LAYOUT_H