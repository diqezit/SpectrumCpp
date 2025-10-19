// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// RealtimeAudioSource.h: Provides spectrum data from a live audio capture.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifndef SPECTRUM_CPP_REALTIMEAUDIOSOURCE_H
#define SPECTRUM_CPP_REALTIMEAUDIOSOURCE_H

#include "IAudioSource.h"
#include "AudioCapture.h"
#include "SpectrumAnalyzer.h"

namespace Spectrum {

    class RealtimeAudioSource : public IAudioSource {
    public:
        explicit RealtimeAudioSource(const AudioConfig& config);

        bool Initialize() override;
        void Update(float deltaTime) override;
        SpectrumData GetSpectrum() override;

        void SetAmplification(float amp) override;
        void SetBarCount(size_t count) override;
        void SetFFTWindow(FFTWindowType type) override;
        void SetScaleType(SpectrumScale type) override;

        void StartCapture() override;
        void StopCapture() override;

    private:
        // Initialization and Configuration
        void ConfigureAnalyzer();
        void ReinitializeCapture();
        bool TryCreateCaptureDevice();
        void SetupNewCaptureDevice();

        // State Management
        void HandleCaptureFaults();
        bool EnsureCaptureIsReady();

        std::unique_ptr<AudioCapture> m_audioCapture;
        std::unique_ptr<SpectrumAnalyzer> m_analyzer;
        AudioConfig m_config;
        bool m_isCapturing = false;
    };

}

#endif