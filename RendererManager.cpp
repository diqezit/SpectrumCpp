// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// RendererManager.cpp: Implementation of the RendererManager class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RendererManager.h"
#include "BarsRenderer.h"
#include "WaveRenderer.h"
#include "CircularWaveRenderer.h"
#include "CubesRenderer.h"
#include "FireRenderer.h"
#include "LedPanelRenderer.h"
#include "Utils.h"

namespace Spectrum {

    RendererManager::RendererManager() {}
    RendererManager::~RendererManager() {}

    bool RendererManager::Initialize() {
        m_renderers[RenderStyle::Bars] =
            std::make_unique<BarsRenderer>();
        m_renderers[RenderStyle::Wave] =
            std::make_unique<WaveRenderer>();
        m_renderers[RenderStyle::CircularWave] =
            std::make_unique<CircularWaveRenderer>();
        m_renderers[RenderStyle::Cubes] =
            std::make_unique<CubesRenderer>();
        m_renderers[RenderStyle::Fire] =
            std::make_unique<FireRenderer>();
        m_renderers[RenderStyle::LedPanel] =
            std::make_unique<LedPanelRenderer>();

        m_currentStyle = RenderStyle::Bars;
        m_currentRenderer = m_renderers[m_currentStyle].get();

        // Set initial quality for all renderers
        SetQuality(m_currentQuality);

        return true;
    }

    void RendererManager::SetCurrentRenderer(RenderStyle style,
        GraphicsContext* graphics) {
        if (m_currentRenderer) {
            m_currentRenderer->OnDeactivate();
        }

        m_currentRenderer = m_renderers[style].get();
        m_currentStyle = style;

        if (m_currentRenderer && graphics) {
            int w = graphics->GetWidth();
            int h = graphics->GetHeight();
            m_currentRenderer->OnActivate(w, h);
            LOG_INFO("Switched to " <<
                m_currentRenderer->GetName().data() <<
                " renderer");
        }
    }

    void RendererManager::SwitchRenderer(int direction,
        GraphicsContext* graphics) {
        int idx = static_cast<int>(m_currentStyle);
        const int cnt = static_cast<int>(RenderStyle::Count);
        idx = (idx + direction + cnt) % cnt;
        SetCurrentRenderer(static_cast<RenderStyle>(idx), graphics);
    }

    void RendererManager::SetQuality(RenderQuality quality) {
        m_currentQuality = quality;

        // Apply quality to all renderers
        for (auto& [style, renderer] : m_renderers) {
            if (renderer) {
                renderer->SetQuality(quality);
            }
        }

        const char* qualityName = nullptr;
        switch (quality) {
        case RenderQuality::Low:
            qualityName = "Low";
            break;
        case RenderQuality::Medium:
            qualityName = "Medium";
            break;
        case RenderQuality::High:
            qualityName = "High";
            break;
        }

        if (qualityName) {
            LOG_INFO("Render quality set to " << qualityName);
        }
    }

    void RendererManager::CycleQuality() {
        int q = static_cast<int>(m_currentQuality);
        q = (q + 1) % 3;
        SetQuality(static_cast<RenderQuality>(q));
    }

    void RendererManager::Render(GraphicsContext& graphics,
        const SpectrumData& spectrum) {
        if (m_currentRenderer) {
            m_currentRenderer->Render(graphics, spectrum);
        }
    }

    void RendererManager::OnResize(int width, int height) {
        if (m_currentRenderer) {
            m_currentRenderer->OnActivate(width, height);
        }
    }

} // namespace Spectrum