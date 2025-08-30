// FrequencyMapper.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// FrequencyMapper.h: Maps FFT bins to frequency bars using different scaling modes.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_FREQUENCY_MAPPER_H
#define SPECTRUM_CPP_FREQUENCY_MAPPER_H

#include "Common.h"
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
        // Frequency range calculators
        struct FrequencyRange {
            float start;
            float end;
        };

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
            bool useAverage
        ) const;

        // Helper methods
        bool ValidateBinRange(size_t& startBin, size_t& endBin, size_t maxBin) const;
        float AggregateValues(
            const SpectrumData& data,
            size_t start,
            size_t end,
            bool useAverage
        ) const;

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