#ifndef SPECTRUM_CPP_TEXT_RENDERER_H
#define SPECTRUM_CPP_TEXT_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the TextRenderer class, which is responsible for all text
// drawing operations. It wraps the DirectWrite API to simplify text
// layout and rendering with various styles.
//
// Key responsibilities:
// - Text rendering based on a comprehensive TextStyle object.
// - Caching of DirectWrite text formats for performance.
// - Handling of text alignment, rotation, and outline effects.
//
// Design notes:
// - All public methods now accept a `const TextStyle&`.
// - Complex layout calculations are encapsulated within the renderer.
// - Uses ComPtr for safe resource lifetime management.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include "Graphics/API/Helpers/Rendering/RenderHelpers.h"
#include <string>
#include <array>

namespace Spectrum {

    struct TextStyle;

    class TextRenderer final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit TextRenderer(IDWriteFactory* writeFactory);

        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) = delete;

        void OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget) override;
        void OnDeviceLost() override;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Text Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawText(
            const std::wstring& text,
            const Rect& layoutRect,
            const TextStyle& style
        ) const;

        void DrawText(
            const std::wstring& text,
            const Point& position,
            const TextStyle& style
        ) const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Format Management (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<IDWriteTextFormat> GetOrCreateTextFormat(
            const TextStyle& style
        ) const;

        void ConfigureTextFormat(
            IDWriteTextFormat* format,
            const TextStyle& style
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Text Layout Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<IDWriteTextLayout> CreateTextLayout(
            const std::wstring& text,
            IDWriteTextFormat* format
        ) const;

        [[nodiscard]] bool GetTextMetrics(
            IDWriteTextLayout* layout,
            DWRITE_TEXT_METRICS& metrics
        ) const;

        [[nodiscard]] Point CalculateTextOrigin(
            const Point& position,
            const DWRITE_TEXT_METRICS& metrics,
            const TextStyle& style
        ) const;

        [[nodiscard]] Point CalculateHorizontalAlignment(
            const Point& origin,
            float textWidth,
            const TextStyle& style
        ) const;

        [[nodiscard]] Point CalculateVerticalAlignment(
            const Point& origin,
            float textHeight,
            const TextStyle& style
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Drawing Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawTextWithFormat(
            const std::wstring& text,
            const Rect& layoutRect,
            IDWriteTextFormat* format,
            ID2D1SolidColorBrush* brush
        ) const;

        void DrawTextWithLayout(
            const Point& origin,
            IDWriteTextLayout* layout,
            ID2D1SolidColorBrush* brush
        ) const;

        void DrawTextInternal(
            const std::wstring& text,
            const Rect& layoutRect,
            const TextStyle& style
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Outline Effect Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct OutlineOffset {
            float dx;
            float dy;
        };

        [[nodiscard]] std::array<OutlineOffset, 8> CalculateOutlineOffsets(
            float outlineWidth
        ) const;

        [[nodiscard]] TextStyle CreateOutlineStyle(const TextStyle& style) const;

        void DrawOutlinePass(
            const std::wstring& text,
            const Rect& layoutRect,
            const TextStyle& style
        ) const;

        void DrawMainTextPass(
            const std::wstring& text,
            const Rect& layoutRect,
            const TextStyle& style
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        wrl::ComPtr<ID2D1RenderTarget> m_renderTarget;
        IDWriteFactory* m_writeFactory;
        mutable Helpers::Rendering::RenderResourceCache<size_t, IDWriteTextFormat> m_formatCache;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_TEXT_RENDERER_H