#ifndef SPECTRUM_CPP_STROKE_OPTIONS_H
#define SPECTRUM_CPP_STROKE_OPTIONS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines StrokeOptions - a detailed configuration for stroke rendering.
// This is an optional companion to Paint for advanced stroke control.
//
// Use this when you need fine-grained control over stroke appearance
// beyond what Paint provides.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Enums/PaintEnums.h"
#include <vector>

namespace Spectrum {

    struct StrokeOptions {
        float width = 1.0f;
        StrokeCap cap = StrokeCap::Flat;
        StrokeJoin join = StrokeJoin::Miter;
        float miterLimit = 10.0f;
        DashStyle dashStyle = DashStyle::Solid;
        std::vector<float> dashPattern;
        float dashOffset = 0.0f;

        // Builder methods
        StrokeOptions& SetWidth(float w) {
            width = w;
            return *this;
        }

        StrokeOptions& SetCap(StrokeCap c) {
            cap = c;
            return *this;
        }

        StrokeOptions& SetJoin(StrokeJoin j) {
            join = j;
            return *this;
        }

        StrokeOptions& SetMiterLimit(float limit) {
            miterLimit = limit;
            return *this;
        }

        StrokeOptions& SetDashStyle(DashStyle style) {
            dashStyle = style;
            return *this;
        }

        StrokeOptions& SetDashPattern(const std::vector<float>& pattern) {
            dashPattern = pattern;
            dashStyle = DashStyle::Custom;
            return *this;
        }

        StrokeOptions& SetDashOffset(float offset) {
            dashOffset = offset;
            return *this;
        }

        // Factory methods
        [[nodiscard]] static StrokeOptions Default() {
            return StrokeOptions{};
        }

        [[nodiscard]] static StrokeOptions Round(float width) {
            return StrokeOptions{}
                .SetWidth(width)
                .SetCap(StrokeCap::Round)
                .SetJoin(StrokeJoin::Round);
        }

        [[nodiscard]] static StrokeOptions Dashed(float width, DashStyle style = DashStyle::Dash) {
            return StrokeOptions{}
                .SetWidth(width)
                .SetDashStyle(style);
        }
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_STROKE_OPTIONS_H