// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the AudioManager, which orchestrates the lifecycle and
// configuration of audio sources, handling transitions between live capture
// and animation, and serving as the primary API for audio control.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_AUDIO_MANAGER_H
#define SPECTRUM_CPP_AUDIO_MANAGER_H

#include "Common/Common.h"

namespace Spectrum {

    class EventBus;
    class IAudioSource;

    class AudioManager {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        explicit AudioManager(EventBus* bus);
        ~AudioManager();

        bool Initialize();
        void Update(float deltaTime);
        [[nodiscard]] SpectrumData GetSpectrum();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // State Control
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void ToggleCapture();
        void ToggleAnimation();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Parameter Control (from UI or Hotkeys)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void ChangeAmplification(float delta);
        void ChangeFFTWindow(int direction);
        void ChangeSpectrumScale(int direction);

        void SetAmplification(float amp);
        void SetSmoothing(float smoothing);
        void SetBarCount(size_t count);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        [[nodiscard]] bool IsCapturing() const;
        [[nodiscard]] bool IsAnimating() const;
        [[nodiscard]] float GetAmplification() const;
        [[nodiscard]] float GetSmoothing() const;
        [[nodiscard]] size_t GetBarCount() const;
        [[nodiscard]] std::string_view GetSpectrumScaleName() const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void SubscribeToEvents(EventBus* bus);
        [[nodiscard]] bool CreateAudioSources();
        void SetCurrentSource(IAudioSource* source);

        void ActivateAnimatedMode();
        void DeactivateAnimatedMode();
        void StartRealtimeCapture();
        void StopRealtimeCapture();

        void ApplyAmplificationChange(float newValue);
        void ApplyFFTWindowChange(FFTWindowType newType);
        void ApplySpectrumScaleChange(SpectrumScale newType);
        void ApplyBarCountChange(size_t newCount);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        std::unique_ptr<IAudioSource> m_realtimeSource;
        std::unique_ptr<IAudioSource> m_animatedSource;
        IAudioSource* m_currentSource;

        AudioConfig m_audioConfig;
        bool m_isCapturing;
        bool m_isAnimating;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_MANAGER_H