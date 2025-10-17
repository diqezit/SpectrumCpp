// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the RendererManager, which handles the lifecycle and
// configuration of all visualization renderers.
//
// Key implementation details:
// - Manages a collection of IRenderer implementations
// - Handles switching between different visualization styles
// - Applies global quality settings across all renderers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RendererManager.h"
#include "Graphics/Visualizers/BarsRenderer.h"
#include "Graphics/Visualizers/CircularWaveRenderer.h"
#include "Graphics/Visualizers/CubesRenderer.h"
#include "Common/EventBus.h"
#include "Graphics/Visualizers/FireRenderer.h"
#include "Graphics/Visualizers/GaugeRenderer.h"
#include "Graphics/Visualizers/KenwoodBarsRenderer.h"
#include "Graphics/Visualizers/LedPanelRenderer.h"
#include "Graphics/Visualizers/MatrixLedRenderer.h"
#include "Graphics/Visualizers/ParticlesRenderer.h"
#include "Graphics/Visualizers/PolylineWaveRenderer.h"
#include "Graphics/Visualizers/SphereRenderer.h"
#include "Common/TemplateUtils.h"
#include "Graphics/Visualizers/WaveRenderer.h"
#include "Platform/WindowManager.h"
#include "Graphics/API/Core/RenderEngine.h"

namespace Spectrum
{
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    RendererManager::RendererManager(EventBus* bus, Platform::WindowManager* windowManager) :
        m_currentRenderer(nullptr),
        m_currentStyle(RenderStyle::Bars),
        m_currentQuality(RenderQuality::Medium),
        m_windowManager(windowManager)
    {
        SubscribeToEvents(bus);
    }

    RendererManager::~RendererManager() = default;

    [[nodiscard]] bool RendererManager::Initialize()
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
        if (m_currentRenderer) m_currentRenderer->OnActivate(width, height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::SetCurrentRenderer(RenderStyle style)
    {
        DeactivateCurrentRenderer();
        ActivateNewRenderer(style);
    }

    void RendererManager::SwitchToNextRenderer()
    {
        const RenderStyle nextStyle = Utils::CycleEnum(m_currentStyle, 1);
        SetCurrentRenderer(nextStyle);
    }

    void RendererManager::SwitchToPrevRenderer()
    {
        const RenderStyle nextStyle = Utils::CycleEnum(m_currentStyle, -1);
        SetCurrentRenderer(nextStyle);
    }

    void RendererManager::CycleQuality(int direction)
    {
        const RenderQuality nextQuality = Utils::CycleEnum(m_currentQuality, direction);
        SetQuality(nextQuality);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] IRenderer* RendererManager::GetCurrentRenderer() const noexcept
    {
        return m_currentRenderer;
    }

    [[nodiscard]] RenderStyle RendererManager::GetCurrentStyle() const noexcept
    {
        return m_currentStyle;
    }

    [[nodiscard]] RenderQuality RendererManager::GetQuality() const noexcept
    {
        return m_currentQuality;
    }

    [[nodiscard]] std::string_view RendererManager::GetCurrentRendererName() const noexcept
    {
        if (m_currentRenderer) return m_currentRenderer->GetName();
        return "None";
    }

    [[nodiscard]] std::string_view RendererManager::GetQualityName() const noexcept
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
        if (!bus) return;

        bus->Subscribe(InputAction::SwitchRenderer, [this]() {
            SwitchToNextRenderer();
            });

        bus->Subscribe(InputAction::CycleQuality, [this]() {
            CycleQuality(1);
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
        m_renderers[RenderStyle::Particles] = std::make_unique<ParticlesRenderer>();
        m_renderers[RenderStyle::MatrixLed] = std::make_unique<MatrixLedRenderer>();
        m_renderers[RenderStyle::Sphere] = std::make_unique<SphereRenderer>();
        m_renderers[RenderStyle::PolylineWave] = std::make_unique<PolylineWaveRenderer>();
    }

    void RendererManager::ActivateInitialRenderer()
    {
        m_currentStyle = RenderStyle::Bars;
        if (auto it = m_renderers.find(m_currentStyle); it != m_renderers.end())
        {
            m_currentRenderer = it->second.get();
            SetQuality(m_currentQuality);
        }
    }

    void RendererManager::DeactivateCurrentRenderer()
    {
        if (m_currentRenderer) m_currentRenderer->OnDeactivate();
    }

    void RendererManager::ActivateNewRenderer(RenderStyle style)
    {
        if (auto it = m_renderers.find(style); it != m_renderers.end())
        {
            m_currentRenderer = it->second.get();
            m_currentStyle = style;
        }

        if (!m_currentRenderer || !m_windowManager) return;

        if (auto* engine = m_windowManager->GetRenderEngine())
        {
            m_currentRenderer->OnActivate(engine->GetWidth(), engine->GetHeight());
            m_currentRenderer->SetQuality(m_currentQuality);

            LOG_INFO("Switched to " << m_currentRenderer->GetName().data() << " renderer");
        }
    }

    void RendererManager::SetQuality(RenderQuality quality)
    {
        m_currentQuality = quality;

        for (auto& [style, renderer] : m_renderers)
        {
            if (renderer) renderer->SetQuality(quality);
        }

        LOG_INFO("Render quality set to " << GetQualityName().data());
    }

} // namespace Spectrum