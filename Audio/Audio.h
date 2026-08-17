#ifndef SPECTRUM_CPP_AUDIO_H
#define SPECTRUM_CPP_AUDIO_H

#include "Common/Common.h"
#include "Common/EventBus.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Audio/Sources/RealtimeAudioSource.hpp"
#include "Audio/Sources/AnimatedAudioSource.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Spectrum {

    class AudioManager final {
    public:
        static constexpr float  kMinAmplification = 0.1f;
        static constexpr float  kMaxAmplification = 5.0f;
        static constexpr float  kAmplificationStep = 0.1f;
        static constexpr float  kMinSmoothing = 0.0f;
        static constexpr float  kMaxSmoothing = 1.0f;
        static constexpr size_t kMinBarCount = 16;
        static constexpr size_t kMaxBarCount = 256;

        explicit AudioManager(EventBus* bus) {
            bus->Subscribe(InputAction::ToggleCapture, [this] { ToggleCapture(); });
            bus->Subscribe(InputAction::ToggleAnimation, [this] { ToggleAnimation(); });
            bus->Subscribe(InputAction::CycleSpectrumScale, [this] { ChangeSpectrumScale(1); });
            bus->Subscribe(InputAction::IncreaseAmplification, [this] { ChangeAmplification(kAmplificationStep); });
            bus->Subscribe(InputAction::DecreaseAmplification, [this] { ChangeAmplification(-kAmplificationStep); });
            bus->Subscribe(InputAction::NextFFTWindow, [this] { ChangeFFTWindow(1); });
            bus->Subscribe(InputAction::PrevFFTWindow, [this] { ChangeFFTWindow(-1); });
        }

        ~AudioManager() { Shutdown(); }

        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;

        bool Initialize() {
            m_realtime = std::make_unique<RealtimeAudioSource>(m_cfg);
            m_animated = std::make_unique<AnimatedAudioSource>(m_cfg);
            if (!m_realtime->Initialize() || !m_animated->Initialize())
                return false;
            m_current = m_realtime.get();
            return true;
        }

        void Shutdown() {
            if (m_capturing) {
                m_capturing = false;
                m_realtime->StopCapture();
            }
            m_current = nullptr;
        }

        void Update(float dt) {
            if (m_current)
                m_current->Update(dt);
        }

        SpectrumData GetSpectrum() {
            return m_current ? m_current->GetSpectrum() : SpectrumData{};
        }

        void ToggleCapture() {
            if (m_animating)
                return;
            m_capturing = !m_capturing;
            if (m_capturing)
                m_realtime->StartCapture();
            else
                m_realtime->StopCapture();
        }

        void ToggleAnimation() {
            m_animating = !m_animating;
            if (m_animating && m_capturing) {
                m_capturing = false;
                m_realtime->StopCapture();
            }
            m_current = m_animating ? m_animated.get() : m_realtime.get();
        }

        void ChangeAmplification(float delta) { SetAmplification(m_cfg.amplification + delta); }

        void ChangeFFTWindow(int dir) {
            m_cfg.windowType = Helpers::Utils::CycleEnum(m_cfg.windowType, dir);
            m_realtime->SetFFTWindow(m_cfg.windowType);
        }

        void ChangeSpectrumScale(int dir) {
            m_cfg.scaleType = Helpers::Utils::CycleEnum(m_cfg.scaleType, dir);
            m_realtime->SetScaleType(m_cfg.scaleType);
        }

        void SetAmplification(float amp) {
            m_cfg.amplification = Clamp(amp, kMinAmplification, kMaxAmplification);
            m_realtime->SetAmplification(m_cfg.amplification);
        }

        void SetSmoothing(float s) {
            m_cfg.smoothing = Clamp(s, kMinSmoothing, kMaxSmoothing);
            m_realtime->SetSmoothing(m_cfg.smoothing);
        }

        void SetBarCount(size_t n) {
            m_cfg.barCount = Clamp(n, kMinBarCount, kMaxBarCount);
            m_realtime->SetBarCount(m_cfg.barCount);
        }

        void SetFFTWindowByName(const std::string& name) {
            m_cfg.windowType =
                name == "Hamming" ? FFTWindowType::Hamming :
                name == "Blackman" ? FFTWindowType::Blackman :
                name == "Rectangular" ? FFTWindowType::Rectangular :
                FFTWindowType::Hann;
            m_realtime->SetFFTWindow(m_cfg.windowType);
        }

        void SetSpectrumScaleByName(const std::string& name) {
            m_cfg.scaleType =
                name == "Logarithmic" ? SpectrumScale::Logarithmic :
                name == "Mel" ? SpectrumScale::Mel :
                SpectrumScale::Linear;
            m_realtime->SetScaleType(m_cfg.scaleType);
        }

        void ResetToDefaults() {
            SetAmplification(DEFAULT_AMPLIFICATION);
            SetSmoothing(DEFAULT_SMOOTHING);
            SetBarCount(DEFAULT_BAR_COUNT);
            SetFFTWindowByName("Hann");
            SetSpectrumScaleByName("Logarithmic");
        }

        bool IsCapturing() const noexcept { return m_capturing; }
        bool IsAnimating() const noexcept { return m_animating; }
        bool HasActiveSource() const noexcept { return m_current != nullptr; }

        float  GetAmplification() const noexcept { return m_cfg.amplification; }
        float  GetSmoothing() const noexcept { return m_cfg.smoothing; }
        size_t GetBarCount() const noexcept { return m_cfg.barCount; }
        float  GetAmplificationMin() const noexcept { return kMinAmplification; }
        float  GetAmplificationMax() const noexcept { return kMaxAmplification; }
        float  GetSmoothingMin() const noexcept { return kMinSmoothing; }
        float  GetSmoothingMax() const noexcept { return kMaxSmoothing; }
        size_t GetBarCountMin() const noexcept { return kMinBarCount; }
        size_t GetBarCountMax() const noexcept { return kMaxBarCount; }

        std::string_view GetFFTWindowName() const noexcept {
            return Helpers::Utils::ToString(m_cfg.windowType);
        }

        std::string_view GetSpectrumScaleName() const noexcept {
            return Helpers::Utils::ToString(m_cfg.scaleType);
        }

        const std::vector<std::string>& GetAvailableFFTWindows() const {
            static const std::vector<std::string> k{ "Hann", "Hamming", "Blackman", "Rectangular" };
            return k;
        }

        const std::vector<std::string>& GetAvailableSpectrumScales() const {
            static const std::vector<std::string> k{ "Linear", "Logarithmic", "Mel" };
            return k;
        }

    private:
        std::unique_ptr<IAudioSource> m_realtime;
        std::unique_ptr<IAudioSource> m_animated;
        IAudioSource* m_current = nullptr;
        AudioConfig m_cfg;
        bool m_capturing = false;
        bool m_animating = false;
    };

} // namespace Spectrum

#endif