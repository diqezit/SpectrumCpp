#include "AudioManager.h"
#include "Common/EventBus.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Audio/Sources/RealtimeAudioSource.h"
#include "Audio/Sources/AnimatedAudioSource.h"

namespace Spectrum {

    using namespace Helpers::Utils;
    using namespace Helpers::Math;

    namespace {
        constexpr float kMinAmplification = 0.1f;
        constexpr float kMaxAmplification = 5.0f;
        constexpr float kAmplificationStep = 0.1f;

        constexpr float kMinSmoothing = 0.0f;
        constexpr float kMaxSmoothing = 1.0f;

        constexpr size_t kMinBarCount = 16;
        constexpr size_t kMaxBarCount = 256;
    }

    AudioManager::AudioManager(EventBus* bus)
        : m_currentSource(nullptr)
        , m_isCapturing(false)
        , m_isAnimating(false)
    {
        LOG_INFO("AudioManager: Initializing...");

        if (!bus) {
            throw std::invalid_argument("AudioManager: EventBus cannot be null");
        }

        SubscribeToEvents(bus);

        LOG_INFO("AudioManager: Construction completed");
    }

    AudioManager::~AudioManager()
    {
        LOG_INFO("AudioManager: Shutting down...");

        if (m_isCapturing && m_realtimeSource) {
            LOG_INFO("AudioManager: Stopping realtime capture...");
            m_isCapturing = false;
            m_realtimeSource->StopCapture();
            LOG_INFO("AudioManager: Realtime capture stopped");
        }

        LOG_INFO("AudioManager: Destroyed");
    }

    bool AudioManager::Initialize()
    {
        LOG_INFO("AudioManager: Starting initialization...");

        if (!CreateAudioSources()) {
            LOG_ERROR("AudioManager: Failed to create audio sources");
            return false;
        }

        m_currentSource = m_realtimeSource.get();

        LOG_INFO("AudioManager: Initialization completed successfully");
        return true;
    }

    void AudioManager::Update(float deltaTime)
    {
        if (m_currentSource) {
            m_currentSource->Update(deltaTime);
        }
    }

    SpectrumData AudioManager::GetSpectrum()
    {
        return m_currentSource ? m_currentSource->GetSpectrum() : SpectrumData{};
    }

    void AudioManager::ToggleCapture()
    {
        if (m_isAnimating) {
            LOG_WARNING("AudioManager: Cannot toggle capture in animation mode");
            return;
        }

        if (!m_realtimeSource) {
            LOG_ERROR("AudioManager: Cannot toggle capture - realtime source is null");
            return;
        }

        m_isCapturing = !m_isCapturing;

        LOG_INFO("AudioManager: " << (m_isCapturing ? "Starting" : "Stopping") << " realtime capture...");

        if (m_isCapturing) {
            m_realtimeSource->StartCapture();
        }
        else {
            m_realtimeSource->StopCapture();
        }

        LOG_INFO("AudioManager: Capture " << (m_isCapturing ? "started" : "stopped"));
    }

    void AudioManager::ToggleAnimation()
    {
        m_isAnimating = !m_isAnimating;

        if (m_isAnimating) {
            LOG_INFO("AudioManager: Activating animation mode...");

            if (m_isCapturing && m_realtimeSource) {
                LOG_INFO("AudioManager: Stopping realtime capture...");
                m_isCapturing = false;
                m_realtimeSource->StopCapture();
                LOG_INFO("AudioManager: Realtime capture stopped");
            }

            LOG_INFO("AudioManager: Switching to animated source");
            m_currentSource = m_animatedSource.get();
            LOG_INFO("AudioManager: Animation mode activated");
        }
        else {
            LOG_INFO("AudioManager: Deactivating animation mode...");
            LOG_INFO("AudioManager: Switching to realtime source");
            m_currentSource = m_realtimeSource.get();
            LOG_INFO("AudioManager: Animation mode deactivated");
        }

        LOG_INFO("AudioManager: Animation mode " << (m_isAnimating ? "ON" : "OFF"));
    }

    void AudioManager::ChangeAmplification(float delta)
    {
        const float newValue = Clamp(
            m_audioConfig.amplification + delta,
            kMinAmplification,
            kMaxAmplification
        );

        if (newValue != m_audioConfig.amplification) {
            SetAmplification(newValue);
        }
    }

    void AudioManager::ChangeFFTWindow(int direction)
    {
        const FFTWindowType newType = CycleEnum(m_audioConfig.windowType, direction);

        m_audioConfig.windowType = newType;

        if (m_realtimeSource) {
            m_realtimeSource->SetFFTWindow(newType);
        }

        LOG_INFO("AudioManager: FFT Window = " << ToString(newType).data());
    }

    void AudioManager::ChangeSpectrumScale(int direction)
    {
        const SpectrumScale newType = CycleEnum(m_audioConfig.scaleType, direction);

        m_audioConfig.scaleType = newType;

        if (m_realtimeSource) {
            m_realtimeSource->SetScaleType(newType);
        }

        LOG_INFO("AudioManager: Spectrum Scale = " << ToString(newType).data());
    }

    void AudioManager::SetAmplification(float amp)
    {
        const float clampedValue = Clamp(amp, kMinAmplification, kMaxAmplification);

        m_audioConfig.amplification = clampedValue;

        if (m_realtimeSource) {
            m_realtimeSource->SetAmplification(clampedValue);
        }

        LOG_INFO("AudioManager: Amplification = " << clampedValue);
    }

    void AudioManager::SetSmoothing(float smoothing)
    {
        const float clampedValue = Clamp(smoothing, kMinSmoothing, kMaxSmoothing);

        m_audioConfig.smoothing = clampedValue;

        if (m_realtimeSource) {
            m_realtimeSource->SetSmoothing(clampedValue);
        }

        LOG_INFO("AudioManager: Smoothing = " << clampedValue);
    }

    void AudioManager::SetBarCount(size_t count)
    {
        const size_t clampedValue = Clamp(count, kMinBarCount, kMaxBarCount);

        m_audioConfig.barCount = clampedValue;

        if (m_realtimeSource) {
            m_realtimeSource->SetBarCount(clampedValue);
        }

        LOG_INFO("AudioManager: Bar Count = " << clampedValue);
    }

    void AudioManager::SetFFTWindowByName(const std::string& name)
    {
        const FFTWindowType newType = StringToFFTWindow(name);
        m_audioConfig.windowType = newType;

        if (m_realtimeSource) {
            m_realtimeSource->SetFFTWindow(newType);
        }

        LOG_INFO("AudioManager: FFT Window = " << ToString(newType).data());
    }

    void AudioManager::SetSpectrumScaleByName(const std::string& name)
    {
        const SpectrumScale newType = StringToSpectrumScale(name);
        m_audioConfig.scaleType = newType;

        if (m_realtimeSource) {
            m_realtimeSource->SetScaleType(newType);
        }

        LOG_INFO("AudioManager: Spectrum Scale = " << ToString(newType).data());
    }

    bool AudioManager::IsCapturing() const noexcept
    {
        return m_isCapturing;
    }

    bool AudioManager::IsAnimating() const noexcept
    {
        return m_isAnimating;
    }

    bool AudioManager::HasActiveSource() const noexcept
    {
        return m_currentSource != nullptr;
    }

    float AudioManager::GetAmplification() const noexcept
    {
        return m_audioConfig.amplification;
    }

    float AudioManager::GetSmoothing() const noexcept
    {
        return m_audioConfig.smoothing;
    }

    size_t AudioManager::GetBarCount() const noexcept
    {
        return m_audioConfig.barCount;
    }

    std::string_view AudioManager::GetSpectrumScaleName() const noexcept
    {
        return ToString(m_audioConfig.scaleType);
    }

    std::string_view AudioManager::GetFFTWindowName() const noexcept
    {
        return ToString(m_audioConfig.windowType);
    }

    std::vector<std::string> AudioManager::GetAvailableFFTWindows() const
    {
        return { "Hann", "Hamming", "Blackman", "Rectangular" };
    }

    std::vector<std::string> AudioManager::GetAvailableSpectrumScales() const
    {
        return { "Linear", "Logarithmic", "Mel" };
    }

    void AudioManager::SubscribeToEvents(EventBus* bus)
    {
        if (!bus) return;

        LOG_INFO("AudioManager: Subscribing to events...");

        try {
            bus->Subscribe(InputAction::ToggleCapture, [this]() { ToggleCapture(); });
            bus->Subscribe(InputAction::ToggleAnimation, [this]() { ToggleAnimation(); });
            bus->Subscribe(InputAction::CycleSpectrumScale, [this]() { ChangeSpectrumScale(1); });
            bus->Subscribe(InputAction::IncreaseAmplification, [this]() { ChangeAmplification(kAmplificationStep); });
            bus->Subscribe(InputAction::DecreaseAmplification, [this]() { ChangeAmplification(-kAmplificationStep); });
            bus->Subscribe(InputAction::NextFFTWindow, [this]() { ChangeFFTWindow(1); });
            bus->Subscribe(InputAction::PrevFFTWindow, [this]() { ChangeFFTWindow(-1); });

            LOG_INFO("AudioManager: Event subscription completed");
        }
        catch (const std::exception&) {
            LOG_ERROR("AudioManager: Event subscription failed");
        }
    }

    bool AudioManager::CreateAudioSources()
    {
        LOG_INFO("AudioManager: Creating audio sources...");

        if (!InitializeSource<RealtimeAudioSource>(m_realtimeSource, "RealtimeAudioSource")) {
            return false;
        }

        if (!InitializeSource<AnimatedAudioSource>(m_animatedSource, "AnimatedAudioSource")) {
            return false;
        }

        LOG_INFO("AudioManager: Audio sources created successfully");
        return true;
    }

    template<typename TSource>
    bool AudioManager::InitializeSource(
        std::unique_ptr<IAudioSource>& source,
        const char* sourceName
    )
    {
        LOG_INFO("AudioManager: Initializing " << sourceName << "...");

        source = std::make_unique<TSource>(m_audioConfig);
        if (!source) {
            LOG_ERROR("AudioManager: Failed to create " << sourceName << " instance");
            return false;
        }

        if (!source->Initialize()) {
            LOG_ERROR("AudioManager: " << sourceName << " initialization failed");
            return false;
        }

        LOG_INFO("AudioManager: " << sourceName << " initialized");
        return true;
    }

    FFTWindowType AudioManager::StringToFFTWindow(const std::string& name) const
    {
        if (name == "Hann") return FFTWindowType::Hann;
        if (name == "Hamming") return FFTWindowType::Hamming;
        if (name == "Blackman") return FFTWindowType::Blackman;
        if (name == "Rectangular") return FFTWindowType::Rectangular;
        return FFTWindowType::Hann;
    }

    SpectrumScale AudioManager::StringToSpectrumScale(const std::string& name) const
    {
        if (name == "Linear") return SpectrumScale::Linear;
        if (name == "Logarithmic") return SpectrumScale::Logarithmic;
        if (name == "Mel") return SpectrumScale::Mel;
        return SpectrumScale::Linear;
    }

} // namespace Spectrum