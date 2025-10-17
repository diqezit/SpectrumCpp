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
#include "Common/MathUtils.h"

namespace Spectrum {

    void ColorWheelRenderer::DrawWheel(
        Canvas& canvas,
        ID2D1Bitmap* bitmap,
        const Rect& bounds,
        float alpha
    ) {
        if (!bitmap) {
            return;
        }

        ID2D1HwndRenderTarget* rt = canvas.GetRenderTarget();
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
        const Point center = {
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        };

        const float radius = bounds.width * 0.5f;

        const float baseAlpha = isHovered ? 1.0f : 0.6f;
        const float alpha = Utils::Lerp(
            0.3f,
            baseAlpha,
            animationProgress
        );

        const float thickness = Utils::Lerp(
            1.0f,
            2.0f,
            animationProgress
        );

        const Color borderColor = Color(0.5f, 0.5f, 0.5f, alpha);
        const Paint paint = Paint::Stroke(borderColor, thickness);

        canvas.DrawCircle(center, radius + 2.0f, paint);
    }

    void ColorWheelRenderer::DrawHoverPreview(
        Canvas& canvas,
        const Rect& bounds,
        const Color& hoverColor,
        float animationProgress
    ) {
        constexpr float previewSize = 24.0f;
        const float radius = bounds.width * 0.5f;

        const float scale = Utils::EaseOutBack(animationProgress);
        const float actualSize = previewSize * scale;

        const float x = bounds.x + radius - actualSize * 0.5f;
        const float y = bounds.y - actualSize - 4.0f;

        Color previewColor = hoverColor;
        previewColor.a *= animationProgress;

        const Rect r(x, y, actualSize, actualSize);
        const Paint fillPaint = Paint::Fill(previewColor);
        canvas.DrawRectangle(r, fillPaint);

        const Rect border(
            x - 1.0f,
            y - 1.0f,
            actualSize + 2.0f,
            actualSize + 2.0f
        );

        const Paint strokePaint = Paint::Stroke(
            Color(0.5f, 0.5f, 0.5f, animationProgress),
            1.0f
        );

        canvas.DrawRectangle(border, strokePaint);
    }

} // namespace Spectrum