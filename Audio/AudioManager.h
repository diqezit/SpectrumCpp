#ifndef SPECTRUM_CPP_AUDIO_MANAGER_H
#define SPECTRUM_CPP_AUDIO_MANAGER_H

#include "Common/Common.h"
#include <memory>
#include <vector>

namespace Spectrum {

    class EventBus;
    class IAudioSource;

    class AudioManager final
    {
    public:
        explicit AudioManager(EventBus* bus);
        ~AudioManager();

        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;
        AudioManager(AudioManager&&) = delete;
        AudioManager& operator=(AudioManager&&) = delete;

        [[nodiscard]] bool Initialize();

        void Update(float deltaTime);
        [[nodiscard]] SpectrumData GetSpectrum();

        void ToggleCapture();
        void ToggleAnimation();

        void ChangeAmplification(float delta);
        void ChangeFFTWindow(int direction);
        void ChangeSpectrumScale(int direction);

        void SetAmplification(float amp);
        void SetSmoothing(float smoothing);
        void SetBarCount(size_t count);
        void SetFFTWindowByName(const std::string& name);
        void SetSpectrumScaleByName(const std::string& name);

        [[nodiscard]] bool IsCapturing() const noexcept;
        [[nodiscard]] bool IsAnimating() const noexcept;
        [[nodiscard]] bool HasActiveSource() const noexcept;

        [[nodiscard]] float GetAmplification() const noexcept;
        [[nodiscard]] float GetSmoothing() const noexcept;
        [[nodiscard]] size_t GetBarCount() const noexcept;
        [[nodiscard]] std::string_view GetSpectrumScaleName() const noexcept;
        [[nodiscard]] std::string_view GetFFTWindowName() const noexcept;

        [[nodiscard]] std::vector<std::string> GetAvailableFFTWindows() const;
        [[nodiscard]] std::vector<std::string> GetAvailableSpectrumScales() const;

    private:
        void SubscribeToEvents(EventBus* bus);
        bool CreateAudioSources();

        template<typename TSource>
        bool InitializeSource(
            std::unique_ptr<IAudioSource>& source,
            const char* sourceName
        );

        FFTWindowType StringToFFTWindow(const std::string& name) const;
        SpectrumScale StringToSpectrumScale(const std::string& name) const;

        std::unique_ptr<IAudioSource> m_realtimeSource;
        std::unique_ptr<IAudioSource> m_animatedSource;
        IAudioSource* m_currentSource;

        AudioConfig m_audioConfig;
        bool m_isCapturing;
        bool m_isAnimating;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_MANAGER_H