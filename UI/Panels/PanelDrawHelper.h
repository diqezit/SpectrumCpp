#ifndef SPECTRUM_CPP_PANEL_DRAW_HELPER_H
#define SPECTRUM_CPP_PANEL_DRAW_HELPER_H

#include "Common.h"
#include "Canvas.h"
#include "MathUtils.h"
#include <string>

namespace Spectrum::PanelDrawHelper
{
    namespace Detail
    {
        constexpr float kModalCornerRadius = 8.0f;
        constexpr float kToggleCornerRadius = 3.0f;
        constexpr float kArrowHalfWidth = 3.0f;
        constexpr float kArrowHalfHeight = 6.0f;
        constexpr float kToggleBorderThickness = 2.0f;
    }

    inline void DrawModalBackground(
        Canvas& canvas,
        const Rect& panelRect,
        float animationProgress)
    {
        const float scale = Utils::EaseInOut(animationProgress);

        const Color bgColor = { 0.1f, 0.1f, 0.12f, 0.95f * animationProgress };
        const Color outlineColor = { 1.0f, 1.0f, 1.0f, 0.1f * animationProgress };

        canvas.PushTransform();

        const Point center = {
            panelRect.x + panelRect.width * 0.5f,
            panelRect.y + panelRect.height * 0.5f
        };

        canvas.ScaleAt(center, scale, scale);

        const Paint fillPaint{ bgColor, 1.0f, true };
        const Paint strokePaint{ outlineColor, 1.0f, false };

        canvas.DrawRoundedRectangle(panelRect, Detail::kModalCornerRadius, fillPaint);
        canvas.DrawRoundedRectangle(panelRect, Detail::kModalCornerRadius, strokePaint);

        canvas.PopTransform();
    }

    inline void DrawTitle(
        Canvas& canvas,
        const std::wstring& text,
        const Point& position,
        float animationProgress)
    {
        const Color textColor = { 1.0f, 1.0f, 1.0f, animationProgress };
        canvas.DrawText(text, position, textColor, 18.0f, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    inline void DrawSlideToggleButton(
        Canvas& canvas,
        const Rect& toggleRect,
        bool isHovered,
        bool isPanelHidden)
    {
        const Color bgColor = isHovered
            ? Color(0.3f, 0.3f, 0.3f, 0.7f)
            : Color(0.1f, 0.1f, 0.1f, 0.7f);

        const Paint fillPaint{ bgColor, 1.0f, true };
        const Paint strokePaint{ Color(1.0f, 1.0f, 1.0f, 0.1f), 1.0f, false };

        canvas.DrawRoundedRectangle(toggleRect, Detail::kToggleCornerRadius, fillPaint);
        canvas.DrawRoundedRectangle(toggleRect, Detail::kToggleCornerRadius, strokePaint);

        const Point center = {
            toggleRect.x + toggleRect.width * 0.5f,
            toggleRect.y + toggleRect.height * 0.5f
        };

        constexpr Color arrowColor = { 1.0f, 1.0f, 1.0f, 0.8f };

        if (isPanelHidden)
        {
            canvas.DrawPolyline(
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
            canvas.DrawPolyline(
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