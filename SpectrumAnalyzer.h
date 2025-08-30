// SpectrumAnalyzer.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// SpectrumAnalyzer.h: Analyzes audio stream and prepares data for visualization.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_SPECTRUM_ANALYZER_H
#define SPECTRUM_CPP_SPECTRUM_ANALYZER_H

#include "Common.h"
#include "FFTProcessor.h"
#include "FrequencyMapper.h"
#include "SpectrumProcessor.h"

namespace Spectrum {

    class SpectrumAnalyzer {
    public:
        SpectrumAnalyzer(size_t barCount = DEFAULT_BAR_COUNT,
            size_t fftSize = DEFAULT_FFT_SIZE);
        ~SpectrumAnalyzer() = default;

        // Main processing
        void ProcessAudioData(const float* data, size_t samples, int channels);
        void GenerateTestData(float timeOffset);

        // Configuration
        void SetBarCount(size_t newBarCount);
        void SetAmplification(float newAmplification);
        void SetSmoothing(float newSmoothing);
        void SetFFTWindow(FFTWindowType windowType);
        void SetScaleType(SpectrumScale scaleType);

        // Getters
        const SpectrumData& GetSpectrum() const noexcept { return m_spectrumBars; }
        const SpectrumData& GetPeakValues() const noexcept { return m_spectrumProcessor.GetPeakValues(); }
        size_t GetBarCount() const noexcept { return m_barCount; }
        float GetAmplification() const noexcept { return m_spectrumProcessor.GetAmplification(); }
        float GetSmoothing() const noexcept { return m_spectrumProcessor.GetSmoothing(); }
        SpectrumScale GetScaleType() const noexcept { return m_scaleType; }

    private:
        // Audio processing pipeline
        void PrepareMonoAudio(const float* data, size_t frames, int channels);
        void ProcessFFTChunks();
        void ProcessSingleFFTChunk();
        void MapAndProcessSpectrum();

        // Buffer management
        void EnsureBufferSizes();
        void ConsumeProcessedAudio(size_t hopSize);

    private:
        // Configuration
        size_t        m_barCount;
        SpectrumScale m_scaleType;
        size_t        m_sampleRate;

        // Processing components
        FFTProcessor      m_fftProcessor;
        FrequencyMapper   m_frequencyMapper;
        SpectrumProcessor m_spectrumProcessor;

        // Audio buffers
        AudioBuffer  m_monoBuffer;
        AudioBuffer  m_processBuffer;

        // Output data
        SpectrumData m_spectrumBars;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_SPECTRUM_ANALYZER_H