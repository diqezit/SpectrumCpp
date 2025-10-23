// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the TextRenderer class. This file handles creating DirectWrite
// text layouts and formats to render text with various styles.
//
// Implementation details:
// - TextFormat objects cached by a hash of the TextStyle object.
// - Outline effect simulated via 8-directional multi-pass rendering.
// - Rotated text uses ScopedTransform for automatic state restoration.
// - Uses D2DHelpers for validation, sanitization, and conversion.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Renderers/TextRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Helpers/Rendering/RenderHelpers.h"
#include "Graphics/API/Structs/TextStyle.h"
#include <array>
#include <functional>

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::HResult;
    using namespace Helpers::EnumConversion;
    using namespace Helpers::Rendering;

    namespace {
        constexpr float kDefaultTextLayoutSize = 4096.0f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    TextRenderer::TextRenderer(IDWriteFactory* writeFactory)
        : m_writeFactory(writeFactory)
    {
    }

    void TextRenderer::OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget)
    {
        m_renderTarget = renderTarget;
        m_formatCache.Clear();
    }

    void TextRenderer::OnDeviceLost()
    {
        m_renderTarget.Reset();
        m_formatCache.Clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Format Management (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<IDWriteTextFormat> TextRenderer::GetOrCreateTextFormat(
        const TextStyle& style
    ) const {
        if (!RenderValidation::ValidateWriteFactory(m_writeFactory)) {
            return nullptr;
        }

        const size_t key = HashGenerator::GenerateTextFormatKey(
            style.fontFamily,
            style.fontSize,
            ToDWriteFontWeight(style.weight),
            ToDWriteFontStyle(style.style),
            ToDWriteFontStretch(style.stretch),
            ToDWriteTextAlign(style.textAlign),
            ToDWriteParagraphAlign(style.paragraphAlign)
        );

        return m_formatCache.GetOrCreate(key, [&]() {
            auto format = FactoryHelper::CreateTextFormat(
                m_writeFactory,
                style.fontFamily,
                style.fontSize,
                ToDWriteFontWeight(style.weight),
                ToDWriteFontStyle(style.style),
                ToDWriteFontStretch(style.stretch)
            );

            if (format) {
                ConfigureTextFormat(format.Get(), style);
            }

            return format;
            });
    }

    void TextRenderer::ConfigureTextFormat(
        IDWriteTextFormat* format,
        const TextStyle& style
    ) const {
        if (!format) return;

        format->SetTextAlignment(ToDWriteTextAlign(style.textAlign));
        format->SetParagraphAlignment(ToDWriteParagraphAlign(style.paragraphAlign));
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, 0.0f);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text Layout Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<IDWriteTextLayout> TextRenderer::CreateTextLayout(
        const std::wstring& text,
        IDWriteTextFormat* format
    ) const {
        return FactoryHelper::CreateTextLayout(
            m_writeFactory,
            text,
            format,
            kDefaultTextLayoutSize,
            kDefaultTextLayoutSize
        );
    }

    bool TextRenderer::GetTextMetrics(
        IDWriteTextLayout* layout,
        DWRITE_TEXT_METRICS& metrics
    ) const {
        if (!layout) {
            return false;
        }

        HRESULT hr = layout->GetMetrics(&metrics);
        if (FAILED(hr)) {
            LOG_ERROR("GetMetrics failed: 0x" << std::hex << hr);
            return false;
        }

        return true;
    }

    Point TextRenderer::CalculateTextOrigin(
        const Point& position,
        const DWRITE_TEXT_METRICS& metrics,
        const TextStyle& style
    ) const {
        Point origin = position;

        origin = CalculateHorizontalAlignment(
            origin,
            metrics.widthIncludingTrailingWhitespace,
            style
        );

        origin = CalculateVerticalAlignment(origin, metrics.height, style);

        return origin;
    }

    Point TextRenderer::CalculateHorizontalAlignment(
        const Point& origin,
        float textWidth,
        const TextStyle& style
    ) const {
        Point result = origin;

        if (style.textAlign == TextAlign::Center) {
            result.x -= textWidth / 2.0f;
        }
        else if (style.textAlign == TextAlign::Trailing) {
            result.x -= textWidth;
        }

        return result;
    }

    Point TextRenderer::CalculateVerticalAlignment(
        const Point& origin,
        float textHeight,
        const TextStyle& style
    ) const {
        Point result = origin;

        if (style.paragraphAlign == ParagraphAlign::Center) {
            result.y -= textHeight / 2.0f;
        }
        else if (style.paragraphAlign == ParagraphAlign::Far) {
            result.y -= textHeight;
        }

        return result;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TextRenderer::DrawTextWithFormat(
        const std::wstring& text,
        const Rect& layoutRect,
        IDWriteTextFormat* format,
        ID2D1SolidColorBrush* brush
    ) const {
        if (!m_renderTarget || !format || !brush) return;

        m_renderTarget->DrawTextW(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format,
            ToD2DRect(layoutRect),
            brush
        );
    }

    void TextRenderer::DrawTextWithLayout(
        const Point& origin,
        IDWriteTextLayout* layout,
        ID2D1SolidColorBrush* brush
    ) const {
        if (!m_renderTarget || !layout || !brush) return;

        m_renderTarget->DrawTextLayout(
            ToD2DPoint(origin),
            layout,
            brush
        );
    }

    void TextRenderer::DrawTextInternal(
        const std::wstring& text,
        const Rect& layoutRect,
        const TextStyle& style
    ) const {
        if (!RenderValidation::ValidateTextRenderingContext(m_renderTarget.Get(), m_writeFactory, text)) {
            return;
        }

        auto format = GetOrCreateTextFormat(style);
        if (!format) return;

        auto brush = BrushManager::CreateSolidBrush(m_renderTarget.Get(), style.color);
        if (!brush) return;

        DrawTextWithFormat(text, layoutRect, format.Get(), brush.Get());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Outline Effect Helpers (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    std::array<TextRenderer::OutlineOffset, 8> TextRenderer::CalculateOutlineOffsets(
        float outlineWidth
    ) const {
        return { {
            {-outlineWidth, -outlineWidth},
            {-outlineWidth,  0.0f},
            {-outlineWidth,  outlineWidth},
            { 0.0f,         -outlineWidth},
            { 0.0f,          outlineWidth},
            { outlineWidth, -outlineWidth},
            { outlineWidth,  0.0f},
            { outlineWidth,  outlineWidth}
        } };
    }

    TextStyle TextRenderer::CreateOutlineStyle(const TextStyle& style) const
    {
        return style.WithColor(style.outlineColor);
    }

    void TextRenderer::DrawOutlinePass(
        const std::wstring& text,
        const Rect& layoutRect,
        const TextStyle& style
    ) const {
        auto offsets = CalculateOutlineOffsets(style.outlineWidth);
        TextStyle outlineStyle = CreateOutlineStyle(style);

        for (const auto& offset : offsets) {
            Rect offsetRect = layoutRect;
            offsetRect.x += offset.dx;
            offsetRect.y += offset.dy;

            DrawTextInternal(text, offsetRect, outlineStyle);
        }
    }

    void TextRenderer::DrawMainTextPass(
        const std::wstring& text,
        const Rect& layoutRect,
        const TextStyle& style
    ) const {
        DrawTextInternal(text, layoutRect, style);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TextRenderer::DrawText(
        const std::wstring& text,
        const Rect& layoutRect,
        const TextStyle& style
    ) const {
        if (style.HasOutline()) {
            DrawOutlinePass(text, layoutRect, style);
        }

        DrawMainTextPass(text, layoutRect, style);
    }

    void TextRenderer::DrawText(
        const std::wstring& text,
        const Point& position,
        const TextStyle& style
    ) const {
        if (!RenderValidation::ValidateTextRenderingContext(m_renderTarget.Get(), m_writeFactory, text)) {
            return;
        }

        auto format = GetOrCreateTextFormat(style);
        if (!format) return;

        auto textLayout = CreateTextLayout(text, format.Get());
        if (!textLayout) return;

        DWRITE_TEXT_METRICS textMetrics;
        if (!GetTextMetrics(textLayout.Get(), textMetrics)) return;

        Point origin = CalculateTextOrigin(position, textMetrics, style);

        auto brush = BrushManager::CreateSolidBrush(m_renderTarget.Get(), style.color);
        if (!brush) return;

        DrawTextWithLayout(origin, textLayout.Get(), brush.Get());
    }

} // namespace Spectrum