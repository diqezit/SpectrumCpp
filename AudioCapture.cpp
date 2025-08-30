// AudioCapture.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioCapture.cpp: Implementation of the AudioCapture class using WASAPI.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "AudioCapture.h"
#include <chrono>

namespace Spectrum {

    using namespace WASAPI;

    namespace {
        constexpr DWORD kWaitTimeoutMs = 2000;

        inline void CloseHandleSafe(HANDLE& h) {
            if (!h) return;
            CloseHandle(h);
            h = nullptr;
        }

        inline bool ProcessPacket(
            IAudioCaptureClient* captureClient,
            IAudioCaptureCallback* callback,
            std::mutex& callbackMutex,
            int channels
        ) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;

            HRESULT hr = captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
            if (FAILED(hr)) return false;

            if (frames && data && !(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                std::lock_guard<std::mutex> lock(callbackMutex);
                if (callback) {
                    callback->OnAudioData(
                        reinterpret_cast<float*>(data),
                        static_cast<size_t>(frames) * channels,
                        channels
                    );
                }
            }

            return SUCCEEDED(captureClient->ReleaseBuffer(frames));
        }
    }

    AudioCapture::AudioCapture()
        : m_waveFormat(nullptr)
        , m_samplesEvent(nullptr)
        , m_capturing(false)
        , m_stopRequested(false)
        , m_isInitialized(false)
        , m_useEventMode(false)
        , m_callback(nullptr) {
    }

    AudioCapture::~AudioCapture() {
        Stop();
        Cleanup();
    }

    void AudioCapture::Cleanup() noexcept {
        if (m_waveFormat) {
            CoTaskMemFree(m_waveFormat);
            m_waveFormat = nullptr;
        }
        CloseHandleSafe(m_samplesEvent);
    }

    bool AudioCapture::Initialize() {
        if (m_isInitialized) return true;
        if (!m_comInitializer.IsInitialized()) return false;

        for (int retry = 0; retry < MAX_INIT_RETRIES; ++retry) {
            if (InitializeDevice() && InitializeClient()) {
                m_isInitialized = true;
                LogAudioInfo();
                return true;
            }

            if (retry < MAX_INIT_RETRIES - 1) {
                LOG_INFO("Initialization attempt " << (retry + 1) << " failed, retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(INIT_RETRY_DELAY_MS));
                ResetAudioClient();
            }
        }

        LOG_ERROR("Failed to initialize audio capture after " << MAX_INIT_RETRIES << " attempts");
        return false;
    }

    void AudioCapture::LogAudioInfo() const {
        LOG_INFO("Audio capture initialized successfully");
        LOG_INFO("Sample rate: " << GetSampleRate() << " Hz");
        LOG_INFO("Channels: " << GetChannels());
        LOG_INFO("Bits per sample: " << GetBitsPerSample());
        LOG_INFO("Mode: " << (m_useEventMode ? "Event-driven" : "Polling"));
    }

    bool AudioCapture::InitializeDevice() {
        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void**>(m_deviceEnumerator.GetAddressOf())
        );
        if (!CheckResult(hr, "Failed to create device enumerator"))
            return false;

        hr = m_deviceEnumerator->GetDefaultAudioEndpoint(
            eRender, eConsole, m_device.GetAddressOf()
        );
        return CheckResult(hr, "Failed to get default audio endpoint");
    }

    bool AudioCapture::InitializeClient() {
        HRESULT hr = m_device->Activate(
            __uuidof(IAudioClient),
            CLSCTX_ALL,
            nullptr,
            reinterpret_cast<void**>(m_audioClient.GetAddressOf())
        );
        if (!CheckResult(hr, "Failed to activate audio client"))
            return false;

        hr = m_audioClient->GetMixFormat(&m_waveFormat);
        if (!CheckResult(hr, "Failed to get mix format"))
            return false;

        m_samplesEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!m_samplesEvent) {
            LOG_ERROR("Failed to create event for audio capture");
            return false;
        }

        // Try event-driven mode first
        const DWORD eventModeFlags = AUDCLNT_STREAMFLAGS_LOOPBACK |
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
            AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;

        if (TryInitializeMode(eventModeFlags, true)) {
            m_useEventMode = true;
        }
        else {
            ResetAudioClient();
            const DWORD pollingModeFlags = AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
            if (TryInitializeMode(pollingModeFlags, false)) {
                m_useEventMode = false;
            }
            else {
                CloseHandleSafe(m_samplesEvent);
                return false;
            }
        }

        return SetupCaptureClient();
    }

    bool AudioCapture::TryInitializeMode(DWORD streamFlags, bool setEventHandle) {
        HRESULT hr = m_audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            streamFlags,
            BUFFER_DURATION,
            0,
            m_waveFormat,
            nullptr
        );

        if (SUCCEEDED(hr) && setEventHandle) {
            hr = m_audioClient->SetEventHandle(m_samplesEvent);
        }

        return SUCCEEDED(hr);
    }

    void AudioCapture::ResetAudioClient() {
        m_audioClient.Reset();
        m_captureClient.Reset();

        if (m_device) {
            m_device->Activate(
                __uuidof(IAudioClient),
                CLSCTX_ALL,
                nullptr,
                reinterpret_cast<void**>(m_audioClient.GetAddressOf())
            );
        }
    }

    bool AudioCapture::SetupCaptureClient() {
        HRESULT hr = m_audioClient->GetService(
            __uuidof(IAudioCaptureClient),
            reinterpret_cast<void**>(m_captureClient.GetAddressOf())
        );
        return CheckResult(hr, "Failed to get capture client service");
    }

    bool AudioCapture::Start() {
        if (!m_isInitialized) {
            LOG_ERROR("Cannot start capture: not initialized.");
            return false;
        }
        if (m_capturing) return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        HRESULT hr = m_audioClient->Start();
        if (FAILED(hr)) {
            LOG_INFO("First start attempt failed, retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            hr = m_audioClient->Start();
        }

        if (!CheckResult(hr, "Failed to start audio client")) return false;

        m_stopRequested = false;
        m_capturing = true;
        m_captureThread = std::thread(&AudioCapture::CaptureLoop, this);

        LOG_INFO("Audio capture started");
        return true;
    }

    void AudioCapture::Stop() noexcept {
        if (!m_capturing) return;

        m_stopRequested = true;
        if (m_samplesEvent && m_useEventMode) SetEvent(m_samplesEvent);
        if (m_captureThread.joinable()) m_captureThread.join();
        if (m_audioClient) m_audioClient->Stop();

        m_capturing = false;
        LOG_INFO("Audio capture stopped");
    }

    void AudioCapture::SetCallback(IAudioCaptureCallback* callback) noexcept {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_callback = callback;
    }

    int AudioCapture::GetSampleRate() const noexcept {
        return m_waveFormat ? static_cast<int>(m_waveFormat->nSamplesPerSec) : 0;
    }

    int AudioCapture::GetChannels() const noexcept {
        return m_waveFormat ? static_cast<int>(m_waveFormat->nChannels) : 0;
    }

    int AudioCapture::GetBitsPerSample() const noexcept {
        return m_waveFormat ? static_cast<int>(m_waveFormat->wBitsPerSample) : 0;
    }

    void AudioCapture::CaptureLoop() {
        WASAPI::ScopedCOMInitializer threadCom;
        if (!threadCom.IsInitialized()) return;

        if (m_useEventMode) {
            RunEventDrivenCapture();
        }
        else {
            RunPollingCapture();
        }
    }

    void AudioCapture::RunEventDrivenCapture() {
        while (!m_stopRequested && m_samplesEvent) {
            DWORD waitResult = WaitForSingleObject(m_samplesEvent, kWaitTimeoutMs);

            if (waitResult == WAIT_OBJECT_0) {
                if (!ProcessAudioPackets()) return;
            }
            else if (waitResult != WAIT_TIMEOUT) {
                LOG_ERROR("WaitForSingleObject failed in CaptureLoop");
                return;
            }
        }
    }

    void AudioCapture::RunPollingCapture() {
        while (!m_stopRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));

            if (!ProcessAudioPackets()) {
                if (!m_stopRequested) {
                    LOG_ERROR("Error in polling mode capture loop");
                }
                return;
            }
        }
    }

    bool AudioCapture::ProcessAudioPackets() {
        const int channels = GetChannels();
        UINT32 packetLen = 0;

        while (true) {
            HRESULT hr = m_captureClient->GetNextPacketSize(&packetLen);
            if (!CheckResult(hr, "GetNextPacketSize failed")) return false;
            if (packetLen == 0) break;

            if (!ProcessPacket(m_captureClient.Get(), m_callback, m_callbackMutex, channels))
                return false;
        }

        return true;
    }

} // namespace Spectrum