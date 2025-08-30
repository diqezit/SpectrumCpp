// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BaseRenderer.h: Base class for renderers providing common functionality.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_BASE_RENDERER_H
#define SPECTRUM_CPP_BASE_RENDERER_H

#include "IRenderer.h"
#include "Common.h"

namespace Spectrum {

    class BaseRenderer : public IRenderer {
    public:
        BaseRenderer();
        ~BaseRenderer() override = default;

        // IRenderer implementation
        void SetQuality(RenderQuality quality) override;
        void SetPrimaryColor(const Color& color) override;
        void SetBackgroundColor(const Color& color) override;
        void OnActivate(int width, int height) override;

        // Template method for render
        void Render(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    protected:
        // Override points for derived classes
        virtual void UpdateSettings() {}
        virtual void UpdateAnimation(const SpectrumData& spectrum,
            float deltaTime) {
        }
        virtual void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) = 0;

        // Common state
        RenderQuality m_quality;
        Color         m_primaryColor;
        Color         m_backgroundColor;
        int           m_width;
        int           m_height;
        float         m_time;

        // Time management
        void UpdateTime(float deltaTime);
        float GetTime() const { return m_time; }

        // Viewport management
        void SetViewport(int width, int height) noexcept;
        bool IsRenderable(const SpectrumData& spectrum) const noexcept;

        // Spectrum analysis helpers
        float GetAverageMagnitude(const SpectrumData& spectrum) const;
        float GetBassMagnitude(const SpectrumData& spectrum) const;
        float GetMidMagnitude(const SpectrumData& spectrum) const;
        float GetHighMagnitude(const SpectrumData& spectrum) const;

        // Averaging utilities
        static float AverageRange(const SpectrumData& spectrum,
            size_t begin, size_t end);
        static float SegmentAverage(const SpectrumData& spectrum,
            size_t segments, size_t index);

        // Layout helpers
        struct GridMetrics {
            int   rows = 0;
            int   cols = 0;
            float cellSize = 0.0f;
            float startX = 0.0f;
            float startY = 0.0f;
        };
        GridMetrics ComputeCenteredGrid(int cols, int rows) const;

        struct BarLayout {
            float totalBarWidth = 0.0f;
            float barWidth = 0.0f;
            float spacing = 0.0f;
        };
        BarLayout ComputeBarLayout(size_t count, float spacing) const;

        // Geometry helpers
        void BuildPolylineFromSpectrum(const SpectrumData& spectrum,
            float midlineFrac,
            float amplitudeFrac,
            std::vector<Point>& out) const;

        float MagnitudeToHeight(float magnitude, float scale = 0.9f) const;

        static constexpr float TIME_RESET_THRESHOLD = 1e6f;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_BASE_RENDERER_H