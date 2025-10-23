#ifndef SPECTRUM_CPP_SOLID_COLOR_BRUSH_H
#define SPECTRUM_CPP_SOLID_COLOR_BRUSH_H

#include "Graphics/API/Brushes/IBrush.h"
#include "Common/Types.h"

namespace Spectrum {

    class SolidColorBrush final : public IBrush {
    public:
        Color color;

        explicit SolidColorBrush(const Color& c) : color(c) {}
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_SOLID_COLOR_BRUSH_H