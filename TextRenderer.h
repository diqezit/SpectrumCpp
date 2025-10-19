#ifndef SPECTRUM_CPP_TEXT_RENDERER_H
#define SPECTRUM_CPP_TEXT_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the TextRenderer class, which is responsible for
// all text drawing operations. It wraps the DirectWrite API to simplify
// text layout and rendering
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <string>

namespace Spectrum {

    class TextRenderer {
    public:
        TextRenderer(
            ID2D1RenderTarget* renderTarget,
            IDWriteFactory* writeFactory,
            ID2D1SolidColorBrush* brush
        );

        void DrawText(
            const std::wstring& text,
            const Point& position,
            const Color& color,
            float fontSize = 12.0f,
            DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING
        );

        void DrawTextWithOutline(
            const std::wstring& text,
            const Point& position,
            const Color& fillColor,
            const Color& outlineColor,
            float fontSize = 12.0f,
            float outlineWidth = 1.0f
        );

        void DrawTextRotated(
            const std::wstring& text,
            const Point& position,
            float angleDegrees,
            const Color& color,
            float fontSize = 12.0f
        );

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        wrl::ComPtr<IDWriteTextFormat> CreateTextFormat(
            float fontSize,
            DWRITE_TEXT_ALIGNMENT alignment
        );

        wrl::ComPtr<IDWriteTextLayout> CreateTextLayout(
            const std::wstring& text,
            IDWriteTextFormat* format
        );

        D2D1_RECT_F CalculateLayoutRect(
            const Point& position,
            DWRITE_TEXT_ALIGNMENT alignment,
            IDWriteTextLayout* layout
        );

        void SetBrushColor(const Color& color);

        ID2D1RenderTarget* m_renderTarget;
        IDWriteFactory* m_writeFactory;
        ID2D1SolidColorBrush* m_brush;
    };

} // namespace Spectrum

#endif