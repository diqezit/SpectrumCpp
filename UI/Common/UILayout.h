#ifndef SPECTRUM_CPP_UI_LAYOUT_H
#define SPECTRUM_CPP_UI_LAYOUT_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines centralized UI layout constants and metrics.
// 
// This namespace serves as the single source of truth for all UI dimensions,
// spacing, and styling constants. It ensures visual consistency across the
// application and simplifies global UI adjustments.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"

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
        constexpr float kNavWidgetHeight = 35.0f;
        constexpr float kNavButtonWidth = 40.0f;
        constexpr size_t kNavControlCount = 3;
        constexpr float kStandaloneButtonHeight = 30.0f;
        constexpr size_t kActionButtonCount = 3;
        constexpr float kControlPanelHeight =
            kPadding +
            (kNavControlCount * kNavWidgetHeight) +
            ((kNavControlCount - 1) * kWidgetSpacing) +
            kGroupSpacing +
            (kActionButtonCount * kStandaloneButtonHeight) +
            ((kActionButtonCount - 1) * kWidgetSpacing) +
            kPadding;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Audio Settings Panel Metrics
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        constexpr float kAudioPanelWidth = 340.0f;
        constexpr float kAudioPanelTitleHeight = 50.0f;
        constexpr float kCloseButtonSize = 25.0f;
        constexpr float kCloseButtonPadding = 10.0f;
        constexpr float kSliderHeight = 15.0f;
        constexpr float kSliderLabelYOffset = -18.0f;
        constexpr float kSliderLabelSpacing = 20.0f;
        constexpr size_t kSliderCount = 3;
        constexpr float kAudioPanelHeight =
            kPadding +
            kAudioPanelTitleHeight +
            (kSliderCount * (kSliderLabelSpacing + kSliderHeight)) +
            ((kSliderCount - 1) * kGroupSpacing) +
            kPadding;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation & Styling Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        constexpr float kAnimationSpeed = 8.0f;
        constexpr float kToggleButtonWidth = 20.0f;
        constexpr float kToggleButtonHeight = 40.0f;
        constexpr Color kSeparatorColor = { 1.0f, 1.0f, 1.0f, 0.1f };
        constexpr Color kPanelBackgroundColor = { 0.1f, 0.1f, 0.12f, 0.95f };
        constexpr Color kPanelBorderColor = { 1.0f, 1.0f, 1.0f, 0.1f };
        constexpr Color kToggleButtonColor = { 0.1f, 0.1f, 0.1f, 0.7f };
        constexpr Color kToggleButtonHoverColor = { 0.3f, 0.3f, 0.3f, 0.7f };
        constexpr Color kToggleButtonBorderColor = { 1.0f, 1.0f, 1.0f, 0.1f };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Control Panel Layout Helper Functions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        constexpr float GetNavControlY(size_t index) noexcept
        {
            return kPadding + index * (kNavWidgetHeight + kWidgetSpacing);
        }

        constexpr float GetSeparatorY() noexcept
        {
            return kPadding +
                (kNavControlCount * kNavWidgetHeight) +
                ((kNavControlCount - 1) * kWidgetSpacing) +
                (kGroupSpacing / 2.0f);
        }

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

        constexpr float GetSliderYOffset(size_t index) noexcept
        {
            return kPadding +
                kAudioPanelTitleHeight +
                kSliderLabelSpacing +
                index * (kSliderLabelSpacing + kSliderHeight + kGroupSpacing);
        }

        constexpr float GetSliderWidth() noexcept
        {
            return kAudioPanelWidth - 2 * kPadding;
        }

        constexpr float GetCloseButtonXOffset() noexcept
        {
            return kCloseButtonSize + kCloseButtonPadding;
        }

        constexpr float GetCloseButtonYOffset() noexcept
        {
            return kCloseButtonPadding;
        }

    } // namespace UILayout

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_LAYOUT_H