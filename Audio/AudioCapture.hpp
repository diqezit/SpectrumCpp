#ifndef SPECTRUM_CPP_AUDIO_CAPTURE_H
#define SPECTRUM_CPP_AUDIO_CAPTURE_H

#include "Common/Common.h"

#include <miniaudio.h>

#include <cstdint>

namespace Spectrum {

    struct IAudioCaptureCallback {
        virtual ~IAudioCaptureCallback() = default;
        virtual void OnAudioData(const float* samples, uint32_t frames, uint32_t channels) = 0;
    };

    class AudioCapture {
    public:
        AudioCapture() = default;
        ~AudioCapture() { Stop(); }

        AudioCapture(const AudioCapture&) = delete;
        AudioCapture& operator=(const AudioCapture&) = delete;
        AudioCapture(AudioCapture&&) = delete;
        AudioCapture& operator=(AudioCapture&&) = delete;

        bool Start() {
            if (ma_device_is_started(&m_device))
                return true;

            Stop();

            ma_device_config cfg = ma_device_config_init(ma_device_type_loopback);
            cfg.capture.format = ma_format_f32;
            cfg.capture.channels = 1; // miniaudio converts WASAPI stereo to mono
            cfg.sampleRate = DEFAULT_SAMPLE_RATE;
            cfg.pUserData = this;
            cfg.dataCallback = [](ma_device* device, void*, const void* input, ma_uint32 frames) {

                // Loopback delivers capture frames in input
                // Never process more than frames
                auto* self = static_cast<AudioCapture*>(device->pUserData);
                if (self->m_cb && input && frames)
                    self->m_cb->OnAudioData(
                        static_cast<const float*>(input),
                        frames,
                        device->capture.channels);
                };

            // miniaudio owns the backend (WASAPI on Win)
            if (ma_device_init(nullptr, &cfg, &m_device) != MA_SUCCESS)
                return false;

            if (ma_device_start(&m_device) != MA_SUCCESS) {
                Stop();
                return false;
            }
            return true;
        }

        void Stop() {
            if (ma_device_get_state(&m_device) == ma_device_state_uninitialized)
                return;
            ma_device_uninit(&m_device);
            m_device = {};
        }

        void SetCallback(IAudioCaptureCallback* cb) { m_cb = cb; }

        // AudioManager checks this only while capturing
        // not started = device is lost
        [[nodiscard]] bool IsFaulted() const noexcept {
            return !ma_device_is_started(&m_device);
        }

        [[nodiscard]] uint32_t GetSampleRate() const noexcept {
            if (m_device.sampleRate)
                return m_device.sampleRate;
            return uint32_t(DEFAULT_SAMPLE_RATE);
        }

    private:
        ma_device              m_device{};
        IAudioCaptureCallback* m_cb = nullptr;
    };

} // namespace Spectrum

#endif