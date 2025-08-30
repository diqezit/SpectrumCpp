// SpectrumProcessor.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// SpectrumProcessor.cpp: Implementation of the SpectrumProcessor class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "SpectrumProcessor.h"
#include "Utils.h"

namespace Spectrum {

    SpectrumProcessor::SpectrumProcessor(size_t barCount)
        : m_barCount(barCount)
        , m_amplificationFactor(DEFAULT_AMPLIFICATION)
        , m_smoothingFactor(DEFAULT_SMOOTHING) {

        Reset();
    }

    void SpectrumProcessor::SetBarCount(size_t count) {
        if (count > 0 && count != m_barCount) {
            m_barCount = count;
            Reset();
        }
    }

    void SpectrumProcessor::SetAmplification(float factor) {
        m_amplificationFactor = Utils::Clamp(factor, MIN_AMPLIFICATION, MAX_AMPLIFICATION);
    }

    void SpectrumProcessor::SetSmoothing(float factor) {
        m_smoothingFactor = Utils::Clamp(factor, MIN_SMOOTHING, MAX_SMOOTHING);
    }

    void SpectrumProcessor::Reset() {
        m_smoothedBars.assign(m_barCount, 0.0f);
        m_peakValues.assign(m_barCount, 0.0f);
    }

    void SpectrumProcessor::ProcessSpectrum(SpectrumData& spectrum) {
        if (spectrum.size() != m_barCount) {
            return;
        }

        ApplyScaling(spectrum);
        UpdatePeakValues(spectrum);
        ApplySmoothing(spectrum);
    }

    float SpectrumProcessor::ScaleValue(float value) const {
        // Apply logarithmic scaling with sensitivity
        float scaled = std::log1p(value * SENSITIVITY) / std::log1p(SENSITIVITY);

        // Apply amplification
        scaled = std::pow(scaled, m_amplificationFactor);

        // Clamp to valid range
        return Utils::Clamp(scaled, 0.0f, 1.0f);
    }

    void SpectrumProcessor::ApplyScaling(SpectrumData& spectrum) {
        for (size_t i = 0; i < m_barCount; ++i) {
            spectrum[i] = ScaleValue(spectrum[i]);
        }
    }

    void SpectrumProcessor::UpdatePeakValues(const SpectrumData& spectrum) {
        for (size_t i = 0; i < m_barCount; ++i) {
            if (spectrum[i] > m_peakValues[i]) {
                m_peakValues[i] = spectrum[i];
            }
            else {
                m_peakValues[i] *= PEAK_DECAY_RATE;
            }
        }
    }

    float SpectrumProcessor::SmoothValue(float current, float target) const {
        // Use different smoothing for attack vs decay
        const float smoothing = (target > current)
            ? m_smoothingFactor * ATTACK_SMOOTHING_FACTOR
            : m_smoothingFactor;

        return current * smoothing + target * (1.0f - smoothing);
    }

    void SpectrumProcessor::ApplySmoothing(SpectrumData& spectrum) {
        for (size_t i = 0; i < m_barCount; ++i) {
            const float smoothed = SmoothValue(m_smoothedBars[i], spectrum[i]);
            m_smoothedBars[i] = smoothed;
            spectrum[i] = smoothed;
        }
    }

} // namespace Spectrum