// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the SpectrumPostProcessor. It orchestrates the
// post-processing pipeline by first normalizing the signal's gain and
// then applying signal shaping and visual effects.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "SpectrumPostProcessor.h"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum {

    using namespace Helpers::Math;

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
        if (!ValidateSpectrumSize(spectrum)) return;

        // Pipeline: Normalize -> Shape -> Apply Visual Effects
        m_normalizer->Process(spectrum);
        ApplyLogarithmicScaling(spectrum);
        ApplyAmplification(spectrum);
        UpdateBarPeaks(spectrum);
        ApplySmoothing(spectrum);
    }

    [[nodiscard]] bool SpectrumPostProcessor::ValidateSpectrumSize(
        const SpectrumData& spectrum
    ) const {
        return spectrum.size() == m_barCount;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void SpectrumPostProcessor::SetBarCount(size_t newBarCount) {
        if (ShouldChangeBarCount(newBarCount)) {
            PerformBarCountChange(newBarCount);
        }
    }

    void SpectrumPostProcessor::SetAmplification(float newAmplification) {
        m_amplificationFactor = Clamp(newAmplification, 0.1f, 5.0f);
    }

    void SpectrumPostProcessor::SetSmoothing(float newSmoothing) {
        m_smoothingFactor = Saturate(newSmoothing);
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
    // Processing Pipeline
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void SpectrumPostProcessor::ApplyLogarithmicScaling(SpectrumData& spectrum) const {
        const float sensitivity = 150.0f;
        const float invLogSensitivity = 1.0f / std::log1p(sensitivity);

        for (float& val : spectrum)
            val = std::log1p(val * sensitivity) * invLogSensitivity;
    }

    void SpectrumPostProcessor::ApplyAmplification(SpectrumData& spectrum) const {
        for (float& val : spectrum)
            val = Saturate(std::pow(val, m_amplificationFactor));
    }

    void SpectrumPostProcessor::UpdateBarPeaks(const SpectrumData& spectrum) {
        for (size_t i = 0; i < m_barCount; ++i) {
            if (ShouldUpdatePeak(spectrum[i], m_peakValues[i]))
                UpdateSingleBarPeak(i, spectrum[i]);
            else
                ApplySingleBarPeakDecay(i);
        }
    }

    void SpectrumPostProcessor::ApplySmoothing(const SpectrumData& spectrum) {
        for (size_t i = 0; i < m_barCount; ++i) {
            const float smoothing = GetAdaptiveSmoothingFactor(
                spectrum[i],
                m_smoothedBars[i]
            );
            // Use Lerp from MathHelpers (DRY principle)
            m_smoothedBars[i] = Lerp(spectrum[i], m_smoothedBars[i], smoothing);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Peak Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] bool SpectrumPostProcessor::ShouldUpdatePeak(
        float newValue,
        float currentPeak
    ) const {
        return newValue > currentPeak;
    }

    void SpectrumPostProcessor::UpdateSingleBarPeak(size_t index, float newValue) {
        m_peakValues[index] = newValue;
    }

    void SpectrumPostProcessor::ApplySingleBarPeakDecay(size_t index) {
        m_peakValues[index] *= kPeakDecayRate;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Smoothing Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] float SpectrumPostProcessor::GetAdaptiveSmoothingFactor(
        float newValue,
        float oldValue
    ) const {
        if (newValue > oldValue)
            return m_smoothingFactor * kAttackSmoothingFactor;

        return m_smoothingFactor;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Bar Count Change Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] bool SpectrumPostProcessor::ShouldChangeBarCount(
        size_t newBarCount
    ) const {
        return newBarCount > 0 && newBarCount != m_barCount;
    }

    void SpectrumPostProcessor::PerformBarCountChange(size_t newBarCount) {
        const OldBarData oldData = SaveCurrentBarData();
        ResizeBarBuffers(newBarCount);
        RestoreInterpolatedData(oldData);
        LogBarCountChange(oldData.barCount, newBarCount);
    }

    [[nodiscard]] SpectrumPostProcessor::OldBarData
        SpectrumPostProcessor::SaveCurrentBarData() const {
        return {
            m_smoothedBars,
            m_peakValues,
            m_barCount
        };
    }

    void SpectrumPostProcessor::ResizeBarBuffers(size_t newBarCount) {
        m_barCount = newBarCount;
        m_smoothedBars.resize(newBarCount, 0.0f);
        m_peakValues.resize(newBarCount, 0.0f);
    }

    void SpectrumPostProcessor::RestoreInterpolatedData(const OldBarData& oldData) {
        if (oldData.barCount > 0 && !oldData.smoothedBars.empty()) {
            InterpolateValues(
                oldData.smoothedBars,
                m_smoothedBars,
                oldData.barCount,
                m_barCount
            );
            InterpolateValues(
                oldData.peakValues,
                m_peakValues,
                oldData.barCount,
                m_barCount
            );
        }
    }

    void SpectrumPostProcessor::LogBarCountChange(
        size_t oldCount,
        size_t newCount
    ) const {
        LOG_INFO("SpectrumPostProcessor: Bar count changed "
            << oldCount << " -> " << newCount
            << " (values interpolated)");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Interpolation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void SpectrumPostProcessor::InterpolateValues(
        const SpectrumData& source,
        SpectrumData& destination,
        size_t sourceCount,
        size_t destCount
    ) {
        if (!CanInterpolate(source, sourceCount, destCount)) return;

        for (size_t i = 0; i < destCount; ++i) {
            const float sourcePos = CalculateSourcePosition(i, destCount, sourceCount);
            destination[i] = GetInterpolatedValue(source, sourcePos, sourceCount);
        }
    }

    [[nodiscard]] bool SpectrumPostProcessor::CanInterpolate(
        const SpectrumData& source,
        size_t sourceCount,
        size_t destCount
    ) {
        return sourceCount > 0 && destCount > 0 && !source.empty();
    }

    [[nodiscard]] float SpectrumPostProcessor::CalculateSourcePosition(
        size_t destIndex,
        size_t destCount,
        size_t sourceCount
    ) {
        return (static_cast<float>(destIndex) / destCount) * sourceCount;
    }

    [[nodiscard]] float SpectrumPostProcessor::GetInterpolatedValue(
        const SpectrumData& source,
        float sourcePos,
        size_t sourceCount
    ) {
        const size_t sourceIndex = static_cast<size_t>(sourcePos);
        const float fraction = sourcePos - sourceIndex;

        // Linear interpolation using Lerp from MathHelpers (DRY principle)
        if (sourceIndex < sourceCount - 1) {
            return Lerp(source[sourceIndex], source[sourceIndex + 1], fraction);
        }

        // Last element without interpolation
        if (sourceIndex < sourceCount) {
            return source[sourceIndex];
        }

        return 0.0f;
    }

} // namespace Spectrum