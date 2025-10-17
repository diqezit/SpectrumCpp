#ifndef SPECTRUM_CPP_COLOR_WHEEL_RENDERER_H
#define SPECTRUM_CPP_COLOR_WHEEL_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the ColorWheelRenderer, a static utility class for
// drawing the visual elements of the ColorPicker.
//
// Responsibilities:
// - Drawing the pre-generated color wheel bitmap
// - Rendering the animated border on hover
// - Drawing the color preview swatch
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Common/Types.h"

namespace Spectrum {

    class Canvas;

    class ColorWheelRenderer {
    public:
        static void DrawWheel(
            Canvas& canvas,
            ID2D1Bitmap* bitmap,
            const Rect& bounds,
            float alpha = 1.0f
        );

        static void DrawBorder(
            Canvas& canvas,
            const Rect& bounds,
            bool isHovered,
            float animationProgress = 1.0f
        );

        static void DrawHoverPreview(
            Canvas& canvas,
            const Rect& bounds,
            const Color& hoverColor,
            float animationProgress = 1.0f
        );
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_COLOR_WHEEL_RENDERER_H