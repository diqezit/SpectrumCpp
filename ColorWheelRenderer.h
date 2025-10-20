#ifndef SPECTRUM_CPP_COLOR_WHEEL_RENDERER_H
#define SPECTRUM_CPP_COLOR_WHEEL_RENDERER_H

#include "Common.h"
#include "GraphicsContext.h"

namespace Spectrum {

    class ColorWheelRenderer {
    public:
        static void DrawWheel(GraphicsContext& context, ID2D1Bitmap* bitmap, const Rect& bounds) {
            if (bitmap) {
                D2D1_RECT_F d2dRect = D2D1::RectF(bounds.x, bounds.y, bounds.GetRight(), bounds.GetBottom());
                context.GetRenderTarget()->DrawBitmap(bitmap, d2dRect);
            }
        }

        static void DrawBorder(GraphicsContext& context, const Rect& bounds, bool isHovered) {
            const Point center = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
            const float radius = bounds.width * 0.5f;
            const Color borderColor = isHovered
                ? Color(0.5f, 0.5f, 0.5f, 1.0f)
                : Color(0.3f, 0.3f, 0.3f, 1.0f);
            context.DrawCircle(center, radius + 2.0f, borderColor, false);
        }

        static void DrawHoverPreview(GraphicsContext& context, const Rect& bounds, const Color& hoverColor) {
            constexpr float previewSize = 24.0f;
            const float radius = bounds.width * 0.5f;
            const float x = bounds.x + radius - previewSize * 0.5f;
            const float y = bounds.y - previewSize - 4.0f;

            const Rect r(x, y, previewSize, previewSize);
            context.DrawRectangle(r, hoverColor, true);

            const Rect border(x - 1.0f, y - 1.0f, previewSize + 2.0f, previewSize + 2.0f);
            context.DrawRectangle(border, Color(0.5f, 0.5f, 0.5f, 1.0f), false);
        }
    };

}

#endif