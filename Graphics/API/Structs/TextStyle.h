#ifndef SPECTRUM_CPP_TEXT_STYLE_H
#define SPECTRUM_CPP_TEXT_STYLE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines TextStyle - comprehensive text rendering configuration.
// Combines font properties, alignment, and visual effects into a single
// reusable configuration object.
//
// Usage:
//   auto style = TextStyle::Default()
//                    .WithFont(L"Arial")
//                    .WithSize(18.0f)
//                    .WithWeight(FontWeight::Bold)
//                    .WithColor(Color::Red());
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Types.h"
#include "Graphics/API/Enums/TextEnums.h"
#include <string>
#include <algorithm>

namespace Spectrum {

    struct TextStyle {
        // Font properties
        std::wstring fontFamily = L"Segoe UI";
        float fontSize = 14.0f;
        FontWeight weight = FontWeight::Normal;
        FontStyle style = FontStyle::Normal;
        FontStretch stretch = FontStretch::Normal;

        // Layout properties
        TextAlign textAlign = TextAlign::Leading;
        ParagraphAlign paragraphAlign = ParagraphAlign::Near;
        float lineHeight = 1.2f;
        float letterSpacing = 0.0f;
        float wordSpacing = 0.0f;

        // Visual properties
        Color color = Color::White();
        Color outlineColor = Color::Transparent();
        float outlineWidth = 0.0f;
        TextDecoration decoration = TextDecoration::None;

        // Advanced properties
        bool kerning = true;
        bool ligatures = true;
        float baseline = 0.0f;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Immutable Builder Methods
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] TextStyle WithFont(const std::wstring& family) const {
            TextStyle result = *this;
            result.fontFamily = family;
            return result;
        }

        [[nodiscard]] TextStyle WithSize(float size) const {
            TextStyle result = *this;
            result.fontSize = std::max(1.0f, size);
            return result;
        }

        [[nodiscard]] TextStyle WithWeight(FontWeight w) const {
            TextStyle result = *this;
            result.weight = w;
            return result;
        }

        [[nodiscard]] TextStyle WithStyle(FontStyle s) const {
            TextStyle result = *this;
            result.style = s;
            return result;
        }

        [[nodiscard]] TextStyle WithStretch(FontStretch s) const {
            TextStyle result = *this;
            result.stretch = s;
            return result;
        }

        [[nodiscard]] TextStyle WithColor(const Color& c) const {
            TextStyle result = *this;
            result.color = c;
            return result;
        }

        [[nodiscard]] TextStyle WithOutline(const Color& c, float width) const {
            TextStyle result = *this;
            result.outlineColor = c;
            result.outlineWidth = width;
            return result;
        }

        [[nodiscard]] TextStyle WithAlign(TextAlign align) const {
            TextStyle result = *this;
            result.textAlign = align;
            return result;
        }

        [[nodiscard]] TextStyle WithParagraphAlign(ParagraphAlign align) const {
            TextStyle result = *this;
            result.paragraphAlign = align;
            return result;
        }

        [[nodiscard]] TextStyle WithLineHeight(float height) const {
            TextStyle result = *this;
            result.lineHeight = std::max(0.0f, height);
            return result;
        }

        [[nodiscard]] TextStyle WithLetterSpacing(float spacing) const {
            TextStyle result = *this;
            result.letterSpacing = spacing;
            return result;
        }

        [[nodiscard]] TextStyle WithWordSpacing(float spacing) const {
            TextStyle result = *this;
            result.wordSpacing = spacing;
            return result;
        }

        [[nodiscard]] TextStyle WithDecoration(TextDecoration dec) const {
            TextStyle result = *this;
            result.decoration = dec;
            return result;
        }

        [[nodiscard]] TextStyle WithUnderline(bool enable = true) const {
            TextStyle result = *this;
            if (enable) {
                result.decoration = result.decoration | TextDecoration::Underline;
            }
            else {
                result.decoration = static_cast<TextDecoration>(
                    static_cast<uint8_t>(result.decoration) & ~static_cast<uint8_t>(TextDecoration::Underline)
                    );
            }
            return result;
        }

        [[nodiscard]] TextStyle WithStrikethrough(bool enable = true) const {
            TextStyle result = *this;
            if (enable) {
                result.decoration = result.decoration | TextDecoration::Strikethrough;
            }
            else {
                result.decoration = static_cast<TextDecoration>(
                    static_cast<uint8_t>(result.decoration) & ~static_cast<uint8_t>(TextDecoration::Strikethrough)
                    );
            }
            return result;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Factory Methods for Common Styles
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] static TextStyle Default() {
            return TextStyle{};
        }

        [[nodiscard]] static TextStyle Title() {
            return TextStyle{}.WithSize(24.0f).WithWeight(FontWeight::Bold);
        }

        [[nodiscard]] static TextStyle Subtitle() {
            return TextStyle{}.WithSize(18.0f).WithWeight(FontWeight::SemiBold);
        }

        [[nodiscard]] static TextStyle Body() {
            return TextStyle{}.WithSize(14.0f).WithWeight(FontWeight::Normal);
        }

        [[nodiscard]] static TextStyle Caption() {
            return TextStyle{}.WithSize(12.0f).WithWeight(FontWeight::Light);
        }

        [[nodiscard]] static TextStyle Code() {
            return TextStyle{}.WithFont(L"Consolas").WithSize(12.0f).WithWeight(FontWeight::Normal);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool HasOutline() const noexcept {
            return outlineWidth > 0.0f && outlineColor.a > 0.0f;
        }

        [[nodiscard]] bool HasDecoration() const noexcept {
            return decoration != TextDecoration::None;
        }

        [[nodiscard]] bool IsUnderlined() const noexcept {
            return (static_cast<uint8_t>(decoration) & static_cast<uint8_t>(TextDecoration::Underline)) != 0;
        }

        [[nodiscard]] bool IsStrikethrough() const noexcept {
            return (static_cast<uint8_t>(decoration) & static_cast<uint8_t>(TextDecoration::Strikethrough)) != 0;
        }
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_TEXT_STYLE_H