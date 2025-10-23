// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the AudioManager for audio subsystem orchestration.
//
// Implementation details:
// - Two audio sources: RealtimeAudioSource and AnimatedAudioSource
// - Current source switched via non-owning pointer for efficiency
// - All parameter changes propagated to relevant sources
// - Capture state managed independently of animation state
// - Configuration preserved across mode switches
//
// Update pipeline:
// 1. Delegate to current source's Update()
// 2. Current source processes audio/generates animation
// 3. Spectrum data available via GetSpectrum()
//
// Mode switching:
// - Animation mode: stops capture, switches to animated source
// - Normal mode: switches to realtime source (capture optional)
// - Capture toggle: only works when not in animation mode
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "AudioManager.h"
#include "Common/EventBus.h"
#include "Graphics/API/Helpers/Utils/TemplateUtils.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "Graphics/API/Helpers/Utils/StringUtils.h"
#include "Audio/Sources/RealtimeAudioSource.h"
#include "Audio/Sources/AnimatedAudioSource.h"

namespace Spectrum {

    using namespace Helpers::Utils;
    using namespace Helpers::Math;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        // Amplification limits
        constexpr float kMinAmplification = 0.1f;
        constexpr float kMaxAmplification = 5.0f;
        constexpr float kAmplificationStep = 0.1f;

        // Smoothing limits
        constexpr float kMinSmoothing = 0.0f;
        constexpr float kMaxSmoothing = 1.0f;

        // Bar count limits
        constexpr size_t kMinBarCount = 16;
        constexpr size_t kMaxBarCount = 256;

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    AudioManager::AudioManager(EventBus* bus)
        : m_currentSource(nullptr)
        , m_isCapturing(false)
        , m_isAnimating(false)
    {
        LOG_INFO("AudioManager: Initializing...");

        if (!ValidateEventBus(bus)) {
            throw std::invalid_argument("AudioManager: EventBus cannot be null");
        }

        SubscribeToEvents(bus);

        LOG_INFO("AudioManager: Construction completed");
    }

    AudioManager::~AudioManager()
    {
        LOG_INFO("AudioManager: Shutting down...");

        if (m_isCapturing && m_realtimeSource) {
            StopRealtimeCapture();
        }

        LOG_INFO("AudioManager: Destroyed");
    }

    [[nodiscard]] bool AudioManager::Initialize()
    {
        LOG_INFO("AudioManager: Starting initialization...");

        if (!CreateAudioSources()) {
            LOG_ERROR("AudioManager: Failed to create audio sources");
            return false;
        }

        SetCurrentSource(m_realtimeSource.get());

        LOG_INFO("AudioManager: Initialization completed successfully");
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::Update(float deltaTime)
    {
        if (!m_currentSource) {
            return;
        }

        m_currentSource->Update(deltaTime);
    }

    [[nodiscard]] SpectrumData AudioManager::GetSpectrum()
    {
        if (!m_currentSource) {
            return {};
        }

        return m_currentSource->GetSpectrum();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Mode Control
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::ToggleCapture()
    {
        if (m_isAnimating) {
            LOG_WARNING("AudioManager: Cannot toggle capture in animation mode");
            return;
        }

        const bool newState = !m_isCapturing;

        if (newState) {
            StartRealtimeCapture();
        }
        else {
            StopRealtimeCapture();
        }

        LogCaptureStateChange(newState);
    }

    void AudioManager::ToggleAnimation()
    {
        m_isAnimating = !m_isAnimating;

        if (m_isAnimating) {
            ActivateAnimatedMode();
        }
        else {
            DeactivateAnimatedMode();
        }

        LogModeChange(m_isAnimating);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Parameter Control
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::ChangeAmplification(float delta)
    {
        const float currentValue = m_audioConfig.amplification;
        const float newValue = ClampAmplification(currentValue + delta);

        if (newValue != currentValue) {
            SetAmplification(newValue);
        }
    }

    void AudioManager::ChangeFFTWindow(int direction)
    {
        const FFTWindowType newType = CycleEnum(
            m_audioConfig.windowType,
            direction
        );

        ApplyFFTWindowChange(newType);
    }

    void AudioManager::ChangeSpectrumScale(int direction)
    {
        const SpectrumScale newType = CycleEnum(
            m_audioConfig.scaleType,
            direction
        );

        ApplySpectrumScaleChange(newType);
    }

    void AudioManager::SetAmplification(float amp)
    {
        if (!ValidateAmplification(amp)) {
            LOG_WARNING("AudioManager: Invalid amplification value: " << amp);
            return;
        }

        const float clampedValue = ClampAmplification(amp);
        ApplyAmplificationChange(clampedValue);
    }

    void AudioManager::SetSmoothing(float smoothing)
    {
        if (!ValidateSmoothing(smoothing)) {
            LOG_WARNING("AudioManager: Invalid smoothing value: " << smoothing);
            return;
        }

        const float clampedValue = ClampSmoothing(smoothing);
        ApplySmoothingChange(clampedValue);
    }

    void AudioManager::SetBarCount(size_t count)
    {
        if (!ValidateBarCount(count)) {
            LOG_WARNING("AudioManager: Invalid bar count: " << count);
            return;
        }

        const size_t clampedValue = ClampBarCount(count);
        ApplyBarCountChange(clampedValue);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool AudioManager::IsCapturing() const noexcept
    {
        return m_isCapturing;
    }

    [[nodiscard]] bool AudioManager::IsAnimating() const noexcept
    {
        return m_isAnimating;
    }

    [[nodiscard]] bool AudioManager::HasActiveSource() const noexcept
    {
        return m_currentSource != nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Parameter Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float AudioManager::GetAmplification() const noexcept
    {
        return m_audioConfig.amplification;
    }

    [[nodiscard]] float AudioManager::GetSmoothing() const noexcept
    {
        return m_audioConfig.smoothing;
    }

    [[nodiscard]] size_t AudioManager::GetBarCount() const noexcept
    {
        return m_audioConfig.barCount;
    }

    [[nodiscard]] std::string_view AudioManager::GetSpectrumScaleName() const noexcept
    {
        return ToString(m_audioConfig.scaleType);
    }

    [[nodiscard]] std::string_view AudioManager::GetFFTWindowName() const noexcept
    {
        return ToString(m_audioConfig.windowType);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::SubscribeToEvents(EventBus* bus)
    {
        if (!bus) {
            return;
        }

        LOG_INFO("AudioManager: Subscribing to events...");

        try {
            bus->Subscribe(InputAction::ToggleCapture, [this]() {
                ToggleCapture();
                });

            bus->Subscribe(InputAction::ToggleAnimation, [this]() {
                ToggleAnimation();
                });

            bus->Subscribe(InputAction::CycleSpectrumScale, [this]() {
                ChangeSpectrumScale(1);
                });

            bus->Subscribe(InputAction::IncreaseAmplification, [this]() {
                ChangeAmplification(kAmplificationStep);
                });

            bus->Subscribe(InputAction::DecreaseAmplification, [this]() {
                ChangeAmplification(-kAmplificationStep);
                });

            bus->Subscribe(InputAction::NextFFTWindow, [this]() {
                ChangeFFTWindow(1);
                });

            bus->Subscribe(InputAction::PrevFFTWindow, [this]() {
                ChangeFFTWindow(-1);
                });

            LOG_INFO("AudioManager: Event subscription completed");
        }
        catch (const std::exception& e) {
            (void)e; // Suppress unused warning
            LOG_ERROR("AudioManager: Event subscription failed: " << e.what());
        }
    }

    [[nodiscard]] bool AudioManager::CreateAudioSources()
    {
        LOG_INFO("AudioManager: Creating audio sources...");

        if (!InitializeRealtimeSource()) {
            LOG_ERROR("AudioManager: Failed to initialize realtime source");
            return false;
        }

        if (!InitializeAnimatedSource()) {
            LOG_ERROR("AudioManager: Failed to initialize animated source");
            return false;
        }

        LOG_INFO("AudioManager: Audio sources created successfully");
        return true;
    }

    bool AudioManager::InitializeRealtimeSource()
    {
        LOG_INFO("AudioManager: Initializing realtime source...");

        m_realtimeSource = std::make_unique<RealtimeAudioSource>(m_audioConfig);
        if (!m_realtimeSource) {
            LOG_ERROR("AudioManager: Failed to create realtime source instance");
            return false;
        }

        if (!m_realtimeSource->Initialize()) {
            LOG_ERROR("AudioManager: Realtime source initialization failed");
            return false;
        }

        LOG_INFO("AudioManager: Realtime source initialized");
        return true;
    }

    bool AudioManager::InitializeAnimatedSource()
    {
        LOG_INFO("AudioManager: Initializing animated source...");

        m_animatedSource = std::make_unique<AnimatedAudioSource>(m_audioConfig);
        if (!m_animatedSource) {
            LOG_ERROR("AudioManager: Failed to create animated source instance");
            return false;
        }

        if (!m_animatedSource->Initialize()) {
            LOG_ERROR("AudioManager: Animated source initialization failed");
            return false;
        }

        LOG_INFO("AudioManager: Animated source initialized");
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Source Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::SetCurrentSource(IAudioSource* source)
    {
        if (!source) {
            LOG_WARNING("AudioManager: Attempting to set null source");
            return;
        }

        m_currentSource = source;
    }

    void AudioManager::SwitchToRealtimeSource()
    {
        LOG_INFO("AudioManager: Switching to realtime source");
        SetCurrentSource(m_realtimeSource.get());
    }

    void AudioManager::SwitchToAnimatedSource()
    {
        LOG_INFO("AudioManager: Switching to animated source");
        SetCurrentSource(m_animatedSource.get());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Mode Transitions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::ActivateAnimatedMode()
    {
        LOG_INFO("AudioManager: Activating animation mode...");

        if (m_isCapturing) {
            StopRealtimeCapture();
        }

        SwitchToAnimatedSource();

        LOG_INFO("AudioManager: Animation mode activated");
    }

    void AudioManager::DeactivateAnimatedMode()
    {
        LOG_INFO("AudioManager: Deactivating animation mode...");

        SwitchToRealtimeSource();

        LOG_INFO("AudioManager: Animation mode deactivated");
    }

    void AudioManager::StartRealtimeCapture()
    {
        if (!m_realtimeSource) {
            LOG_ERROR("AudioManager: Cannot start capture - realtime source is null");
            return;
        }

        LOG_INFO("AudioManager: Starting realtime capture...");

        m_isCapturing = true;
        m_realtimeSource->StartCapture();

        LOG_INFO("AudioManager: Realtime capture started");
    }

    void AudioManager::StopRealtimeCapture()
    {
        if (!m_realtimeSource) {
            LOG_ERROR("AudioManager: Cannot stop capture - realtime source is null");
            return;
        }

        LOG_INFO("AudioManager: Stopping realtime capture...");

        m_isCapturing = false;
        m_realtimeSource->StopCapture();

        LOG_INFO("AudioManager: Realtime capture stopped");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Parameter Application
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::ApplyAmplificationChange(float newValue)
    {
        m_audioConfig.amplification = newValue;

        if (m_realtimeSource) {
            m_realtimeSource->SetAmplification(newValue);
        }

        LogParameterChange("Amplification", newValue);
    }

    void AudioManager::ApplyFFTWindowChange(FFTWindowType newType)
    {
        m_audioConfig.windowType = newType;

        if (m_realtimeSource) {
            m_realtimeSource->SetFFTWindow(newType);
        }

        LogParameterChange("FFT Window", ToString(newType));
    }

    void AudioManager::ApplySpectrumScaleChange(SpectrumScale newType)
    {
        m_audioConfig.scaleType = newType;

        if (m_realtimeSource) {
            m_realtimeSource->SetScaleType(newType);
        }

        LogParameterChange("Spectrum Scale", ToString(newType));
    }

    void AudioManager::ApplyBarCountChange(size_t newCount)
    {
        m_audioConfig.barCount = newCount;

        if (m_realtimeSource) {
            m_realtimeSource->SetBarCount(newCount);
        }

        if (m_animatedSource) {
            m_animatedSource->SetBarCount(newCount);
        }

        LogParameterChange("Bar Count", newCount);
    }

    void AudioManager::ApplySmoothingChange(float newValue)
    {
        m_audioConfig.smoothing = newValue;

        if (m_realtimeSource) {
            m_realtimeSource->SetSmoothing(newValue);
        }

        if (m_animatedSource) {
            m_animatedSource->SetSmoothing(newValue);
        }

        LogParameterChange("Smoothing", newValue);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool AudioManager::ValidateEventBus(EventBus* bus) const noexcept
    {
        return bus != nullptr;
    }

    [[nodiscard]] bool AudioManager::ValidateAmplification(float amp) const noexcept
    {
        return amp >= kMinAmplification && amp <= kMaxAmplification;
    }

    [[nodiscard]] bool AudioManager::ValidateSmoothing(float smoothing) const noexcept
    {
        return smoothing >= kMinSmoothing && smoothing <= kMaxSmoothing;
    }

    [[nodiscard]] bool AudioManager::ValidateBarCount(size_t count) const noexcept
    {
        return count >= kMinBarCount && count <= kMaxBarCount;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float AudioManager::ClampAmplification(float amp) const noexcept
    {
        return Clamp(amp, kMinAmplification, kMaxAmplification);
    }

    [[nodiscard]] float AudioManager::ClampSmoothing(float smoothing) const noexcept
    {
        return Clamp(smoothing, kMinSmoothing, kMaxSmoothing);
    }

    [[nodiscard]] size_t AudioManager::ClampBarCount(size_t count) const noexcept
    {
        return Clamp(count, kMinBarCount, kMaxBarCount);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Logging Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioManager::LogModeChange(bool isAnimating) const
    {
        LOG_INFO("AudioManager: Animation mode "
            << (isAnimating ? "ON" : "OFF"));
    }

    void AudioManager::LogCaptureStateChange(bool isCapturing) const
    {
        LOG_INFO("AudioManager: Capture "
            << (isCapturing ? "started" : "stopped"));
    }

    void AudioManager::LogParameterChange(const char* paramName, float value) const
    {
        LOG_INFO("AudioManager: " << paramName << " = " << value);
    }

    void AudioManager::LogParameterChange(const char* paramName, size_t value) const
    {
        LOG_INFO("AudioManager: " << paramName << " = " << value);
    }

    void AudioManager::LogParameterChange(
        const char* paramName,
        std::string_view value
    ) const
    {
        LOG_INFO("AudioManager: " << paramName << " = " << value.data());
    }

} // namespace Spectrum