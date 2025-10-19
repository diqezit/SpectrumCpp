#ifndef SPECTRUM_CPP_SPECTRUM_TYPES_H
#define SPECTRUM_CPP_SPECTRUM_TYPES_H

#include "Common.h"
#include <vector>

namespace Spectrum {

    // Стиль отрисовки баров спектра
    struct BarStyle {
        float spacing = 2.0f;
        float cornerRadius = 0.0f;
        bool useGradient = false;
        std::vector<D2D1_GRADIENT_STOP> gradientStops;
    };

} // namespace Spectrum

#endif