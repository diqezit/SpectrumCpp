#ifndef SPECTRUM_CPP_PANEL_DRAW_HELPER_H
#define SPECTRUM_CPP_PANEL_DRAW_HELPER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines PanelDrawHelper, a collection of static utility functions for
// drawing common panel elements like backgrounds and titles.
// 
// This centralizes the visual style of panels and simplifies the Draw()
// methods of individual panel classes.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Common/MathUtils.h"
#include "UI/Common/UILayout.h"
#include <string>

namespace Spectrum {

    namespace PanelDrawHelper
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
            float animationProgress
        )
        {
            const float scale = Utils::EaseInOut(animationProgress);
            const float alpha = animationProgress;

            const Color bgColor = UILayout::kPanelBackgroundColor.WithAlpha(0.95f * alpha);
            const Color outlineColor = UILayout::kPanelBorderColor.WithAlpha(0.1f * alpha);

            canvas.PushTransform();
            const Point center = {
                panelRect.x + panelRect.width * 0.5f,
                panelRect.y + panelRect.height * 0.5f
            };
            canvas.ScaleAt(center, scale, scale);

            const Paint fillPaint = Paint::Fill(bgColor);
            const Paint strokePaint = Paint::Stroke(outlineColor, 1.0f);

            canvas.DrawRoundedRectangle(panelRect, Detail::kModalCornerRadius, fillPaint);
            canvas.DrawRoundedRectangle(panelRect, Detail::kModalCornerRadius, strokePaint);

            canvas.PopTransform();
        }

        inline void DrawTitle(
            Canvas& canvas,
            const std::wstring& text,
            const Point& position,
            float animationProgress
        )
        {
            const Color textColor = Color::White().WithAlpha(animationProgress);

            TextStyle style = TextStyle::Default()
                .WithColor(textColor)
                .WithSize(18.0f)
                .WithAlign(TextAlign::Center)
                .WithParagraphAlign(ParagraphAlign::Center);

            canvas.DrawText(
                text,
                position,
                style
            );
        }

        inline void DrawSlideToggleButton(
            Canvas& canvas,
            const Rect& toggleRect,
            bool isHovered,
            bool isPanelHidden
        )
        {
            const Color bgColor = isHovered
                ? UILayout::kToggleButtonHoverColor
                : UILayout::kToggleButtonColor;

            const Paint fillPaint = Paint::Fill(bgColor);
            const Paint strokePaint = Paint::Stroke(
                UILayout::kToggleButtonBorderColor.WithAlpha(bgColor.a),
                1.0f
            );

            canvas.DrawRoundedRectangle(toggleRect, Detail::kToggleCornerRadius, fillPaint);
            canvas.DrawRoundedRectangle(toggleRect, Detail::kToggleCornerRadius, strokePaint);

            const Point center = {
                toggleRect.x + toggleRect.width * 0.5f,
                toggleRect.y + toggleRect.height * 0.5f
            };

            const Color arrowColor = { 1.0f, 1.0f, 1.0f, 0.8f };
            const Paint arrowPaint = Paint::Stroke(arrowColor, Detail::kToggleBorderThickness);

            if (isPanelHidden)
            {
                canvas.DrawPolyline(
                    {
                        { center.x - Detail::kArrowHalfWidth, center.y - Detail::kArrowHalfHeight },
                        { center.x + Detail::kArrowHalfWidth, center.y },
                        { center.x - Detail::kArrowHalfWidth, center.y + Detail::kArrowHalfHeight }
                    },
                    arrowPaint
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
                    arrowPaint
                );
            }
        }

    } // namespace PanelDrawHelper

} // namespace Spectrum

#endif // SPECTRUM_CPP_PANEL_DRAW_HELPER_H