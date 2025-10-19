// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the TextRenderer class
// It handles creating DirectWrite text layouts and formats to render
// text with various styles, including outlines and rotations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "TextRenderer.h"
#include "ColorUtils.h"
#include "StringUtils.h"

namespace Spectrum {

    namespace {
        inline D2D1_COLOR_F ToD2DColor(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        inline D2D1_POINT_2F ToD2DPoint(const Point& p) {
            return D2D1::Point2F(p.x, p.y);
        }
    }

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

    void TextRenderer::SetBrushColor(const Color& color) {
        if (m_brush) {
            m_brush->SetColor(ToD2DColor(color));
        }
    }

    void TextRenderer::DrawText(
        const std::wstring& text,
        const Point& position,
        const Color& color,
        float fontSize,
        DWRITE_TEXT_ALIGNMENT alignment
    ) {
        if (!m_renderTarget || !m_brush || text.empty()) {
            return;
        }

        auto format = CreateTextFormat(fontSize, alignment);
        if (!format) {
            return;
        }

        auto layout = CreateTextLayout(text, format.Get());
        if (!layout) {
            return;
        }

        D2D1_RECT_F layoutRect = CalculateLayoutRect(position, alignment, layout.Get());
        SetBrushColor(color);

        m_renderTarget->DrawTextW(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format.Get(),
            &layoutRect,
            m_brush
        );
    }

    // fake an outline by drawing the text 8 times around the center
    // this avoids creating complex geometry for a true outline
    void TextRenderer::DrawTextWithOutline(
        const std::wstring& text,
        const Point& position,
        const Color& fillColor,
        const Color& outlineColor,
        float fontSize,
        float outlineWidth
    ) {
        if (!m_renderTarget || text.empty()) {
            return;
        }

        // draw outline in 8 directions
        float offsets[] = { -outlineWidth, 0, outlineWidth };
        for (float dx : offsets) {
            for (float dy : offsets) {
                if (dx != 0 || dy != 0) {
                    DrawText(
                        text,
                        { position.x + dx, position.y + dy },
                        outlineColor,
                        fontSize
                    );
                }
            }
        }

        // draw main text on top
        DrawText(text, position, fillColor, fontSize);
    }

    // use a temporary transform to rotate text without affecting other elements
    void TextRenderer::DrawTextRotated(
        const std::wstring& text,
        const Point& position,
        float angleDegrees,
        const Color& color,
        float fontSize
    ) {
        if (!m_renderTarget) {
            return;
        }

        D2D1_MATRIX_3X2_F oldTransform;
        m_renderTarget->GetTransform(&oldTransform);

        D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(
            angleDegrees,
            ToD2DPoint(position)
        );
        m_renderTarget->SetTransform(rotation * oldTransform);

        // draw text, forcing center alignment for correct rotation pivot
        DrawText(text, position, color, fontSize, DWRITE_TEXT_ALIGNMENT_CENTER);

        m_renderTarget->SetTransform(oldTransform);
    }

    void TextRenderer::UpdateRenderTarget(ID2D1RenderTarget* renderTarget) {
        m_renderTarget = renderTarget;
    }

    wrl::ComPtr<IDWriteTextFormat> TextRenderer::CreateTextFormat(
        float fontSize,
        DWRITE_TEXT_ALIGNMENT alignment
    ) {
        if (!m_writeFactory) {
            return nullptr;
        }

        // note: text format objects could be cached for performance
        wrl::ComPtr<IDWriteTextFormat> textFormat;
        if (FAILED(m_writeFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"en-us",
            textFormat.GetAddressOf()
        ))) {
            return nullptr;
        }

        textFormat->SetTextAlignment(alignment);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        return textFormat;
    }

    wrl::ComPtr<IDWriteTextLayout> TextRenderer::CreateTextLayout(
        const std::wstring& text,
        IDWriteTextFormat* format
    ) {
        if (!m_writeFactory || !format) {
            return nullptr;
        }

        // note: text layout objects could be cached for performance
        wrl::ComPtr<IDWriteTextLayout> textLayout;
        if (FAILED(m_writeFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format,
            4096.0f, // use large bounds to avoid premature wrapping
            4096.0f,
            textLayout.GetAddressOf()
        ))) {
            return nullptr;
        }

        return textLayout;
    }

    // calculate bounding box for text based on alignment
    // this ensures text is positioned correctly relative to the anchor point
    D2D1_RECT_F TextRenderer::CalculateLayoutRect(
        const Point& position,
        DWRITE_TEXT_ALIGNMENT alignment,
        IDWriteTextLayout* layout
    ) {
        DWRITE_TEXT_METRICS textMetrics = {};
        if (!layout || FAILED(layout->GetMetrics(&textMetrics))) {
            return D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
        }

        // D2D draws text relative to top-left of layout rect
        // we must calculate this rect based on desired alignment
        switch (alignment) {
        case DWRITE_TEXT_ALIGNMENT_LEADING:
        case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
            return {
                position.x,
                position.y - textMetrics.height / 2.0f,
                position.x + textMetrics.width,
                position.y + textMetrics.height / 2.0f
            };

        case DWRITE_TEXT_ALIGNMENT_TRAILING:
            return {
                position.x - textMetrics.width,
                position.y - textMetrics.height / 2.0f,
                position.x,
                position.y + textMetrics.height / 2.0f
            };

        case DWRITE_TEXT_ALIGNMENT_CENTER:
            return {
                position.x - textMetrics.width / 2.0f,
                position.y - textMetrics.height / 2.0f,
                position.x + textMetrics.width / 2.0f,
                position.y + textMetrics.height / 2.0f
            };

        default:
            return D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

} // namespace Spectrum