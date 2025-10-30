#ifndef SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H
#define SPECTRUM_CPP_POLYLINE_WAVE_RENDERER_H

#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"
#include <vector>

namespace Spectrum {

    class Canvas;

    class PolylineWaveRenderer final : public BaseRenderer<PolylineWaveRenderer>
    {
    public:
        PolylineWaveRenderer();
        ~PolylineWaveRenderer() override = default;

        [[nodiscard]] RenderStyle GetStyle() const override {
            return RenderStyle::PolylineWave;
        }

        [[nodiscard]] std::string_view GetName() const override {
            return "Sunburst";
        }

    protected:
        void UpdateSettings() override;

        void UpdateAnimation(
            const SpectrumData& spectrum,
            float deltaTime
        ) override;

        void DoRender(Canvas& canvas, const SpectrumData& spectrum) override;

    private:
        using Settings = Settings::PolylineWaveSettings;

        void EnsureBarDirections(size_t barCount);

        void RenderBars(
            Canvas& canvas,
            const SpectrumData& spectrum,
            const Point& center,
            float baseRadius,
            float barWidth
        ) const;

        [[nodiscard]] float CalculateBarLength(
            float magnitude,
            float radius
        ) const;

        Settings m_settings;
        std::vector<Point> m_barDirections;
        float m_currentCoreRadius;
        float m_targetCoreRadius;
    };

} // namespace Spectrum

#endif