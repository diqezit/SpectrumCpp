#ifndef SPECTRUM_CPP_PEAK_TRACKER_H
#define SPECTRUM_CPP_PEAK_TRACKER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the PeakTracker component for peak value tracking with decay.
//
// This reusable component tracks peak values across multiple channels with
// configurable hold time and decay behavior. It's designed to be used by
// visualizers that need sticky peak indicators (e.g., LED panels, bars).
//
// Key features:
// - Per-channel peak tracking with automatic decay
// - Configurable hold time before decay starts
// - Smooth decay animation with configurable rate
// - Visibility threshold for rendering optimization
// - Thread-safe resize operations
//
// Design notes:
// - Zero-cost abstraction when not used by renderer
// - Minimal memory footprint (two float vectors)
// - All operations are O(n) where n is channel count
// - Uses MathHelpers::Saturate for value clamping
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <vector>

namespace Spectrum {

    class PeakTracker {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration Structure
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct Config {
            float holdTime = 0.5f;      // Duration to hold peak before decay (seconds)
            float decayRate = 0.95f;    // Decay multiplier per frame (0.0 = instant, 1.0 = never)
            float minVisible = 0.01f;   // Minimum value for visibility checks
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit PeakTracker(size_t channelCount = 0, Config config = {});

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(const SpectrumData& values, float deltaTime);
        void Reset();
        void Resize(size_t newSize);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] const SpectrumData& GetPeaks() const { return m_peaks; }
        [[nodiscard]] float GetPeak(size_t index) const;
        [[nodiscard]] bool IsPeakVisible(size_t index) const;
        [[nodiscard]] size_t GetSize() const { return m_peaks.size(); }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetConfig(const Config& config) { m_config = config; }
        [[nodiscard]] const Config& GetConfig() const { return m_config; }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Update Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateChannel(size_t index, float value, float deltaTime);
        void SetNewPeak(size_t index, float value);
        void UpdateHoldTimer(size_t index, float deltaTime);
        void ApplyDecay(size_t index);

        [[nodiscard]] bool ShouldSetNewPeak(size_t index, float value) const;
        [[nodiscard]] bool IsHolding(size_t index) const;
        [[nodiscard]] bool IsValidIndex(size_t index) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Config m_config;
        SpectrumData m_peaks;
        std::vector<float> m_holdTimers;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PEAK_TRACKER_H