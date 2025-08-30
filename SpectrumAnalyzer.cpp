// SpectrumAnalyzer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// SpectrumAnalyzer.cpp: Implementation of the SpectrumAnalyzer class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "SpectrumAnalyzer.h"
#include "Utils.h"

namespace Spectrum {

    SpectrumAnalyzer::SpectrumAnalyzer(size_t barCount, size_t fftSize)
        : m_barCount(barCount)
        , m_scaleType(SpectrumScale::Logarithmic)
        , m_sampleRate(DEFAULT_SAMPLE_RATE)
        , m_fftProcessor(fftSize)
        , m_frequencyMapper(barCount, DEFAULT_SAMPLE_RATE)
        , m_spectrumProcessor(barCount) {

        m_spectrumBars.assign(barCount, 0.0f);
        m_processBuffer.resize(fftSize);
    }

    void SpectrumAnalyzer::ProcessAudioData(
        const float* data,
        size_t samples,
        int channels
    ) {
        if (!data || samples == 0 || channels <= 0) return;

        const size_t frames = samples / static_cast<size_t>(channels);
        if (frames == 0) return;

        PrepareMonoAudio(data, frames, channels);
        ProcessFFTChunks();
    }

    void SpectrumAnalyzer::PrepareMonoAudio(
        const float* data,
        size_t frames,
        int channels
    ) {
        m_monoBuffer.reserve(m_monoBuffer.size() + frames);

        const float invChannels = 1.0f / static_cast<float>(channels);

        for (size_t frame = 0; frame < frames; ++frame) {
            const float* frameData = data + frame * static_cast<size_t>(channels);

            float monoSample = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                monoSample += frameData[ch];
            }

            m_monoBuffer.push_back(monoSample * invChannels);
        }
    }

    void SpectrumAnalyzer::EnsureBufferSizes() {
        const size_t fftSize = m_fftProcessor.GetFFTSize();
        if (m_processBuffer.size() != fftSize) {
            m_processBuffer.resize(fftSize);
        }
    }

    void SpectrumAnalyzer::ProcessFFTChunks() {
        const size_t fftSize = m_fftProcessor.GetFFTSize();
        const size_t hopSize = fftSize / 2; // 50% overlap

        EnsureBufferSizes();

        while (m_monoBuffer.size() >= fftSize) {
            ProcessSingleFFTChunk();
            ConsumeProcessedAudio(hopSize);
        }
    }

    void SpectrumAnalyzer::ProcessSingleFFTChunk() {
        const size_t fftSize = m_fftProcessor.GetFFTSize();

        // Copy data to process buffer
        std::copy_n(m_monoBuffer.begin(), fftSize, m_processBuffer.begin());

        // Perform FFT
        m_fftProcessor.Process(m_processBuffer);

        // Map and process spectrum
        MapAndProcessSpectrum();
    }

    void SpectrumAnalyzer::MapAndProcessSpectrum() {
        // Map FFT magnitudes to frequency bars
        m_frequencyMapper.MapFFTToBars(
            m_fftProcessor.GetMagnitudes(),
            m_spectrumBars,
            m_scaleType
        );

        // Apply scaling, smoothing, and peak detection
        m_spectrumProcessor.ProcessSpectrum(m_spectrumBars);
    }

    void SpectrumAnalyzer::ConsumeProcessedAudio(size_t hopSize) {
        m_monoBuffer.erase(
            m_monoBuffer.begin(),
            m_monoBuffer.begin() + hopSize
        );
    }

    void SpectrumAnalyzer::GenerateTestData(float timeOffset) {
        for (size_t i = 0; i < m_barCount; ++i) {
            const float frequency = static_cast<float>(i) / static_cast<float>(m_barCount);
            const float phase = timeOffset * 2.0f + static_cast<float>(i) * 0.3f;

            float value = (std::sin(phase) + 1.0f) * 0.5f;
            value *= (1.0f - frequency * 0.7f);
            value += Utils::Random::Instance().Float(-0.05f, 0.05f);

            m_spectrumBars[i] = Utils::Clamp(value, 0.0f, 1.0f);
        }

        m_spectrumProcessor.ProcessSpectrum(m_spectrumBars);
    }

    void SpectrumAnalyzer::SetBarCount(size_t newBarCount) {
        if (newBarCount == 0 || newBarCount == m_barCount) return;

        m_barCount = newBarCount;
        m_spectrumBars.assign(newBarCount, 0.0f);

        m_frequencyMapper.SetBarCount(newBarCount);
        m_spectrumProcessor.SetBarCount(newBarCount);
    }

    void SpectrumAnalyzer::SetAmplification(float newAmplification) {
        m_spectrumProcessor.SetAmplification(newAmplification);
    }

    void SpectrumAnalyzer::SetSmoothing(float newSmoothing) {
        m_spectrumProcessor.SetSmoothing(newSmoothing);
    }

    void SpectrumAnalyzer::SetFFTWindow(FFTWindowType windowType) {
        m_fftProcessor.SetWindowType(windowType);
    }

    void SpectrumAnalyzer::SetScaleType(SpectrumScale scaleType) {
        m_scaleType = scaleType;
    }

} // namespace Spectrum