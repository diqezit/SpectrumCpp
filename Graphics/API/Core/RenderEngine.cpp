// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the main rendering system facade
//
// Implementation details:
// - Delegates all resource logic to the DeviceResourceManager
// - Delegates all drawing state logic to the DrawStateController
// - Manages the startup and shutdown sequence for all graphics objects
// - Hides internal complexity behind a simple, stable public API
//
// Class role:
// - Acts as the primary orchestrator for the entire rendering pipeline
// - Owns all core rendering managers and components
// - Serves as the single entry point for the rest of the application
// - Ensures correct initialization order and dependency injection
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Core/RenderEngine.h"
#include "Graphics/API/D2DHelpers.h"

// Core management components
#include "Graphics/API/Core/DeviceResourceManager.h"
#include "Graphics/API/Core/DrawStateController.h"

// Rendering components
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Core/ResourceCache.h"
#include "Graphics/API/Core/GeometryBuilder.h"
#include "Graphics/API/Core/TransformManager.h"
#include "Graphics/API/Renderers/PrimitiveRenderer.h"
#include "Graphics/API/Renderers/TextRenderer.h"
#include "Graphics/API/Renderers/EffectsRenderer.h"
#include "Graphics/API/Renderers/SpectrumRenderer.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::HResult;

    namespace {
        constexpr int kMinWindowSize = 1;
        constexpr int kMaxWindowSize = 16384;

        [[nodiscard]] bool IsValidSize(int width, int height) noexcept
        {
            return width >= kMinWindowSize && width <= kMaxWindowSize &&
                height >= kMinWindowSize && height <= kMaxWindowSize;
        }

        [[nodiscard]] std::pair<int, int> GetClientSize(HWND hwnd) noexcept
        {
            RECT rc{};
            if (!hwnd || !::IsWindow(hwnd) || !GetClientRect(hwnd, &rc)) {
                return { 1, 1 };
            }

            const int width = std::max(1L, rc.right - rc.left);
            const int height = std::max(1L, rc.bottom - rc.top);

            return { width, height };
        }
    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // DrawScope Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    RenderEngine::DrawScope::DrawScope(RenderEngine& engine)
        : m_engine(engine)
        , m_begun(false)
    {
        // FIX: Check the return value of BeginDraw inside the scope.
        // If it fails, the scope is not considered "begun".
        if (m_engine.BeginDraw()) {
            m_begun = true;
        }
        else {
            LOG_ERROR("DrawScope: Failed to begin drawing.");
        }
    }

    RenderEngine::DrawScope::~DrawScope() noexcept
    {
        if (!m_begun) {
            return;
        }

        const HRESULT hr = m_engine.EndDraw();

        if (hr == D2DERR_RECREATE_TARGET) {
            LOG_WARNING("DrawScope: Device lost during frame rendering");
        }
        else if (FAILED(hr)) {
            LOG_ERROR("DrawScope: EndDraw failed with 0x" << std::hex << hr);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    RenderEngine::RenderEngine(HWND hwnd, bool isOverlay)
        : m_hwnd(hwnd)
        , m_isOverlay(isOverlay)
        , m_width(1)
        , m_height(1)
    {
        if (m_hwnd && ::IsWindow(m_hwnd)) {
            const auto [width, height] = GetClientSize(m_hwnd);
            if (IsValidSize(width, height)) {
                m_width = width;
                m_height = height;
            }
        }
        else {
            LOG_ERROR("RenderEngine: Invalid HWND provided");
        }
    }

    RenderEngine::~RenderEngine()
    {
        // unique_ptrs handle cleanup automatically.
        // A check can be useful for debugging.
        if (m_drawStateController && m_drawStateController->IsDrawing()) {
            LOG_ERROR("RenderEngine destroyed while drawing in progress");
        }
    }

    bool RenderEngine::Initialize()
    {
        if (!m_hwnd || !::IsWindow(m_hwnd)) {
            LOG_ERROR("Cannot initialize RenderEngine with invalid HWND");
            return false;
        }

        if (!InitializeManagers()) {
            LOG_ERROR("Failed to initialize core managers");
            return false;
        }

        if (!InitializeComponents()) {
            LOG_ERROR("Failed to initialize rendering components");
            return false;
        }

        LOG_INFO("RenderEngine initialized successfully");
        return true;
    }

    void RenderEngine::Resize(int width, int height)
    {
        if (!IsValidSize(width, height)) {
            LOG_WARNING("Invalid window size: " << width << "x" << height << " - ignoring");
            return;
        }

        if (m_drawStateController && !m_drawStateController->CanResize()) {
            LOG_ERROR("Resize() called during drawing operation - ignoring");
            return;
        }

        if (m_width == width && m_height == height) {
            return;
        }

        LOG_INFO("Resizing RenderEngine to " << width << "x" << height);

        m_width = width;
        m_height = height;

        if (m_drawStateController) {
            m_drawStateController->UpdateSize(width, height);
        }

        if (m_resourceManager) {
            (void)m_resourceManager->RecreateDeviceResources(width, height);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Control (Delegated)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool RenderEngine::BeginDraw() // FIX: Function signature is now bool
    {
        if (!m_drawStateController) {
            LOG_ERROR("BeginDraw() called before initialization");
            return false;
        }

        // Ensure resources are available before drawing
        if (m_resourceManager && !m_resourceManager->HasResources()) {
            if (!m_resourceManager->CreateDeviceResources(m_width, m_height)) {
                LOG_ERROR("Failed to create resources before drawing");
                return false;
            }
            // If creation succeeded, reset error state to allow drawing
            m_drawStateController->ResetErrorState();
        }

        // FIX: Return the result of the delegated call
        return m_drawStateController->BeginDraw();
    }

    HRESULT RenderEngine::EndDraw()
    {
        if (!m_drawStateController) {
            LOG_ERROR("EndDraw() called before initialization");
            return E_FAIL;
        }

        return m_drawStateController->EndDraw();
    }

    RenderEngine::DrawScope RenderEngine::CreateDrawScope()
    {
        return DrawScope(*this);
    }

    void RenderEngine::Clear(const Color& color)
    {
        if (!m_drawStateController) {
            LOG_ERROR("Clear() called before initialization");
            return;
        }

        m_drawStateController->Clear(color);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Accessors
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Canvas& RenderEngine::GetCanvas()
    {
        return *m_canvas;
    }

    const Canvas& RenderEngine::GetCanvas() const
    {
        return *m_canvas;
    }

    ID2D1RenderTarget* RenderEngine::GetRenderTarget() const
    {
        return m_resourceManager ? m_resourceManager->GetRenderTarget() : nullptr;
    }

    bool RenderEngine::IsDrawing() const noexcept
    {
        return m_drawStateController ? m_drawStateController->IsDrawing() : false;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation - Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool RenderEngine::InitializeManagers()
    {
        m_resourceManager = std::make_unique<DeviceResourceManager>(m_hwnd, m_isOverlay);
        if (!m_resourceManager->InitializeFactories()) {
            return false;
        }

        m_drawStateController = std::make_unique<DrawStateController>(m_resourceManager.get());
        m_drawStateController->UpdateSize(m_width, m_height);

        return m_resourceManager->CreateDeviceResources(m_width, m_height);
    }

    bool RenderEngine::InitializeComponents()
    {
        try {
            CreateCoreComponents();
            CreateRenderers();
            CreateCanvas();
            RegisterAllComponents();
            return true;
        }
        catch (const std::exception& e) {
            (void)e; // Suppress unreferenced variable warning in release builds
            LOG_ERROR("Failed to initialize components: " << e.what());
            return false;
        }
    }

    void RenderEngine::CreateCoreComponents()
    {
        auto* d2dFactory = m_resourceManager->GetD2DFactory();
        if (!d2dFactory) throw std::runtime_error("D2D Factory not available");

        m_geometryBuilder = std::make_unique<GeometryBuilder>(d2dFactory);
        m_resourceCache = std::make_unique<ResourceCache>(d2dFactory);
        m_transformManager = std::make_unique<TransformManager>();
    }

    void RenderEngine::CreateRenderers()
    {
        auto* dwriteFactory = m_resourceManager->GetDWriteFactory();
        if (!dwriteFactory) throw std::runtime_error("DWrite Factory not available");

        m_primitiveRenderer = std::make_unique<PrimitiveRenderer>(
            m_geometryBuilder.get(), m_resourceCache.get()
        );
        m_textRenderer = std::make_unique<TextRenderer>(dwriteFactory);
        m_effectsRenderer = std::make_unique<EffectsRenderer>();
        m_spectrumRenderer = std::make_unique<SpectrumRenderer>(
            m_primitiveRenderer.get(), m_geometryBuilder.get()
        );
    }

    void RenderEngine::CreateCanvas()
    {
        m_canvas = std::make_unique<Canvas>(
            m_primitiveRenderer.get(),
            m_textRenderer.get(),
            m_effectsRenderer.get(),
            m_transformManager.get(),
            m_spectrumRenderer.get()
        );
    }

    void RenderEngine::RegisterAllComponents()
    {
        m_resourceManager->RegisterComponent(m_resourceCache.get());
        m_resourceManager->RegisterComponent(m_transformManager.get());
        m_resourceManager->RegisterComponent(m_primitiveRenderer.get());
        m_resourceManager->RegisterComponent(m_textRenderer.get());
        m_resourceManager->RegisterComponent(m_effectsRenderer.get());
        m_resourceManager->RegisterComponent(m_canvas.get());

        m_resourceManager->NotifyComponentsTargetChanged();
    }

} // namespace Spectrum