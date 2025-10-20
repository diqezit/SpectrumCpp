#ifndef SPECTRUM_CPP_PANEL_DRAW_HELPER_H
#define SPECTRUM_CPP_PANEL_DRAW_HELPER_H

#include "Common.h"
#include "GraphicsContext.h"
#include "MathUtils.h"

namespace Spectrum {

    class PanelDrawHelper {
    public:
        static void DrawModalBackground(
            GraphicsContext& context,
            const Rect& panelRect,
            float animationProgress
        ) {
            const float alpha = animationProgress;
            const float scale = Utils::EaseInOut(alpha);

            const Color bgColor = { 0.1f, 0.1f, 0.12f, 0.95f * alpha };
            const Color outlineColor = { 1.0f, 1.0f, 1.0f, 0.1f * alpha };

            context.PushTransform();
            const Point center = {
                panelRect.x + panelRect.width / 2.0f,
                panelRect.y + panelRect.height / 2.0f
            };
            context.ScaleAt(center, scale, scale);

            context.DrawRoundedRectangle(panelRect, 8.0f, bgColor, true);
            context.DrawRoundedRectangle(panelRect, 8.0f, outlineColor, false);

            context.PopTransform();
        }

        static void DrawTitle(
            GraphicsContext& context,
            const std::wstring& text,
            const Point& position,
            float animationProgress
        ) {
            const Color fgColor = { 1.0f, 1.0f, 1.0f, animationProgress };
            context.DrawText(text, position, fgColor, 18.0f, DWRITE_TEXT_ALIGNMENT_CENTER);
        }

        static void DrawSlideToggleButton(
            GraphicsContext& context,
            const Rect& toggleRect,
            bool isHovered,
            bool isPanelHidden
        ) {
            const Color bgColor = isHovered
                ? Color(0.3f, 0.3f, 0.3f, 0.7f)
                : Color(0.1f, 0.1f, 0.1f, 0.7f);

            context.DrawRoundedRectangle(toggleRect, 3.0f, bgColor, true);
            context.DrawRoundedRectangle(toggleRect, 3.0f, Color(1.0f, 1.0f, 1.0f, 0.1f), false);

            const Point center = {
                toggleRect.x + toggleRect.width / 2.0f,
                toggleRect.y + toggleRect.height / 2.0f
            };

            const Color arrowColor = { 1.0f, 1.0f, 1.0f, 0.8f };
            if (isPanelHidden) {
                context.DrawPolyline(
                    { {center.x - 3, center.y - 6}, {center.x + 3, center.y}, {center.x - 3, center.y + 6} },
                    arrowColor, 2.0f
                );
            }
            else {
                context.DrawPolyline(
                    { {center.x + 3, center.y - 6}, {center.x - 3, center.y}, {center.x + 3, center.y + 6} },
                    arrowColor, 2.0f
                );
            }
        }
    };

}

#endif