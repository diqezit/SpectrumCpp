// AudioManager.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioManager.h: Manages audio capture, analysis, configuration, and test animation.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_AUDIO_MANAGER_H
#define SPECTRUM_CPP_AUDIO_MANAGER_H

#include "Common.h"
#include "AudioCapture.h"
#include "SpectrumAnalyzer.h"
#include "Utils.h"

#include <mutex>

namespace Spectrum {

    class AudioManager : public IAudioCaptureCallback {
    public:
        AudioManager();
        ~AudioManager();

        bool Initialize();
        void Update(float deltaTime);
        SpectrumData GetSpectrum();

        void OnAudioData(const float* data, size_t samples, int channels) override;

        // User actions
        void ToggleCapture();
        void ToggleAnimation();
        void ChangeAmplification(float delta);
        void ChangeBarCount(int delta);
        void ChangeFFTWindow(int direction);
        void ChangeSpectrumScale(int direction);

        // Helpers
        std::string_view GetWindowTypeName(FFTWindowType type) const;
        std::string_view GetScaleTypeName(SpectrumScale type) const;

        bool IsCapturing() const { return m_isCapturing; }
        bool IsAnimating() const { return m_isAnimating; }

    private:
        bool StartCaptureInternal();
        void StopCaptureInternal();

    private:
        std::unique_ptr<AudioCapture>     m_audioCapture;
        std::unique_ptr<SpectrumAnalyzer> m_analyzer;

        AudioConfig m_audioConfig;

        bool m_isCapturing = false;
        bool m_isAnimating = false;
        float m_animationTime = 0.0f;

        std::mutex m_spectrumMutex;
        SpectrumData m_currentSpectrum;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_MANAGER_H