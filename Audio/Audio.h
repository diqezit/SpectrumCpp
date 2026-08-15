#ifndef SPECTRUM_CPP_AUDIO_H
#define SPECTRUM_CPP_AUDIO_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Audio sources, spectrum config, capture / animation switching.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Common/EventBus.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Audio/Sources/RealtimeAudioSource.h"
#include "Audio/Sources/AnimatedAudioSource.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Spectrum {

    using Helpers::Math::Clamp;
    using Helpers::Utils::CycleEnum;

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

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] bool Initialize() {
            if (!MakeSource<RealtimeAudioSource>(m_realtime)) return false;
            if (!MakeSource<AnimatedAudioSource>(m_animated)) return false;
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
            if (m_current) m_current->Update(dt);
        }

        [[nodiscard]] SpectrumData GetSpectrum() {
            return m_current ? m_current->GetSpectrum() : SpectrumData{};
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Actions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void ToggleCapture() {
            if (m_animating) return;
            m_capturing = !m_capturing;
            if (m_capturing) m_realtime->StartCapture();
            else             m_realtime->StopCapture();
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

        void ChangeFFTWindow(int direction) {
            m_cfg.windowType = CycleEnum(m_cfg.windowType, direction);
            m_realtime->SetFFTWindow(m_cfg.windowType);
        }

        void ChangeSpectrumScale(int direction) {
            m_cfg.scaleType = CycleEnum(m_cfg.scaleType, direction);
            m_realtime->SetScaleType(m_cfg.scaleType);
        }

        void SetAmplification(float amp) {
            m_cfg.amplification = Clamp(amp, kMinAmplification, kMaxAmplification);
            m_realtime->SetAmplification(m_cfg.amplification);
        }

        void SetSmoothing(float smoothing) {
            m_cfg.smoothing = Clamp(smoothing, kMinSmoothing, kMaxSmoothing);
            m_realtime->SetSmoothing(m_cfg.smoothing);
        }

        void SetBarCount(size_t count) {
            m_cfg.barCount = Clamp(count, kMinBarCount, kMaxBarCount);
            m_realtime->SetBarCount(m_cfg.barCount);
        }

        void SetFFTWindowByName(const std::string& name) {
            m_cfg.windowType = ToFFTWindow(name);
            m_realtime->SetFFTWindow(m_cfg.windowType);
        }

        void SetSpectrumScaleByName(const std::string& name) {
            m_cfg.scaleType = ToSpectrumScale(name);
            m_realtime->SetScaleType(m_cfg.scaleType);
        }

        void ResetToDefaults() {
            SetAmplification(DEFAULT_AMPLIFICATION);
            SetSmoothing(DEFAULT_SMOOTHING);
            SetBarCount(DEFAULT_BAR_COUNT);
            SetFFTWindowByName("Hann");
            SetSpectrumScaleByName("Logarithmic");
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] bool IsCapturing()     const noexcept { return m_capturing; }
        [[nodiscard]] bool IsAnimating()     const noexcept { return m_animating; }
        [[nodiscard]] bool HasActiveSource() const noexcept { return m_current != nullptr; }

        [[nodiscard]] float  GetAmplification()    const noexcept { return m_cfg.amplification; }
        [[nodiscard]] float  GetSmoothing()        const noexcept { return m_cfg.smoothing; }
        [[nodiscard]] size_t GetBarCount()         const noexcept { return m_cfg.barCount; }
        [[nodiscard]] float  GetAmplificationMin() const noexcept { return kMinAmplification; }
        [[nodiscard]] float  GetAmplificationMax() const noexcept { return kMaxAmplification; }
        [[nodiscard]] float  GetSmoothingMin()     const noexcept { return kMinSmoothing; }
        [[nodiscard]] float  GetSmoothingMax()     const noexcept { return kMaxSmoothing; }
        [[nodiscard]] size_t GetBarCountMin()      const noexcept { return kMinBarCount; }
        [[nodiscard]] size_t GetBarCountMax()      const noexcept { return kMaxBarCount; }

        [[nodiscard]] std::string_view GetFFTWindowName()     const noexcept { return Helpers::Utils::ToString(m_cfg.windowType); }
        [[nodiscard]] std::string_view GetSpectrumScaleName() const noexcept { return Helpers::Utils::ToString(m_cfg.scaleType); }

        [[nodiscard]] const std::vector<std::string>& GetAvailableFFTWindows() const {
            static const std::vector<std::string> k{ "Hann", "Hamming", "Blackman", "Rectangular" };
            return k;
        }

        [[nodiscard]] const std::vector<std::string>& GetAvailableSpectrumScales() const {
            static const std::vector<std::string> k{ "Linear", "Logarithmic", "Mel" };
            return k;
        }

    private:
        template<typename T>
        bool MakeSource(std::unique_ptr<IAudioSource>& dst) {
            dst = std::make_unique<T>(m_cfg);
            return dst->Initialize();
        }

        static FFTWindowType ToFFTWindow(const std::string& name) {
            if (name == "Hamming")     return FFTWindowType::Hamming;
            if (name == "Blackman")    return FFTWindowType::Blackman;
            if (name == "Rectangular") return FFTWindowType::Rectangular;
            return FFTWindowType::Hann;
        }

        static SpectrumScale ToSpectrumScale(const std::string& name) {
            if (name == "Logarithmic") return SpectrumScale::Logarithmic;
            if (name == "Mel")         return SpectrumScale::Mel;
            return SpectrumScale::Linear;
        }

        std::unique_ptr<IAudioSource> m_realtime;
        std::unique_ptr<IAudioSource> m_animated;
        IAudioSource* m_current = nullptr;
        AudioConfig   m_cfg;
        bool          m_capturing = false;
        bool          m_animating = false;
    };

} // namespace Spectrum

#endif