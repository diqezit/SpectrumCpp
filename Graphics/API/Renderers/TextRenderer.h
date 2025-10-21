// TextRenderer.h
#ifndef SPECTRUM_CPP_TEXT_RENDERER_H
#define SPECTRUM_CPP_TEXT_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the TextRenderer class, which is responsible for all text
// drawing operations. It wraps the DirectWrite API to simplify text
// layout and rendering with various styles.
//
// Key responsibilities:
// - Basic text rendering with alignment and font size control
// - Text with outline effect (simulated via multi-pass rendering)
// - Rotated text rendering with proper pivot handling
// - IDWriteTextFormat caching for performance optimization
//
// Design notes:
// - All render methods are const (stateless rendering)
// - Caches IDWriteTextFormat objects by {fontSize, alignment} key
// - Uses RAII ScopedTransform for rotation operations
// - Non-owning pointers to D2D/DWrite resources (lifetime managed externally)
//
// Performance optimizations:
// - TextFormat caching reduces DirectWrite object creation overhead
// - Layout rect calculation minimizes GetMetrics() calls
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <string>
#include <unordered_map>

namespace Spectrum {

    class TextRenderer final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        TextRenderer(
            ID2D1RenderTarget* renderTarget,
            IDWriteFactory* writeFactory,
            ID2D1SolidColorBrush* brush
        );

        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Text Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawText(
            const std::wstring& text,
            const Point& position,
            const Color& color,
            float fontSize = 12.0f,
            DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING
        ) const;

        void DrawTextWithOutline(
            const std::wstring& text,
            const Point& position,
            const Color& fillColor,
            const Color& outlineColor,
            float fontSize = 12.0f,
            float outlineWidth = 1.0f
        ) const;

        void DrawTextRotated(
            const std::wstring& text,
            const Point& position,
            float angleDegrees,
            const Color& color,
            float fontSize = 12.0f
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<IDWriteTextFormat> CreateTextFormat(
            float fontSize,
            DWRITE_TEXT_ALIGNMENT alignment
        ) const;

        [[nodiscard]] wrl::ComPtr<IDWriteTextLayout> CreateTextLayout(
            const std::wstring& text,
            IDWriteTextFormat* format
        ) const;

        [[nodiscard]] D2D1_RECT_F CalculateLayoutRect(
            const Point& position,
            DWRITE_TEXT_ALIGNMENT alignment,
            IDWriteTextLayout* layout
        ) const;

        void SetBrushColor(const Color& color) const;

        [[nodiscard]] uint64_t GenerateFormatKey(
            float fontSize,
            DWRITE_TEXT_ALIGNMENT alignment
        ) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1RenderTarget* m_renderTarget;
        IDWriteFactory* m_writeFactory;
        ID2D1SolidColorBrush* m_brush;

        mutable std::unordered_map<uint64_t, wrl::ComPtr<IDWriteTextFormat>> m_formatCache;
    };

} // namespace Spectrum

#endif