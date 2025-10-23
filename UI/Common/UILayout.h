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
        // Animation & Styling Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        namespace Animation
        {
            constexpr float kSpeed = 8.0f;
        }

        namespace Colors
        {
            constexpr Color kSeparator = { 1.0f, 1.0f, 1.0f, 0.1f };
            constexpr Color kPanelBackground = { 0.1f, 0.1f, 0.12f, 0.95f };
            constexpr Color kPanelBorder = { 1.0f, 1.0f, 1.0f, 0.1f };
            constexpr Color kToggleButton = { 0.1f, 0.1f, 0.1f, 0.7f };
            constexpr Color kToggleButtonHover = { 0.3f, 0.3f, 0.3f, 0.7f };
            constexpr Color kToggleButtonBorder = { 1.0f, 1.0f, 1.0f, 0.1f };
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Control Panel Layout
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        namespace ControlPanel
        {
            // Panel dimensions
            constexpr float kWidth = 220.0f;

            // Navigation controls
            namespace Navigation
            {
                constexpr float kWidgetHeight = 35.0f;
                constexpr float kButtonWidth = 40.0f;
                constexpr size_t kControlCount = 3;

                [[nodiscard]] constexpr float GetTotalHeight() noexcept
                {
                    return (kControlCount * kWidgetHeight) +
                        ((kControlCount - 1) * kWidgetSpacing);
                }

                [[nodiscard]] constexpr float GetControlY(size_t index) noexcept
                {
                    return kPadding + index * (kWidgetHeight + kWidgetSpacing);
                }
            }

            // Action buttons
            namespace ActionButtons
            {
                constexpr float kButtonHeight = 30.0f;
                constexpr size_t kButtonCount = 3;

                [[nodiscard]] constexpr float GetTotalHeight() noexcept
                {
                    return (kButtonCount * kButtonHeight) +
                        ((kButtonCount - 1) * kWidgetSpacing);
                }

                [[nodiscard]] constexpr float GetButtonY(size_t index) noexcept
                {
                    return kPadding +
                        Navigation::GetTotalHeight() +
                        kGroupSpacing +
                        index * (kButtonHeight + kWidgetSpacing);
                }
            }

            // Toggle button
            namespace ToggleButton
            {
                constexpr float kWidth = 20.0f;
                constexpr float kHeight = 40.0f;
            }

            // Calculated panel height
            [[nodiscard]] constexpr float GetPanelHeight() noexcept
            {
                return kPadding +
                    Navigation::GetTotalHeight() +
                    kGroupSpacing +
                    ActionButtons::GetTotalHeight() +
                    kPadding;
            }

            [[nodiscard]] constexpr float GetSeparatorY() noexcept
            {
                return kPadding +
                    Navigation::GetTotalHeight() +
                    (kGroupSpacing * 0.5f);
            }
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Audio Settings Panel Layout
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        namespace AudioSettingsPanel
        {
            // Panel dimensions
            constexpr float kWidth = 340.0f;
            constexpr float kTitleHeight = 50.0f;

            // Close button
            namespace CloseButton
            {
                constexpr float kSize = 25.0f;
                constexpr float kPadding = 10.0f;

                [[nodiscard]] constexpr float GetXOffset() noexcept
                {
                    return kSize + kPadding;
                }

                [[nodiscard]] constexpr float GetYOffset() noexcept
                {
                    return kPadding;
                }
            }

            // Sliders
            namespace Sliders
            {
                constexpr float kHeight = 15.0f;
                constexpr float kLabelYOffset = -18.0f;
                constexpr float kLabelSpacing = 20.0f;
                constexpr size_t kCount = 3;

                [[nodiscard]] constexpr float GetWidth() noexcept
                {
                    return kWidth - 2.0f * kPadding;
                }

                [[nodiscard]] constexpr float GetTotalHeight() noexcept
                {
                    return (kCount * (kLabelSpacing + kHeight)) +
                        ((kCount - 1) * kGroupSpacing);
                }

                [[nodiscard]] constexpr float GetSliderY(size_t index) noexcept
                {
                    return kPadding +
                        kTitleHeight +
                        kLabelSpacing +
                        index * (kLabelSpacing + kHeight + kGroupSpacing);
                }
            }

            // Calculated panel height
            [[nodiscard]] constexpr float GetPanelHeight() noexcept
            {
                return kPadding +
                    kTitleHeight +
                    Sliders::GetTotalHeight() +
                    kPadding;
            }
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Legacy Compatibility Layer (backward compatibility with old code)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // Control Panel - old names
        constexpr float kControlPanelWidth = ControlPanel::kWidth;
        constexpr float kNavWidgetHeight = ControlPanel::Navigation::kWidgetHeight;
        constexpr float kNavButtonWidth = ControlPanel::Navigation::kButtonWidth;
        constexpr size_t kNavControlCount = ControlPanel::Navigation::kControlCount;
        constexpr float kStandaloneButtonHeight = ControlPanel::ActionButtons::kButtonHeight;
        constexpr size_t kActionButtonCount = ControlPanel::ActionButtons::kButtonCount;
        constexpr float kControlPanelHeight = ControlPanel::GetPanelHeight();
        constexpr float kToggleButtonWidth = ControlPanel::ToggleButton::kWidth;
        constexpr float kToggleButtonHeight = ControlPanel::ToggleButton::kHeight;

        // Audio Settings Panel - old names
        constexpr float kAudioPanelWidth = AudioSettingsPanel::kWidth;
        constexpr float kAudioPanelTitleHeight = AudioSettingsPanel::kTitleHeight;
        constexpr float kCloseButtonSize = AudioSettingsPanel::CloseButton::kSize;
        constexpr float kCloseButtonPadding = AudioSettingsPanel::CloseButton::kPadding;
        constexpr float kSliderHeight = AudioSettingsPanel::Sliders::kHeight;
        constexpr float kSliderLabelYOffset = AudioSettingsPanel::Sliders::kLabelYOffset;
        constexpr float kSliderLabelSpacing = AudioSettingsPanel::Sliders::kLabelSpacing;
        constexpr size_t kSliderCount = AudioSettingsPanel::Sliders::kCount;
        constexpr float kAudioPanelHeight = AudioSettingsPanel::GetPanelHeight();

        // Animation & Colors - old names
        constexpr float kAnimationSpeed = Animation::kSpeed;
        constexpr Color kSeparatorColor = Colors::kSeparator;
        constexpr Color kPanelBackgroundColor = Colors::kPanelBackground;
        constexpr Color kPanelBorderColor = Colors::kPanelBorder;
        constexpr Color kToggleButtonColor = Colors::kToggleButton;
        constexpr Color kToggleButtonHoverColor = Colors::kToggleButtonHover;
        constexpr Color kToggleButtonBorderColor = Colors::kToggleButtonBorder;

        // Helper functions - old names
        [[nodiscard]] constexpr float GetNavControlY(size_t index) noexcept
        {
            return ControlPanel::Navigation::GetControlY(index);
        }

        [[nodiscard]] constexpr float GetSeparatorY() noexcept
        {
            return ControlPanel::GetSeparatorY();
        }

        [[nodiscard]] constexpr float GetActionButtonY(size_t index) noexcept
        {
            return ControlPanel::ActionButtons::GetButtonY(index);
        }

        [[nodiscard]] constexpr float GetSliderYOffset(size_t index) noexcept
        {
            return AudioSettingsPanel::Sliders::GetSliderY(index);
        }

        [[nodiscard]] constexpr float GetSliderWidth() noexcept
        {
            return AudioSettingsPanel::Sliders::GetWidth();
        }

        [[nodiscard]] constexpr float GetCloseButtonXOffset() noexcept
        {
            return AudioSettingsPanel::CloseButton::GetXOffset();
        }

        [[nodiscard]] constexpr float GetCloseButtonYOffset() noexcept
        {
            return AudioSettingsPanel::CloseButton::GetYOffset();
        }

    } // namespace UILayout

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_LAYOUT_H