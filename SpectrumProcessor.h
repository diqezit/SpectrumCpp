// SpectrumProcessor.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// SpectrumProcessor.h: Processes spectrum data with smoothing, scaling and peaks.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_SPECTRUM_PROCESSOR_H
#define SPECTRUM_CPP_SPECTRUM_PROCESSOR_H

#include "Common.h"

namespace Spectrum {

    class SpectrumProcessor {
    public:
        SpectrumProcessor(size_t barCount);
        ~SpectrumProcessor() = default;

        // Main processing
        void ProcessSpectrum(SpectrumData& spectrum);

        // Configuration
        void SetAmplification(float factor);
        void SetSmoothing(float factor);
        void SetBarCount(size_t count);

        // Reset
        void Reset();

        // Getters
        const SpectrumData& GetSmoothedBars() const noexcept { return m_smoothedBars; }
        const SpectrumData& GetPeakValues() const noexcept { return m_peakValues; }
        float GetAmplification() const noexcept { return m_amplificationFactor; }
        float GetSmoothing() const noexcept { return m_smoothingFactor; }

    private:
        // Processing steps
        void ApplyScaling(SpectrumData& spectrum);
        void ApplySmoothing(SpectrumData& spectrum);
        void UpdatePeakValues(const SpectrumData& spectrum);

        // Helper methods
        float ScaleValue(float value) const;
        float SmoothValue(float current, float target) const;

    private:
        static constexpr float SENSITIVITY = 150.0f;
        static constexpr float PEAK_DECAY_RATE = 0.98f;
        static constexpr float ATTACK_SMOOTHING_FACTOR = 0.5f;
        static constexpr float MIN_AMPLIFICATION = 0.1f;
        static constexpr float MAX_AMPLIFICATION = 5.0f;
        static constexpr float MIN_SMOOTHING = 0.0f;
        static constexpr float MAX_SMOOTHING = 0.99f;

        size_t m_barCount;
        float m_amplificationFactor;
        float m_smoothingFactor;

        SpectrumData m_smoothedBars;
        SpectrumData m_peakValues;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_SPECTRUM_PROCESSOR_H