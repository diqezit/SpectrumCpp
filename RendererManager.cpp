// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the RendererManager, which is responsible for managing
// all available visualizers, handling switching between them, and
// orchestrating the main scene rendering process.
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
#include "ColorPicker.h"

namespace Spectrum {

    RendererManager::RendererManager(EventBus* bus, WindowManager* windowManager)
        : m_windowManager(windowManager)
    {
        SubscribeToEvents(bus);
    }

    RendererManager::~RendererManager() = default;

    void RendererManager::SubscribeToEvents(EventBus* bus) {
        bus->Subscribe(InputAction::SwitchRenderer, [this]() {
            if (m_windowManager) this->SwitchToNextRenderer(m_windowManager->GetGraphics());
            });
        bus->Subscribe(InputAction::CycleQuality, [this]() {
            this->CycleQuality();
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

    bool RendererManager::Initialize() {
        CreateRenderers();
        ActivateInitialRenderer();
        return true;
    }

    bool RendererManager::ShouldSkipRendering(GraphicsContext& graphics) const {
        auto rt = graphics.GetRenderTarget();
        if (rt && (rt->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)) return true;
        return false;
    }

    Color RendererManager::GetClearColor(bool isOverlay) const {
        return isOverlay ? Color::Transparent() : Color::FromRGB(13, 13, 26);
    }

    void RendererManager::RenderVisualizer(GraphicsContext& graphics, const SpectrumData& spectrum) {
        if (m_currentRenderer) m_currentRenderer->Render(graphics, spectrum);
    }

    void RendererManager::RenderUI(GraphicsContext& graphics, ColorPicker* colorPicker, bool isOverlay) {
        if (colorPicker && colorPicker->IsVisible() && !isOverlay) {
            colorPicker->Draw(graphics);
        }
    }

    void RendererManager::RenderScene(
        GraphicsContext& graphics,
        const SpectrumData& spectrum,
        ColorPicker* colorPicker,
        bool isOverlay
    ) {
        if (ShouldSkipRendering(graphics)) return;

        graphics.BeginDraw();
        graphics.Clear(GetClearColor(isOverlay));

        RenderVisualizer(graphics, spectrum);
        RenderUI(graphics, colorPicker, isOverlay);
    }

    void RendererManager::DeactivateCurrentRenderer() {
        if (m_currentRenderer) m_currentRenderer->OnDeactivate();
    }

    void RendererManager::ActivateNewRenderer(RenderStyle style, GraphicsContext* graphics) {
        m_currentRenderer = m_renderers[style].get();
        m_currentStyle = style;

        if (!m_currentRenderer || !graphics) return;

        int w = graphics->GetWidth();
        int h = graphics->GetHeight();
        m_currentRenderer->OnActivate(w, h);
        LOG_INFO("Switched to " << m_currentRenderer->GetName().data() << " renderer");
    }

    void RendererManager::SetCurrentRenderer(
        RenderStyle style, GraphicsContext* graphics
    ) {
        DeactivateCurrentRenderer();
        ActivateNewRenderer(style, graphics);
    }

    void RendererManager::SwitchToNextRenderer(GraphicsContext* graphics) {
        RenderStyle nextStyle = Utils::CycleEnum(m_currentStyle, 1);
        SetCurrentRenderer(nextStyle, graphics);
    }

    const char* RendererManager::GetQualityName(RenderQuality quality) const {
        switch (quality) {
        case RenderQuality::Low:    return "Low";
        case RenderQuality::Medium: return "Medium";
        case RenderQuality::High:   return "High";
        default:                    return "Unknown";
        }
    }

    void RendererManager::SetQuality(RenderQuality quality) {
        m_currentQuality = quality;
        for (auto& [style, renderer] : m_renderers) {
            if (renderer) renderer->SetQuality(quality);
        }
        LOG_INFO("Render quality set to " << GetQualityName(quality));
    }

    void RendererManager::CycleQuality() {
        RenderQuality nextQuality = Utils::CycleEnum(m_currentQuality, 1);
        SetQuality(nextQuality);
    }

    void RendererManager::OnResize(int width, int height) {
        if (m_currentRenderer) {
            m_currentRenderer->OnActivate(width, height);
        }
    }

} // namespace Spectrum