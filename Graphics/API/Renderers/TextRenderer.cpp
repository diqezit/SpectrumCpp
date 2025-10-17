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
#include "Graphics/API/Structs/TextStyle.h"
#include <array>
#include <functional>

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::Validate;
    using namespace Helpers::HResult;
    using namespace Helpers::EnumConversion;
    using namespace Helpers::Scopes;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    TextRenderer::TextRenderer(
        IDWriteFactory* writeFactory
    )
        : m_renderTarget(nullptr)
        , m_writeFactory(writeFactory)
    {
    }

    void TextRenderer::OnRenderTargetChanged(
        ID2D1RenderTarget* renderTarget
    )
    {
        m_renderTarget = renderTarget;
        m_formatCache.clear();
    }

    void TextRenderer::OnDeviceLost()
    {
        m_renderTarget = nullptr;
        m_formatCache.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Text Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TextRenderer::DrawText(
        const std::wstring& text,
        const Rect& layoutRect,
        const TextStyle& style
    ) const
    {
        if (style.HasOutline()) {
            constexpr std::array<float, 3> offsets = { -1.0f, 0.0f, 1.0f };
            const float outlineWidth = style.outlineWidth;
            TextStyle outlineStyle = style.WithColor(style.outlineColor);

            for (float dx : offsets) {
                for (float dy : offsets) {
                    if (dx == 0.0f && dy == 0.0f) continue;

                    Rect offsetRect = layoutRect;
                    offsetRect.x += dx * outlineWidth;
                    offsetRect.y += dy * outlineWidth;

                    DrawTextInternal(text, offsetRect, outlineStyle);
                }
            }
        }

        DrawTextInternal(text, layoutRect, style);
    }

    void TextRenderer::DrawText(
        const std::wstring& text,
        const Point& position,
        const TextStyle& style
    ) const
    {
        if (!m_renderTarget || text.empty()) {
            return;
        }

        auto format = GetOrCreateTextFormat(style);
        if (!format) {
            return;
        }

        wrl::ComPtr<ID2D1SolidColorBrush> brush;
        HRESULT hr = m_renderTarget->CreateSolidColorBrush(ToD2DColor(style.color), &brush);
        if (FAILED(hr)) {
            return;
        }

        wrl::ComPtr<IDWriteTextLayout> textLayout;
        hr = m_writeFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format.Get(),
            4096.0f,
            4096.0f,
            &textLayout
        );

        if (FAILED(hr)) {
            return;
        }

        DWRITE_TEXT_METRICS textMetrics;
        textLayout->GetMetrics(&textMetrics);

        Point origin = position;

        if (style.textAlign == TextAlign::Center) {
            origin.x -= textMetrics.widthIncludingTrailingWhitespace / 2.0f;
        }
        else if (style.textAlign == TextAlign::Trailing) {
            origin.x -= textMetrics.widthIncludingTrailingWhitespace;
        }

        if (style.paragraphAlign == ParagraphAlign::Center) {
            origin.y -= textMetrics.height / 2.0f;
        }
        else if (style.paragraphAlign == ParagraphAlign::Far) {
            origin.y -= textMetrics.height;
        }

        m_renderTarget->DrawTextLayout(
            ToD2DPoint(origin),
            textLayout.Get(),
            brush.Get()
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TextRenderer::DrawTextInternal(
        const std::wstring& text,
        const Rect& layoutRect,
        const TextStyle& style
    ) const
    {
        if (!m_renderTarget || text.empty()) {
            return;
        }

        auto format = GetOrCreateTextFormat(style);
        if (!format) {
            return;
        }

        wrl::ComPtr<ID2D1SolidColorBrush> brush;
        HRESULT hr = m_renderTarget->CreateSolidColorBrush(ToD2DColor(style.color), &brush);
        if (FAILED(hr)) {
            return;
        }

        m_renderTarget->DrawTextW(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format.Get(),
            ToD2DRect(layoutRect),
            brush.Get()
        );
    }

    wrl::ComPtr<IDWriteTextFormat> TextRenderer::GetOrCreateTextFormat(
        const TextStyle& style
    ) const
    {
        if (!m_writeFactory) {
            return nullptr;
        }

        const size_t key = GenerateFormatKey(style);

        auto it = m_formatCache.find(key);
        if (it != m_formatCache.end()) {
            return it->second;
        }

        wrl::ComPtr<IDWriteTextFormat> textFormat;
        const HRESULT hr = m_writeFactory->CreateTextFormat(
            style.fontFamily.c_str(),
            nullptr,
            ToDWriteFontWeight(style.weight),
            ToDWriteFontStyle(style.style),
            ToDWriteFontStretch(style.stretch),
            style.fontSize,
            L"en-us", // Locale
            textFormat.GetAddressOf()
        );

        if (!CheckComCreation(hr, "IDWriteFactory::CreateTextFormat", textFormat)) {
            return nullptr;
        }

        textFormat->SetTextAlignment(ToDWriteTextAlign(style.textAlign));
        textFormat->SetParagraphAlignment(ToDWriteParagraphAlign(style.paragraphAlign));
        textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        textFormat->SetLineSpacing(
            DWRITE_LINE_SPACING_METHOD_DEFAULT,
            0.0f,
            0.0f
        );

        m_formatCache[key] = textFormat;
        return textFormat;
    }

    size_t TextRenderer::GenerateFormatKey(
        const TextStyle& style
    ) const
    {
        size_t seed = 0;
        auto hash_combine = [&](size_t& seed, size_t hash) {
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

        hash_combine(seed, std::hash<std::wstring>{}(style.fontFamily));
        hash_combine(seed, std::hash<float>{}(style.fontSize));
        hash_combine(seed, static_cast<size_t>(style.weight));
        hash_combine(seed, static_cast<size_t>(style.style));
        hash_combine(seed, static_cast<size_t>(style.stretch));
        hash_combine(seed, static_cast<size_t>(style.textAlign));
        hash_combine(seed, static_cast<size_t>(style.paragraphAlign));

        return seed;
    }
}