// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the BaseRenderer class
// It handles the core rendering loop (Update -> Render) and provides
// common state and helper functions for all derived visualizer classes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "BaseRenderer.h"
#include <algorithm>

namespace Spectrum {

    BaseRenderer::BaseRenderer()
        : m_quality(RenderQuality::Medium)
        , m_primaryColor(Color::FromRGB(33, 150, 243))
        , m_isOverlay(false)
        , m_width(0)
        , m_height(0)
        , m_time(0.0f)
        , m_aspectRatio(0.0f) // default to no fixed aspect ratio
        , m_padding(1.0f)
    {
    }

    // if quality changes, derived classes can adjust their settings
    // e.g. disable expensive effects on low quality
    void BaseRenderer::SetQuality(RenderQuality quality) {
        if (m_quality == quality) return;
        m_quality = quality;
        UpdateSettings();
    }

    // overlay mode often requires different visual styles
    // e.g. less intense colors or smaller elements
    void BaseRenderer::SetOverlayMode(bool isOverlay) {
        if (m_isOverlay == isOverlay) return;
        m_isOverlay = isOverlay;
        UpdateSettings();
    }

    void BaseRenderer::SetPrimaryColor(const Color& color) {
        m_primaryColor = color;
    }

    void BaseRenderer::OnActivate(int width, int height) {
        SetViewport(width, height);
    }

    // provides a template method pattern for rendering
    // handles boilerplate checks and timing updates
    void BaseRenderer::Render(
        GraphicsContext& context,
        const SpectrumData& spectrum
    ) {
        if (!IsRenderable(spectrum)) return;

        // animation logic is separated from rendering logic
        UpdateTime(FRAME_TIME);
        UpdateAnimation(spectrum, FRAME_TIME);
        DoRender(context, spectrum);
    }

    // for renderers with a fixed aspect ratio, calculate a centered rect
    // this ensures the visualization is not stretched or distorted
    Rect BaseRenderer::CalculatePaddedRect() const {
        float viewWidth = static_cast<float>(m_width);
        float viewHeight = static_cast<float>(m_height);

        // use full view if no aspect ratio is specified
        if (m_aspectRatio <= 0.0f) {
            return Rect(0.f, 0.f, viewWidth, viewHeight);
        }

        float renderWidth;
        float renderHeight;

        // fit to height (letterbox) or width (pillarbox) to maintain aspect ratio
        if (viewWidth / viewHeight > m_aspectRatio) {
            renderHeight = viewHeight * m_padding;
            renderWidth = renderHeight * m_aspectRatio;
        }
        else {
            renderWidth = viewWidth * m_padding;
            renderHeight = renderWidth / m_aspectRatio;
        }

        return Rect(
            (viewWidth - renderWidth) / 2.0f,
            (viewHeight - renderHeight) / 2.0f,
            renderWidth,
            renderHeight
        );
    }

    // prevent floating point precision issues if app runs for a very long time
    void BaseRenderer::UpdateTime(float deltaTime) {
        m_time += deltaTime;
        if (m_time > TIME_RESET_THRESHOLD) m_time = 0.0f;
    }

    void BaseRenderer::SetViewport(int width, int height) noexcept {
        m_width = width;
        m_height = height;
    }

    // skip rendering if there is no data or window is not visible
    bool BaseRenderer::IsRenderable(const SpectrumData& spectrum) const noexcept {
        if (spectrum.empty()) return false;
        if (m_width <= 0 || m_height <= 0) return false;
        return true;
    }

} // namespace Spectrum