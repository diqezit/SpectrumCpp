#ifndef SPECTRUM_CPP_RADIAL_GRADIENT_BRUSH_H
#define SPECTRUM_CPP_RADIAL_GRADIENT_BRUSH_H

#include "Graphics/API/Brushes/IBrush.h"
#include "Graphics/API/Brushes/GradientStop.h"
#include "Common/Types.h"
#include <vector>

namespace Spectrum {

    class RadialGradientBrush final : public IBrush {
    public:
        Point center;
        float radiusX;
        float radiusY;
        std::vector<GradientStop> stops;

        RadialGradientBrush(
            const Point& c,
            float rX,
            float rY,
            const std::vector<GradientStop>& gradientStops
        ) : center(c), radiusX(rX), radiusY(rY), stops(gradientStops) {
        }

        RadialGradientBrush(
            const Point& c,
            float r,
            const std::vector<GradientStop>& gradientStops
        ) : center(c), radiusX(r), radiusY(r), stops(gradientStops) {
        }
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RADIAL_GRADIENT_BRUSH_H