// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// SpectrumAnalyzer.cpp: Analyzes audio data to produce a frequency spectrum.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "SpectrumAnalyzer.h"

namespace Spectrum {

    SpectrumAnalyzer::SpectrumAnalyzer(size_t barCount, size_t fftSize)
        : m_barCount(barCount),
        m_scaleType(SpectrumScale::Logarithmic),
        m_sampleRate(DEFAULT_SAMPLE_RATE),
        m_fftProcessor(fftSize),
        m_frequencyMapper(barCount, DEFAULT_SAMPLE_RATE),
        m_postProcessor(barCount) {
        m_processBuffer.resize(fftSize);
    }

    bool SpectrumAnalyzer::ValidateAudioInput(
        const float* data,
        size_t samples,
        int channels,
        size_t& outFrames
    ) const {
        if (!data || samples == 0 || channels <= 0) return false;
        outFrames = samples / static_cast<size_t>(channels);
        return outFrames > 0;
    }

    void SpectrumAnalyzer::OnAudioData(
        const float* data,
        size_t samples,
        int channels
    ) {
        size_t frames = 0;
        if (!ValidateAudioInput(data, samples, channels, frames)) return;
        m_bufferManager.Add(data, frames, channels);
    }

    void SpectrumAnalyzer::Update() {
        const size_t fftSize = m_fftProcessor.GetFFTSize();
        const size_t hopSize = fftSize / 2;

        while (m_bufferManager.HasEnoughData(fftSize)) {
            ProcessSingleFFTChunk();
            m_bufferManager.Consume(hopSize);
        }
    }

    void SpectrumAnalyzer::CopyChunkToProcessBuffer() {
        m_bufferManager.CopyTo(m_processBuffer, m_fftProcessor.GetFFTSize());
    }

    void SpectrumAnalyzer::ExecuteFFT() {
        m_fftProcessor.Process(m_processBuffer);
    }

    void SpectrumAnalyzer::MapMagnitudesToBars(SpectrumData& outBars) {
        m_frequencyMapper.MapFFTToBars(
            m_fftProcessor.GetMagnitudes(),
            outBars,
            m_scaleType
        );
    }

    void SpectrumAnalyzer::ApplyPostProcessing(SpectrumData& bars) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_postProcessor.Process(bars);
    }

    void SpectrumAnalyzer::ProcessSingleFFTChunk() {
        CopyChunkToProcessBuffer();
        ExecuteFFT();

        SpectrumData currentBars(m_barCount, 0.0f);
        MapMagnitudesToBars(currentBars);

        ApplyPostProcessing(currentBars);
    }

    SpectrumData SpectrumAnalyzer::GetSpectrum() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_postProcessor.GetSmoothedBars();
    }

    void SpectrumAnalyzer::SetBarCount(size_t newBarCount) {
        if (newBarCount == 0 || newBarCount == m_barCount) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        m_barCount = newBarCount;
        m_frequencyMapper.SetBarCount(newBarCount);
        m_postProcessor.SetBarCount(newBarCount);
    }

    void SpectrumAnalyzer::SetAmplification(float newAmplification) {
        m_postProcessor.SetAmplification(newAmplification);
    }

    void SpectrumAnalyzer::SetSmoothing(float newSmoothing) {
        m_postProcessor.SetSmoothing(newSmoothing);
    }

    void SpectrumAnalyzer::SetFFTWindow(FFTWindowType windowType) {
        m_fftProcessor.SetWindowType(windowType);
    }

    void SpectrumAnalyzer::SetScaleType(SpectrumScale scaleType) {
        m_scaleType = scaleType;
    }

    const SpectrumData& SpectrumAnalyzer::GetPeakValues() const {
        return m_postProcessor.GetPeakValues();
    }
    size_t SpectrumAnalyzer::GetBarCount() const { return m_barCount; }
    float SpectrumAnalyzer::GetAmplification() const {
        return m_postProcessor.GetAmplification();
    }
    float SpectrumAnalyzer::GetSmoothing() const {
        return m_postProcessor.GetSmoothing();
    }
    SpectrumScale SpectrumAnalyzer::GetScaleType() const { return m_scaleType; }

}