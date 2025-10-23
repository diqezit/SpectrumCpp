// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the RealtimeAudioSource, which provides spectrum data
// by capturing and analyzing live system audio using the WASAPI loopback
// mechanism and the full FFT processing pipeline.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_REALTIMEAUDIOSOURCE_H
#define SPECTRUM_CPP_REALTIMEAUDIOSOURCE_H

#include "IAudioSource.h"
#include "Audio/Capture/AudioCapture.h"
#include "Audio/Processing/SpectrumAnalyzer.h"

namespace Spectrum {

    class RealtimeAudioSource : public IAudioSource {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        explicit RealtimeAudioSource(const AudioConfig& config);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // IAudioSource Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        bool Initialize() override;
        void Update(float deltaTime) override;
        [[nodiscard]] SpectrumData GetSpectrum() override;

        void SetAmplification(float amp) override;
        void SetBarCount(size_t count) override;
        void SetFFTWindow(FFTWindowType type) override;
        void SetScaleType(SpectrumScale type) override;
        void SetSmoothing(float smoothing) override;

        void StartCapture() override;
        void StopCapture() override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        void ConfigureAnalyzer();
        void ReinitializeCapture();
        [[nodiscard]] bool TryCreateCaptureDevice();
        void SetupNewCaptureDevice();
        void HandleCaptureFaults();
        [[nodiscard]] bool EnsureCaptureIsReady();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        std::unique_ptr<AudioCapture> m_audioCapture;
        std::unique_ptr<SpectrumAnalyzer> m_analyzer;
        AudioConfig m_config;
        bool m_isCapturing;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_REALTIMEAUDIOSOURCE_H