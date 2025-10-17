// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// FrequencyMapper.h: Maps FFT bins to frequency bars using different scaling modes.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_FREQUENCY_MAPPER_H
#define SPECTRUM_CPP_FREQUENCY_MAPPER_H

#include "Common/Common.h"
#include "FFTProcessor.h"

namespace Spectrum {

    class FrequencyMapper {
    public:
        FrequencyMapper(size_t barCount, size_t sampleRate);
        ~FrequencyMapper() = default;

        // Main mapping function
        void MapFFTToBars(
            const SpectrumData& fftMagnitudes,
            SpectrumData& outputBars,
            SpectrumScale scaleType
        );

        // Configuration
        void SetBarCount(size_t newBarCount);
        void SetSampleRate(size_t newSampleRate);

        // Frequency calculations
        float GetFrequencyForBin(size_t bin, size_t fftSize) const;
        size_t GetBinForFrequency(float frequency, size_t fftSize) const;

        // Getters
        size_t GetBarCount() const noexcept { return m_barCount; }
        float GetNyquistFrequency() const noexcept { return m_nyquistFrequency; }

    private:
        // Helper types for function pointers
        struct FrequencyRange {
            float start = 0.0f;
            float end = 0.0f;
        };
        using RangeFunc = FrequencyRange(FrequencyMapper::*)(size_t) const;
        using AggregationFunc = float (FrequencyMapper::*)(const SpectrumData&, size_t, size_t) const;

        // Generic mapping function
        void MapGenericScale(
            const SpectrumData& mags,
            SpectrumData& bars,
            RangeFunc getRange,
            AggregationFunc aggFunc
        );

        // Frequency range calculators
        FrequencyRange GetLinearRange(size_t barIndex) const;
        FrequencyRange GetLogarithmicRange(size_t barIndex) const;
        FrequencyRange GetMelRange(size_t barIndex) const;

        // Mapping implementations
        void MapLinearScale(const SpectrumData& mags, SpectrumData& bars);
        void MapLogarithmicScale(const SpectrumData& mags, SpectrumData& bars);
        void MapMelScale(const SpectrumData& mags, SpectrumData& bars);

        // Bar value calculation
        float CalculateBarValue(
            const SpectrumData& magnitudes,
            size_t startBin,
            size_t endBin,
            AggregationFunc aggFunc
        ) const;

        // Aggregation methods
        float AverageRange(const SpectrumData& data, size_t start, size_t end) const;
        float MaxInRange(const SpectrumData& data, size_t start, size_t end) const;

        // Helper methods
        bool ValidateBinRange(size_t& startBin, size_t& endBin, size_t maxBin) const;

    private:
        static constexpr float LOG_MIN_FREQ = 20.0f;
        static constexpr float LOG_MAX_FREQ = 20000.0f;

        size_t m_barCount;
        size_t m_sampleRate;
        float m_nyquistFrequency;
        size_t m_currentFFTSize;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_FREQUENCY_MAPPER_H