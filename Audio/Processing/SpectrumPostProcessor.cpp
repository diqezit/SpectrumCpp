// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the SpectrumPostProcessor. It orchestrates the
// post-processing pipeline by first normalizing the signal's gain and
// then applying signal shaping and visual effects.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "SpectrumPostProcessor.h"
#include "Common/MathUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SpectrumPostProcessor::SpectrumPostProcessor(
        size_t barCount
    ) :
        m_barCount(barCount),
        m_amplificationFactor(DEFAULT_AMPLIFICATION),
        m_smoothingFactor(DEFAULT_SMOOTHING),
        m_normalizer(std::make_unique<GainNormalizer>())
    {
        Reset();
    }

    void SpectrumPostProcessor::Reset() {
        m_smoothedBars.assign(m_barCount, 0.0f);
        m_peakValues.assign(m_barCount, 0.0f);
        if (m_normalizer)
            m_normalizer->Reset();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void SpectrumPostProcessor::Process(SpectrumData& spectrum) {
        if (spectrum.size() != m_barCount) return;

        // Pipeline: Normalize -> Shape -> Apply Visual Effects
        m_normalizer->Process(spectrum);
        ApplyLogarithmicScaling(spectrum);
        ApplyAmplification(spectrum);
        UpdateBarPeaks(spectrum);
        ApplySmoothing(spectrum);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void SpectrumPostProcessor::SetBarCount(size_t newBarCount) {
        if (newBarCount > 0 && newBarCount != m_barCount) {
            m_barCount = newBarCount;
            Reset();
        }
    }

    void SpectrumPostProcessor::SetAmplification(float newAmplification) {
        m_amplificationFactor = Utils::Clamp(newAmplification, 0.1f, 5.0f);
    }

    void SpectrumPostProcessor::SetSmoothing(float newSmoothing) {
        m_smoothingFactor = Utils::Saturate(newSmoothing);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] const SpectrumData& SpectrumPostProcessor::GetSmoothedBars() const {
        return m_smoothedBars;
    }

    [[nodiscard]] const SpectrumData& SpectrumPostProcessor::GetPeakValues() const {
        return m_peakValues;
    }

    [[nodiscard]] float SpectrumPostProcessor::GetAmplification() const {
        return m_amplificationFactor;
    }

    [[nodiscard]] float SpectrumPostProcessor::GetSmoothing() const {
        return m_smoothingFactor;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void SpectrumPostProcessor::ApplyLogarithmicScaling(SpectrumData& spectrum) const {
        const float sensitivity = 150.0f;
        const float invLogSensitivity = 1.0f / std::log1p(sensitivity);

        for (float& val : spectrum)
            val = std::log1p(val * sensitivity) * invLogSensitivity;
    }

    void SpectrumPostProcessor::ApplyAmplification(SpectrumData& spectrum) const {
        for (float& val : spectrum)
            val = Utils::Saturate(std::pow(val, m_amplificationFactor));
    }

    void SpectrumPostProcessor::UpdateBarPeaks(const SpectrumData& spectrum) {
        for (size_t i = 0; i < m_barCount; ++i) {
            if (spectrum[i] > m_peakValues[i])
                UpdateSingleBarPeak(i, spectrum[i]);
            else
                ApplySingleBarPeakDecay(i);
        }
    }

    void SpectrumPostProcessor::UpdateSingleBarPeak(size_t index, float newValue) {
        m_peakValues[index] = newValue;
    }

    void SpectrumPostProcessor::ApplySingleBarPeakDecay(size_t index) {
        m_peakValues[index] *= kPeakDecayRate;
    }

    [[nodiscard]] float SpectrumPostProcessor::GetAdaptiveSmoothingFactor(
        float newValue,
        float oldValue
    ) const {
        const float attackSmoothingFactor = 0.5f;
        if (newValue > oldValue)
            return m_smoothingFactor * attackSmoothingFactor;

        return m_smoothingFactor;
    }

    void SpectrumPostProcessor::ApplySmoothing(const SpectrumData& spectrum) {
        for (size_t i = 0; i < m_barCount; ++i) {
            const float smoothing = GetAdaptiveSmoothingFactor(spectrum[i], m_smoothedBars[i]);
            m_smoothedBars[i] = m_smoothedBars[i] * smoothing + spectrum[i] * (1.0f - smoothing);
        }
    }

} // namespace Spectrum