// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the BaseRenderer class for common visualizer functionality.
//
// Implementation details:
// - Template Method pattern for rendering pipeline
// - Quality changes trigger UpdateSettings() callback
// - Aspect ratio calculations support letterbox/pillarbox modes
// - Frame timing with overflow protection (resets at threshold)
// - Uses D2DHelpers for parameter sanitization
// - GetQualitySettings<>() template method eliminates duplication
// - CreatePeakConfig() provides consistent peak tracker initialization
// - Uses unified validation system (Validation.h) for data validation
//
// Rendering pipeline:
// 1. IsRenderable() - validate spectrum and viewport (uses Validation.h)
// 2. UpdateTime() - advance animation timer
// 3. UpdateAnimation() - derived class animation logic
// 4. DoRender() - derived class rendering logic
//
// DRY improvements:
// - GetQualitySettings<>() replaces manual QualityPresets::Get calls
// - CreatePeakConfig() centralizes peak configuration creation
// - Unified validation for consistent error reporting
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Validate;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    BaseRenderer::BaseRenderer()
        : m_quality(RenderQuality::Medium)
        , m_primaryColor(Color::FromRGB(33, 150, 243))
        , m_isOverlay(false)
        , m_width(0)
        , m_height(0)
        , m_aspectRatio(0.0f)
        , m_padding(1.0f)
        , m_time(0.0f)
    {
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // IRenderer Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BaseRenderer::SetQuality(RenderQuality quality)
    {
        if (m_quality == quality) return;

        m_quality = quality;
        UpdateSettings();
    }

    void BaseRenderer::SetOverlayMode(bool isOverlay)
    {
        if (m_isOverlay == isOverlay) return;

        m_isOverlay = isOverlay;
        UpdateSettings();
    }

    void BaseRenderer::SetPrimaryColor(const Color& color)
    {
        m_primaryColor = color;
    }

    void BaseRenderer::OnActivate(int width, int height)
    {
        SetViewport(width, height);
    }

    void BaseRenderer::Render(
        Canvas& canvas,
        const SpectrumData& spectrum
    )
    {
        if (!IsRenderable(spectrum)) return;

        UpdateTime(kDefaultFrameTime);
        UpdateAnimation(spectrum, kDefaultFrameTime);
        DoRender(canvas, spectrum);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Helper Methods
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool BaseRenderer::IsRenderable(const SpectrumData& spectrum) const noexcept
    {
        if (!Condition(!spectrum.empty(), "Spectrum data is empty", "BaseRenderer")) {
            return false;
        }

        if (!Condition(m_width > 0 && m_height > 0, "Invalid viewport dimensions", "BaseRenderer")) {
            return false;
        }

        return true;
    }

    Rect BaseRenderer::CalculatePaddedRect() const noexcept
    {
        const float viewWidth = static_cast<float>(m_width);
        const float viewHeight = static_cast<float>(m_height);

        if (m_aspectRatio <= 0.0f) {
            return Rect{ 0.0f, 0.0f, viewWidth, viewHeight };
        }

        const float sanitizedPadding = NormalizedFloat(m_padding);
        float renderWidth;
        float renderHeight;

        const float viewAspect = viewWidth / viewHeight;

        if (viewAspect > m_aspectRatio) {
            renderHeight = viewHeight * sanitizedPadding;
            renderWidth = renderHeight * m_aspectRatio;
        }
        else {
            renderWidth = viewWidth * sanitizedPadding;
            renderHeight = renderWidth / m_aspectRatio;
        }

        return Rect{
            (viewWidth - renderWidth) * 0.5f,
            (viewHeight - renderHeight) * 0.5f,
            renderWidth,
            renderHeight
        };
    }

    PeakTracker::Config BaseRenderer::CreatePeakConfig(
        float holdTime,
        float decayRate,
        float minVisible
    )
    {
        PeakTracker::Config config;
        config.holdTime = holdTime;
        config.decayRate = decayRate;
        config.minVisible = minVisible;
        return config;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void BaseRenderer::UpdateTime(float deltaTime) const
    {
        m_time += deltaTime;
        if (m_time > kTimeResetThreshold) {
            m_time = 0.0f;
        }
    }

    void BaseRenderer::SetViewport(int width, int height) noexcept
    {
        m_width = std::max(width, 0);
        m_height = std::max(height, 0);
    }

} // namespace Spectrum