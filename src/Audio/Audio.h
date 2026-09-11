#ifndef SPECTRUM_CPP_AUDIO_H
#define SPECTRUM_CPP_AUDIO_H

#include "Common/Common.h"
#include "Common/EventBus.h"
#include "Audio/AudioCapture.hpp"
#include "Audio/Spectrum.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace Spectrum {

    class AudioManager final {
    public:
        static constexpr float  kAmplificationStep = 0.1f;
        static constexpr size_t kMinBarCount = 16;

        explicit AudioManager(EventBus* bus)
            : m_analyzer(m_cfg.barCount, m_cfg.fftSize)
        {
            ApplyConfig();
            m_capture.SetCallback(&m_analyzer);

            bus->Subscribe(InputAction::ToggleCapture,
                [this] { ToggleCapture(); });
            bus->Subscribe(InputAction::IncreaseAmplification,
                [this] { SetAmplification(m_cfg.amplification + kAmplificationStep); });
            bus->Subscribe(InputAction::DecreaseAmplification,
                [this] { SetAmplification(m_cfg.amplification - kAmplificationStep); });
            bus->Subscribe(InputAction::NextFFTWindow,
                [this] { CycleWindow(+1); });
            bus->Subscribe(InputAction::PrevFFTWindow,
                [this] { CycleWindow(-1); });
            bus->Subscribe(InputAction::IncreaseBarCount,
                [this] { SetBarCount(m_cfg.barCount + 1); });
            bus->Subscribe(InputAction::DecreaseBarCount,
                [this] { SetBarCount(m_cfg.barCount - 1); });
        }

        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;
        AudioManager(AudioManager&&) = delete;
        AudioManager& operator=(AudioManager&&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Frame
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void Update(float dt) {
            if (m_capturing && m_capture.IsFaulted())
                StopCapture();
            m_analyzer.Update(dt);
        }

        [[nodiscard]] const SpectrumData& GetSpectrum() const {
            return m_analyzer.GetSpectrum();
        }

        void ToggleCapture() {
            if (m_capturing)
                StopCapture();
            else
                StartCapture();
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void SetAmplification(float v) {
            m_cfg.amplification = Clamp(v, Analyzer::kAmpMin, Analyzer::kAmpMax);
            m_analyzer.SetAmplification(m_cfg.amplification);
        }

        void SetSmoothing(float v) {
            m_cfg.smoothing = Clamp(v, Analyzer::kSmoothMin, Analyzer::kSmoothMax);
            m_analyzer.SetSmoothing(m_cfg.smoothing);
        }

        void SetBarCount(size_t n) {
            m_cfg.barCount = Clamp(n, kMinBarCount, size_t(Analyzer::MAX_BARS));
            m_analyzer.SetBarCount(m_cfg.barCount);
        }

        void SetFFTWindowByName(std::string_view name) {
            for (int i = 0; i < int(FFTWindowType::Count); ++i) {
                const auto t = FFTWindowType(i);
                if (Helpers::Utils::ToString(t) == name) {
                    SetWindow(t);
                    return;
                }
            }
        }

        void ResetToDefaults() {
            m_cfg = {};
            ApplyConfig();
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] bool   IsCapturing()      const noexcept { return m_capturing; }
        [[nodiscard]] float  GetAmplification() const noexcept { return m_cfg.amplification; }
        [[nodiscard]] float  GetSmoothing()     const noexcept { return m_cfg.smoothing; }
        [[nodiscard]] size_t GetBarCount()      const noexcept { return m_cfg.barCount; }

        [[nodiscard]] std::string_view GetFFTWindowName() const noexcept {
            return Helpers::Utils::ToString(m_cfg.windowType);
        }

        [[nodiscard]] const std::vector<std::string>& GetAvailableFFTWindows() const {
            static const auto names = [] {
                std::vector<std::string> v;
                v.reserve(size_t(FFTWindowType::Count));
                for (int i = 0; i < int(FFTWindowType::Count); ++i)
                    v.emplace_back(Helpers::Utils::ToString(FFTWindowType(i)));
                return v;
                }();
            return names;
        }

    private:
        void ApplyConfig() {
            SetAmplification(m_cfg.amplification);
            SetSmoothing(m_cfg.smoothing);
            SetBarCount(m_cfg.barCount);
            SetWindow(m_cfg.windowType);
        }

        void CycleWindow(int dir) {
            SetWindow(Helpers::Utils::CycleEnum(m_cfg.windowType, dir));
        }

        void SetWindow(FFTWindowType t) {
            m_cfg.windowType = t;
            m_analyzer.SetFFTWindow(t);
        }

        void StartCapture() {
            if (!m_capture.Start())
                return;
            m_capturing = true;
            m_analyzer.SetSampleRate(m_capture.GetSampleRate());
        }

        void StopCapture() {
            m_capture.Stop();
            m_capturing = false;
        }

        AudioConfig  m_cfg;
        Analyzer     m_analyzer;
        AudioCapture m_capture;
        bool         m_capturing = false;
    };

} // namespace Spectrum

#endif