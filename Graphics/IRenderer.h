#ifndef SPECTRUM_CPP_IRENDERER_H
#define SPECTRUM_CPP_IRENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Contract for all visualizer implementations.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <string_view>

namespace Spectrum {

    class Canvas;

    class IRenderer {
    public:
        virtual ~IRenderer() = default;

        virtual void Render(Canvas& canvas, const SpectrumData& spectrum) = 0;
        virtual void SetQuality(RenderQuality quality) = 0;
        virtual void SetPrimaryColor(const Color&) {}
        virtual void SetOverlayMode(bool) {}

        [[nodiscard]] virtual RenderStyle      GetStyle() const = 0;
        [[nodiscard]] virtual std::string_view  GetName()  const = 0;
        [[nodiscard]] virtual bool SupportsPrimaryColor()  const { return true; }

        virtual void OnActivate(int, int) {}
        virtual void OnResize(int w, int h) { OnActivate(w, h); }
        virtual void OnDeactivate() {}
    };

} // namespace Spectrum

#endif