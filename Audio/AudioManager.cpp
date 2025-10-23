// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the AudioManager. It creates and manages audio
// sources and responds to user input to change audio processing parameters,
// acting as a facade for the entire audio subsystem.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "AudioManager.h"
#include "Common/EventBus.h"
#include "Common/TemplateUtils.h"
#include "Common/MathUtils.h"
#include "Common/StringUtils.h"
#include "Audio/Sources/RealtimeAudioSource.h"
#include "Audio/Sources/AnimatedAudioSource.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    AudioManager::AudioManager(
        EventBus* bus
    ) :
        m_currentSource(nullptr),
        m_isCapturing(false),
        m_isAnimating(false)
    {
        SubscribeToEvents(bus);
    }

    AudioManager::~AudioManager() {
        if (m_isCapturing && m_realtimeSource)
            m_realtimeSource->StopCapture();
    }

    bool AudioManager::Initialize() {
        if (!CreateAudioSources()) return false;
        SetCurrentSource(m_realtimeSource.get());
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void AudioManager::Update(float deltaTime) {
        if (m_currentSource)
            m_currentSource->Update(deltaTime);
    }

    [[nodiscard]] SpectrumData AudioManager::GetSpectrum() {
        if (m_currentSource)
            return m_currentSource->GetSpectrum();
        return {};
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // State Control
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void AudioManager::ToggleCapture() {
        if (m_isAnimating) return;

        if (m_isCapturing)
            StopRealtimeCapture();
        else
            StartRealtimeCapture();
    }

    void AudioManager::ToggleAnimation() {
        m_isAnimating = !m_isAnimating;
        if (m_isAnimating)
            ActivateAnimatedMode();
        else
            DeactivateAnimatedMode();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Parameter Control (from UI or Hotkeys)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void AudioManager::ChangeAmplification(float delta) {
        float newValue = Utils::Clamp(m_audioConfig.amplification + delta, 0.1f, 5.0f);
        SetAmplification(newValue);
    }

    void AudioManager::ChangeFFTWindow(int direction) {
        FFTWindowType newType = Utils::CycleEnum(m_audioConfig.windowType, direction);
        ApplyFFTWindowChange(newType);
    }

    void AudioManager::ChangeSpectrumScale(int direction) {
        SpectrumScale newType = Utils::CycleEnum(m_audioConfig.scaleType, direction);
        ApplySpectrumScaleChange(newType);
    }

    void AudioManager::SetAmplification(float amp) {
        ApplyAmplificationChange(amp);
    }

    void AudioManager::SetSmoothing(float smoothing) {
        m_audioConfig.smoothing = Utils::Saturate(smoothing);
        if (m_realtimeSource) m_realtimeSource->SetSmoothing(m_audioConfig.smoothing);
        if (m_animatedSource) m_animatedSource->SetSmoothing(m_audioConfig.smoothing);
    }

    void AudioManager::SetBarCount(size_t count) {
        size_t newCount = Utils::Clamp<size_t>(count, 16, 256);
        ApplyBarCountChange(newCount);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] bool AudioManager::IsCapturing() const {
        return m_isCapturing;
    }

    [[nodiscard]] bool AudioManager::IsAnimating() const {
        return m_isAnimating;
    }

    [[nodiscard]] float AudioManager::GetAmplification() const {
        return m_audioConfig.amplification;
    }

    [[nodiscard]] float AudioManager::GetSmoothing() const {
        return m_audioConfig.smoothing;
    }

    [[nodiscard]] size_t AudioManager::GetBarCount() const {
        return m_audioConfig.barCount;
    }

    [[nodiscard]] std::string_view AudioManager::GetSpectrumScaleName() const {
        return Utils::ToString(m_audioConfig.scaleType);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void AudioManager::SubscribeToEvents(EventBus* bus) {
        bus->Subscribe(InputAction::ToggleCapture, [this]() { this->ToggleCapture(); });
        bus->Subscribe(InputAction::ToggleAnimation, [this]() { this->ToggleAnimation(); });
        bus->Subscribe(InputAction::CycleSpectrumScale, [this]() { this->ChangeSpectrumScale(1); });
        bus->Subscribe(InputAction::IncreaseAmplification, [this]() { this->ChangeAmplification(0.1f); });
        bus->Subscribe(InputAction::DecreaseAmplification, [this]() { this->ChangeAmplification(-0.1f); });
        bus->Subscribe(InputAction::NextFFTWindow, [this]() { this->ChangeFFTWindow(1); });
        bus->Subscribe(InputAction::PrevFFTWindow, [this]() { this->ChangeFFTWindow(-1); });
    }

    [[nodiscard]] bool AudioManager::CreateAudioSources() {
        m_realtimeSource = std::make_unique<RealtimeAudioSource>(m_audioConfig);
        m_animatedSource = std::make_unique<AnimatedAudioSource>(m_audioConfig);
        return m_realtimeSource->Initialize() && m_animatedSource->Initialize();
    }

    void AudioManager::SetCurrentSource(IAudioSource* source) {
        m_currentSource = source;
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

    void AudioManager::StartRealtimeCapture() {
        m_isCapturing = true;
        m_realtimeSource->StartCapture();
    }

    void AudioManager::StopRealtimeCapture() {
        m_isCapturing = false;
        m_realtimeSource->StopCapture();
    }

    void AudioManager::ApplyAmplificationChange(float newValue) {
        m_audioConfig.amplification = newValue;
        if (m_realtimeSource) m_realtimeSource->SetAmplification(m_audioConfig.amplification);
        // Animated source doesn't use amplification
        LOG_INFO("Amplification Factor: " << m_audioConfig.amplification);
    }

    void AudioManager::ApplyBarCountChange(size_t newCount) {
        m_audioConfig.barCount = newCount;
        if (m_realtimeSource) m_realtimeSource->SetBarCount(m_audioConfig.barCount);
        if (m_animatedSource) m_animatedSource->SetBarCount(m_audioConfig.barCount);
        LOG_INFO("Bar Count: " << m_audioConfig.barCount);
    }

    void AudioManager::ApplyFFTWindowChange(FFTWindowType newType) {
        m_audioConfig.windowType = newType;
        if (m_realtimeSource) m_realtimeSource->SetFFTWindow(m_audioConfig.windowType);
        LOG_INFO("FFT Window: " << Utils::ToString(m_audioConfig.windowType));
    }

    void AudioManager::ApplySpectrumScaleChange(SpectrumScale newType) {
        m_audioConfig.scaleType = newType;
        if (m_realtimeSource) m_realtimeSource->SetScaleType(m_audioConfig.scaleType);
        LOG_INFO("Spectrum Scale: " << Utils::ToString(m_audioConfig.scaleType));
    }

} // namespace Spectrum