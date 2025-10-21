// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the RendererManager, which handles the lifecycle and
// configuration of all visualization renderers.
// 
// Key implementation details:
// - Manages a collection of IRenderer implementations
// - Handles switching between different visualization styles
// - Applies global quality settings across all renderers
// - Responds to window resize events
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RendererManager.h"
#include "BarsRenderer.h"
#include "CircularWaveRenderer.h"
#include "CubesRenderer.h"
#include "EventBus.h"
#include "FireRenderer.h"
#include "GaugeRenderer.h"
#include "GraphicsContext.h"
#include "KenwoodBarsRenderer.h"
#include "LedPanelRenderer.h"
#include "TemplateUtils.h"
#include "WaveRenderer.h"
#include "WindowManager.h"

namespace Spectrum
{
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    RendererManager::RendererManager(EventBus* bus, WindowManager* windowManager) :
        m_currentRenderer(nullptr),
        m_currentStyle(RenderStyle::Bars),
        m_currentQuality(RenderQuality::Medium),
        m_windowManager(windowManager)
    {
        SubscribeToEvents(bus);
    }

    RendererManager::~RendererManager() = default;

    bool RendererManager::Initialize()
    {
        CreateRenderers();
        ActivateInitialRenderer();
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Event Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::OnResize(int width, int height)
    {
        if (m_currentRenderer)
            m_currentRenderer->OnActivate(width, height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::SetCurrentRenderer(RenderStyle style, GraphicsContext* graphics)
    {
        DeactivateCurrentRenderer();
        ActivateNewRenderer(style, graphics);
    }

    void RendererManager::SwitchToNextRenderer(GraphicsContext* graphics)
    {
        const RenderStyle nextStyle = Utils::CycleEnum(m_currentStyle, 1);
        SetCurrentRenderer(nextStyle, graphics);
    }

    void RendererManager::SwitchToPrevRenderer(GraphicsContext* graphics)
    {
        const RenderStyle nextStyle = Utils::CycleEnum(m_currentStyle, -1);
        SetCurrentRenderer(nextStyle, graphics);
    }

    void RendererManager::CycleQuality(int direction)
    {
        const RenderQuality nextQuality = Utils::CycleEnum(m_currentQuality, direction);
        SetQuality(nextQuality);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    IRenderer* RendererManager::GetCurrentRenderer() const
    {
        return m_currentRenderer;
    }

    RenderStyle RendererManager::GetCurrentStyle() const
    {
        return m_currentStyle;
    }

    RenderQuality RendererManager::GetQuality() const
    {
        return m_currentQuality;
    }

    std::string_view RendererManager::GetCurrentRendererName() const
    {
        if (m_currentRenderer)
            return m_currentRenderer->GetName();
        return "None";
    }

    std::string_view RendererManager::GetQualityName() const
    {
        switch (m_currentQuality)
        {
        case RenderQuality::Low:    return "Low";
        case RenderQuality::Medium: return "Medium";
        case RenderQuality::High:   return "High";
        }
        return "Unknown";
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::SubscribeToEvents(EventBus* bus)
    {
        bus->Subscribe(InputAction::SwitchRenderer, [this]() {
            if (m_windowManager) this->SwitchToNextRenderer(m_windowManager->GetGraphics());
            });

        bus->Subscribe(InputAction::CycleQuality, [this]() {
            this->CycleQuality(1);
            });
    }

    void RendererManager::CreateRenderers()
    {
        m_renderers[RenderStyle::Bars] = std::make_unique<BarsRenderer>();
        m_renderers[RenderStyle::Wave] = std::make_unique<WaveRenderer>();
        m_renderers[RenderStyle::CircularWave] = std::make_unique<CircularWaveRenderer>();
        m_renderers[RenderStyle::Cubes] = std::make_unique<CubesRenderer>();
        m_renderers[RenderStyle::Fire] = std::make_unique<FireRenderer>();
        m_renderers[RenderStyle::LedPanel] = std::make_unique<LedPanelRenderer>();
        m_renderers[RenderStyle::Gauge] = std::make_unique<GaugeRenderer>();
        m_renderers[RenderStyle::KenwoodBars] = std::make_unique<KenwoodBarsRenderer>();
    }

    void RendererManager::ActivateInitialRenderer()
    {
        // Start with a default style
        m_currentStyle = RenderStyle::Bars;
        auto it = m_renderers.find(m_currentStyle);
        if (it != m_renderers.end())
        {
            m_currentRenderer = it->second.get();
            SetQuality(m_currentQuality);
        }
    }

    void RendererManager::DeactivateCurrentRenderer()
    {
        if (m_currentRenderer)
            m_currentRenderer->OnDeactivate();
    }

    void RendererManager::ActivateNewRenderer(RenderStyle style, GraphicsContext* graphics)
    {
        auto it = m_renderers.find(style);
        if (it != m_renderers.end())
        {
            m_currentRenderer = it->second.get();
            m_currentStyle = style;
        }

        if (!m_currentRenderer || !graphics) return;

        const int width = graphics->GetWidth();
        const int height = graphics->GetHeight();
        m_currentRenderer->OnActivate(width, height);
        m_currentRenderer->SetQuality(m_currentQuality); // Apply current quality to new renderer

        LOG_INFO("Switched to " << m_currentRenderer->GetName().data() << " renderer");
    }

    void RendererManager::SetQuality(RenderQuality quality)
    {
        m_currentQuality = quality;

        // Apply to all renderers so the setting is consistent when switching
        for (auto& [style, renderer] : m_renderers)
        {
            if (renderer)
                renderer->SetQuality(quality);
        }

        LOG_INFO("Render quality set to " << GetQualityName().data());
    }

} // namespace Spectrum