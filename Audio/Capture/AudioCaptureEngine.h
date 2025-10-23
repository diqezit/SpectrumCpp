// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioCaptureEngine.h: Internal helper classes for the audio capture process.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifndef SPECTRUM_CPP_AUDIO_CAPTURE_ENGINE_H
#define SPECTRUM_CPP_AUDIO_CAPTURE_ENGINE_H

#include "Common/Common.h"

namespace Spectrum {

    class IAudioCaptureCallback;

    namespace Internal {

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Data structure to hold WASAPI initialization results.
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        struct WasapiInitData {
            wrl::ComPtr<IAudioClient> audioClient;
            wrl::ComPtr<IAudioCaptureClient> captureClient;
            WAVEFORMATEX* waveFormat = nullptr;
            HANDLE samplesEvent = nullptr;
            bool useEventMode = false;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Handles low-level WASAPI device initialization.
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        class WasapiInitializer {
        public:
            std::unique_ptr<WasapiInitData> Initialize();

        private:
            bool TryInitializeOnce(std::unique_ptr<WasapiInitData>& data);

            bool CreateDeviceEnumerator(wrl::ComPtr<IMMDeviceEnumerator>& enumerator) const;
            bool GetDefaultAudioDevice(
                IMMDeviceEnumerator* enumerator,
                wrl::ComPtr<IMMDevice>& device
            ) const;

            bool ActivateClientInterface(
                IMMDevice* device,
                wrl::ComPtr<IAudioClient>& client
            ) const;
            bool GetClientMixFormat(
                IAudioClient* client,
                WAVEFORMATEX** waveFormat
            ) const;

            bool TryInitializeInPreferredMode(
                WasapiInitData& data,
                wrl::ComPtr<IMMDevice>& device
            ) const;
            bool TryEventDrivenInitialization(WasapiInitData& data) const;
            bool TryPollingInitialization(
                WasapiInitData& data,
                wrl::ComPtr<IMMDevice>& device
            ) const;

            bool TryInitializeMode(
                IAudioClient* client,
                WAVEFORMATEX* wf,
                DWORD flags,
                bool setEvent,
                HANDLE h
            ) const;

            void ResetClient(
                wrl::ComPtr<IMMDevice>& device,
                wrl::ComPtr<IAudioClient>& client
            ) const;

            bool SetupCaptureClient(
                IAudioClient* audioClient,
                IAudioCaptureClient** captureClient
            ) const;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Processes audio packets from the buffer.
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        class AudioPacketProcessor {
        public:
            AudioPacketProcessor(IAudioCaptureClient* client, int channels);
            void SetCallback(IAudioCaptureCallback* callback) noexcept;
            HRESULT ProcessAvailablePackets();

        private:
            HRESULT ProcessSinglePacket();
            void InvokeCallbackWithData(
                BYTE* data,
                UINT32 frames,
                DWORD flags
            );

            IAudioCaptureClient* m_captureClient;
            int m_channels;
            IAudioCaptureCallback* m_callback;
            std::mutex m_callbackMutex;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Defines the strategy for the capture loop.
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        class ICaptureEngine {
        public:
            virtual ~ICaptureEngine() = default;
            virtual HRESULT Run(
                const std::atomic<bool>& stopRequested,
                AudioPacketProcessor& processor
            ) = 0;
        };

        class EventDrivenEngine : public ICaptureEngine {
        public:
            explicit EventDrivenEngine(HANDLE event);
            HRESULT Run(
                const std::atomic<bool>& stopRequested,
                AudioPacketProcessor& processor
            ) override;
        private:
            HANDLE m_samplesEvent;
        };

        class PollingEngine : public ICaptureEngine {
        public:
            HRESULT Run(
                const std::atomic<bool>& stopRequested,
                AudioPacketProcessor& processor
            ) override;
        };

    }
}

#endif // SPECTRUM_CPP_AUDIO_CAPTURE_ENGINE_H