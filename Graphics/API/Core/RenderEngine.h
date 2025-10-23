#ifndef SPECTRUM_CPP_RENDER_ENGINE_H
#define SPECTRUM_CPP_RENDER_ENGINE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the main facade for the entire rendering system
//
// Responsibilities:
// - Provides the single public entry point for all rendering tasks
// - Owns and orchestrates all core graphics subsystems
// - Creates and provides access to the Canvas for drawing operations
// - Delegates complex logic to specialized managers to keep API simple
//
// Implementation details:
// - Delegates all resource logic to the DeviceResourceManager
// - Delegates all drawing state logic to the DrawStateController
// - Manages object lifetimes with std::unique_ptr for automatic cleanup
// - Declares member variables in specific order to ensure safe destruction
// - Keeps all implementation in .cpp file to reduce application build times
//
// Architecture pattern:
// - Facade: Hides complex subsystem interactions behind a simple API
// - Coordinator: Manages the lifecycle and interaction of subsystems
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <memory>

namespace Spectrum {

    // Forward declarations
    class Canvas;
    class DeviceResourceManager;
    class DrawStateController;
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
            explicit DrawScope(RenderEngine& engine);
            ~DrawScope() noexcept;

            DrawScope(const DrawScope&) = delete;
            DrawScope& operator=(const DrawScope&) = delete;

        private:
            RenderEngine& m_engine;
            bool m_begun;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit RenderEngine(HWND hwnd, bool isOverlay = false);
        ~RenderEngine();

        RenderEngine(const RenderEngine&) = delete;
        RenderEngine& operator=(const RenderEngine&) = delete;

        [[nodiscard]] bool Initialize();
        void Resize(int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Drawing Control (Delegates to DrawStateController)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool BeginDraw();
        [[nodiscard]] HRESULT EndDraw();
        [[nodiscard]] DrawScope CreateDrawScope();
        void Clear(const Color& color);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Canvas Access (Primary API for Drawing)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Canvas& GetCanvas();
        [[nodiscard]] const Canvas& GetCanvas() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource & State Access (for internal use or advanced scenarios)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] ID2D1RenderTarget* GetRenderTarget() const;
        [[nodiscard]] int GetWidth() const noexcept { return m_width; }
        [[nodiscard]] int GetHeight() const noexcept { return m_height; }
        [[nodiscard]] bool IsDrawing() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation - Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool InitializeManagers();
        [[nodiscard]] bool InitializeComponents();
        void CreateCoreComponents();
        void CreateRenderers();
        void CreateCanvas();
        void RegisterAllComponents();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        HWND m_hwnd;
        int m_width;
        int m_height;
        bool m_isOverlay;

        //
        // The order of declaration here is CRITICAL for correct destruction.
        // Components must be declared BEFORE managers that use them.
        // Destruction happens in reverse order of declaration.
        //

        // Rendering components (owned)
        // These are declared first so they are destroyed last.
        std::unique_ptr<ResourceCache> m_resourceCache;
        std::unique_ptr<GeometryBuilder> m_geometryBuilder;
        std::unique_ptr<PrimitiveRenderer> m_primitiveRenderer;
        std::unique_ptr<TextRenderer> m_textRenderer;
        std::unique_ptr<EffectsRenderer> m_effectsRenderer;
        std::unique_ptr<TransformManager> m_transformManager;
        std::unique_ptr<SpectrumRenderer> m_spectrumRenderer;

        // Drawing facade (owned)
        std::unique_ptr<Canvas> m_canvas;

        // Core managers (owned)
        // These are declared last so they are destroyed first.
        // Their destructors might need to access the components above.
        std::unique_ptr<DeviceResourceManager> m_resourceManager;
        std::unique_ptr<DrawStateController> m_drawStateController;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDER_ENGINE_H