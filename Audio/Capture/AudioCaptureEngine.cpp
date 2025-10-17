// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioCaptureEngine.cpp: Implementation of the internal audio capture logic.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "AudioCaptureEngine.h"
#include "AudioCapture.h"
#include "WASAPIHelper.h"
#include <chrono>

namespace Spectrum {
    namespace Internal {

        using namespace WASAPI;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // WasapiInitializer Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        std::unique_ptr<WasapiInitData> WasapiInitializer::Initialize() {
            static constexpr int MAX_INIT_RETRIES = 3;
            static constexpr DWORD INIT_RETRY_DELAY_MS = 200;

            ScopedCOMInitializer com;
            if (!com.IsInitialized()) return nullptr;

            for (int retry = 0; retry < MAX_INIT_RETRIES; ++retry) {
                auto data = std::make_unique<WasapiInitData>();
                if (TryInitializeOnce(data)) return data;

                if (retry < MAX_INIT_RETRIES - 1) {
                    LOG_INFO(
                        "Initialization attempt " << (retry + 1)
                        << " failed, retrying..."
                    );
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(INIT_RETRY_DELAY_MS)
                    );
                }
            }
            LOG_ERROR(
                "Failed to initialize audio capture after " << MAX_INIT_RETRIES
                << " attempts"
            );
            return nullptr;
        }

        bool WasapiInitializer::TryInitializeOnce(std::unique_ptr<WasapiInitData>& data) {
            wrl::ComPtr<IMMDeviceEnumerator> enumerator;
            if (!CreateDeviceEnumerator(enumerator)) return false;

            wrl::ComPtr<IMMDevice> device;
            if (!GetDefaultAudioDevice(enumerator.Get(), device)) return false;

            if (!ActivateClientInterface(device.Get(), data->audioClient)) return false;

            if (!GetClientMixFormat(data->audioClient.Get(), &data->waveFormat)) return false;

            if (!TryInitializeInPreferredMode(*data, device)) return false;

            return SetupCaptureClient(data->audioClient.Get(), data->captureClient.GetAddressOf());
        }

        bool WasapiInitializer::CreateDeviceEnumerator(wrl::ComPtr<IMMDeviceEnumerator>& enumerator) const {
            HRESULT hr = CoCreateInstance(
                __uuidof(MMDeviceEnumerator),
                nullptr,
                CLSCTX_ALL,
                __uuidof(IMMDeviceEnumerator),
                &enumerator
            );
            return CheckResult(hr, "Failed to create device enumerator");
        }

        bool WasapiInitializer::GetDefaultAudioDevice(
            IMMDeviceEnumerator* enumerator,
            wrl::ComPtr<IMMDevice>& device
        ) const {
            HRESULT hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
            return CheckResult(hr, "Failed to get default audio endpoint");
        }

        bool WasapiInitializer::ActivateClientInterface(
            IMMDevice* device,
            wrl::ComPtr<IAudioClient>& client
        ) const {
            HRESULT hr = device->Activate(
                __uuidof(IAudioClient),
                CLSCTX_ALL,
                nullptr,
                &client
            );
            return CheckResult(hr, "Failed to activate audio client");
        }

        bool WasapiInitializer::GetClientMixFormat(
            IAudioClient* client,
            WAVEFORMATEX** waveFormat
        ) const {
            HRESULT hr = client->GetMixFormat(waveFormat);
            return CheckResult(hr, "Failed to get mix format");
        }

        bool WasapiInitializer::TryInitializeInPreferredMode(
            WasapiInitData& data,
            wrl::ComPtr<IMMDevice>& device
        ) const {
            data.samplesEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!data.samplesEvent) {
                LOG_ERROR("Failed to create capture event");
                return false;
            }

            if (TryEventDrivenInitialization(data)) return true;
            if (TryPollingInitialization(data, device)) return true;

            CloseHandle(data.samplesEvent);
            data.samplesEvent = nullptr;
            return false;
        }

        bool WasapiInitializer::TryEventDrivenInitialization(WasapiInitData& data) const {
            const DWORD eventFlags = AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
            if (TryInitializeMode(data.audioClient.Get(), data.waveFormat, eventFlags, true, data.samplesEvent)) {
                data.useEventMode = true;
                return true;
            }
            return false;
        }

        bool WasapiInitializer::TryPollingInitialization(
            WasapiInitData& data,
            wrl::ComPtr<IMMDevice>& device
        ) const {
            ResetClient(device, data.audioClient);
            const DWORD pollingFlags = AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
            if (TryInitializeMode(data.audioClient.Get(), data.waveFormat, pollingFlags, false, nullptr)) {
                data.useEventMode = false;
                CloseHandle(data.samplesEvent);
                data.samplesEvent = nullptr;
                return true;
            }
            return false;
        }

        bool WasapiInitializer::TryInitializeMode(
            IAudioClient* client,
            WAVEFORMATEX* wf,
            DWORD flags,
            bool setEvent,
            HANDLE h
        ) const {
            static constexpr REFERENCE_TIME REFTIMES_PER_SEC = 10000000;
            static constexpr REFERENCE_TIME BUFFER_DURATION = REFTIMES_PER_SEC / 2;

            HRESULT hr = client->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                flags,
                BUFFER_DURATION,
                0,
                wf,
                nullptr
            );
            if (SUCCEEDED(hr) && setEvent) hr = client->SetEventHandle(h);
            return SUCCEEDED(hr);
        }

        void WasapiInitializer::ResetClient(
            wrl::ComPtr<IMMDevice>& device,
            wrl::ComPtr<IAudioClient>& client
        ) const {
            client.Reset();
            if (device) device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &client);
        }

        bool WasapiInitializer::SetupCaptureClient(
            IAudioClient* audioClient,
            IAudioCaptureClient** captureClient
        ) const {
            HRESULT hr = audioClient->GetService(
                __uuidof(IAudioCaptureClient),
                reinterpret_cast<void**>(captureClient)
            );
            return CheckResult(hr, "Failed to get capture client service");
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // AudioPacketProcessor Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        AudioPacketProcessor::AudioPacketProcessor(IAudioCaptureClient* client, int channels)
            : m_captureClient(client), m_channels(channels), m_callback(nullptr) {
        }

        void AudioPacketProcessor::SetCallback(IAudioCaptureCallback* callback) noexcept {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_callback = callback;
        }

        void AudioPacketProcessor::InvokeCallbackWithData(
            BYTE* data,
            UINT32 frames,
            DWORD flags
        ) {
            if (frames > 0 && data != nullptr && !(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_callback) {
                    m_callback->OnAudioData(
                        reinterpret_cast<float*>(data),
                        static_cast<size_t>(frames) * m_channels,
                        m_channels
                    );
                }
            }
        }

        HRESULT AudioPacketProcessor::ProcessSinglePacket() {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;

            HRESULT hr = m_captureClient->GetBuffer(
                &data,
                &frames,
                &flags,
                nullptr,
                nullptr
            );
            if (FAILED(hr)) return hr;

            InvokeCallbackWithData(data, frames, flags);

            return m_captureClient->ReleaseBuffer(frames);
        }

        HRESULT AudioPacketProcessor::ProcessAvailablePackets() {
            HRESULT hr = S_OK;
            UINT32 packetLen = 0;

            while (SUCCEEDED(hr = m_captureClient->GetNextPacketSize(&packetLen)) && packetLen > 0) {
                hr = ProcessSinglePacket();
                if (FAILED(hr)) break;
            }
            return hr;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Capture Engine Implementations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        EventDrivenEngine::EventDrivenEngine(HANDLE event) : m_samplesEvent(event) {}

        HRESULT EventDrivenEngine::Run(
            const std::atomic<bool>& stopRequested,
            AudioPacketProcessor& processor
        ) {
            constexpr DWORD kWaitTimeoutMs = 2000;
            HRESULT hr = S_OK;

            while (!stopRequested) {
                DWORD waitResult = WaitForSingleObject(m_samplesEvent, kWaitTimeoutMs);
                if (stopRequested) break;

                if (waitResult == WAIT_OBJECT_0) {
                    hr = processor.ProcessAvailablePackets();
                    if (FAILED(hr)) break;
                }
                else if (waitResult != WAIT_TIMEOUT) {
                    LOG_ERROR("Event-driven capture loop failed on wait.");
                    hr = E_FAIL;
                    break;
                }
            }
            return hr;
        }

        HRESULT PollingEngine::Run(
            const std::atomic<bool>& stopRequested,
            AudioPacketProcessor& processor
        ) {
            constexpr auto kPollingInterval = std::chrono::milliseconds(20);
            HRESULT hr = S_OK;

            while (!stopRequested) {
                hr = processor.ProcessAvailablePackets();
                if (FAILED(hr)) break;
                std::this_thread::sleep_for(kPollingInterval);
            }
            return hr;
        }

    }
}