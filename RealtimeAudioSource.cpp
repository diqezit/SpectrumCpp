// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// RealtimeAudioSource.cpp: Implementation of the live audio source.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "RealtimeAudioSource.h"

namespace Spectrum {

    RealtimeAudioSource::RealtimeAudioSource(const AudioConfig& config) : m_config(config) {
        m_analyzer = std::make_unique<SpectrumAnalyzer>(m_config.barCount, m_config.fftSize);
        ConfigureAnalyzer();
    }

    void RealtimeAudioSource::ConfigureAnalyzer() {
        m_analyzer->SetAmplification(m_config.amplification);
        m_analyzer->SetSmoothing(m_config.smoothing);
        m_analyzer->SetFFTWindow(m_config.windowType);
        m_analyzer->SetScaleType(m_config.scaleType);
    }

    bool RealtimeAudioSource::Initialize() {
        ReinitializeCapture();
        return m_audioCapture != nullptr;
    }

    void RealtimeAudioSource::HandleCaptureFaults() {
        if (m_isCapturing && m_audioCapture && m_audioCapture->IsFaulted()) {
            LOG_ERROR("Realtime source detected a fault. Capture stopped.");
            StopCapture();
        }
    }

    void RealtimeAudioSource::Update(float /*deltaTime*/) {
        HandleCaptureFaults();
        m_analyzer->Update();
    }

    SpectrumData RealtimeAudioSource::GetSpectrum() {
        return m_analyzer->GetSpectrum();
    }

    bool RealtimeAudioSource::EnsureCaptureIsReady() {
        if (!m_audioCapture || m_audioCapture->IsFaulted()) {
            LOG_INFO("Audio device is in a faulted state. Attempting to recover...");
            ReinitializeCapture();
        }
        return m_audioCapture != nullptr;
    }

    void RealtimeAudioSource::StartCapture() {
        if (m_isCapturing) return;

        if (EnsureCaptureIsReady() && m_audioCapture->Start()) {
            m_isCapturing = true;
            LOG_INFO("Realtime source: capture started.");
        }
        else {
            LOG_ERROR("Failed to start audio capture. Device may be unavailable.");
        }
    }

    void RealtimeAudioSource::StopCapture() {
        if (m_audioCapture) {
            m_audioCapture->Stop();
        }
        if (m_isCapturing) {
            m_isCapturing = false;
            LOG_INFO("Realtime source: capture stopped.");
        }
    }

    bool RealtimeAudioSource::TryCreateCaptureDevice() {
        m_audioCapture = std::make_unique<AudioCapture>();
        if (!m_audioCapture->Initialize()) {
            m_audioCapture = nullptr;
            LOG_ERROR("Failed to initialize audio capture device.");
            return false;
        }
        return true;
    }

    void RealtimeAudioSource::SetupNewCaptureDevice() {
        m_audioCapture->SetCallback(m_analyzer.get());
        LOG_INFO("Audio capture device initialized successfully.");
    }

    void RealtimeAudioSource::ReinitializeCapture() {
        if (TryCreateCaptureDevice()) {
            SetupNewCaptureDevice();
        }
    }

    void RealtimeAudioSource::SetAmplification(float amp) { m_analyzer->SetAmplification(amp); }
    void RealtimeAudioSource::SetBarCount(size_t count) { m_analyzer->SetBarCount(count); }
    void RealtimeAudioSource::SetFFTWindow(FFTWindowType type) { m_analyzer->SetFFTWindow(type); }
    void RealtimeAudioSource::SetScaleType(SpectrumScale type) { m_analyzer->SetScaleType(type); }

}