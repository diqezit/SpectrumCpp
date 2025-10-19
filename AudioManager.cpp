// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Implements the AudioManager, which creates and manages audio sources,
// and responds to user input to change audio processing parameters.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "AudioManager.h"
#include "EventBus.h"
#include "TemplateUtils.h"
#include "MathUtils.h"
#include "StringUtils.h"
#include "RealtimeAudioSource.h"
#include "AnimatedAudioSource.h"

namespace Spectrum {

    AudioManager::AudioManager(EventBus* bus) {
        SubscribeToEvents(bus);
    }

    AudioManager::~AudioManager() {
        if (m_isCapturing && m_realtimeSource) {
            m_realtimeSource->StopCapture();
        }
    }

    void AudioManager::SubscribeToEvents(EventBus* bus) {
        bus->Subscribe(InputAction::ToggleCapture, [this]() { this->ToggleCapture(); });
        bus->Subscribe(InputAction::ToggleAnimation, [this]() { this->ToggleAnimation(); });
        bus->Subscribe(InputAction::CycleSpectrumScale, [this]() { this->ChangeSpectrumScale(1); });
        bus->Subscribe(InputAction::IncreaseAmplification, [this]() { this->ChangeAmplification(0.1f); });
        bus->Subscribe(InputAction::DecreaseAmplification, [this]() { this->ChangeAmplification(-0.1f); });
        bus->Subscribe(InputAction::NextFFTWindow, [this]() { this->ChangeFFTWindow(1); });
        bus->Subscribe(InputAction::PrevFFTWindow, [this]() { this->ChangeFFTWindow(-1); });
        bus->Subscribe(InputAction::IncreaseBarCount, [this]() { this->ChangeBarCount(4); });
        bus->Subscribe(InputAction::DecreaseBarCount, [this]() { this->ChangeBarCount(-4); });
    }

    bool AudioManager::CreateAudioSources() {
        m_realtimeSource = std::make_unique<RealtimeAudioSource>(m_audioConfig);
        m_animatedSource = std::make_unique<AnimatedAudioSource>(m_audioConfig);
        return m_realtimeSource->Initialize() && m_animatedSource->Initialize();
    }

    bool AudioManager::Initialize() {
        if (!CreateAudioSources()) return false;
        SetCurrentSource(m_realtimeSource.get());
        return true;
    }

    void AudioManager::Update(float deltaTime) {
        if (m_currentSource) {
            m_currentSource->Update(deltaTime);
        }
    }

    SpectrumData AudioManager::GetSpectrum() {
        if (m_currentSource) {
            return m_currentSource->GetSpectrum();
        }
        return {};
    }

    void AudioManager::SetCurrentSource(IAudioSource* source) {
        m_currentSource = source;
    }

    void AudioManager::StartRealtimeCapture() {
        m_isCapturing = true;
        m_realtimeSource->StartCapture();
    }

    void AudioManager::StopRealtimeCapture() {
        m_isCapturing = false;
        m_realtimeSource->StopCapture();
    }

    void AudioManager::ToggleCapture() {
        if (m_isAnimating) return;

        if (m_isCapturing) {
            StopRealtimeCapture();
        }
        else {
            StartRealtimeCapture();
        }
    }

    void AudioManager::ActivateAnimatedMode() {
        if (m_isCapturing) StopRealtimeCapture();
        SetCurrentSource(m_animatedSource.get());
        LOG_INFO("Animation mode ON.");
    }

    void AudioManager::DeactivateAnimatedMode() {
        SetCurrentSource(m_realtimeSource.get());
        LOG_INFO("Animation mode OFF.");
    }

    void AudioManager::ToggleAnimation() {
        m_isAnimating = !m_isAnimating;
        if (m_isAnimating) {
            ActivateAnimatedMode();
        }
        else {
            DeactivateAnimatedMode();
        }
    }

    void AudioManager::ApplyAmplificationChange(float newValue) {
        m_audioConfig.amplification = newValue;
        if (m_realtimeSource) m_realtimeSource->SetAmplification(m_audioConfig.amplification);
        LOG_INFO("Amplification Factor: " << m_audioConfig.amplification);
    }

    void AudioManager::ChangeAmplification(float delta) {
        float newValue = Utils::Clamp(m_audioConfig.amplification + delta, 0.1f, 5.0f);
        ApplyAmplificationChange(newValue);
    }

    void AudioManager::ApplyBarCountChange(size_t newCount) {
        m_audioConfig.barCount = newCount;
        if (m_realtimeSource) m_realtimeSource->SetBarCount(m_audioConfig.barCount);
        if (m_animatedSource) m_animatedSource->SetBarCount(m_audioConfig.barCount);
        LOG_INFO("Bar Count: " << m_audioConfig.barCount);
    }

    void AudioManager::ChangeBarCount(int delta) {
        int newCountInt = static_cast<int>(m_audioConfig.barCount) + delta;
        size_t newCount = Utils::Clamp<size_t>(newCountInt, 16, 256);
        ApplyBarCountChange(newCount);
    }

    void AudioManager::ApplyFFTWindowChange(FFTWindowType newType) {
        m_audioConfig.windowType = newType;
        if (m_realtimeSource) m_realtimeSource->SetFFTWindow(m_audioConfig.windowType);
        LOG_INFO("FFT Window: " << Utils::ToString(m_audioConfig.windowType));
    }

    void AudioManager::ChangeFFTWindow(int direction) {
        FFTWindowType newType = Utils::CycleEnum(m_audioConfig.windowType, direction);
        ApplyFFTWindowChange(newType);
    }

    void AudioManager::ApplySpectrumScaleChange(SpectrumScale newType) {
        m_audioConfig.scaleType = newType;
        if (m_realtimeSource) m_realtimeSource->SetScaleType(m_audioConfig.scaleType);
        LOG_INFO("Spectrum Scale: " << Utils::ToString(m_audioConfig.scaleType));
    }

    void AudioManager::ChangeSpectrumScale(int direction) {
        SpectrumScale newType = Utils::CycleEnum(m_audioConfig.scaleType, direction);
        ApplySpectrumScaleChange(newType);
    }

}