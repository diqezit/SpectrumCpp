#ifndef SPECTRUM_CPP_AUDIO_CAPTURE_H
#define SPECTRUM_CPP_AUDIO_CAPTURE_H

#include "Common/Common.h"

#include <miniaudio.h>

namespace Spectrum {

    struct IAudioCaptureCallback {
        virtual ~IAudioCaptureCallback() = default;
        virtual void OnAudioData(const float* samples, uint32_t frames, uint32_t channels) = 0;
    };

    class AudioCapture {
    public:
        AudioCapture() {
            m_names.emplace_back(kDefaultSource);
            m_ids.emplace_back();

            ma_context ctx{};
            if (ma_context_init(nullptr, 0, nullptr, &ctx) != MA_SUCCESS)
                return;

            ma_device_info* info = nullptr;
            ma_uint32 n = 0;
            ma_context_get_devices(&ctx, nullptr, nullptr, &info, &n);
            for (ma_uint32 i = 0; i < n; ++i) {
                m_names.emplace_back(info[i].name);
                m_ids.push_back(info[i].id);
            }
            ma_context_uninit(&ctx);
        }
        ~AudioCapture() { Stop(); }

        AudioCapture(const AudioCapture&) = delete;
        AudioCapture& operator=(const AudioCapture&) = delete;
        AudioCapture(AudioCapture&&) = delete;
        AudioCapture& operator=(AudioCapture&&) = delete;

        bool Start() {
            if (ma_device_is_started(&m_device))
                return true;

            Stop();

            ma_device_type type = ma_device_type_loopback;
            const ma_device_id* id = nullptr;
            if (m_src) {
                type = ma_device_type_capture;
                id = &m_ids[m_src];
            }

            ma_device_config cfg = ma_device_config_init(type);
            cfg.capture.format = ma_format_f32;
            cfg.capture.channels = 1; // miniaudio converts WASAPI stereo to mono
            cfg.capture.pDeviceID = id;
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

        void SetSourceByName(std::string_view name) {
            const auto it = std::find(m_names.begin(), m_names.end(), name);
            if (it == m_names.end())
                return;
            const size_t i = size_t(it - m_names.begin());
            if (i == m_src)
                return;
            Stop();
            m_src = i;
        }

        [[nodiscard]] std::string_view GetSourceName() const noexcept {
            return m_names[m_src];
        }

        [[nodiscard]] const std::vector<std::string>& GetAvailableSource() const {
            return m_names;
        }

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
        static constexpr const char* kDefaultSource = "Default Output";

        ma_device              m_device{};
        IAudioCaptureCallback* m_cb = nullptr;

        size_t                    m_src = 0;
        std::vector<std::string>  m_names;
        std::vector<ma_device_id> m_ids;
    };

} // namespace Spectrum

#endif