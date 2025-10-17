#ifndef SPECTRUM_CPP_TEXT_ENUMS_H
#define SPECTRUM_CPP_TEXT_ENUMS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines enumerations for text rendering configuration.
// Provides type-safe wrappers around DirectWrite constants.
//
// Design notes:
// - FontWeight uses actual weight values (100-900) for CSS compatibility
// - Alignment enums match DirectWrite for zero-cost abstraction
// - Extensible for future text features (decoration, baseline, etc.)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <cstdint>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text Alignment - Horizontal text positioning
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class TextAlign : uint8_t {
        Leading = 0,    // Left for LTR, Right for RTL
        Trailing = 1,   // Right for LTR, Left for RTL
        Center = 2,     // Horizontally centered
        Justified = 3   // Justified with word spacing
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Paragraph Alignment - Vertical text positioning
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class ParagraphAlign : uint8_t {
        Near = 0,    // Top alignment
        Far = 1,     // Bottom alignment
        Center = 2   // Vertically centered
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Font Weight - Thickness of font strokes (100-900 scale)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class FontWeight : uint16_t {
        Thin = 100,
        ExtraLight = 200,
        Light = 300,
        Regular = 400,    // Normal
        Medium = 500,
        SemiBold = 600,
        Bold = 700,
        ExtraBold = 800,
        Black = 900,

        // Aliases for common usage
        Normal = Regular,
        Heavy = Black
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Font Style - Slant of the font
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class FontStyle : uint8_t {
        Normal = 0,   // Upright text
        Italic = 1,   // True italic (designed slant)
        Oblique = 2   // Artificial slant
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Font Stretch - Width of font characters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class FontStretch : uint8_t {
        UltraCondensed = 1,
        ExtraCondensed = 2,
        Condensed = 3,
        SemiCondensed = 4,
        Normal = 5,
        SemiExpanded = 6,
        Expanded = 7,
        ExtraExpanded = 8,
        UltraExpanded = 9
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text Decoration - Underline, strikethrough, etc.
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    enum class TextDecoration : uint8_t {
        None = 0,
        Underline = 1 << 0,
        Strikethrough = 1 << 1,
        Overline = 1 << 2
    };

    // Enable bitwise operations for TextDecoration
    inline TextDecoration operator|(TextDecoration a, TextDecoration b) {
        return static_cast<TextDecoration>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
            );
    }

    inline bool operator&(TextDecoration a, TextDecoration b) {
        return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
    }

} // namespace Spectrum

#endif // SPECTRUM_CPP_TEXT_ENUMS_H