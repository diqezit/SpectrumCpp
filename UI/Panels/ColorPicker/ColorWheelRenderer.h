#ifndef SPECTRUM_CPP_COLOR_WHEEL_RENDERER_H
#define SPECTRUM_CPP_COLOR_WHEEL_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Color wheel rendering utilities
//
// Direct bitmap rendering without intermediate abstractions
// Border and preview drawing inlined for minimal overhead
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