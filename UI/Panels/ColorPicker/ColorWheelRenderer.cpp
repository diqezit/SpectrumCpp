// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Color wheel renderer implementation with inlined calculations
//
// All geometry calculations inlined to avoid function call overhead
// Direct canvas operations without intermediate abstractions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Panels/ColorPicker/ColorWheelRenderer.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"

namespace Spectrum {

    void ColorWheelRenderer::DrawWheel(
        Canvas& canvas,
        ID2D1Bitmap* bitmap,
        const Rect& bounds,
        float alpha
    )
    {
        if (!bitmap)
        {
            return;
        }

        ID2D1RenderTarget* rt = canvas.GetRenderTarget();
        if (!rt)
        {
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
    )
    {
        const Point center = {
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        };
        const float radius = bounds.width * 0.5f;

        const float baseAlpha = isHovered ? 1.0f : 0.6f;
        const float alpha = Helpers::Math::Lerp(0.3f, baseAlpha, animationProgress);
        const float thickness = Helpers::Math::Lerp(1.0f, 2.0f, animationProgress);

        const Color borderColor = Color(0.5f, 0.5f, 0.5f, alpha);
        const Paint paint = Paint::Stroke(borderColor, thickness);

        canvas.DrawCircle(center, radius + 2.0f, paint);
    }

    void ColorWheelRenderer::DrawHoverPreview(
        Canvas& canvas,
        const Rect& bounds,
        const Color& hoverColor,
        float animationProgress
    )
    {
        const float scale = Helpers::Math::EaseOutBack(animationProgress);
        const float actualSize = 24.0f * scale;
        const float radius = bounds.width * 0.5f;

        const float x = bounds.x + radius - actualSize * 0.5f;
        const float y = bounds.y - actualSize - 4.0f;

        const Rect previewRect(x, y, actualSize, actualSize);

        Color previewColor = hoverColor;
        previewColor.a *= animationProgress;

        const Paint fillPaint = Paint::Fill(previewColor);
        canvas.DrawRectangle(previewRect, fillPaint);

        const Rect border(
            previewRect.x - 1.0f,
            previewRect.y - 1.0f,
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