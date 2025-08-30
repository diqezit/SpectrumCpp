// AudioManager.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioManager.cpp: Implementation of the AudioManager class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "AudioManager.h"

namespace Spectrum {

    AudioManager::AudioManager() {}

    AudioManager::~AudioManager() {
        if (m_audioCapture && m_isCapturing) m_audioCapture->Stop();
    }

    bool AudioManager::Initialize() {
        m_audioCapture = std::make_unique<AudioCapture>();
        m_audioCapture->SetCallback(this);
        if (!m_audioCapture->Initialize()) {
            LOG_ERROR("Warning: Failed to initialize audio capture. Will run in test mode.");
        }

        m_analyzer = std::make_unique<SpectrumAnalyzer>(
            m_audioConfig.barCount, m_audioConfig.fftSize
        );
        m_analyzer->SetAmplification(m_audioConfig.amplification);
        m_analyzer->SetFFTWindow(m_audioConfig.windowType);
        m_analyzer->SetScaleType(m_audioConfig.scaleType);

        return true;
    }

    void AudioManager::Update(float deltaTime) {
        if (m_isAnimating) {
            m_animationTime += deltaTime;
            m_analyzer->GenerateTestData(m_animationTime);
        }
    }

    SpectrumData AudioManager::GetSpectrum() {
        std::lock_guard<std::mutex> lock(m_spectrumMutex);
        return m_analyzer->GetSpectrum();
    }

    void AudioManager::OnAudioData(const float* data, size_t samples, int channels) {
        if (!m_analyzer || !data || samples == 0) return;
        std::lock_guard<std::mutex> lock(m_spectrumMutex);
        m_analyzer->ProcessAudioData(data, samples, channels);
    }

    void AudioManager::ToggleCapture() {
        if (m_isCapturing) StopCaptureInternal();
        else StartCaptureInternal();
    }

    bool AudioManager::StartCaptureInternal() {
        if (!m_audioCapture || !m_audioCapture->IsInitialized()) {
            LOG_ERROR("Audio capture not available.");
            return false;
        }
        if (!m_audioCapture->Start()) {
            LOG_ERROR("Failed to start audio capture.");
            return false;
        }
        m_isCapturing = true;
        m_isAnimating = false;
        return true;
    }

    void AudioManager::StopCaptureInternal() {
        if (m_audioCapture) m_audioCapture->Stop();
        m_isCapturing = false;
    }

    void AudioManager::ToggleAnimation() {
        m_isAnimating = !m_isAnimating;

        if (m_isAnimating) {
            if (m_isCapturing) {
                if (m_audioCapture) m_audioCapture->Stop();
                m_isCapturing = false;
            }
            LOG_INFO("Animation mode ON");
        }
        else {
            LOG_INFO("Animation mode OFF");
        }
    }

    void AudioManager::ChangeAmplification(float delta) {
        m_audioConfig.amplification += delta;
        m_analyzer->SetAmplification(m_audioConfig.amplification);
        LOG_INFO("Amplification Factor: " << m_analyzer->GetAmplification());
    }

    void AudioManager::ChangeBarCount(int delta) {
        int newCount = static_cast<int>(m_audioConfig.barCount) + delta;
        m_audioConfig.barCount = Utils::Clamp(
            static_cast<size_t>(newCount), size_t(16), size_t(256)
        );
        m_analyzer->SetBarCount(m_audioConfig.barCount);
        LOG_INFO("Bar Count: " << m_audioConfig.barCount);
    }

    std::string_view AudioManager::GetWindowTypeName(FFTWindowType type) const {
        switch (type) {
        case FFTWindowType::Hann:        return "Hann";
        case FFTWindowType::Hamming:     return "Hamming";
        case FFTWindowType::Blackman:    return "Blackman";
        case FFTWindowType::Rectangular: return "Rectangular";
        default:                         return "Unknown";
        }
    }

    void AudioManager::ChangeFFTWindow(int direction) {
        int cur = static_cast<int>(m_audioConfig.windowType);
        const int cnt = static_cast<int>(FFTWindowType::Count);
        cur = (cur + direction + cnt) % cnt;
        m_audioConfig.windowType = static_cast<FFTWindowType>(cur);
        m_analyzer->SetFFTWindow(m_audioConfig.windowType);
        LOG_INFO("FFT Window: " << GetWindowTypeName(m_audioConfig.windowType));
    }

    std::string_view AudioManager::GetScaleTypeName(SpectrumScale type) const {
        switch (type) {
        case SpectrumScale::Linear:      return "Linear";
        case SpectrumScale::Logarithmic: return "Logarithmic";
        case SpectrumScale::Mel:         return "Mel";
        default:                         return "Unknown";
        }
    }

    void AudioManager::ChangeSpectrumScale(int direction) {
        int cur = static_cast<int>(m_audioConfig.scaleType);
        const int cnt = static_cast<int>(SpectrumScale::Count);
        cur = (cur + direction + cnt) % cnt;
        m_audioConfig.scaleType = static_cast<SpectrumScale>(cur);
        m_analyzer->SetScaleType(m_audioConfig.scaleType);
        LOG_INFO("Spectrum Scale: " << GetScaleTypeName(m_audioConfig.scaleType));
    }

} // namespace Spectrum