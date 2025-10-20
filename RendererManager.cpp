// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the RendererManager, which is responsible for managing
// all available visualizers, handling switching between them, and
// applying global settings like render quality.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RendererManager.h"
#include "BarsRenderer.h"
#include "WaveRenderer.h"
#include "CircularWaveRenderer.h"
#include "CubesRenderer.h"
#include "FireRenderer.h"
#include "LedPanelRenderer.h"
#include "GaugeRenderer.h"
#include "KenwoodBarsRenderer.h"
#include "TemplateUtils.h"
#include "EventBus.h"
#include "WindowManager.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    RendererManager::RendererManager(
        EventBus* bus,
        WindowManager* windowManager
    ) :
        m_currentRenderer(nullptr),
        m_currentStyle(RenderStyle::Bars),
        m_currentQuality(RenderQuality::Medium),
        m_windowManager(windowManager)
    {
        SubscribeToEvents(bus);
    }

    RendererManager::~RendererManager() = default;

    bool RendererManager::Initialize() {
        CreateRenderers();
        ActivateInitialRenderer();
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Public Interface
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void RendererManager::OnResize(int width, int height) {
        if (m_currentRenderer)
            m_currentRenderer->OnActivate(width, height);
    }

    void RendererManager::SetCurrentRenderer(
        RenderStyle style,
        GraphicsContext* graphics
    ) {
        DeactivateCurrentRenderer();
        ActivateNewRenderer(style, graphics);
    }

    void RendererManager::SwitchToNextRenderer(GraphicsContext* graphics) {
        RenderStyle nextStyle = Utils::CycleEnum(m_currentStyle, 1);
        SetCurrentRenderer(nextStyle, graphics);
    }

    void RendererManager::SwitchToPrevRenderer(GraphicsContext* graphics) {
        RenderStyle nextStyle = Utils::CycleEnum(m_currentStyle, -1);
        SetCurrentRenderer(nextStyle, graphics);
    }

    void RendererManager::CycleQuality(int direction) {
        RenderQuality nextQuality = Utils::CycleEnum(m_currentQuality, direction);
        SetQuality(nextQuality);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] IRenderer* RendererManager::GetCurrentRenderer() const {
        return m_currentRenderer;
    }

    [[nodiscard]] RenderStyle RendererManager::GetCurrentStyle() const {
        return m_currentStyle;
    }

    [[nodiscard]] RenderQuality RendererManager::GetQuality() const {
        return m_currentQuality;
    }

    [[nodiscard]] std::string_view RendererManager::GetCurrentRendererName() const {
        if (m_currentRenderer)
            return m_currentRenderer->GetName();
        return "None";
    }

    [[nodiscard]] std::string_view RendererManager::GetQualityName() const {
        switch (m_currentQuality) {
        case RenderQuality::Low:    return "Low";
        case RenderQuality::Medium: return "Medium";
        case RenderQuality::High:   return "High";
        default:                    return "Unknown";
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void RendererManager::SubscribeToEvents(EventBus* bus) {
        bus->Subscribe(InputAction::SwitchRenderer, [this]() {
            if (m_windowManager) this->SwitchToNextRenderer(m_windowManager->GetGraphics());
            });
        bus->Subscribe(InputAction::CycleQuality, [this]() {
            this->CycleQuality(1);
            });
    }

    void RendererManager::CreateRenderers() {
        m_renderers[RenderStyle::Bars] = std::make_unique<BarsRenderer>();
        m_renderers[RenderStyle::Wave] = std::make_unique<WaveRenderer>();
        m_renderers[RenderStyle::CircularWave] = std::make_unique<CircularWaveRenderer>();
        m_renderers[RenderStyle::Cubes] = std::make_unique<CubesRenderer>();
        m_renderers[RenderStyle::Fire] = std::make_unique<FireRenderer>();
        m_renderers[RenderStyle::LedPanel] = std::make_unique<LedPanelRenderer>();
        m_renderers[RenderStyle::Gauge] = std::make_unique<GaugeRenderer>();
        m_renderers[RenderStyle::KenwoodBars] = std::make_unique<KenwoodBarsRenderer>();
    }

    void RendererManager::ActivateInitialRenderer() {
        m_currentStyle = RenderStyle::Bars;
        m_currentRenderer = m_renderers[m_currentStyle].get();
        SetQuality(m_currentQuality);
    }

    void RendererManager::DeactivateCurrentRenderer() {
        if (m_currentRenderer)
            m_currentRenderer->OnDeactivate();
    }

    void RendererManager::ActivateNewRenderer(
        RenderStyle style,
        GraphicsContext* graphics
    ) {
        if (m_renderers.count(style)) {
            m_currentRenderer = m_renderers[style].get();
            m_currentStyle = style;
        }

        if (!m_currentRenderer || !graphics) return;

        int w = graphics->GetWidth();
        int h = graphics->GetHeight();
        m_currentRenderer->OnActivate(w, h);
        m_currentRenderer->SetQuality(m_currentQuality); // Apply current quality to new renderer

        LOG_INFO("Switched to " << m_currentRenderer->GetName().data() << " renderer");
    }

    void RendererManager::SetQuality(RenderQuality quality) {
        m_currentQuality = quality;
        // Apply to all renderers so the setting is consistent when switching
        for (auto& [style, renderer] : m_renderers) {
            if (renderer)
                renderer->SetQuality(quality);
        }
        LOG_INFO("Render quality set to " << GetQualityName().data());
    }

} // namespace Spectrum