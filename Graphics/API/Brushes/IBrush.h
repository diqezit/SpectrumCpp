#ifndef SPECTRUM_CPP_IBRUSH_H
#define SPECTRUM_CPP_IBRUSH_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the IBrush interface, a base for all paint sources (solid, gradient).
// This enables polymorphic handling of different brush types within the Paint object.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

namespace Spectrum {

    class IBrush {
    public:
        virtual ~IBrush() = default;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_IBRUSH_H