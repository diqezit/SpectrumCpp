#ifndef SPECTRUM_CPP_RENDERER_MANAGER_H
#define SPECTRUM_CPP_RENDERER_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Owns visualizers, switches style / quality.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Common/EventBus.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/BaseRenderer.h"

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

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Spectrum {

    class RendererManager final {
    public:
        explicit RendererManager(EventBus* bus) {
            bus->Subscribe(InputAction::SwitchRenderer, [this] { SwitchToNextRenderer(); });
            bus->Subscribe(InputAction::CycleQuality, [this] { CycleQuality(); });
        }

        ~RendererManager() noexcept {
            if (m_current) m_current->OnDeactivate();
        }

        RendererManager(const RendererManager&) = delete;
        RendererManager& operator=(const RendererManager&) = delete;

        [[nodiscard]] bool Initialize(int w, int h);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Switching
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void SetCurrentRenderer(RenderStyle style) {
            if (style != m_style) Activate(style);
        }

        void SetCurrentRendererByName(std::string_view name) {
            for (const auto& [style, r] : m_renderers)
                if (r->GetName() == name) {
                    SetCurrentRenderer(style);
                    return;
                }
        }

        void SwitchToNextRenderer() {
            SetCurrentRenderer(Helpers::Utils::CycleEnum(m_style, 1));
        }

        void CycleQuality(int direction = 1) {
            m_quality = Helpers::Utils::CycleEnum(m_quality, direction);
            for (auto& [_, r] : m_renderers)
                r->SetQuality(m_quality);
        }

        void OnResize(int w, int h) {
            m_w = w;
            m_h = h;
            m_current->OnResize(w, h);
            m_current->SetQuality(m_quality);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] Renderer* GetCurrentRenderer()      const noexcept { return m_current; }
        [[nodiscard]] RenderStyle      GetCurrentStyle()         const noexcept { return m_style; }
        [[nodiscard]] RenderQuality    GetQuality()              const noexcept { return m_quality; }
        [[nodiscard]] std::string_view GetCurrentRendererName()  const noexcept { return m_current->GetName(); }

        [[nodiscard]] const std::vector<std::string>& GetAvailableRendererNames() const noexcept {
            return m_names;
        }

        [[nodiscard]] std::string_view GetQualityName() const noexcept {
            constexpr std::string_view k[] = { "Low", "Medium", "High", "Ultra" };
            return k[size_t(m_quality)];
        }

    private:
        template<typename T, RenderStyle Style>
        void Add() { m_renderers[Style] = std::make_unique<T>(); }

        void Activate(RenderStyle style);

        std::map<RenderStyle, std::unique_ptr<Renderer>> m_renderers;
        std::vector<std::string> m_names;
        Renderer* m_current = nullptr;
        RenderStyle   m_style = RenderStyle::Bars;
        RenderQuality m_quality = RenderQuality::Medium;
        int           m_w = 0;
        int           m_h = 0;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Init / activate
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline bool RendererManager::Initialize(int w, int h) {
        using S = RenderStyle;
        Add<BarsRenderer, S::Bars>();
        Add<WaveRenderer, S::Wave>();
        Add<CircularWaveRenderer, S::CircularWave>();
        Add<CubesRenderer, S::Cubes>();
        Add<FireRenderer, S::Fire>();
        Add<LedPanelRenderer, S::LedPanel>();
        Add<GaugeRenderer, S::Gauge>();
        Add<KenwoodBarsRenderer, S::KenwoodBars>();
        Add<ParticlesRenderer, S::Particles>();
        Add<MatrixLedRenderer, S::MatrixLed>();
        Add<SphereRenderer, S::Sphere>();
        Add<PolylineWaveRenderer, S::PolylineWave>();

        m_names.reserve(m_renderers.size());
        for (const auto& [_, r] : m_renderers)
            m_names.emplace_back(r->GetName());

        m_w = w;
        m_h = h;
        Activate(S::Bars);
        return true;
    }

    inline void RendererManager::Activate(RenderStyle style) {
        if (m_current) m_current->OnDeactivate();
        m_current = m_renderers[style].get();
        m_current->OnActivate(m_w, m_h);
        m_current->SetQuality(m_quality);
        m_style = style;
    }

} // namespace Spectrum

#endif