#ifndef SPECTRUM_CPP_RENDER_ENGINE_H
#define SPECTRUM_CPP_RENDER_ENGINE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the RenderEngine, the central manager for graphics resources.
//
// This class is responsible for the entire lifecycle of Direct2D/DirectWrite
// resources, including creation, resizing, and handling device-lost scenarios.
// It acts as a factory for the Canvas object, which provides the actual
// drawing API.
//
// Key responsibilities:
// - Direct2D/DirectWrite resource lifecycle management
// - Device lost scenario handling and recovery
// - Creating and providing access to the Canvas drawing facade
//
// Design notes:
// - All drawing operations have been moved to Canvas
// - RenderEngine now focuses purely on resource management
// - Uses unique_ptr for sub-components (ownership)
// - Non-owning HWND pointer (lifetime managed by OS)
//
// Architecture pattern: Resource Manager + Factory
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include <memory>
#include <vector>

namespace Spectrum {

    // Forward-declarations to improve compile times
    class Canvas;
    class ResourceCache;
    class GeometryBuilder;
    class PrimitiveRenderer;
    class TextRenderer;
    class EffectsRenderer;
    class TransformManager;
    class SpectrumRenderer;

    class RenderEngine final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // RAII Draw Scope
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        class DrawScope final
        {
        public:
            explicit DrawScope(RenderEngine& engine)
                : m_engine(engine)
            {
                m_engine.BeginDraw();
            }

            ~DrawScope() noexcept
            {
                (void)m_engine.EndDraw();
            }

            DrawScope(const DrawScope&) = delete;
            DrawScope& operator=(const DrawScope&) = delete;

        private:
            RenderEngine& m_engine;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit RenderEngine(
            HWND hwnd,
            bool isOverlay = false
        );
        ~RenderEngine();

        RenderEngine(const RenderEngine&) = delete;
        RenderEngine& operator=(const RenderEngine&) = delete;

        [[nodiscard]] bool Initialize();
        void Resize(int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Drawing Control
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void BeginDraw();
        [[nodiscard]] HRESULT EndDraw();
        [[nodiscard]] DrawScope CreateDrawScope() { return DrawScope(*this); }
        void Clear(const Color& color) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Canvas Access (Primary API for Drawing)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Canvas& GetCanvas() { return *m_canvas; }
        [[nodiscard]] const Canvas& GetCanvas() const { return *m_canvas; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Access (for internal use or advanced scenarios)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] ID2D1HwndRenderTarget* GetRenderTarget() const noexcept;
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CreateD2DFactory();
        [[nodiscard]] bool CreateDWriteFactory();
        [[nodiscard]] bool CreateDeviceResources();
        [[nodiscard]] bool CreateHwndRenderTarget();

        void DiscardDeviceResources();
        void RegisterComponent(IRenderComponent* component);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        HWND m_hwnd;
        int m_width;
        int m_height;
        bool m_isOverlay;

        // D2D core resources
        wrl::ComPtr<ID2D1Factory> m_d2dFactory;
        wrl::ComPtr<IDWriteFactory> m_writeFactory;
        wrl::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;

        // Owned sub-components (worker classes)
        std::unique_ptr<ResourceCache> m_resourceCache;
        std::unique_ptr<GeometryBuilder> m_geometryBuilder;
        std::unique_ptr<PrimitiveRenderer> m_primitiveRenderer;
        std::unique_ptr<TextRenderer> m_textRenderer;
        std::unique_ptr<EffectsRenderer> m_effectsRenderer;
        std::unique_ptr<TransformManager> m_transformManager;
        std::unique_ptr<SpectrumRenderer> m_spectrumRenderer;

        // The drawing facade, owned by the engine
        std::unique_ptr<Canvas> m_canvas;

        // Polymorphic list of all device-dependent components
        std::vector<IRenderComponent*> m_components;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDER_ENGINE_H