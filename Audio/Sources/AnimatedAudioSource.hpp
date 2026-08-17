#pragma once

#include "IAudioSource.hpp"
#include "Audio/Processing/Spectrum.hpp"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum {

    class AnimatedAudioSource : public IAudioSource {
    public:
        explicit AnimatedAudioSource(const AudioConfig& cfg)
            : m_n(cfg.barCount)
            , m_post(cfg.barCount)
        {
            m_post.SetSmoothing(cfg.smoothing);
        }

        bool         Initialize()      override { return true; }
        SpectrumData GetSpectrum()     override { return m_post.GetSmoothedBars(); }
        void         SetBarCount(size_t n) override { m_n = n; m_post.SetBarCount(n); }
        void         SetSmoothing(float s) override { m_post.SetSmoothing(s); }

        void Update(float dt) override {
            m_t += dt;
            auto& rng = Helpers::Utils::Random::Instance();
            SpectrumData spec(m_n);
            for (size_t i = 0; i < m_n; ++i) {
                const float f = float(i) / float(m_n);
                const float v = (std::sin(m_t * 2.0f + float(i) * 0.3f) + 1.0f) * 0.5f;
                spec[i] = Saturate(v * (1.0f - f * 0.7f) + rng.Float(-0.05f, 0.05f));
            }
            m_post.Process(spec);
        }

    private:
        float         m_t = 0.0f;
        size_t        m_n;
        PostProcessor m_post;
    };

} // namespace Spectrum