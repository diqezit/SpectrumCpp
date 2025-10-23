#ifndef SPECTRUM_CPP_BASE_RENDERER_H
#define SPECTRUM_CPP_BASE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the BaseRenderer class, an abstract base for all visualizers.
//
// This class implements the IRenderer interface and provides common
// functionality like quality settings, timing, and viewport management.
// It follows the Template Method pattern for rendering pipeline.
//
// Key responsibilities:
// - Quality and overlay mode management with settings propagation
// - Viewport and aspect ratio calculations
// - Frame timing with overflow protection
// - Common rendering pipeline (Update -> Render)
// - Helper for PeakTracker configuration creation
//
// Design notes:
// - All public methods are virtual (IRenderer interface)
// - Protected virtual methods for customization by derived classes
// - m_time is mutable and protected for animation state access
// - Template Method pattern: Render() calls UpdateAnimation() -> DoRender()
// - CreatePeakConfig() provides consistent peak configuration
// - BaseRenderer does NOT contain PeakTracker as member (opt-in by children)
// - Uses unified validation system (Graphics/API/Helpers/Core/Validation.h)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/IRenderer.h"
#include "Graphics/Base/PeakTracker.h"
#include "Common/Common.h"

namespace Spectrum {

    class Canvas;

    class BaseRenderer : public IRenderer
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static inline constexpr float kTimeResetThreshold = 1e6f;
        static inline constexpr float kDefaultFrameTime = 1.0f / 60.0f;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        BaseRenderer();
        ~BaseRenderer() override = default;

        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer& operator=(const BaseRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // IRenderer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetQuality(RenderQuality quality) override;
        void SetPrimaryColor(const Color& color) override;
        void SetOverlayMode(bool isOverlay) override;
        void OnActivate(int width, int height) override;

        void Render(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) override;

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Virtual Methods for Derived Classes
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        virtual void UpdateSettings() {}

        virtual void UpdateAnimation(
            const SpectrumData& /*spectrum*/,
            float /*deltaTime*/
        ) {
        }

        virtual void DoRender(
            Canvas& canvas,
            const SpectrumData& spectrum
        ) = 0;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helper Methods for Derived Classes
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsRenderable(const SpectrumData& spectrum) const noexcept;
        [[nodiscard]] Rect CalculatePaddedRect() const noexcept;
        [[nodiscard]] float GetTime() const noexcept { return m_time; }
        [[nodiscard]] int GetWidth() const noexcept { return m_width; }
        [[nodiscard]] int GetHeight() const noexcept { return m_height; }
        [[nodiscard]] RenderQuality GetQuality() const noexcept { return m_quality; }
        [[nodiscard]] bool IsOverlay() const noexcept { return m_isOverlay; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Peak Tracker Helper (DRY Principle)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] static PeakTracker::Config CreatePeakConfig(
            float holdTime,
            float decayRate = 0.95f,
            float minVisible = 0.01f
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Protected Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        RenderQuality m_quality;
        Color m_primaryColor;
        bool m_isOverlay;
        int m_width;
        int m_height;
        float m_aspectRatio;
        float m_padding;
        mutable float m_time;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateTime(float deltaTime) const;
        void SetViewport(int width, int height) noexcept;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_BASE_RENDERER_H