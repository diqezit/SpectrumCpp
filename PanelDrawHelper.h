#ifndef SPECTRUM_CPP_PANEL_DRAW_HELPER_H
#define SPECTRUM_CPP_PANEL_DRAW_HELPER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines PanelDrawHelper namespace with utility functions for common UI
// drawing operations.
// 
// This namespace provides functions for drawing standard UI elements like
// modal backgrounds, titles, and toggle buttons with consistent styling.
// All functions are stateless and operate purely on provided parameters.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "GraphicsContext.h"
#include "MathUtils.h"
#include <string>

namespace Spectrum::PanelDrawHelper
{
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Detail
    {
        constexpr float kModalCornerRadius = 8.0f;
        constexpr float kToggleCornerRadius = 3.0f;
        constexpr float kArrowHalfWidth = 3.0f;
        constexpr float kArrowHalfHeight = 6.0f;
        constexpr float kToggleBorderThickness = 2.0f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Utilities
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline void DrawModalBackground(
        GraphicsContext& context,
        const Rect& panelRect,
        float animationProgress)
    {
        const float scale = Utils::EaseInOut(animationProgress);

        const Color bgColor = { 0.1f, 0.1f, 0.12f, 0.95f * animationProgress };
        const Color outlineColor = { 1.0f, 1.0f, 1.0f, 0.1f * animationProgress };

        context.PushTransform();

        const Point center = {
            panelRect.x + panelRect.width * 0.5f,
            panelRect.y + panelRect.height * 0.5f
        };

        context.ScaleAt(center, scale, scale);

        context.DrawRoundedRectangle(panelRect, Detail::kModalCornerRadius, bgColor, true);
        context.DrawRoundedRectangle(panelRect, Detail::kModalCornerRadius, outlineColor, false);

        context.PopTransform();
    }

    inline void DrawTitle(
        GraphicsContext& context,
        const std::wstring& text,
        const Point& position,
        float animationProgress)
    {
        const Color textColor = { 1.0f, 1.0f, 1.0f, animationProgress };
        context.DrawText(text, position, textColor, 18.0f, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    inline void DrawSlideToggleButton(
        GraphicsContext& context,
        const Rect& toggleRect,
        bool isHovered,
        bool isPanelHidden)
    {
        const Color bgColor = isHovered
            ? Color(0.3f, 0.3f, 0.3f, 0.7f)
            : Color(0.1f, 0.1f, 0.1f, 0.7f);

        context.DrawRoundedRectangle(toggleRect, Detail::kToggleCornerRadius, bgColor, true);
        context.DrawRoundedRectangle(toggleRect, Detail::kToggleCornerRadius, Color(1.0f, 1.0f, 1.0f, 0.1f), false);

        const Point center = {
            toggleRect.x + toggleRect.width * 0.5f,
            toggleRect.y + toggleRect.height * 0.5f
        };

        constexpr Color arrowColor = { 1.0f, 1.0f, 1.0f, 0.8f };

        if (isPanelHidden)
        {
            context.DrawPolyline(
                {
                    { center.x - Detail::kArrowHalfWidth, center.y - Detail::kArrowHalfHeight },
                    { center.x + Detail::kArrowHalfWidth, center.y },
                    { center.x - Detail::kArrowHalfWidth, center.y + Detail::kArrowHalfHeight }
                },
                arrowColor,
                Detail::kToggleBorderThickness
            );
        }
        else
        {
            context.DrawPolyline(
                {
                    { center.x + Detail::kArrowHalfWidth, center.y - Detail::kArrowHalfHeight },
                    { center.x - Detail::kArrowHalfWidth, center.y },
                    { center.x + Detail::kArrowHalfWidth, center.y + Detail::kArrowHalfHeight }
                },
                arrowColor,
                Detail::kToggleBorderThickness
            );
        }
    }

} // namespace Spectrum::PanelDrawHelper

#endif // SPECTRUM_CPP_PANEL_DRAW_HELPER_H