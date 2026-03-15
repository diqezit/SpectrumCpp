#ifndef SPECTRUM_CPP_RENDERER_MANAGER_H
#define SPECTRUM_CPP_RENDERER_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// RendererManager — creates, switches, and configures visualizers.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/IRenderer.h"
#include <map>
#include <memory>
#include <string_view>

namespace Spectrum {

    class EventBus;
    namespace Platform { class WindowManager; }

    class RendererManager final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        RendererManager(EventBus* bus, Platform::WindowManager* wm);
        ~RendererManager() noexcept;

        RendererManager(const RendererManager&) = delete;
        RendererManager& operator=(const RendererManager&) = delete;

        [[nodiscard]] bool Initialize();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Renderer control
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void SetCurrentRenderer(RenderStyle style);
        void SwitchToNextRenderer();
        void CycleQuality(int direction = 1);
        void OnResize(int w, int h);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] IRenderer* GetCurrentRenderer()     const noexcept { return m_current; }
        [[nodiscard]] RenderStyle      GetCurrentStyle()        const noexcept { return m_style; }
        [[nodiscard]] RenderQuality    GetQuality()             const noexcept { return m_quality; }
        [[nodiscard]] std::string_view GetCurrentRendererName() const noexcept;
        [[nodiscard]] std::string_view GetQualityName()         const noexcept;

    private:
        bool CreateRenderers();
        bool Activate(RenderStyle style);
        bool GetDimensions(int& w, int& h) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // State
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        IRenderer* m_current = nullptr;
        RenderStyle               m_style = RenderStyle::Bars;
        RenderQuality             m_quality = RenderQuality::Medium;
        Platform::WindowManager* m_wm;
    };

} // namespace Spectrum

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Inline implementation
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/EventBus.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Platform/WindowManager.h"

#include "Graphics/Visualizers/BarsRenderer.h"
#include "Graphics/Visualizers/CircularWaveRenderer.h"
#include "Graphics/Visualizers/CubesRenderer.h"
#include "Graphics/Visualizers/FireRenderer.h"
#include "Graphics/Visualizers/GaugeRenderer.h"
#include "Graphics/Visualizers/KenwoodBarsRenderer.h"
#include "Graphics/Visualizers/LedPanelRenderer.h"
#include "Graphics/Visualizers/MatrixLedRenderer.h"
#include "Graphics/Visualizers/ParticlesRenderer.h"
#include "Graphics/Visualizers/PolylineWaveRenderer.h"
#include "Graphics/Visualizers/SphereRenderer.h"
#include "Graphics/Visualizers/WaveRenderer.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline RendererManager::RendererManager(EventBus* bus, Platform::WindowManager* wm)
        : m_wm(wm)
    {
        if (bus) {
            bus->Subscribe(InputAction::SwitchRenderer, [this] { SwitchToNextRenderer(); });
            bus->Subscribe(InputAction::CycleQuality, [this] { CycleQuality(); });
        }
    }

    inline RendererManager::~RendererManager() noexcept {
        if (m_current) {
            try { m_current->OnDeactivate(); }
            catch (...) {}
        }
    }

    inline bool RendererManager::Initialize() {
        if (!m_wm || !CreateRenderers()) return false;
        return Activate(RenderStyle::Bars);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Renderer creation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline bool RendererManager::CreateRenderers() {
        try {
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
            return true;
        }
        catch (...) {
            LOG_ERROR("RendererManager: Failed to create renderers");
            return false;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Activation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline bool RendererManager::Activate(RenderStyle style) {
        auto it = m_renderers.find(style);
        if (it == m_renderers.end() || !it->second) return false;

        int w = 0, h = 0;
        if (!GetDimensions(w, h)) return false;

        try {
            if (m_current) m_current->OnDeactivate();

            auto* r = it->second.get();
            r->OnActivate(w, h);
            r->SetQuality(m_quality);

            m_current = r;
            m_style = style;
            return true;
        }
        catch (...) {
            LOG_ERROR("RendererManager: Activation failed");
            return false;
        }
    }

    inline bool RendererManager::GetDimensions(int& w, int& h) const {
        if (!m_wm) return false;
        auto* e = m_wm->GetVisualizationEngine();
        if (!e) return false;
        w = e->GetWidth();
        h = e->GetHeight();
        return w > 0 && h > 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Switching & quality
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline void RendererManager::SetCurrentRenderer(RenderStyle style) {
        if (style == m_style && m_current) return;
        Activate(style);
    }

    inline void RendererManager::SwitchToNextRenderer() {
        SetCurrentRenderer(Helpers::Utils::CycleEnum(m_style, 1));
    }

    inline void RendererManager::CycleQuality(int direction) {
        m_quality = Helpers::Utils::CycleEnum(m_quality, direction);

        for (auto& [_, r] : m_renderers)
            if (r) try { r->SetQuality(m_quality); }
        catch (...) {}
    }

    inline void RendererManager::OnResize(int w, int h) {
        if (!m_current || w <= 0 || h <= 0) return;
        try {
            m_current->OnResize(w, h);
            m_current->SetQuality(m_quality);
        }
        catch (...) {}
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Name queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    inline std::string_view RendererManager::GetCurrentRendererName() const noexcept {
        if (!m_current) return "None";
        try { return m_current->GetName(); }
        catch (...) { return "Error"; }
    }

    inline std::string_view RendererManager::GetQualityName() const noexcept {
        constexpr const char* k[] = { "Low", "Medium", "High", "Ultra" };
        const auto i = static_cast<size_t>(m_quality);
        return i < 4 ? k[i] : "Unknown";
    }

} // namespace Spectrum

#endif