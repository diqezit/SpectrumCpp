// TextRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the TextRenderer class. This file handles creating DirectWrite
// text layouts and formats to render text with various styles.
//
// Implementation details:
// - TextFormat objects cached by {fontSize, alignment} to reduce overhead
// - Outline effect simulated via 8-directional multi-pass rendering
// - Rotated text uses ScopedTransform for automatic state restoration
// - Uses D2DHelpers for validation, sanitization, and conversion
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "TextRenderer.h"
#include "D2DHelpers.h"
#include <array>

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    TextRenderer::TextRenderer(
        ID2D1RenderTarget* renderTarget,
        IDWriteFactory* writeFactory,
        ID2D1SolidColorBrush* brush
    )
        : m_renderTarget(renderTarget)
        , m_writeFactory(writeFactory)
        , m_brush(brush)
    {
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TextRenderer::DrawText(
        const std::wstring& text,
        const Point& position,
        const Color& color,
        float fontSize,
        DWRITE_TEXT_ALIGNMENT alignment
    ) const
    {
        if (!Validate::RenderTargetAndBrush(m_renderTarget, m_brush)) return;
        if (text.empty()) return;

        const float sanitizedFontSize = Sanitize::PositiveFloat(fontSize, 12.0f);

        auto format = CreateTextFormat(sanitizedFontSize, alignment);
        if (!format) return;

        auto layout = CreateTextLayout(text, format.Get());
        if (!layout) return;

        const D2D1_RECT_F layoutRect = CalculateLayoutRect(position, alignment, layout.Get());

        SetBrushColor(color);

        m_renderTarget->DrawTextW(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format.Get(),
            &layoutRect,
            m_brush
        );
    }

    void TextRenderer::DrawTextWithOutline(
        const std::wstring& text,
        const Point& position,
        const Color& fillColor,
        const Color& outlineColor,
        float fontSize,
        float outlineWidth
    ) const
    {
        if (!m_renderTarget || text.empty()) return;

        const float sanitizedFontSize = Sanitize::PositiveFloat(fontSize, 12.0f);
        const float sanitizedOutlineWidth = Sanitize::PositiveFloat(outlineWidth, 1.0f);

        constexpr std::array<float, 3> offsets = { -1.0f, 0.0f, 1.0f };

        for (float dx : offsets) {
            for (float dy : offsets) {
                if (dx == 0.0f && dy == 0.0f) continue;

                DrawText(
                    text,
                    { position.x + dx * sanitizedOutlineWidth, position.y + dy * sanitizedOutlineWidth },
                    outlineColor,
                    sanitizedFontSize,
                    DWRITE_TEXT_ALIGNMENT_LEADING
                );
            }
        }

        DrawText(text, position, fillColor, sanitizedFontSize, DWRITE_TEXT_ALIGNMENT_LEADING);
    }

    void TextRenderer::DrawTextRotated(
        const std::wstring& text,
        const Point& position,
        float angleDegrees,
        const Color& color,
        float fontSize
    ) const
    {
        if (!m_renderTarget) return;

        const float sanitizedFontSize = Sanitize::PositiveFloat(fontSize, 12.0f);

        const ScopedTransform transform(
            m_renderTarget,
            D2D1::Matrix3x2F::Rotation(angleDegrees, ToD2DPoint(position))
        );

        DrawText(text, position, color, sanitizedFontSize, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TextRenderer::UpdateRenderTarget(ID2D1RenderTarget* renderTarget)
    {
        m_renderTarget = renderTarget;
        m_formatCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<IDWriteTextFormat> TextRenderer::CreateTextFormat(
        float fontSize,
        DWRITE_TEXT_ALIGNMENT alignment
    ) const
    {
        if (!m_writeFactory) return nullptr;

        const uint64_t key = GenerateFormatKey(fontSize, alignment);

        auto it = m_formatCache.find(key);
        if (it != m_formatCache.end()) return it->second;

        wrl::ComPtr<IDWriteTextFormat> textFormat;

        const HRESULT hr = m_writeFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"en-us",
            textFormat.GetAddressOf()
        );

        if (!HResult::CheckComCreation(hr, "IDWriteFactory::CreateTextFormat", textFormat)) {
            return nullptr;
        }

        textFormat->SetTextAlignment(alignment);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        m_formatCache[key] = textFormat;

        return textFormat;
    }

    wrl::ComPtr<IDWriteTextLayout> TextRenderer::CreateTextLayout(
        const std::wstring& text,
        IDWriteTextFormat* format
    ) const
    {
        if (!m_writeFactory || !format) return nullptr;

        wrl::ComPtr<IDWriteTextLayout> textLayout;

        constexpr float kMaxLayoutSize = 4096.0f;

        const HRESULT hr = m_writeFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format,
            kMaxLayoutSize,
            kMaxLayoutSize,
            textLayout.GetAddressOf()
        );

        if (!HResult::CheckComCreation(hr, "IDWriteFactory::CreateTextLayout", textLayout)) {
            return nullptr;
        }

        textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        return textLayout;
    }

    D2D1_RECT_F TextRenderer::CalculateLayoutRect(
        const Point& position,
        DWRITE_TEXT_ALIGNMENT alignment,
        IDWriteTextLayout* layout
    ) const
    {
        DWRITE_TEXT_METRICS textMetrics = {};

        if (!layout || FAILED(layout->GetMetrics(&textMetrics))) {
            return D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
        }

        const float halfHeight = textMetrics.height * 0.5f;

        switch (alignment) {
        case DWRITE_TEXT_ALIGNMENT_LEADING:
        case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
            return D2D1::RectF(
                position.x,
                position.y - halfHeight,
                position.x + textMetrics.width,
                position.y + halfHeight
            );

        case DWRITE_TEXT_ALIGNMENT_TRAILING:
            return D2D1::RectF(
                position.x - textMetrics.width,
                position.y - halfHeight,
                position.x,
                position.y + halfHeight
            );

        case DWRITE_TEXT_ALIGNMENT_CENTER:
            return D2D1::RectF(
                position.x - textMetrics.width * 0.5f,
                position.y - halfHeight,
                position.x + textMetrics.width * 0.5f,
                position.y + halfHeight
            );

        default:
            return D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    void TextRenderer::SetBrushColor(const Color& color) const
    {
        if (!m_brush) return;

        const_cast<ID2D1SolidColorBrush*>(m_brush)->SetColor(ToD2DColor(color));
    }

    uint64_t TextRenderer::GenerateFormatKey(
        float fontSize,
        DWRITE_TEXT_ALIGNMENT alignment
    ) const noexcept
    {
        const uint32_t fontSizeInt = static_cast<uint32_t>(fontSize * 100.0f);
        const uint32_t alignmentInt = static_cast<uint32_t>(alignment);

        return (static_cast<uint64_t>(fontSizeInt) << 32) | alignmentInt;
    }

} // namespace Spectrum