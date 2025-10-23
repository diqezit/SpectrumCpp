// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the SpectrumPostProcessor, which applies final
// shaping and visual effects to the frequency spectrum. It coordinates
// gain normalization and then applies logarithmic scaling, user-defined
// amplification, smoothing, and peak detection.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_SPECTRUM_POST_PROCESSOR_H
#define SPECTRUM_CPP_SPECTRUM_POST_PROCESSOR_H

#include "Common/Common.h"
#include "GainNormalizer.h"
#include <memory>

namespace Spectrum {

    class SpectrumPostProcessor {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        explicit SpectrumPostProcessor(size_t barCount);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Main Execution Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void Process(SpectrumData& spectrum);
        void Reset();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Configuration & Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void SetBarCount(size_t newBarCount);
        void SetAmplification(float newAmplification);
        void SetSmoothing(float newSmoothing);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        [[nodiscard]] const SpectrumData& GetSmoothedBars() const;
        [[nodiscard]] const SpectrumData& GetPeakValues() const;
        [[nodiscard]] float GetAmplification() const;
        [[nodiscard]] float GetSmoothing() const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Processing Pipeline
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        [[nodiscard]] bool ValidateSpectrumSize(const SpectrumData& spectrum) const;
        void ApplyLogarithmicScaling(SpectrumData& spectrum) const;
        void ApplyAmplification(SpectrumData& spectrum) const;
        void UpdateBarPeaks(const SpectrumData& spectrum);
        void ApplySmoothing(const SpectrumData& spectrum);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Peak Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void UpdateSingleBarPeak(size_t index, float newValue);
        void ApplySingleBarPeakDecay(size_t index);
        [[nodiscard]] bool ShouldUpdatePeak(float newValue, float currentPeak) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Smoothing Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        [[nodiscard]] float GetAdaptiveSmoothingFactor(
            float newValue,
            float oldValue
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Bar Count Change Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        [[nodiscard]] bool ShouldChangeBarCount(size_t newBarCount) const;
        void PerformBarCountChange(size_t newBarCount);

        struct OldBarData {
            SpectrumData smoothedBars;
            SpectrumData peakValues;
            size_t barCount;
        };
        [[nodiscard]] OldBarData SaveCurrentBarData() const;
        void ResizeBarBuffers(size_t newBarCount);
        void RestoreInterpolatedData(const OldBarData& oldData);
        void LogBarCountChange(size_t oldCount, size_t newCount) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Interpolation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        static void InterpolateValues(
            const SpectrumData& source,
            SpectrumData& destination,
            size_t sourceCount,
            size_t destCount
        );
        [[nodiscard]] static bool CanInterpolate(
            const SpectrumData& source,
            size_t sourceCount,
            size_t destCount
        );
        [[nodiscard]] static float CalculateSourcePosition(
            size_t destIndex,
            size_t destCount,
            size_t sourceCount
        );
        [[nodiscard]] static float GetInterpolatedValue(
            const SpectrumData& source,
            float sourcePos,
            size_t sourceCount
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        static constexpr float kPeakDecayRate = 0.98f;
        static constexpr float kAttackSmoothingFactor = 0.5f;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        size_t m_barCount;
        float m_amplificationFactor;
        float m_smoothingFactor;

        std::unique_ptr<GainNormalizer> m_normalizer;

        SpectrumData m_smoothedBars;
        SpectrumData m_peakValues;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_SPECTRUM_POST_PROCESSOR_H