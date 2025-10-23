// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// RenderersController.cpp: Implementation of the RenderersController class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Graphics/RenderersController.h"
#include "Graphics/Visualizers/BarsRenderer.h"
#include "Graphics/Visualizers/WaveRenderer.h"
#include "Graphics/Visualizers/CircularWaveRenderer.h"
#include "Graphics/Visualizers/CubesRenderer.h"
#include "Graphics/Visualizers/FireRenderer.h"
#include "Graphics/Visualizers/LedPanelRenderer.h"
#include "Common/Logger.h"

namespace Spectrum {

    RenderersController::RenderersController()
        : m_currentRenderer(nullptr)
        , m_currentStyle(RenderStyle::Bars)
        , m_initialized(false) {
    }

    RenderersController::~RenderersController() {
        Shutdown();
    }

    bool RenderersController::InitializeRenderers() {
        if (m_initialized) return true;

        try {
            InitializeRendererInstances();
            SetCurrentRenderer(RenderStyle::Bars);
            m_initialized = true;
            return true;
        }
        catch (const std::exception&) {
            LOG_ERROR("Failed to initialize renderers");
            return false;
        }
    }

    void RenderersController::Shutdown() {
        if (m_currentRenderer) {
            m_currentRenderer->OnDeactivate();
            m_currentRenderer = nullptr;
        }
        m_renderers.clear();
        m_initialized = false;
    }

    void RenderersController::RenderCurrentVisualizer(
        const SpectrumData& /*spectrum*/
    ) {
        // Note: Rendering logic will be implemented by individual renderers
        // This method serves as a controller interface
    }

    void RenderersController::SetCurrentRenderer(RenderStyle style) {
        if (m_currentRenderer) {
            m_currentRenderer->OnDeactivate();
        }

        auto it = m_renderers.find(style);
        if (it == m_renderers.end()) {
            LOG_ERROR("Renderer style not found: " << static_cast<int>(style));
            return;
        }

        m_currentRenderer = it->second.get();
        m_currentStyle = style;

        if (m_currentRenderer) {
            m_currentRenderer->OnActivate(0, 0);
            LOG_INFO("Switched to renderer: " << static_cast<int>(style));
        }
    }

    void RenderersController::SwitchRenderer(int direction) {
        int currentIndex = static_cast<int>(m_currentStyle);
        const int totalCount = static_cast<int>(RenderStyle::Count);

        currentIndex = (currentIndex + direction + totalCount) % totalCount;
        SetCurrentRenderer(static_cast<RenderStyle>(currentIndex));
    }

    RenderStyle RenderersController::GetCurrentRendererStyle() const {
        return m_currentStyle;
    }

    IRenderer* RenderersController::GetCurrentRenderer() const {
        return m_currentRenderer;
    }

    void RenderersController::OnWindowResize(int width, int height) {
        if (m_currentRenderer) {
            m_currentRenderer->OnActivate(width, height);
        }
    }

    void RenderersController::SetPrimaryColor(const Color& color) {
        if (m_currentRenderer) {
            m_currentRenderer->SetPrimaryColor(color);
        }
    }

    void RenderersController::InitializeRendererInstances() {
        m_renderers[RenderStyle::Bars] = std::make_unique<BarsRenderer>();
        m_renderers[RenderStyle::Wave] = std::make_unique<WaveRenderer>();
        m_renderers[RenderStyle::CircularWave] = std::make_unique<CircularWaveRenderer>();
        m_renderers[RenderStyle::Cubes] = std::make_unique<CubesRenderer>();
        m_renderers[RenderStyle::Fire] = std::make_unique<FireRenderer>();
        m_renderers[RenderStyle::LedPanel] = std::make_unique<LedPanelRenderer>();
    }

} // namespace Spectrum