#ifndef SPECTRUM_CPP_RENDER_ENUMS_H
#define SPECTRUM_CPP_RENDER_ENUMS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines enumerations for rendering quality and composition modes.
// These control how primitives are rasterized and blended.
//
// Design notes:
// - Maps directly to Direct2D equivalents for zero-cost abstraction
// - Provides semantic names instead of D2D1_ prefixed constants
// - Extensible for future rendering modes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <cstdint>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Anti-Aliasing Mode - Controls edge smoothing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class AntiAliasMode : uint8_t {
        None = 0,           // Aliased/pixelated edges (faster)
        PerPrimitive = 1    // Anti-aliased per shape (smoother)
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Fill Rule - How to determine the interior of complex paths
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class FillRule : uint8_t {
        EvenOdd = 0,     // Alternate/even-odd rule
        Winding = 1      // Non-zero winding rule
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Blend Mode - How colors are composited
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class BlendMode : uint8_t {
        SourceOver = 0,     // Normal alpha blending (default)
        SourceIn = 1,       // Source inside destination alpha
        SourceOut = 2,      // Source outside destination alpha
        DestinationOver = 3,// Destination over source
        DestinationIn = 4,  // Destination inside source alpha
        DestinationOut = 5, // Destination outside source alpha
        Add = 6,            // Additive blending (glow effect)
        Multiply = 7,       // Multiplicative blending (shadow effect)
        Screen = 8,         // Screen blending (lighten)
        Overlay = 9,        // Overlay blending (contrast)
        Xor = 10           // Exclusive OR
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Filter Quality - Image/gradient sampling quality
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class FilterQuality : uint8_t {
        None = 0,      // Nearest neighbor (pixelated)
        Low = 1,       // Bilinear interpolation
        Medium = 2,    // Anisotropic filtering
        High = 3       // High quality bicubic
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Composite Mode - How layers are combined
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class CompositeMode : uint8_t {
        SourceCopy = 0,     // Replace destination
        DestinationCopy = 1,// Keep destination
        SourceOver = 2,     // Standard alpha blend
        DestinationOver = 3,// Destination on top
        Clear = 4           // Clear to transparent
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDER_ENUMS_H