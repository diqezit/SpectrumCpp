#ifndef SPECTRUM_CPP_PAINT_ENUMS_H
#define SPECTRUM_CPP_PAINT_ENUMS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines enumerations for paint styles and stroke configurations.
// These enums provide type-safe alternatives to boolean flags and magic numbers.
//
// Design notes:
// - All enums use explicit underlying types (uint8_t) for memory efficiency
// - Values are explicitly numbered for serialization compatibility
// - Follows SkiaSharp naming conventions for familiarity
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <cstdint>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Paint Style - Determines how shapes are rendered
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class PaintStyle : uint8_t {
        Fill = 0,           // Fill the shape interior only
        Stroke = 1,         // Draw the shape outline only
        FillAndStroke = 2   // Both fill and stroke
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Stroke Cap - How to render line endpoints
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class StrokeCap : uint8_t {
        Flat = 0,    // No cap extension (D2D1_CAP_STYLE_FLAT)
        Round = 1,   // Semicircular cap (D2D1_CAP_STYLE_ROUND)
        Square = 2   // Square cap extending by strokeWidth/2 (D2D1_CAP_STYLE_SQUARE)
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Stroke Join - How to render corners where lines meet
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class StrokeJoin : uint8_t {
        Miter = 0,   // Sharp corners with miter limit (D2D1_LINE_JOIN_MITER)
        Round = 1,   // Rounded corners (D2D1_LINE_JOIN_ROUND)
        Bevel = 2    // Beveled/cut corners (D2D1_LINE_JOIN_BEVEL)
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Dash Style - Predefined line dash patterns
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class DashStyle : uint8_t {
        Solid = 0,      // Continuous line (no dashing)
        Dash = 1,       // Pattern: ---- ---- ----
        Dot = 2,        // Pattern: . . . . . .
        DashDot = 3,    // Pattern: ---- . ---- .
        DashDotDot = 4, // Pattern: ---- . . ---- . .
        Custom = 5      // User-defined dash array
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Path Effect - Special effects for paths (future extension)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class PathEffect : uint8_t {
        None = 0,       // No effect
        Discrete = 1,   // Break path into segments
        Corner = 2      // Round sharp corners
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PAINT_ENUMS_H