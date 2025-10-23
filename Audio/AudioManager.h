#ifndef SPECTRUM_CPP_AUDIO_MANAGER_H
#define SPECTRUM_CPP_AUDIO_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the AudioManager for audio subsystem orchestration.
//
// This class serves as the central coordinator for all audio operations,
// managing the lifecycle of audio sources and handling transitions between
// live capture and animation modes. Acts as the primary facade for the
// entire audio subsystem.
//
// Key features:
// - Dual-source management (realtime capture / animation)
// - Seamless mode switching with state preservation
// - Centralized audio parameter control
// - Event-driven configuration updates
// - Spectrum data provision to visualizers
//
// Design notes:
// - Non-owning pointer to current source for fast switching
// - All parameter changes logged for debugging
// - Validates dependencies at construction time
// - Smooth transitions between capture and animation
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <memory>

namespace Spectrum {

    class EventBus;
    class IAudioSource;

    class AudioManager final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit AudioManager(EventBus* bus);
        ~AudioManager();

        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;
        AudioManager(AudioManager&&) = delete;
        AudioManager& operator=(AudioManager&&) = delete;

        [[nodiscard]] bool Initialize();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(float deltaTime);

        [[nodiscard]] SpectrumData GetSpectrum();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Mode Control
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ToggleCapture();
        void ToggleAnimation();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Parameter Control
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ChangeAmplification(float delta);
        void ChangeFFTWindow(int direction);
        void ChangeSpectrumScale(int direction);

        void SetAmplification(float amp);
        void SetSmoothing(float smoothing);
        void SetBarCount(size_t count);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsCapturing() const noexcept;
        [[nodiscard]] bool IsAnimating() const noexcept;
        [[nodiscard]] bool HasActiveSource() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Parameter Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetAmplification() const noexcept;
        [[nodiscard]] float GetSmoothing() const noexcept;
        [[nodiscard]] size_t GetBarCount() const noexcept;
        [[nodiscard]] std::string_view GetSpectrumScaleName() const noexcept;
        [[nodiscard]] std::string_view GetFFTWindowName() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SubscribeToEvents(EventBus* bus);
        bool CreateAudioSources();
        bool InitializeRealtimeSource();
        bool InitializeAnimatedSource();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Source Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetCurrentSource(IAudioSource* source);
        void SwitchToRealtimeSource();
        void SwitchToAnimatedSource();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Mode Transitions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ActivateAnimatedMode();
        void DeactivateAnimatedMode();
        void StartRealtimeCapture();
        void StopRealtimeCapture();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Parameter Application
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ApplyAmplificationChange(float newValue);
        void ApplyFFTWindowChange(FFTWindowType newType);
        void ApplySpectrumScaleChange(SpectrumScale newType);
        void ApplyBarCountChange(size_t newCount);
        void ApplySmoothingChange(float newValue);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ValidateEventBus(EventBus* bus) const noexcept;
        [[nodiscard]] bool ValidateAmplification(float amp) const noexcept;
        [[nodiscard]] bool ValidateSmoothing(float smoothing) const noexcept;
        [[nodiscard]] bool ValidateBarCount(size_t count) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float ClampAmplification(float amp) const noexcept;
        [[nodiscard]] float ClampSmoothing(float smoothing) const noexcept;
        [[nodiscard]] size_t ClampBarCount(size_t count) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Logging Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void LogModeChange(bool isAnimating) const;
        void LogCaptureStateChange(bool isCapturing) const;
        void LogParameterChange(const char* paramName, float value) const;
        void LogParameterChange(const char* paramName, size_t value) const;
        void LogParameterChange(const char* paramName, std::string_view value) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        std::unique_ptr<IAudioSource> m_realtimeSource;
        std::unique_ptr<IAudioSource> m_animatedSource;
        IAudioSource* m_currentSource; // Non-owning

        AudioConfig m_audioConfig;
        bool m_isCapturing;
        bool m_isAnimating;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_MANAGER_H