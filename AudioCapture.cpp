// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioCapture.cpp: Implementation for a single audio capture session.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "AudioCapture.h"
#include "AudioCaptureEngine.h"
#include "WASAPIHelper.h"
#include <chrono>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // PIMPL implementation: Hides all WASAPI and threading details.
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    struct AudioCapture::Implementation {
        std::unique_ptr<Internal::WasapiInitData> initData;
        std::unique_ptr<Internal::AudioPacketProcessor> processor;
        std::unique_ptr<Internal::ICaptureEngine> engine;

        std::thread captureThread;
        std::atomic<bool> isCapturing{ false };
        std::atomic<bool> stopRequested{ false };
        std::atomic<bool> isInitialized{ false };
        // A faulted state indicates a non-recoverable error like device loss
        std::atomic<bool> isFaulted{ false };
        std::atomic<HRESULT> lastError{ S_OK };

        ~Implementation();

        // Main loop for the dedicated capture thread
        void CaptureLoop();
        // Helper functions for Initialize()
        bool InitializeWasapi();
        bool CreateAudioProcessor();
        bool SelectCaptureEngine();
        void ResetState();
    };

    AudioCapture::Implementation::~Implementation() {
        if (initData && initData->waveFormat) {
            CoTaskMemFree(initData->waveFormat);
            initData->waveFormat = nullptr;
        }
        if (initData && initData->samplesEvent) {
            CloseHandle(initData->samplesEvent);
            initData->samplesEvent = nullptr;
        }
    }

    // Each thread using COM must initialize it separately
    // A scoped helper ensures CoUninitialize is always called on thread exit
    void AudioCapture::Implementation::CaptureLoop() {
        WASAPI::ScopedCOMInitializer threadCom;
        if (!threadCom.IsInitialized()) {
            isFaulted = true;
            lastError = CO_E_NOTINITIALIZED;
            return;
        }

        if (engine && processor) {
            lastError = engine->Run(stopRequested, *processor);
            // An error is only a fault if it wasn't caused by a user stop request
            if (FAILED(lastError) && !stopRequested) {
                isFaulted = true;
                if (lastError == AUDCLNT_E_DEVICE_INVALIDATED) {
                    LOG_ERROR("Audio device was lost. Please restart the application");
                }
                else {
                    LOG_ERROR("Audio capture thread exited with error: " << lastError);
                }
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Initialization Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    void AudioCapture::Implementation::ResetState() {
        isFaulted = false;
        lastError = S_OK;
    }

    bool AudioCapture::Implementation::InitializeWasapi() {
        Internal::WasapiInitializer initializer;
        initData = initializer.Initialize();
        if (!initData) {
            isFaulted = true;
            return false;
        }
        return true;
    }

    bool AudioCapture::Implementation::CreateAudioProcessor() {
        auto* data = initData.get();
        int channels = data->waveFormat ? data->waveFormat->nChannels : 0;
        processor = std::make_unique<Internal::AudioPacketProcessor>(
            data->captureClient.Get(),
            channels
        );
        return true;
    }

    // Choose the most efficient capture strategy supported by the audio driver
    // Event-driven is preferred as it's more efficient than constant polling
    bool AudioCapture::Implementation::SelectCaptureEngine() {
        auto* data = initData.get();
        if (data->useEventMode) {
            engine = std::make_unique<Internal::EventDrivenEngine>(data->samplesEvent);
        }
        else {
            engine = std::make_unique<Internal::PollingEngine>();
            // The event handle is not needed in polling mode, so release it
            if (data->samplesEvent) {
                CloseHandle(data->samplesEvent);
                data->samplesEvent = nullptr;
            }
        }
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Public API Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    AudioCapture::AudioCapture() : m_pimpl(std::make_unique<Implementation>()) {}
    AudioCapture::~AudioCapture() { Stop(); }

    // Orchestrator for the multi-step initialization process
    bool AudioCapture::Initialize() {
        if (m_pimpl->isInitialized)
            return true;

        m_pimpl->ResetState();

        if (!m_pimpl->InitializeWasapi() ||
            !m_pimpl->CreateAudioProcessor() ||
            !m_pimpl->SelectCaptureEngine()) {
            return false;
        }

        m_pimpl->isInitialized = true;
        LOG_INFO(
            "Audio capture initialized. Mode: "
            << (m_pimpl->initData->useEventMode ? "Event-driven" : "Polling")
        );
        LOG_INFO(
            "Format: " << GetSampleRate() << " Hz, " << GetChannels()
            << " channels, " << GetBitsPerSample() << " bits"
        );
        return true;
    }

    bool AudioCapture::Start() {
        if (!IsInitialized() || IsCapturing() || IsFaulted())
            return false;

        HRESULT hr = m_pimpl->initData->audioClient->Start();
        if (!WASAPI::CheckResult(hr, "Failed to start audio client")) {
            m_pimpl->isFaulted = true;
            m_pimpl->lastError = hr;
            return false;
        }

        m_pimpl->stopRequested = false;
        m_pimpl->isCapturing = true;
        m_pimpl->captureThread = std::thread(&Implementation::CaptureLoop, m_pimpl.get());
        return true;
    }

    void AudioCapture::Stop() noexcept {
        if (!m_pimpl->isCapturing && !m_pimpl->captureThread.joinable())
            return;

        m_pimpl->stopRequested = true;

        // In event-driven mode the thread might be blocked waiting for an event
        // We must signal the event to unblock it so it can see the stop request
        if (m_pimpl->initData && m_pimpl->initData->useEventMode && m_pimpl->initData->samplesEvent) {
            SetEvent(m_pimpl->initData->samplesEvent);
        }

        if (m_pimpl->captureThread.joinable()) {
            m_pimpl->captureThread.join();
        }

        if (m_pimpl->initData && m_pimpl->initData->audioClient) {
            m_pimpl->initData->audioClient->Stop();
        }

        m_pimpl->isCapturing = false;
    }

    void AudioCapture::SetCallback(IAudioCaptureCallback* callback) noexcept {
        if (m_pimpl->processor) {
            m_pimpl->processor->SetCallback(callback);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    bool AudioCapture::IsCapturing() const noexcept { return m_pimpl->isCapturing; }
    bool AudioCapture::IsInitialized() const noexcept { return m_pimpl->isInitialized; }
    bool AudioCapture::IsFaulted() const noexcept { return m_pimpl->isFaulted; }
    HRESULT AudioCapture::GetLastError() const noexcept { return m_pimpl->lastError; }

    int AudioCapture::GetSampleRate() const noexcept {
        if (m_pimpl->initData && m_pimpl->initData->waveFormat)
            return static_cast<int>(m_pimpl->initData->waveFormat->nSamplesPerSec);
        return 0;
    }

    int AudioCapture::GetChannels() const noexcept {
        if (m_pimpl->initData && m_pimpl->initData->waveFormat)
            return static_cast<int>(m_pimpl->initData->waveFormat->nChannels);
        return 0;
    }

    int AudioCapture::GetBitsPerSample() const noexcept {
        if (m_pimpl->initData && m_pimpl->initData->waveFormat)
            return static_cast<int>(m_pimpl->initData->waveFormat->wBitsPerSample);
        return 0;
    }

} // namespace Spectrum