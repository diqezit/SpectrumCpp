// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CircularWaveRenderer.h: Renders spectrum as pulsating concentric rings.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H
#define SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H

#include "BaseRenderer.h"

namespace Spectrum {

    class CircularWaveRenderer final : public BaseRenderer {
    public:
        CircularWaveRenderer();
        ~CircularWaveRenderer() override = default;

        RenderStyle GetStyle() const override {
            return RenderStyle::CircularWave;
        }
        std::string_view GetName() const override {
            return "Circular Wave";
        }
        void OnActivate(int width, int height) override;

    protected:
        void UpdateSettings() override;
        void UpdateAnimation(const SpectrumData& spectrum,
            float deltaTime) override;
        void DoRender(GraphicsContext& context,
            const SpectrumData& spectrum) override;

    private:
        void PrecomputeCirclePoints();

        struct Settings {
            int   pointsPerCircle;
            bool  useGlow;
            float maxStroke;
            int   maxRings;
            float rotationSpeed;
            float waveSpeed;
        };

        Settings m_settings;
        float m_angle;
        float m_waveTime;
        std::vector<Point> m_circlePoints;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CIRCULAR_WAVE_RENDERER_H