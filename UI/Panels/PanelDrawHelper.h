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
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "UI/Common/UILayout.h"
#include <string>
#include <vector>

namespace Spectrum {

    namespace PanelDrawHelper
    {
        namespace Detail
        {
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Constants
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            constexpr float kModalCornerRadius = 8.0f;
            constexpr float kToggleCornerRadius = 3.0f;
            constexpr float kArrowHalfWidth = 3.0f;
            constexpr float kArrowHalfHeight = 6.0f;
            constexpr float kToggleBorderThickness = 2.0f;
            constexpr float kTitleFontSize = 18.0f;
            constexpr float kModalBackgroundAlpha = 0.95f;
            constexpr float kModalBorderAlpha = 0.1f;
            constexpr float kModalBorderWidth = 1.0f;
            constexpr float kToggleBorderWidth = 1.0f;
            constexpr float kArrowAlpha = 0.8f;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Modal Background - Color Calculations
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] inline Color CalculateModalBackgroundColor(float alpha) noexcept
            {
                return UILayout::kPanelBackgroundColor.WithAlpha(kModalBackgroundAlpha * alpha);
            }

            [[nodiscard]] inline Color CalculateModalBorderColor(float alpha) noexcept
            {
                return UILayout::kPanelBorderColor.WithAlpha(kModalBorderAlpha * alpha);
            }

            [[nodiscard]] inline float CalculateModalScale(float animationProgress) noexcept
            {
                return Helpers::Math::EaseInOut(animationProgress);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Modal Background - Paint Creation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] inline Paint CreateModalBackgroundPaint(float alpha)
            {
                const Color bgColor = CalculateModalBackgroundColor(alpha);
                return Paint::Fill(bgColor);
            }

            [[nodiscard]] inline Paint CreateModalBorderPaint(float alpha)
            {
                const Color borderColor = CalculateModalBorderColor(alpha);
                return Paint::Stroke(borderColor, kModalBorderWidth);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Modal Background - Geometry
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] inline Point CalculateRectCenter(const Rect& rect) noexcept
            {
                return {
                    rect.x + rect.width * 0.5f,
                    rect.y + rect.height * 0.5f
                };
            }

            inline void ApplyModalTransform(Canvas& canvas, const Rect& rect, float scale)
            {
                const Point center = CalculateRectCenter(rect);
                canvas.ScaleAt(center, scale, scale);
            }

            inline void DrawModalRectangle(
                Canvas& canvas,
                const Rect& rect,
                const Paint& paint
            )
            {
                canvas.DrawRoundedRectangle(rect, kModalCornerRadius, paint);
            }

            inline void DrawModalBackgroundFill(Canvas& canvas, const Rect& rect, float alpha)
            {
                const Paint fillPaint = CreateModalBackgroundPaint(alpha);
                DrawModalRectangle(canvas, rect, fillPaint);
            }

            inline void DrawModalBackgroundBorder(Canvas& canvas, const Rect& rect, float alpha)
            {
                const Paint strokePaint = CreateModalBorderPaint(alpha);
                DrawModalRectangle(canvas, rect, strokePaint);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Title - Text Style Creation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] inline Color CalculateTitleColor(float animationProgress) noexcept
            {
                return Color::White().WithAlpha(animationProgress);
            }

            [[nodiscard]] inline TextStyle CreateTitleTextStyle(float animationProgress)
            {
                const Color textColor = CalculateTitleColor(animationProgress);

                return TextStyle::Default()
                    .WithColor(textColor)
                    .WithSize(kTitleFontSize)
                    .WithAlign(TextAlign::Center)
                    .WithParagraphAlign(ParagraphAlign::Center);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Toggle Button - Color Calculations
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] inline Color CalculateToggleBackgroundColor(bool isHovered) noexcept
            {
                return isHovered
                    ? UILayout::kToggleButtonHoverColor
                    : UILayout::kToggleButtonColor;
            }

            [[nodiscard]] inline Color CalculateToggleBorderColor(const Color& bgColor) noexcept
            {
                return UILayout::kToggleButtonBorderColor.WithAlpha(bgColor.a);
            }

            [[nodiscard]] inline Color GetArrowColor() noexcept
            {
                return { 1.0f, 1.0f, 1.0f, kArrowAlpha };
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Toggle Button - Paint Creation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] inline Paint CreateToggleBackgroundPaint(bool isHovered)
            {
                const Color bgColor = CalculateToggleBackgroundColor(isHovered);
                return Paint::Fill(bgColor);
            }

            [[nodiscard]] inline Paint CreateToggleBorderPaint(bool isHovered)
            {
                const Color bgColor = CalculateToggleBackgroundColor(isHovered);
                const Color borderColor = CalculateToggleBorderColor(bgColor);
                return Paint::Stroke(borderColor, kToggleBorderWidth);
            }

            [[nodiscard]] inline Paint CreateArrowPaint()
            {
                const Color arrowColor = GetArrowColor();
                return Paint::Stroke(arrowColor, kToggleBorderThickness);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Toggle Button - Geometry
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            [[nodiscard]] inline Point CalculateToggleCenter(const Rect& toggleRect) noexcept
            {
                return {
                    toggleRect.x + toggleRect.width * 0.5f,
                    toggleRect.y + toggleRect.height * 0.5f
                };
            }

            [[nodiscard]] inline std::vector<Point> CreateRightArrowPoints(const Point& center)
            {
                return {
                    { center.x - kArrowHalfWidth, center.y - kArrowHalfHeight },
                    { center.x + kArrowHalfWidth, center.y },
                    { center.x - kArrowHalfWidth, center.y + kArrowHalfHeight }
                };
            }

            [[nodiscard]] inline std::vector<Point> CreateLeftArrowPoints(const Point& center)
            {
                return {
                    { center.x + kArrowHalfWidth, center.y - kArrowHalfHeight },
                    { center.x - kArrowHalfWidth, center.y },
                    { center.x + kArrowHalfWidth, center.y + kArrowHalfHeight }
                };
            }

            [[nodiscard]] inline std::vector<Point> CreateArrowPoints(const Point& center, bool isPanelHidden)
            {
                return isPanelHidden ? CreateRightArrowPoints(center) : CreateLeftArrowPoints(center);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Toggle Button - Drawing
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            inline void DrawToggleBackground(Canvas& canvas, const Rect& toggleRect, bool isHovered)
            {
                const Paint fillPaint = CreateToggleBackgroundPaint(isHovered);
                canvas.DrawRoundedRectangle(toggleRect, kToggleCornerRadius, fillPaint);
            }

            inline void DrawToggleBorder(Canvas& canvas, const Rect& toggleRect, bool isHovered)
            {
                const Paint strokePaint = CreateToggleBorderPaint(isHovered);
                canvas.DrawRoundedRectangle(toggleRect, kToggleCornerRadius, strokePaint);
            }

            inline void DrawToggleArrow(
                Canvas& canvas,
                const Point& center,
                bool isPanelHidden
            )
            {
                const std::vector<Point> arrowPoints = CreateArrowPoints(center, isPanelHidden);
                const Paint arrowPaint = CreateArrowPaint();

                canvas.DrawPolyline(arrowPoints, arrowPaint);
            }

        } // namespace Detail

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public API
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        inline void DrawModalBackground(
            Canvas& canvas,
            const Rect& panelRect,
            float animationProgress
        )
        {
            const float scale = Detail::CalculateModalScale(animationProgress);

            canvas.PushTransform();
            Detail::ApplyModalTransform(canvas, panelRect, scale);

            Detail::DrawModalBackgroundFill(canvas, panelRect, animationProgress);
            Detail::DrawModalBackgroundBorder(canvas, panelRect, animationProgress);

            canvas.PopTransform();
        }

        inline void DrawTitle(
            Canvas& canvas,
            const std::wstring& text,
            const Point& position,
            float animationProgress
        )
        {
            const TextStyle style = Detail::CreateTitleTextStyle(animationProgress);
            canvas.DrawText(text, position, style);
        }

        inline void DrawSlideToggleButton(
            Canvas& canvas,
            const Rect& toggleRect,
            bool isHovered,
            bool isPanelHidden
        )
        {
            Detail::DrawToggleBackground(canvas, toggleRect, isHovered);
            Detail::DrawToggleBorder(canvas, toggleRect, isHovered);

            const Point center = Detail::CalculateToggleCenter(toggleRect);
            Detail::DrawToggleArrow(canvas, center, isPanelHidden);
        }

    } // namespace PanelDrawHelper

} // namespace Spectrum

#endif // SPECTRUM_CPP_PANEL_DRAW_HELPER_H