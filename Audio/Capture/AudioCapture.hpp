#pragma once

#include "Common/Common.h"

#include <miniaudio.h>
#include <atomic>

namespace Spectrum {

    class IAudioCaptureCallback {
    public:
        virtual ~IAudioCaptureCallback() = default;
        virtual void OnAudioData(
            const float* data, uint32_t frames, uint32_t channels) = 0;
    };

    class AudioCapture {
    public:
        AudioCapture() = default;
        ~AudioCapture() { Close(); }

        AudioCapture(const AudioCapture&) = delete;
        AudioCapture& operator=(const AudioCapture&) = delete;

        bool Initialize() {
            Close();
            ma_device_config cfg = ma_device_config_init(ma_device_type_loopback);
            cfg.capture.format = ma_format_f32;
            cfg.dataCallback = OnData;
            cfg.pUserData = this;
            m_err = ma_device_init(nullptr, &cfg, &m_dev);
            m_open = m_err == MA_SUCCESS;
            if (!m_open)
                LOG_ERROR("Failed to initialize audio capture: "
                    << ma_result_description(m_err));
            return m_open;
        }

        bool Start() {
            m_err = ma_device_start(&m_dev);
            return m_err == MA_SUCCESS;
        }

        void Stop() noexcept {
            if (IsCapturing())
                ma_device_stop(&m_dev);
        }

        bool IsCapturing() const noexcept {
            return m_open && ma_device_is_started(&m_dev);
        }

        bool IsInitialized() const noexcept { return m_open; }
        bool IsFaulted() const noexcept { return m_err != MA_SUCCESS; }
        ma_result GetLastError() const noexcept { return m_err; }

        void SetCallback(IAudioCaptureCallback* cb) noexcept {
            m_cb.store(cb, std::memory_order_release);
        }

        int GetSampleRate() const noexcept {
            return m_open ? int(m_dev.sampleRate) : 0;
        }

        int GetChannels() const noexcept {
            return m_open ? int(m_dev.capture.channels) : 0;
        }

        int GetBitsPerSample() const noexcept { return 32; }

    private:
        void Close() {
            if (!m_open)
                return;
            ma_device_stop(&m_dev);
            ma_device_uninit(&m_dev);
            m_open = false;
        }

        static void OnData(
            ma_device* d, void*, const void* in, ma_uint32 frames)
        {
            auto* self = static_cast<AudioCapture*>(d->pUserData);
            if (auto* cb = self->m_cb.load(std::memory_order_acquire)) {
                cb->OnAudioData(
                    static_cast<const float*>(in),
                    frames,
                    d->capture.channels);
            }
        }

        ma_device m_dev{};
        std::atomic<IAudioCaptureCallback*> m_cb{ nullptr };
        ma_result m_err = MA_SUCCESS;
        bool m_open = false;
    };

} // namespace Spectrum