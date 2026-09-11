#ifndef SPECTRUM_CPP_PEAK_TRACKER_H
#define SPECTRUM_CPP_PEAK_TRACKER_H

#include "Common/Common.h"
#include <algorithm>
#include <vector>

namespace Spectrum {

    class PeakTracker {
    public:
        struct Config {
            float holdTime = 0.5f;
            float decayRate = 0.95f;
            float minVisible = 0.01f;
        };

        explicit PeakTracker(size_t channels = 0, Config cfg = {})
            : m_config(cfg) {
            Resize(channels);
        }

        void Update(const SpectrumData& values, float dt) {
            for (size_t i = 0; i < m_peaks.size(); ++i) {
                if (values[i] >= m_peaks[i]) {
                    m_peaks[i] = values[i];
                    m_holdTimers[i] = m_config.holdTime;
                }
                else if (m_holdTimers[i] > 0.0f) {
                    m_holdTimers[i] -= dt;
                }
                else {
                    m_peaks[i] *= m_config.decayRate;
                }
            }
        }

        void Reset() {
            std::fill(m_peaks.begin(), m_peaks.end(), 0.0f);
            std::fill(m_holdTimers.begin(), m_holdTimers.end(), 0.0f);
        }

        void Resize(size_t n) {
            m_peaks.assign(n, 0.0f);
            m_holdTimers.assign(n, 0.0f);
        }

        [[nodiscard]] const SpectrumData& GetPeaks() const { return m_peaks; }
        [[nodiscard]] float GetPeak(size_t i) const { return m_peaks[i]; }
        [[nodiscard]] bool IsPeakVisible(size_t i) const { return m_peaks[i] > m_config.minVisible; }
        [[nodiscard]] size_t GetSize() const { return m_peaks.size(); }

        void SetConfig(const Config& cfg) { m_config = cfg; }
        [[nodiscard]] const Config& GetConfig() const { return m_config; }

    private:
        Config m_config;
        SpectrumData m_peaks;
        std::vector<float> m_holdTimers;
    };

} // namespace Spectrum

#endif