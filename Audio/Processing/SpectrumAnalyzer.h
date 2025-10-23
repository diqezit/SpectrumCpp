// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// SpectrumAnalyzer.h: Analyzes audio data to produce a frequency spectrum.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifndef SPECTRUM_CPP_SPECTRUM_ANALYZER_H
#define SPECTRUM_CPP_SPECTRUM_ANALYZER_H

#include "Common/Common.h"
#include "Audio/Capture/AudioCapture.h"
#include "AudioBuffer.h"
#include "FFTProcessor.h"
#include "FrequencyMapper.h"
#include "SpectrumPostProcessor.h"

namespace Spectrum {

    class SpectrumAnalyzer : public IAudioCaptureCallback {
    public:
        SpectrumAnalyzer(size_t barCount = DEFAULT_BAR_COUNT, size_t fftSize = DEFAULT_FFT_SIZE);

        void OnAudioData(const float* data, size_t samples, int channels) override;
        void Update();

        void SetBarCount(size_t newBarCount);
        void SetAmplification(float newAmplification);
        void SetSmoothing(float newSmoothing);
        void SetFFTWindow(FFTWindowType windowType);
        void SetScaleType(SpectrumScale scaleType);

        SpectrumData GetSpectrum();
        const SpectrumData& GetPeakValues() const;
        size_t GetBarCount() const;
        float GetAmplification() const;
        float GetSmoothing() const;
        SpectrumScale GetScaleType() const;

    private:
        // Processing pipeline
        void ProcessSingleFFTChunk();
        void CopyChunkToProcessBuffer();
        void ExecuteFFT();
        void MapMagnitudesToBars(SpectrumData& outBars);
        void ApplyPostProcessing(SpectrumData& bars);

        // Helpers
        bool ValidateAudioInput(const float* data, size_t samples, int channels, size_t& outFrames) const;

        size_t m_barCount;
        SpectrumScale m_scaleType;
        size_t m_sampleRate;

        FFTProcessor m_fftProcessor;
        FrequencyMapper m_frequencyMapper;
        SpectrumPostProcessor m_postProcessor;
        ThreadSafeAudioBuffer m_bufferManager;

        AudioBuffer m_processBuffer;
        std::mutex m_mutex;
    };

}

#endif