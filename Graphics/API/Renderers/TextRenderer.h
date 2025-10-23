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
// - Non-owning pointers to D2D/DWrite resources (lifetime managed externally).
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include <string>
#include <unordered_map>

namespace Spectrum {

    struct TextStyle; // Forward declaration

    class TextRenderer final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit TextRenderer(
            IDWriteFactory* writeFactory
        );

        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) = delete;

        // IRenderComponent implementation
        void OnRenderTargetChanged(
            ID2D1RenderTarget* renderTarget
        ) override;

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
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<IDWriteTextFormat> GetOrCreateTextFormat(
            const TextStyle& style
        ) const;

        void DrawTextInternal(
            const std::wstring& text,
            const Rect& layoutRect,
            const TextStyle& style
        ) const;

        [[nodiscard]] size_t GenerateFormatKey(
            const TextStyle& style
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1RenderTarget* m_renderTarget;
        IDWriteFactory* m_writeFactory;
        mutable std::unordered_map<size_t, wrl::ComPtr<IDWriteTextFormat>> m_formatCache;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_TEXT_RENDERER_H