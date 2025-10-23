#include "FrequencyMapper.h"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum {

    using namespace Helpers::Math;

    FrequencyMapper::FrequencyMapper(size_t barCount, size_t sampleRate)
        : m_barCount(barCount)
        , m_sampleRate(sampleRate)
        , m_nyquistFrequency(sampleRate * 0.5f)
        , m_currentFFTSize(0) {
    }

    void FrequencyMapper::SetBarCount(size_t newBarCount) {
        if (newBarCount > 0 && newBarCount != m_barCount) {
            m_barCount = newBarCount;
        }
    }

    void FrequencyMapper::SetSampleRate(size_t newSampleRate) {
        if (newSampleRate > 0 && newSampleRate != m_sampleRate) {
            m_sampleRate = newSampleRate;
            m_nyquistFrequency = newSampleRate * 0.5f;
        }
    }

    void FrequencyMapper::MapFFTToBars(
        const SpectrumData& fftMagnitudes,
        SpectrumData& outputBars,
        SpectrumScale scaleType
    ) {
        if (fftMagnitudes.empty() || outputBars.size() != m_barCount) {
            return;
        }

        // Store FFT size for frequency calculations
        m_currentFFTSize = (fftMagnitudes.size() - 1) * 2;

        switch (scaleType) {
        case SpectrumScale::Linear:
            MapLinearScale(fftMagnitudes, outputBars);
            break;
        case SpectrumScale::Logarithmic:
            MapLogarithmicScale(fftMagnitudes, outputBars);
            break;
        case SpectrumScale::Mel:
            MapMelScale(fftMagnitudes, outputBars);
            break;
        default:
            MapLinearScale(fftMagnitudes, outputBars);
            break;
        }
    }

    float FrequencyMapper::GetFrequencyForBin(size_t bin, size_t fftSize) const {
        if (fftSize == 0) return 0.0f;
        return (static_cast<float>(bin) * static_cast<float>(m_sampleRate)) /
            static_cast<float>(fftSize);
    }

    size_t FrequencyMapper::GetBinForFrequency(float frequency, size_t fftSize) const {
        if (fftSize == 0 || m_sampleRate == 0) return 0;

        const float n = static_cast<float>(fftSize);
        size_t bin = static_cast<size_t>((frequency * n) / static_cast<float>(m_sampleRate));
        return std::min(bin, fftSize / 2);
    }

    FrequencyMapper::FrequencyRange FrequencyMapper::GetLinearRange(size_t barIndex) const {
        FrequencyRange range;
        range.start = (barIndex * m_nyquistFrequency) / static_cast<float>(m_barCount);
        range.end = ((barIndex + 1) * m_nyquistFrequency) / static_cast<float>(m_barCount);
        return range;
    }

    FrequencyMapper::FrequencyRange FrequencyMapper::GetLogarithmicRange(size_t barIndex) const {
        const float minLog = std::log10(LOG_MIN_FREQ);
        const float maxLog = std::log10(m_nyquistFrequency);

        const float t0 = static_cast<float>(barIndex) / static_cast<float>(m_barCount);
        const float t1 = static_cast<float>(barIndex + 1) / static_cast<float>(m_barCount);

        FrequencyRange range;
        range.start = std::pow(10.0f, minLog + (maxLog - minLog) * t0);
        range.end = std::pow(10.0f, minLog + (maxLog - minLog) * t1);
        return range;
    }

    FrequencyMapper::FrequencyRange FrequencyMapper::GetMelRange(size_t barIndex) const {
        const float maxMel = FreqToMel(m_nyquistFrequency);

        const float melStart = (barIndex * maxMel) / static_cast<float>(m_barCount);
        const float melEnd = ((barIndex + 1) * maxMel) / static_cast<float>(m_barCount);

        FrequencyRange range;
        range.start = MelToFreq(melStart);
        range.end = MelToFreq(melEnd);
        return range;
    }

    bool FrequencyMapper::ValidateBinRange(size_t& startBin, size_t& endBin, size_t maxBin) const {
        if (maxBin < 2) return false;

        startBin = std::max<size_t>(1, std::min(startBin, maxBin - 1));
        endBin = std::min(endBin + 1, maxBin);

        return startBin < endBin;
    }

    float FrequencyMapper::AverageRange(const SpectrumData& data, size_t start, size_t end) const {
        float sum = 0.0f;
        for (size_t i = start; i < end; ++i) {
            sum += data[i];
        }
        return sum / static_cast<float>(end - start);
    }

    float FrequencyMapper::MaxInRange(const SpectrumData& data, size_t start, size_t end) const {
        float maxVal = 0.0f;
        for (size_t i = start; i < end; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
        return maxVal;
    }

    float FrequencyMapper::CalculateBarValue(
        const SpectrumData& magnitudes,
        size_t startBin,
        size_t endBin,
        AggregationFunc aggFunc
    ) const {
        size_t validStart = startBin;
        size_t validEnd = endBin;

        if (!ValidateBinRange(validStart, validEnd, magnitudes.size())) {
            return 0.0f;
        }

        const size_t clampedEnd = std::min(validEnd, magnitudes.size());
        if (validStart >= clampedEnd) return 0.0f;

        return (this->*aggFunc)(magnitudes, validStart, clampedEnd);
    }

    void FrequencyMapper::MapGenericScale(
        const SpectrumData& mags,
        SpectrumData& bars,
        RangeFunc getRange,
        AggregationFunc aggFunc
    ) {
        for (size_t i = 0; i < m_barCount; ++i) {
            auto range = (this->*getRange)(i);
            const size_t startBin = GetBinForFrequency(range.start, m_currentFFTSize);
            const size_t endBin = GetBinForFrequency(range.end, m_currentFFTSize);
            bars[i] = CalculateBarValue(mags, startBin, endBin, aggFunc);
        }
    }

    void FrequencyMapper::MapLinearScale(const SpectrumData& mags, SpectrumData& bars) {
        MapGenericScale(mags, bars, &FrequencyMapper::GetLinearRange, &FrequencyMapper::MaxInRange);
    }

    void FrequencyMapper::MapLogarithmicScale(const SpectrumData& mags, SpectrumData& bars) {
        MapGenericScale(mags, bars, &FrequencyMapper::GetLogarithmicRange, &FrequencyMapper::AverageRange);
    }

    void FrequencyMapper::MapMelScale(const SpectrumData& mags, SpectrumData& bars) {
        MapGenericScale(mags, bars, &FrequencyMapper::GetMelRange, &FrequencyMapper::MaxInRange);
    }

} // namespace Spectrum