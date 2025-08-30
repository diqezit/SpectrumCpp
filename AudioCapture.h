// AudioCapture.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioCapture.h: Captures audio from the default playback device (loopback).
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_AUDIO_CAPTURE_H
#define SPECTRUM_CPP_AUDIO_CAPTURE_H

#include "Common.h"
#include "WASAPIHelper.h"

namespace Spectrum {

    class IAudioCaptureCallback {
    public:
        virtual ~IAudioCaptureCallback() = default;
        virtual void OnAudioData(const float* data, size_t samples, int channels) = 0;
    };

    class AudioCapture {
    public:
        AudioCapture();
        ~AudioCapture();

        bool Initialize();
        bool Start();
        void Stop() noexcept;

        bool IsCapturing() const noexcept { return m_capturing; }
        bool IsInitialized() const noexcept { return m_isInitialized; }

        void SetCallback(IAudioCaptureCallback* callback) noexcept;

        // Audio info getters
        int GetSampleRate() const noexcept;
        int GetChannels() const noexcept;
        int GetBitsPerSample() const noexcept;

    private:
        // Initialization methods
        bool InitializeDevice();
        bool InitializeClient();
        bool TryInitializeMode(DWORD streamFlags, bool setEventHandle);
        bool SetupCaptureClient();
        void ResetAudioClient();

        // Capture loop methods
        void CaptureLoop();
        void RunEventDrivenCapture();
        void RunPollingCapture();
        bool ProcessAudioPackets();

        // Helper methods
        void Cleanup() noexcept;
        void LogAudioInfo() const;

    private:
        // COM and WASAPI interfaces
        WASAPI::ScopedCOMInitializer m_comInitializer;
        wrl::ComPtr<IMMDeviceEnumerator> m_deviceEnumerator;
        wrl::ComPtr<IMMDevice> m_device;
        wrl::ComPtr<IAudioClient> m_audioClient;
        wrl::ComPtr<IAudioCaptureClient> m_captureClient;

        // Audio format
        WAVEFORMATEX* m_waveFormat;

        // Event for event-driven capture
        HANDLE m_samplesEvent;

        // Threading and state management
        std::thread m_captureThread;
        std::atomic<bool> m_capturing;
        std::atomic<bool> m_stopRequested;
        std::atomic<bool> m_isInitialized;
        bool m_useEventMode;

        // Callback handling
        std::mutex m_callbackMutex;
        IAudioCaptureCallback* m_callback;

        static constexpr REFERENCE_TIME REFTIMES_PER_SEC = 10000000;
        static constexpr REFERENCE_TIME BUFFER_DURATION = REFTIMES_PER_SEC / 2; // 500ms
        static constexpr DWORD POLLING_INTERVAL_MS = 10;
        static constexpr DWORD INIT_RETRY_DELAY_MS = 200;
        static constexpr int MAX_INIT_RETRIES = 3;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_CAPTURE_H