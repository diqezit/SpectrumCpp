#ifndef SPECTRUM_CPP_LINEAR_GRADIENT_BRUSH_H
#define SPECTRUM_CPP_LINEAR_GRADIENT_BRUSH_H

#include "Graphics/API/Brushes/IBrush.h"
#include "Graphics/API/Brushes/GradientStop.h"
#include "Common/Types.h"
#include <vector>

namespace Spectrum {

    class LinearGradientBrush final : public IBrush {
    public:
        Point startPoint;
        Point endPoint;
        std::vector<GradientStop> stops;

        LinearGradientBrush(
            const Point& start,
            const Point& end,
            const std::vector<GradientStop>& gradientStops
        ) : startPoint(start), endPoint(end), stops(gradientStops) {
        }
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_LINEAR_GRADIENT_BRUSH_H