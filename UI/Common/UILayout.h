#ifndef SPECTRUM_CPP_UI_LAYOUT_H
#define SPECTRUM_CPP_UI_LAYOUT_H

#include "Common/Types.h"

namespace Spectrum::UILayout {

    constexpr float kPadding = 16.0f;
    constexpr float kSpacing = 8.0f;
    constexpr float kLargeSpacing = 16.0f;

    constexpr float kToggleButtonWidth = 24.0f;
    constexpr float kToggleButtonHeight = 48.0f;

    inline Color kPanelBackgroundColor = Color(0.05f, 0.05f, 0.10f, 0.95f);
    inline Color kPanelBorderColor = Color(0.3f, 0.3f, 0.4f, 0.8f);
    inline Color kSeparatorColor = Color(0.3f, 0.3f, 0.4f, 0.5f);

    namespace VisualProperties
    {
        constexpr float kPanelCornerRadius = 8.0f;
        constexpr float kDefaultBorderWidth = 1.0f;
    }

    namespace Animation
    {
        constexpr float kSpeed = 8.0f;
        constexpr float kFastSpeed = 12.0f;
    }

    namespace ControlPanel
    {
        constexpr float kWidth = 200.0f;
        constexpr float kTitleHeight = 40.0f;
        constexpr float kButtonHeight = 32.0f;

        constexpr float GetPanelHeight()
        {
            return kTitleHeight + (kButtonHeight + kSpacing) * 3 + kPadding * 2;
        }

        constexpr float GetSeparatorY()
        {
            return kPadding + kTitleHeight;
        }
    }

    namespace AudioSettingsPanel
    {
        constexpr float kWidth = 400.0f;
        constexpr float kTitleHeight = 60.0f;
        constexpr float kSliderHeight = 40.0f;
        constexpr float kButtonHeight = 36.0f;
        constexpr float kLabelYOffset = -20.0f;

        constexpr float GetPanelHeight()
        {
            return kTitleHeight + (kSliderHeight + kSpacing * 3) * 3 + kButtonHeight + kPadding * 3;
        }
    }

    inline Rect CalculateCenteredRect(
        float windowWidth,
        float windowHeight,
        float rectWidth,
        float rectHeight
    )
    {
        return Rect{
            (windowWidth - rectWidth) * 0.5f,
            (windowHeight - rectHeight) * 0.5f,
            rectWidth,
            rectHeight
        };
    }

    inline Point CalculateTopRightPosition(
        float windowWidth,
        float objectWidth,
        float objectHeight
    )
    {
        return Point{
            windowWidth - objectWidth - kPadding,
            kPadding
        };
    }

    inline Point CalculateTopLeftPosition()
    {
        return Point{ kPadding, kPadding };
    }

} // namespace Spectrum::UILayout

#endif // SPECTRUM_CPP_UI_LAYOUT_H