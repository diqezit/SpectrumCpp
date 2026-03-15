#ifndef SPECTRUM_CPP_PEAK_TRACKER_H
#define SPECTRUM_CPP_PEAK_TRACKER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PeakTracker — per-channel peak tracking with hold and decay.
// Header-only.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <vector>
#include <algorithm>

namespace Spectrum {

    class PeakTracker {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Configuration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        struct Config {
            float holdTime = 0.5f;
            float decayRate = 0.95f;
            float minVisible = 0.01f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        explicit PeakTracker(size_t channels = 0, Config cfg = {})
            : m_config(cfg)
        {
            Resize(channels);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void Update(const SpectrumData& values, float deltaTime) {
            const size_t count = std::min(values.size(), m_peaks.size());

            for (size_t i = 0; i < count; ++i) {
                const float v = Helpers::Math::Saturate(values[i]);

                if (v >= m_peaks[i]) {
                    m_peaks[i] = v;
                    m_holdTimers[i] = m_config.holdTime;
                }
                else if (m_holdTimers[i] > 0.0f) {
                    m_holdTimers[i] = std::max(0.0f, m_holdTimers[i] - deltaTime);
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

        void Resize(size_t newSize) {
            m_peaks.resize(newSize, 0.0f);
            m_holdTimers.resize(newSize, 0.0f);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] const SpectrumData& GetPeaks() const {
            return m_peaks;
        }

        [[nodiscard]] float GetPeak(size_t i) const {
            return i < m_peaks.size() ? m_peaks[i] : 0.0f;
        }

        [[nodiscard]] bool IsPeakVisible(size_t i) const {
            return i < m_peaks.size()
                && m_peaks[i] > m_config.minVisible;
        }

        [[nodiscard]] size_t GetSize() const {
            return m_peaks.size();
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Config
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void SetConfig(const Config& cfg) { m_config = cfg; }

        [[nodiscard]] const Config& GetConfig() const { return m_config; }

    private:
        Config            m_config;
        SpectrumData      m_peaks;
        std::vector<float> m_holdTimers;
    };

} // namespace Spectrum

#endif