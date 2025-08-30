// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// RendererManager.h: Manages all available renderers and switching between them.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_RENDERER_MANAGER_H
#define SPECTRUM_CPP_RENDERER_MANAGER_H

#include "Common.h"
#include "IRenderer.h"
#include "GraphicsContext.h"

#include <map>
#include <memory>

namespace Spectrum {

    class RendererManager {
    public:
        RendererManager();
        ~RendererManager();

        bool Initialize();
        void Render(GraphicsContext& graphics,
            const SpectrumData& spectrum);
        void OnResize(int width, int height);

        void SetCurrentRenderer(RenderStyle style,
            GraphicsContext* graphics);
        void SwitchRenderer(int direction, GraphicsContext* graphics);

        // Quality management
        void SetQuality(RenderQuality quality);
        void CycleQuality();
        RenderQuality GetQuality() const { return m_currentQuality; }

        IRenderer* GetCurrentRenderer() const {
            return m_currentRenderer;
        }
        RenderStyle GetCurrentStyle() const {
            return m_currentStyle;
        }

    private:
        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        IRenderer* m_currentRenderer = nullptr;
        RenderStyle m_currentStyle = RenderStyle::Bars;
        RenderQuality m_currentQuality = RenderQuality::Medium;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDERER_MANAGER_H