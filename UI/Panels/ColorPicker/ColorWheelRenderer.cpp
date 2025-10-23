// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ColorWheelRenderer static utility functions.
//
// Implementation details:
// - DrawWheel: Direct bitmap rendering with opacity
// - DrawBorder: Interpolates alpha and thickness for smooth animation
// - DrawHoverPreview: Uses EaseOutBack for a subtle "pop" effect
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Panels/ColorPicker/ColorWheelRenderer.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Anonymous namespace for internal constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr float kPreviewSize = 24.0f;
        constexpr float kPreviewOffset = 4.0f;
        constexpr float kBorderOffset = 2.0f;
        constexpr float kBorderMinAlpha = 0.3f;
        constexpr float kBorderHoveredAlpha = 1.0f;
        constexpr float kBorderUnhoveredAlpha = 0.6f;
        constexpr float kBorderMinThickness = 1.0f;
        constexpr float kBorderMaxThickness = 2.0f;
        constexpr float kBorderGray = 0.5f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Drawing Methods
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ColorWheelRenderer::DrawWheel(
        Canvas& canvas,
        ID2D1Bitmap* bitmap,
        const Rect& bounds,
        float alpha
    ) {
        if (!bitmap) {
            return;
        }

        ID2D1RenderTarget* rt = canvas.GetRenderTarget();
        if (!rt) {
            return;
        }

        const D2D1_RECT_F d2dRect = D2D1::RectF(
            bounds.x,
            bounds.y,
            bounds.GetRight(),
            bounds.GetBottom()
        );

        rt->DrawBitmap(bitmap, d2dRect, alpha);
    }

    void ColorWheelRenderer::DrawBorder(
        Canvas& canvas,
        const Rect& bounds,
        bool isHovered,
        float animationProgress
    ) {
        const Point center = CalculateCenter(bounds);
        const float radius = CalculateRadius(bounds);
        const float alpha = CalculateBorderAlpha(isHovered, animationProgress);
        const float thickness = CalculateBorderThickness(animationProgress);

        const Color borderColor = Color(kBorderGray, kBorderGray, kBorderGray, alpha);
        const Paint paint = Paint::Stroke(borderColor, thickness);

        canvas.DrawCircle(center, radius + kBorderOffset, paint);
    }

    void ColorWheelRenderer::DrawHoverPreview(
        Canvas& canvas,
        const Rect& bounds,
        const Color& hoverColor,
        float animationProgress
    ) {
        const float scale = Helpers::Math::EaseOutBack(animationProgress);
        const float actualSize = kPreviewSize * scale;

        const Rect previewRect = CalculatePreviewRect(bounds, actualSize);

        // Draw preview color
        Color previewColor = hoverColor;
        previewColor.a *= animationProgress;

        const Paint fillPaint = Paint::Fill(previewColor);
        canvas.DrawRectangle(previewRect, fillPaint);

        // Draw border
        const Rect border(
            previewRect.x - 1.0f,
            previewRect.y - 1.0f,
            actualSize + 2.0f,
            actualSize + 2.0f
        );

        const Paint strokePaint = Paint::Stroke(
            Color(kBorderGray, kBorderGray, kBorderGray, animationProgress),
            1.0f
        );

        canvas.DrawRectangle(border, strokePaint);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] Point ColorWheelRenderer::CalculateCenter(const Rect& bounds) {
        return {
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        };
    }

    [[nodiscard]] float ColorWheelRenderer::CalculateRadius(const Rect& bounds) {
        return bounds.width * 0.5f;
    }

    [[nodiscard]] float ColorWheelRenderer::CalculateBorderAlpha(
        bool isHovered,
        float animationProgress
    ) {
        const float baseAlpha = isHovered ? kBorderHoveredAlpha : kBorderUnhoveredAlpha;
        return Helpers::Math::Lerp(kBorderMinAlpha, baseAlpha, animationProgress);
    }

    [[nodiscard]] float ColorWheelRenderer::CalculateBorderThickness(float animationProgress) {
        return Helpers::Math::Lerp(kBorderMinThickness, kBorderMaxThickness, animationProgress);
    }

    [[nodiscard]] Rect ColorWheelRenderer::CalculatePreviewRect(
        const Rect& bounds,
        float actualSize
    ) {
        const float radius = CalculateRadius(bounds);
        const float x = bounds.x + radius - actualSize * 0.5f;
        const float y = bounds.y - actualSize - kPreviewOffset;

        return Rect(x, y, actualSize, actualSize);
    }

} // namespace Spectrum